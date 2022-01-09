

int main(int argc, char const *argv[]) {


  int fd1,fd2;
  char * rq_p = NULL;
  char * rp_p = NULL;
  rq_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-request-pipe")));
  rp_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-reply-pipe")));
  strcpy(rp_p,pipes_directory); //we copy our pipes_directory into a new var
  strcpy(rq_p,pipes_directory); //we copy our pipes_directory into a new var
  fd1 = open(strcat(rp_p,"/saturnd-reply-pipe"), O_WRONLY);//we open our request pipe in write only mode
  fd2 = open(strcat(rq_p,"/saturnd-request-pipe"), O_RDONLY);//we open our reply pipe in read only mode
  free(rq_p);
  free(rp_p);
  if (fd1 < 0 || fd2 < 0){ //if we can't open one of the two pipes, we go to error
    goto error;
  }

  uint64_t taskid;
  uint16_t buffer2; //buffer for uint16_t
  uint16_t operation = read(fd2,&buffer2,2);
  boolean success;

  uint16_t new_rsp = (success)?htobe16(SERVER_REPLY_OK):htobe16(SERVER_REPLY_ERROR);

  switch (operation) {
    case CLIENT_REQUEST_CREATE_TASK:
      break;
    case CLIENT_REQUEST_TERMINATE:
      break;
    case CLIENT_REQUEST_GET_STDOUT:
      break;
    case CLIENT_REQUEST_REMOVE_TASK:
      break;
    case CLIENT_REQUEST_GET_STDERR:
      write(fd1,&new_rsp,sizeof(uint16_t));
      break;
    case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
      break;
    case
  }
  return 0;
  error:
   close(fd1);
   close(fd2);
   if (errno != 0) perror("main");
   free(pipes_directory);
   pipes_directory = NULL;
   return EXIT_FAILURE;
}
