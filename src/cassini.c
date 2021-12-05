#include "cassini.h"
#include "stdint.h"

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
  int fd;
  int fd1,fd2;
  fd1 = open("./run/pipes/saturnd-request-pipe", O_WRONLY);//we open our request pipe in write only mode
  fd2 = open("./run/pipes/saturnd-reply-pipe", O_RDONLY);//we open our reply pipe in read only mode
  if (fd1 < 0 || fd2 < 0){
    goto error;
  }
  int nb;
  char buf[1025];
  char buf2[1025];
  uint64_t buffer8;
  uint32_t buffer4;
  uint16_t buffer2;
  uint8_t buffer1;
  // we then convert our operation to big endian if needed
  uint16_t new_opr = htobe16(operation);
  uint64_t new_task = htobe64(taskid);
  //then we start a switch to send to our client a request according to the operation specified
  switch(operation){
    case CLIENT_REQUEST_CREATE_TASK:
      write(fd1,&new_opr,sizeof(uint16_t));//"CR"
      //timing
      struct timing t;
      timing_from_strings(&t,minutes_str,hours_str,daysofweek_str);
      t.minutes= htobe64(t.minutes);
      t.hours= htobe32(t.hours);
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
      write(fd1,&new_argc,sizeof(uint32_t));
      for(int i=count;i<argc;i++){
        int size_l = strlen(argv[i]);
        uint32_t size_l2 = htobe32(size_l);
        write(fd1,&size_l2,sizeof(uint32_t));
        write(fd1,argv[i],size_l);
      }
      //reply responde on stdout
      nb = read(fd2,&buffer2,2);
      nb = read(fd2,&buffer8,8);
      printf("%ld\n", be64toh(buffer8));
      break;
    case CLIENT_REQUEST_TERMINATE://Terminate just like List takes an unsigned integer of 16 bytes previously converted to big endian
      write(fd1,&new_opr,sizeof(uint16_t));
      break;
    case CLIENT_REQUEST_GET_STDOUT:
      write(fd1,&new_opr,sizeof(uint16_t));
      write(fd1,&new_task,sizeof(uint64_t));
      if((nb = read(fd2, buf,1024)) >= 0){
    		buf[nb] = 0;
    		printf("%s",buf+6);
    	} else{
        close(fd2);
    		goto error;
    	}
    	if(buf[0] == 'E'){
    		exit(1);
    	}
      break;
    case CLIENT_REQUEST_REMOVE_TASK:
      write(fd1,&new_opr,sizeof(uint16_t));
      write(fd1,&new_task,sizeof(uint64_t));
      break;
    default:// ls for now is default
      write(fd1,&new_opr,sizeof(uint16_t));
      break;
  }
  close(fd1);//we close our request pipe
  close(fd2);//we close our reply pipe
  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
