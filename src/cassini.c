#include "cassini.h"

const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

int main(int argc, char * argv[]) {
  errno = 0;

  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;

  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;

  int opt;
  char * strtoull_endp;
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }

  // TESTS
  // TEST 1 : -l
  // TEST 2 : -c echo test-1
  // TEST 3 : -l
  // TEST 4 : -c echo test-2
  // TEST 5 : -c -m 1 -H 2 -d * date
  // TEST 6 : -c -H 1,3,5,7 -m 1 date
  // TEST 7 : -c -d 1,3 -m 4,10 ls NONEXISTENT_FILE
  // TEST 8 : -l
  // TEST 9 : -r 2
  // TEST 10 : -c -m 1,3-6,9-15 date
  // TEST 11 : -x 0
  // TEST 12 : -o 0
  // TEST 13 : -e 4
  // TEST 14 : -x 60
  // TEST 15 : -o 3
  // TEST 16 : -q

  // --------
  // | TODO |
  // --------
  // We first open the pipe in write only mode
  int fd1,fd2;
  char * rq_p = NULL;
  char * rp_p = NULL;
  rq_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-request-pipe")));
  rp_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-reply-pipe")));
  strcpy(rp_p,pipes_directory);
  strcpy(rq_p,pipes_directory);
  fd1 = open(strcat(rq_p,"/saturnd-request-pipe"), O_WRONLY);//we open our request pipe in write only mode
  fd2 = open(strcat(rp_p,"/saturnd-reply-pipe"), O_RDONLY);//we open our reply pipe in read only mode
  free(rq_p);
  free(rp_p);
  if (fd1 < 0 || fd2 < 0){ //if we can't open one of the two pipes, we go to error
    goto error;
  }
  int nb; //Stores return values of reads, we don't check it because it's always > 0 since we opened fd2 in read only mode
  char buf[1025] = {}; //Buffer used for CLIENT_REQUEST_GET_STDOUT
  char * buf2 = NULL; //Buffer used for ls commandline's string part
  // Not very efficient way of using buffers, but still works
  uint64_t buffer8; //buffer for uint64_t
  uint32_t buffer4; //buffer for uint32_t
  uint16_t buffer2; //buffer for uint16_t
  uint8_t buffer1; //buffer for uint8_t
  int64_t buff8;  //buffer for int64_t

  //we then convert our operation  and taskid to big endian if needed
  uint16_t new_opr = htobe16(operation);
  uint64_t new_task = htobe64(taskid);
  //then we start a switch to send to our client a request according to the operation specified and print to STDOUT the reply result when needed
  switch(operation){
    case CLIENT_REQUEST_CREATE_TASK:
      write(fd1,&new_opr,sizeof(uint16_t));//"CR"
      //timing
      struct timing t;
      timing_from_strings(&t,minutes_str,hours_str,daysofweek_str); //we create a new timing structure using the information we got
      t.minutes= htobe64(t.minutes); //and we sitll make sure to convert minutes to big endian
      t.hours= htobe32(t.hours); //same with hours
      //we now write to fd1 all this data
      write(fd1,&t.minutes,sizeof(uint64_t));
      write(fd1,&t.hours,sizeof(uint32_t));
      write(fd1,&t.daysofweek,sizeof(uint8_t));
      //commandline
      int count=4; //counting how many args our commandline has
      for(int i=4;i<argc;i++){
        if(argv[i][0]=='-'){//we skip options (and their single argument:minutes,hours,days)
          count+=2;
        }
      }
      uint32_t new_argc = htobe32(argc-count); //our new final argc count
      write(fd1,&new_argc,sizeof(uint32_t)); //we write it
      for(int i=count;i<argc;i++){
        int size_l = strlen(argv[i]);
        uint32_t size_l2 = htobe32(size_l);
        write(fd1,&size_l2,sizeof(uint32_t)); //we write the size of the current argv
        write(fd1,argv[i],size_l); //then we write the current argv
      }
      //reply's response on stdout
      nb = read(fd2,&buffer2,2); //we skip the two first bytes
      nb = read(fd2,&buffer8,8); //task_id of the new task
      printf("%ld\n", htobe64(buffer8)); //we print this task_id
      break;
    case CLIENT_REQUEST_TERMINATE://Terminate just like List takes an unsigned integer of 16 bytes previously converted to big endian
      write(fd1,&new_opr,sizeof(uint16_t));
      break;
    case CLIENT_REQUEST_GET_STDOUT:
      write(fd1,&new_opr,sizeof(uint16_t)); //we write the appropriate operation 
      write(fd1,&new_task,sizeof(uint64_t)); //then we do the same with its task
      if((nb = read(fd2, buf,1024)) >= 0){ //we retrieve the content of the reply pipe and if the buffer nb retrieved all of it without errors
    		buf[nb] = 0; //we close the string
    		printf("%s",buf+6); //and we print the appropriate output
    	} else{ 
    		goto error; //if an error occured with the buffer
    	}
    	if(buf[0] == 'E'){ //If we have 'ERNR' as response instead of 'OK'
      	goto error;
    	}
      break;
    case CLIENT_REQUEST_REMOVE_TASK:
      write(fd1,&new_opr,sizeof(uint16_t)); //Same as with STDOUT, the difference being there's no output to print
      write(fd1,&new_task,sizeof(uint64_t));
      break;
    case CLIENT_REQUEST_GET_STDERR:
      write(fd1,&new_opr,sizeof(uint16_t)); //Same as for STDOUT
      write(fd1,&new_task,sizeof(uint64_t));
      if((nb = read(fd2, buf,1024)) >= 0){
    		buf[nb] = 0;
    		printf("%s",buf+6);
    	} else{
    		goto error;
    	}
    	if(buf[0] == 'E'){
    		goto error;
    	}
      break;
    case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
      write(fd1,&new_opr,sizeof(uint16_t));
      write(fd1,&new_task,sizeof(uint64_t));
      nb = read(fd2,&buffer2,2);           //Reading the response from deamon

      if (htobe16(buffer2) == 20299) {    //if its OK(20299)
        nb = read(fd2,&buffer4,4);        //Reading how many times the task runs
        uint32_t nb_runs = htobe32(buffer4);

        int64_t tmp;
        struct tm * letemps;      //Converting the time to a struct form
        for (int i = 0; i < nb_runs; i++) {
          nb = read(fd2,&buff8,8);
          tmp = htobe64(buff8);
          letemps = localtime(&tmp);
          printf( "%04d-%02d-%02d %02d:%02d:%02d",      //Printing the time
          letemps->tm_year+1900, letemps->tm_mon+1, letemps->tm_mday,
          letemps->tm_hour, letemps->tm_min, letemps->tm_sec);
          nb = read(fd2,&buffer2,2);
          printf(" %d\n", htobe16(buffer4));    //Printing the return value
        }
      }else{
         goto error;    //if the response is not a 'OK'
      }
      break;
    default: // "-l" option or ls is default
      write(fd1,&new_opr,sizeof(uint16_t)); //we write this operation
      nb = read(fd2,&buffer2,2); //we skip two bytes
      nb = read(fd2,&buffer4,4);
      uint32_t nb_tasks = htobe32(buffer4); //NBTASKS
      for(int i=0;i<nb_tasks;i++){
        nb = read(fd2,&buffer8,8);//task_id
        printf("%ld: ", htobe64(buffer8));
        nb = read(fd2,&buffer8,8);//mins
        uint64_t new_mins = htobe64(buffer8);
        if(new_mins & (1UL << 59)){ //if there is one more bit than needed, that means mins is undefined
          printf("*");
        }else{
          int res_d [60] = {0};
          int count = 0;
          for(int i = 0;i<59;i++){ //we store bits that are equal to one in res_d, and increment count each time there is one
            if(new_mins & (1UL << i)){
              res_d[i] = 1;
              count++;
            }
          }
          for(int i=0; i<59;i++){ //we print our results the way needed
            if(res_d[i]!=0){
              printf("%d",i);
              if(count>1){
                printf(",");
                count--;
              }
            }
          }
        }

        nb = read(fd2,&buffer4,4);//hours
        uint32_t new_hrs = htobe32(buffer4);
        if(new_hrs & (1UL << 23)){  //if there is one more bit than needed, that means hours is undefined
          printf(" * ");
        }else{
          printf(" ");
          int res_d [24] = {0};
          int count = 0;
          for(int i = 0;i<23;i++){ //we store bits that are equal to one in res_d, and increment count each time there is one
            if(new_hrs & (1UL << i)){
              res_d[i] = 1;
              count++;
            }
          }
          for(int i=0; i<23;i++){ //we print our results the way needed
            if(res_d[i]!=0){
              printf("%d",i);
              if(count>1){
                printf(",");
                count--;
              }
            }
          }
          printf(" ");
        }

        nb = read(fd2,&buffer1,1);//days
        if(buffer1 & (1UL << 6)){ //if there is one more bit than needed, that means days is undefined
          printf("*");
        }else{
          int res_d [7] = {0};
          int count = 0;
          for(int i = 0;i<6;i++){ //we store bits that are equal to one in res_d, and increment count each time there is one
            if(buffer1 & (1UL << i)){
              res_d[i] = 1;
              count++;
            }
          }
          for(int i=0; i<6;i++){ //we print our results the way needed
            if(res_d[i]!=0){
              printf("%d",i);
              if(count>1){
                printf(",");
                count--;
              }
            }
          }
          // for "-", need aux function -> not taken into account in the tests
          // int pos = 0;
          // int start = 0;
          // int end = 0;
          // for(int i=0;i<5;i++){
          //   if(res_d[i]!=0 && res_d[i+1]!=0){
          //     start=i;
          //     end=i+1;
          //   }
          // }
        }

        nb = read(fd2,&buffer4,4);
        uint32_t arg_count = htobe32(buffer4); //the argc of commands inside commandline
        for(int i=0; i<arg_count;i++){
          nb = read(fd2,&buffer4,4);
          uint32_t len_argv = htobe32(buffer4); //length of the current argv
          buf2 = (char*) malloc(len_argv+1); //we allocate the exact size needed to store argv
          nb = read(fd2,buf2,len_argv);
          printf(" %s",buf2); //then we print argv
          free(buf2); //and we don't forget our free()
        }
        printf("\n");
      }
      break;
  }
  close(fd1);//we close our request pipe
  close(fd2);//we close our reply pipe
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_SUCCESS;

 error:
  close(fd1);
  close(fd2);
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
