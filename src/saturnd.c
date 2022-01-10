#include <stdio.h>
#include <inttypes.h>
#include <server-reply.h>
#include "cassini.h"
#include <poll.h>

// boolean applyRequest(uint16_t op){
//   switch (op) {
//     case CLIENT_REQUEST_CREATE_TASK:
//       //ajouter une nouvelle task dans le fichier tasks avec les infos necessaire
//       read();
//       read();
//       read();
//       break;
//     case CLIENT_REQUEST_TERMINATE:
//       //terminer le demon
//       break;
//     case CLIENT_REQUEST_GET_STDOUT:
//       break;
//     case CLIENT_REQUEST_REMOVE_TASK:
//
//       break;
//     case CLIENT_REQUEST_GET_STDERR:
//
//       break;
//     case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
//       break;
//   }
// }

int main(int argc, char const *argv[]) {

  errno = 0;

  char * pipes_directory = NULL;
  int fd1,fd2;
  char * rq_p = NULL;
  char * rp_p = NULL;
  rq_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-request-pipe")));
  rp_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-reply-pipe")));
  strcpy(rp_p,pipes_directory); //we copy our pipes_directory into a new var
  strcpy(rq_p,pipes_directory); //we copy our pipes_directory into a new var
  int tasks_fd = open("/tasks.txt",O_WRONLY);
  fd1 = open(strcat(rp_p,"/saturnd-reply-pipe"), O_WRONLY);//we open our request pipe in write only mode
  fd2 = open(strcat(rq_p,"/saturnd-request-pipe"), O_RDONLY);//we open our reply pipe in read only mode
  free(rq_p);
  free(rp_p);
  if (fd1 < 0 || fd2 < 0){ //if we can't open one of the two pipes, we go to error
    goto error;
  }

  uint64_t taskid;
  uint16_t buffer2; //buffer for uint16_t

  int success;
  struct timing t;

  struct pollfd pfd[2];
  pfd[0].fd = fd2;
  pfd[0].events = POLLIN;
  pfd[1].fd = fd1;
  pfd[1].events = POLLIN;

  while (1) {
    poll(pfd,2,3);

    if(pfd[0].revents & POLLIN){
      uint16_t operation = read(fd2,&buffer2,2);

      switch (operation) {
        case CLIENT_REQUEST_CREATE_TASK:
          //ajouter une nouvelle task dans le fichier tasks avec les infos necessaire
          read(fd2,&t.minutes,sizeof(uint64_t));
          read(fd2,&t.hours,sizeof(uint32_t));
          read(fd2,&t.daysofweek,sizeof(uint8_t));


          if(tasks_fd<0)goto error;
          write(tasks_fd,&t.minutes,sizeof(uint64_t));
          write(tasks_fd,&t.hours,sizeof(uint32_t));
          write(tasks_fd,&t.daysofweek,sizeof(uint8_t));
          char spacebuf = '\n';
          write(tasks_fd,&spacebuf,1);

          //read la ligne de commande et la reecrire dans tasks.txt
          break;
        case CLIENT_REQUEST_TERMINATE:
          //terminer le demon
          break;
        case CLIENT_REQUEST_GET_STDOUT:
          break;
        case CLIENT_REQUEST_REMOVE_TASK:

          break;
        case CLIENT_REQUEST_GET_STDERR:

          break;
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
          break;
      }
      uint16_t new_rsp = (success)?htobe16(SERVER_REPLY_OK):htobe16(SERVER_REPLY_ERROR);
      uint16_t nf_err = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
      uint16_t nr_err = htobe16(SERVER_REPLY_ERROR_NEVER_RUN);

      switch (operation) {
        case CLIENT_REQUEST_CREATE_TASK:
          break;
        case CLIENT_REQUEST_TERMINATE:
          // write(fd1,&resp_ok,sizeof(uint16_t));
          break;
        case CLIENT_REQUEST_GET_STDOUT:
          // int replen = read(fd2,&taskid,8);
          // if(replen == 8){
          //   success = true; //we set success to true for new_rsp
          //   char buf_stdout[256];
          //   snprintf(buf_stdout,sizeof(buf_stdout), "%"PRIu64, &taskid); //we convert the taskid from uint64 to string
          //   write(fd1,&new_rsp,sizeof(uint16_t)); //we send OK and then the converted taskid
          //   write(fd1,&buf_stdout,sizeof(buf_stdout));
          // }else{
          //   success = false;
          //   write(fd1,&new_rsp,sizeof(uint16_t));
          //   write(fd1,&nr_err,sizeof(uint16_t)); //ou nf il faut encore les differencier
          // }
          break;
        case CLIENT_REQUEST_REMOVE_TASK:
          // int replen = read(fd2,&taskid,8);
          // if(replen == 8){
          //   success = true; //we set success to true for new_rsp
          //   write(fd1,&new_rsp,sizeof(uint16_t)); //we send OK
          // }else{
          //   success = false;
          //   write(fd1,&new_rsp,sizeof(uint16_t));
          //   write(fd1,&nf_err,sizeof(uint16_t));
          // }
          break;
        case CLIENT_REQUEST_GET_STDERR:
          // write(fd1,&new_rsp,sizeof(uint16_t));
          break;
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
          break;

      }

    }

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
