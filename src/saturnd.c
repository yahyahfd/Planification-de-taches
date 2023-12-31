#include "cassini.h"

int err_type_for_stdout(char* task){
	FILE *fd = fopen("/tasks.txt", "r");
	bool ret = false;
	char buffer[256];
	while(fgets(buffer,256,fd) != NULL){
		if(strcmp(buffer,task) == 0){
			ret = true;
			break;
		}
		memset(buffer,0,sizeof(buffer));
	}
	return ret;
}

int main(int argc, char * argv[]) {
    int fd1,fd2;
    char * rq_p = NULL;
    char * rp_p = NULL;
    char * pipes_directory = argv[1];
    rq_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-request-pipe")));
    rp_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-reply-pipe")));
    strcpy(rp_p,pipes_directory); //we copy our pipes_directory into a new var
    strcpy(rq_p,pipes_directory); //we copy our pipes_directory into a new var
    fd1 = open(strcat(rq_p,"/saturnd-request-pipe"), O_RDONLY);//we open our request pipe in read only mode
    fd2 = open(strcat(rp_p,"/saturnd-reply-pipe"), O_WRONLY);//we open our reply pipe in write only mode
    free(rq_p);
    free(rp_p);
    if (fd1 < 0 || fd2 < 0){ //if we can't open one of the two pipes, we go to error
        goto error;
    }
    
    uint64_t buffer8; //buffer for uint64_t
    uint32_t buffer4; //buffer for uint32_t
    uint16_t buffer2; //buffer for uint16_t
    uint8_t buffer1; //buffer for uint8_t
    // int64_t buff8;  //buffer for int64_t
    char taskidbuf[64];
    char taskerrorbuf[1024]; //buffer for standard error output

    uint16_t ok = SERVER_REPLY_OK;
    uint16_t not_found = SERVER_REPLY_ERROR_NOT_FOUND;
    uint16_t never_run = SERVER_REPLY_ERROR_NEVER_RUN;
    uint16_t error = SERVER_REPLY_ERROR;
    read(fd1,&buffer2,2);
    uint16_t operation = htobe16(buffer2);
    char spacebuf = '\n';
    int tasks_fd,tasks_res_fd;
    tasks_fd = open("tasks.txt", O_RDWR |O_CREAT |O_APPEND , 0600);
    tasks_res_fd = open("tasks_res.txt", O_RDWR  | O_CREAT|O_APPEND, 0600);
    uint64_t taskID = 0;
    uint64_t new_taskID;
    int tasks_errors_fd ; //descriptor for error text file of task
    int tasks_stdout_fd; //descriptor for stdout text file of task
    switch(operation){
        case CLIENT_REQUEST_CREATE_TASK:
        {
            if(tasks_fd<0 || tasks_res_fd < 0) goto error;

            new_taskID = taskID;
            write(tasks_fd, &new_taskID,8);
            read(fd1,&buffer8,8); //minutes
            read(fd1,&buffer4,4); //hours
            read(fd1,&buffer1,1); //days
            write(tasks_fd,&buffer8,8);
            write(tasks_fd,&buffer4,4);
            write(tasks_fd,&buffer1,1);
            read(fd1,&buffer4,4); //ARGC
            uint32_t new_argc = htobe32(buffer4);
            write(tasks_fd,&buffer4,sizeof(uint32_t));
            for(int i = 0;i<new_argc;i++){
                read(fd1,&buffer4,4);//string: length + string
                write(tasks_fd,&buffer4,sizeof(uint32_t));
            }
            write(tasks_fd,&spacebuf,1);
            write(tasks_res_fd,&new_taskID,sizeof(uint64_t));
            uint32_t sizel = htobe32(0);
            write(tasks_res_fd,&sizel,sizeof(uint32_t));
            char tmp;
            write(tasks_res_fd,&tmp,sizel);
            uint16_t tmp2 = (uint16_t) 0;
            write(tasks_res_fd,&tmp2,sizeof(uint16_t));
            write(tasks_res_fd,&spacebuf,1);
            write(fd2,&ok,2);
            write(fd2,&new_taskID,8);
            break;
        }
        case CLIENT_REQUEST_GET_STDOUT:
     		read(fd2,&buffer2,8);
     		read(fd2,&new_taskID,sizeof(uint64_t));
        	char buf_oid[256];
        	snprintf(buf_oid,sizeof(buf_oid), "%"PRIu64, new_taskID); //we convert the taskid from uint64 to string
        	if(err_type_for_stdout(buf_oid)){
        		write(fd1,&ok,sizeof(uint16_t)); //we send OK and then the converted taskid
        		write(fd1,&buf_oid,sizeof(buf_oid));
     		}else{
        		write(fd1,&error,sizeof(uint16_t));
        		write(fd1,&not_found,sizeof(uint16_t));
      		}
      		break;
    	case CLIENT_REQUEST_REMOVE_TASK:
      		read(fd2,&buffer2,8);
      		read(fd2,&new_taskID,sizeof(uint64_t));
      		char buf_rid[256];
        	snprintf(buf_rid,sizeof(buf_rid), "%"PRIu64, new_taskID);
      		if(err_type_for_stdout(buf_rid)){
        		write(fd1,&ok,sizeof(uint16_t)); //we send OK 
      		}else{
        		write(fd1,&error,sizeof(uint16_t));
        		write(fd1,&not_found,sizeof(uint16_t));
      		}
      		break;
        case CLIENT_REQUEST_GET_STDERR:
            read(fd2,&taskID,sizeof(uint64_t)); // reading the taskid
            memset(taskidbuf, 0x00, 64);
            sprintf(taskidbuf, "%"PRIu64, htobe64(taskID)); //converting taskid to string
            strcat("/tasks/",taskidbuf);
            tasks_errors_fd = open(strcat(taskidbuf,"/task_errors.txt"),O_RDONLY); //open the error text file of the specific task
            if (tasks_errors_fd<0) {
              //error task not found
              write(fd2,&error,sizeof(uint16_t));
              write(fd2,&not_found,sizeof(uint16_t));
              close(tasks_errors_fd);
              break;
            }
            if (read(tasks_errors_fd,&taskerrorbuf,sizeof(taskerrorbuf))<=0) { //error task never run
              write(fd2,&error,sizeof(uint16_t));
              write(fd2,&never_run,sizeof(uint16_t));
              close(tasks_errors_fd);
              break;
            }
            write(fd2,&ok,sizeof(uint16_t)); //all good we return ok and the error string
            write(fd2,&taskerrorbuf,sizeof(taskerrorbuf));
            close(tasks_errors_fd);
            break;
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
            read(fd2,&taskID,sizeof(uint64_t)); //reading the task id
            memset(taskidbuf, 0x00, 64);
            sprintf(taskidbuf, "%"PRIu64, htobe64(taskID));
            strcat("/tasks/",taskidbuf);
            tasks_stdout_fd = open(strcat(taskidbuf,"/task_stdout.txt"),O_RDONLY);
            if (tasks_stdout_fd<0) {
              //error task not found
              write(fd2,&error,sizeof(uint16_t));
              write(fd2,&not_found,sizeof(uint16_t));
              close(tasks_stdout_fd);
              break;
            }
            //Read the task text file to get time and return value
            close(tasks_stdout_fd);
            break;
        case CLIENT_REQUEST_TERMINATE:
            write(fd2,&ok,2);
            kill(getpid(),SIGKILL);
            break;
        default:
            write(fd2,&ok,2);
            write(fd2,&new_taskID,8);
            int nb;
            while((nb = read(tasks_fd,&buffer8,8))>0 ){
                write(fd2,&buffer8,8);
                read(tasks_fd,&buffer8,8);
                write(fd2,&buffer8,8);
                read(tasks_fd,&buffer4,4);
                write(fd2,&buffer4,4);
                read(tasks_fd,&buffer1,1);
                write(fd2,&buffer1,1);
                read(tasks_fd,&buffer4,4);
                write(fd2,&buffer4,4);
                uint32_t new_arg = htobe32(buffer4);
                for(int i = 0; new_arg; i++){
                    read(tasks_fd,&buffer4,4);
                    write(fd2,&buffer4,4);
                }
                char tmp;
                read(tasks_fd,&tmp,1);
            }
            goto error;
    }

    close(tasks_fd);
    close(tasks_res_fd);
    close(fd1);//we close our request pipe
    close(fd2);//we close our reply pipe
    return EXIT_SUCCESS;
    error:
        close(tasks_fd);
        close(tasks_res_fd);
        close(fd1);
        close(fd2);
        return EXIT_FAILURE;
}
