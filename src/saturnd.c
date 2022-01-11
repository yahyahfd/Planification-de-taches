#include "cassini.h"

int main(int argc, char * argv[]) {
    int fd1,fd2;
    // char * rq_p = NULL;
    // char * rp_p = NULL;
    // char * pipes_directory = argv[1];
    // rq_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-request-pipe")));
    // rp_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-reply-pipe")));
    // strcpy(rp_p,pipes_directory); //we copy our pipes_directory into a new var
    // strcpy(rq_p,pipes_directory); //we copy our pipes_directory into a new var
    // fd1 = open(strcat(rq_p,"/saturnd-request-pipe"), O_RDONLY);//we open our request pipe in read only mode
    // fd2 = open(strcat(rp_p,"/saturnd-reply-pipe"), O_WRONLY);//we open our reply pipe in write only mode
    // free(rq_p);
    // free(rp_p);
    fd1 = open("./run/pipes/saturnd-request-pipe", O_RDONLY);//we open our request pipe in read only mode
    fd2 = open("./run/pipes/saturnd-request-pipe", O_WRONLY);//we open our reply pipe in write only mode
    if (fd1 < 0 || fd2 < 0){ //if we can't open one of the two pipes, we go to error
        goto error;
    }

    // uint64_t taskid;
    uint16_t buffer;
    read(fd1,&buffer,2);
    uint16_t operation = htobe16(buffer); 
    struct timing t;
    int tasks_fd = open("tasks.txt", O_WRONLY |O_CREAT | O_APPEND);
    switch(operation){
        case CLIENT_REQUEST_CREATE_TASK:
            read(fd2,&t.minutes,sizeof(uint64_t));
            read(fd2,&t.hours,sizeof(uint32_t));
            read(fd2,&t.daysofweek,sizeof(uint8_t));

            if(tasks_fd<0) goto error;
            write(tasks_fd,&t.minutes,sizeof(uint64_t));
            write(tasks_fd,&t.hours,sizeof(uint32_t));
            write(tasks_fd,&t.daysofweek,sizeof(uint8_t));
            char spacebuf = '\n';
            write(tasks_fd,&spacebuf,1);
            break;
        case CLIENT_REQUEST_TERMINATE:
            kill(getpid(),SIGKILL);
            break;
        default:
            goto error;
    }
    

    // close(tasks_fd);
    close(fd1);//we close our request pipe
    close(fd2);//we close our reply pipe
    return EXIT_SUCCESS;
    error:
        close(fd1);
        close(fd2);
        return EXIT_FAILURE;
}
