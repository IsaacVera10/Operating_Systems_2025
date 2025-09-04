#include<sys/wait.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void create_pipe(int *pipefd){
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }
}

int create_child(){
    int pid = fork();
    return pid;
}

int main(){
    int pipefd[2];
    char buff[256];
    int pid;

    create_pipe(pipefd);

    while(1){
        pid = create_child();
        if(pid == 0){ // Child process
            printf("Child process created with PID %d\n", getpid());
            /*
                1. Recibir entrada del usuario y almacenarla en buff
                2. Enviarla al pipe
                3. Terminar el proceso hijo
            */
            // Paso 1
            printf("Enter a message: ");
            fgets(buff, sizeof(buff), stdin);
            //limpiar el pipe
            write(pipefd[1], buff, sizeof(buff)); //Escritura
            close(pipefd[1]);
            exit(0);
        }else{// Parent process
            printf("Parent process with PID %d\n", getpid());
            /*
                1. Esperar a que el hijo termine de enviar el mensaje
                2. Leer el contenido del pipe y almacenarlo en buff
            */
            wait(NULL);
            read(pipefd[0], buff, sizeof(buff)); // Lectura
            //Pasar el puntero de lectura al inicio del buffer
            lseek(pipefd[0], 0, SEEK_SET);
            printf("Child sent message: %s\n\n", buff);
            close(pipefd[0]);
        }
    }

    return 0;
}