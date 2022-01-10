#include "cassini.h"

int main(char * pipes_directory) {
    int fd1,fd2;
    char * rq_p = NULL;
    char * rp_p = NULL;
    rq_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-request-pipe")));
    rp_p = (char *) malloc((sizeof(pipes_directory)*strlen(pipes_directory)+sizeof("/saturnd-reply-pipe")));
    strcpy(rp_p,pipes_directory); //we copy our pipes_directory into a new var
    strcpy(rq_p,pipes_directory); //we copy our pipes_directory into a new var
    fd2 = open(strcat(rp_p,"/saturnd-reply-pipe"), O_WRONLY);//we open our reply pipe in write only mode
    fd1 = open(strcat(rq_p,"/saturnd-request-pipe"), O_RDONLY);//we open our request pipe in read only mode
    free(rq_p);
    free(rp_p);
    if (fd1 < 0 || fd2 < 0){ //if we can't open one of the two pipes, we go to error
        goto error;
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
