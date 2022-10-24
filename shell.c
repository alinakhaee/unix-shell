#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>

char *read_fifo, *write_fifo;

void *read_thread(void *vargp){
    int fd_read = open(read_fifo, O_RDONLY|O_NONBLOCK);
    char s[100];
    while (1){
        char  get_message[100]="";
        read(fd_read, get_message, 100);
        if(strcmp(get_message, "")!=0){
            printf("\033[31;1m\nyou received : %s\n\033[0m", get_message);
            printf("\033[36;1m%s : -> \033[0m", getcwd(s, 100));
        }
        fflush(stdout);
        sleep(1);
    }
}
void pipeHandler(char * args[]){
    int odd_fd[2], even_fd[2];
    int counter = 0, end = 0, i = 0, j = 0, k = 0;
    
    char *command[512];
    
    while (args[i] != NULL){
        if (strcmp(args[i],"|") == 0)
            counter++;
        i++;
    }
    counter++;
    
    i=0;
    while (args[j] != NULL && end != 1){
        k = 0;
        while (strcmp(args[j],"|") != 0){
            command[k] = args[j];
            j++;
            if (args[j] == NULL){
                end = 1;
                k++;
                break;
            }
            k++;
        }
        command[k] = NULL;
        j++;    
        
        if(i%2 == 0) pipe(even_fd);
        else pipe(odd_fd);
        
        if(fork()==0){
            if(i == 0) dup2(even_fd[1], 1);
            else if(i == counter-1)
                if(counter % 2 == 0) dup2(even_fd[0],0);
                else dup2(odd_fd[0],0);
            else
                if(i % 2 == 0){
                    dup2(odd_fd[0],0); 
                    dup2(even_fd[1],1);
                }
                else{
                    dup2(even_fd[0],0); 
                    dup2(odd_fd[1],1);          
                } 
            if(execvp(command[0],command)<0){
                fprintf(stderr, "command doesn't exist\n");
                kill(getpid(),SIGTERM);
            }
        }
        else{
            wait(NULL);
            if(i == 0) close(even_fd[1]);
            else if(i == counter-1)
                if(counter % 2 == 0) close(even_fd[0]);
                else close(odd_fd[0]);
            else
                if(i % 2 == 0){          
                    close(odd_fd[0]);
                    close(even_fd[1]);
                }
                else{          
                    close(even_fd[0]);
                    close(odd_fd[1]);
                }  
            i++;
        }  
    }
}
void add_to_history(char* command){
    FILE *file = fopen("/home/ali/history.txt", "a");
    fputs(command, file);
    fclose(file);
}
int clear_history(){
    FILE *file = fopen("/home/ali/history.txt", "w");
    fclose(file);
    return 0;
}
int show_history(){
    int counter=1;
    FILE *file = fopen("/home/ali/history.txt", "r");
    char line[512];
    while(fgets(line, 512, file)!=NULL)
        printf("%d- %s", counter++, line);
    fclose(file);
    return 0;
}
int cd(char* address){
    if(address==NULL || strcmp(address, "~")==0) chdir("/home/ali");
    else if(chdir(address)!=0)
      fprintf(stderr, "no such directory\n");
    return 0;
}
int send_message(char* words[20], int number){
    char message[100]=" "; int i=0;
    for(i=1 ; i<number ; i++){
        strcat(message, words[i]);
        strcat(message, " ");
    }
    int fd_write = open(write_fifo, O_WRONLY);
    write(fd_write, message, strlen(message)+1);
    close(fd_write);
    return 0;
}
int execute_line(char* line){
    add_to_history(line);
    if(strcmp(line, "exit\n")==0) return 1;
    if(strcmp(line, "clhis\n")==0) return clear_history();
    if(strcmp(line, "history\n")==0) return show_history();
    char *parsed[20], line_copy[1024];
    strcpy(line_copy, line);
    int numTokens=1;
    parsed[0] = strtok(line_copy, " \n\t");
    while((parsed[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;
    if(strcmp(parsed[0], "cd")==0) return cd(parsed[1]);
    if(strcmp(parsed[0], "msg")==0) return send_message(parsed, numTokens);
    if(strchr(line, '|')!=NULL)
        pipeHandler(parsed);
    else
        if(fork()==0){
            if(execvp(parsed[0], parsed)<0){
                fprintf(stderr, "command doesn't exist\n");
                kill(getpid(),SIGTERM);
            }
        }
        else wait(NULL);
    return 0;
}
void batch_mode(char* file_address){
    char line[1024];
    FILE *file = fopen(file_address, "r");
    while(fgets(line, 512, file)!=NULL){
        if(strcmp(line, "\n")==0) continue;
        printf("\033[36;1mcommand is : \033[0m");
        printf("%s", line);
        if(execute_line(line)==1) break;
        printf("\n");
    }
}
void set_fifo(){
    mkfifo("/tmp/fifo1", 0666);
    mkfifo("/tmp/fifo2", 0666);

    char temp[5];
    FILE *file = fopen("instance.txt", "r");
    int counter=0;
    while(fgets(temp, 5, file)!=NULL) counter++;
    fclose(file);
    if(counter==1){
        FILE *file = fopen("instance.txt", "a");
        fputs("2\n", file);
        fclose(file);
    }
    if(counter==0 || counter>=2){
        FILE *file = fopen("instance.txt", "w");
        fputs("1\n", file);
        fclose(file);
    }
    read_fifo = counter==1 ? "/tmp/fifo1" : "/tmp/fifo2";
    write_fifo = counter==1 ? "/tmp/fifo2" : "/tmp/fifo1";
}
void interactive_mode(){
    set_fifo();
    pthread_t tid1;
    pthread_create(&tid1, NULL, read_thread, NULL);

    char line[1024], s[100];
    while(1){
        printf("\033[36;1m%s : -> \033[0m", getcwd(s, 100));
        if(fgets(line, 1024, stdin)==NULL){
            printf("^D signal received. Shell Terminated\n");
            exit(1);
        }
        if(strlen(line)>512){
            fprintf(stderr, "command length more than 512\n");
            continue;
        }
        if(strcmp(line, "\n")==0) continue;
        if(execute_line(line)==1) break;
    }
}
void handle_signal(int sig){
    printf(" signal received. Command Terminated\n");
}

int main(int argc, char *argv[]){
  if(argc>2){
    fprintf(stderr, "wrong number of arguments\n");
    return 1;
  }
  if(argc==2){
    if(access(argv[1], F_OK) != 0){
      fprintf(stderr, "file doesnt't exist\n");
      return 1;
    }

    batch_mode(argv[1]);
    return 0;
  }

  signal(SIGINT, handle_signal);
  interactive_mode();
  return 0;  
}
