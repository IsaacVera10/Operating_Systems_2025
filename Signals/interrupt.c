#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#define FILENAME "temp.txt"

int fd;

static void myHandler(int signum){
    printf("Deleting file %s\n", FILENAME);

    close(fd);
    remove(FILENAME);
    exit(0);
}

int main(){
    fd = open(FILENAME, O_CREAT);
    unsigned long int i=0;

    int iRet;
    struct sigaction sAction;
    sAction.sa_flags = 0;
    sAction.sa_handler = myHandler;
    sigemptyset(&sAction.sa_mask);
    iRet = sigaction(SIGINT, &sAction, NULL);
    assert(iRet == 0);

    while(1){
        printf("Working with file %s...press Ctrl+C to delete it\n", FILENAME);
        sleep(1);
    }

    return 0;
}