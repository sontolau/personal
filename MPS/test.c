#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main ()
{
    int oldfd,newfd;


    oldfd = open ("./test.c", O_RDWR);
    if (oldfd < 0) {
        fprintf (stderr, "open failed");
        exit (EXIT_FAILURE);
    }

    newfd = 101;
    if (dup2 (oldfd, newfd) < 0) {
        fprintf (stderr, "dup2 failed");
        perror ("dup2");
        exit (EXIT_FAILURE);
    } 

    close (oldfd);
    printf ("%d,%d\n", newfd,oldfd);
    char buf[5] = {0};
    if (read (newfd, buf, 4) < 0) {
        fprintf (stderr, "read failed");
        exit (1);
    }

    printf ("%s\n", buf);

    close (oldfd);
    close (newfd);
    return 0;
} 
