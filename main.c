//
//  main.c
//  yash
//
//  Created by Matt Owens on 9/12/16.
//  Copyright © 2016 Matt Owens. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

int status;
pid_t mainPID;
pid_t foregroundPID;



/* SIGNAL HANDLER */
void signalHandler(int signalType){
    //printf("Signal being handled");
    
    if(signalType == SIGINT){
        //printf("SIGINT being handled");
        //printf("SIGINT being handled");
        //printf("SIGINT being handled");
        //printf("SIGINT being handled");
        
        if(foregroundPID != 0){
            printf("SIGINT being handled\n");
            printf("pid, child's pid: %d, %d\n\n", getpid(), foregroundPID);
            kill(foregroundPID, SIGKILL);
        }
        foregroundPID = 0;
    } else if(signalType == SIGTSTP){
        if(foregroundPID != 0){
            printf("SIGTSTP being handled\n");
            printf("pid, child's pid: %d, %d\n\n", getpid(), foregroundPID);
        }
        foregroundPID = 0;
    }
}



/* REDIRECTION HANDLER */
void handleRedirection(char ** cmdMinusRedirs, char ** cmdRedirs, int numRedirArgs){
    int j;
    for(j = 0; j < numRedirArgs; j+=2){
        int newfd;
        if(strcmp(cmdRedirs[j], "<") == 0){
            newfd = open(cmdRedirs[j+1], O_RDONLY);
            dup2(newfd, 0);
            //printf("Std in became %s\n", cmd1Redirs[j+1]);
        } if(strcmp(cmdRedirs[j], ">") == 0){
            newfd = open(cmdRedirs[j+1], O_CREAT | O_WRONLY, S_IRWXU);
            dup2(newfd, 1);
            //printf("Std out became %s\n", cmd1Redirs[j+1]);
            
        } if(strcmp(cmdRedirs[j], "2>") == 0){
            newfd = open(cmdRedirs[j+1], O_WRONLY);
            dup2(newfd, 2);
            //printf("Std err became %s\n", cmd1Redirs[j+1]);
        }
    }
}



/* DO COMMAND */
void doCommand(char ** cmd1MinusRedirs, char ** cmd1Redirs, int num1RedirArgs, bool bgd){
    /* FORKING STUFF */
    printf("\nParent  pid, pgid: %d, %d: \n", getpid(), getpgid(getpid()));
    pid_t rc = fork();
    
    if (rc < 0) {
        // fork failed; exit
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0) {
        setpgid(getpid(), getpid());
        printf("\nChild pid, pgid: %d, %d: \n", getpid(), getpgid(getpid()));
        handleRedirection(cmd1MinusRedirs, cmd1Redirs, num1RedirArgs);
        execvp(cmd1MinusRedirs[0], cmd1MinusRedirs);
    }
    else {
        //printf("Parent pid, pgid: %d, %d\n", (int)getpid(), (int)getpgid(getpid()));
        if(!bgd){
            //printf("Waited\n");
            foregroundPID = rc;
            waitpid(rc,&status,0);
            foregroundPID = 0;
        }
    }
}


/* PIPE COMMAND */
void pipeCommand(char ** cmd1MinusRedirs, char ** cmd1Redirs, int num1RedirArgs, char ** cmd2MinusRedirs, char ** cmd2Redirs, int num2RedirArgs, bool bgd){
    printf("\nParent  pid, pgid: %d, %d: \n", getpid(), getpgid(getpid()));
    /* PIPE STUFF */
    int pipefd[2];
    pid_t cpid;
    char buf;
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    /* FORK STUFF */
    pid_t pidc1 = fork();
    //child 1
    if(pidc1 == 0){
        setpgid(getpid(), pidc1);
        printf("\nChild pid, pgid, cmd: %d, %d, %s: \n", getpid(), getpgid(getpid()), cmd2MinusRedirs[0]);
        //printf("%s", cmd2MinusRedirs[0]);
        dup2(pipefd[0], 0);
        close(pipefd[1]);
        handleRedirection(cmd2MinusRedirs, cmd2Redirs, num2RedirArgs);
        execvp(cmd2MinusRedirs[0], cmd2MinusRedirs);
        
    }
    // parent and child 2
    else{
        pid_t pidc2 = fork();
        //child2
        if(pidc2 == 0){
            setpgid(getpid(), pidc1);
            printf("\nChild pid, pgid, cmd: %d, %d, %s: \n", getpid(), getpgid(getpid()), cmd1MinusRedirs[0]);
            //printf("%s", cmd1MinusRedirs[0]);
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            handleRedirection(cmd1MinusRedirs, cmd1Redirs, num1RedirArgs);
            execvp(cmd1MinusRedirs[0], cmd1MinusRedirs);
        }
        //parent
        else{
            if(!bgd){
                printf("Waited\n");
                //foregroundPID = rc;
                waitpid(pidc1 * -1,&status,WUNTRACED);
                printf("Done waiting for process 1");
                printf("Done waiting for process 2");
                //foregroundPID = 0;
                printf("Done waiting\n");
            }
            
        }
    }
    
    
    /*
    if (pidc1 == 0) {
        setpgid(getpid(), getpid());
        printf("\nChild pid, pgid: %d, %d: \n", getpid(), getpgid(getpid()));
        int pipefd[2];
        pid_t cpid;
        char buf;
        
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        
        pid_t rc2 = fork();
        if(rc2 == 0){
            printf("\nChild pid, pgid: %d, %d: \n", getpid(), getpgid(getpid()));
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            handleRedirection(cmd1MinusRedirs, cmd1Redirs, num1RedirArgs);
            execvp(cmd1MinusRedirs[0], cmd1MinusRedirs);
        }
        else{
            dup2(pipefd[0], 0);
            close(pipefd[1]);
            handleRedirection(cmd2MinusRedirs, cmd2Redirs, num2RedirArgs);
            execvp(cmd2MinusRedirs[0], cmd2MinusRedirs);
        }
    }
    else {
        if(!bgd){
            foregroundPID = rc;
            waitpid(rc,&status,WUNTRACED);
            foregroundPID = 0;
        }
    }*/
}


/* MAIN */
int main(int argc, const char * argv[]) {
    //pid_t recentSuspendedPID = -1;
    //mainPID = getpid();
    
    //CREATE SIGINT HANDLER
    if(signal(SIGINT, signalHandler) == SIG_ERR){
        printf("SIGINT error");
    }
    if(signal(SIGTSTP, signalHandler) == SIG_ERR){
        printf("SIGTSTP error");
    }
    
    //printf("%d", getpid());
    
    while(1){
        bool bgd = false;
        char * instructionFP;
        char * instructionSP;
        size_t maxCommSize = 2001;
        instructionFP = (char *)malloc(maxCommSize * sizeof(char));
        if(instructionFP == NULL){
            printf("Error mallocing instruction\n");
            exit(1);
        }
        
        //Get input
        printf("# ");
        //get input, trim new line, check for &
        if(getline(&instructionFP, &maxCommSize, stdin) == -1){
            printf("\n");
            exit(0);
        }
        
        int nLnFinder = 0;
        while(instructionFP[nLnFinder] != NULL){
            nLnFinder++;
        }
        instructionFP[nLnFinder-1] = NULL;
        if(strcmp(&instructionFP[nLnFinder-2], "&") == 0){
            bgd = true;
            //printf("BGD\n");
            instructionFP[nLnFinder-2] = NULL;
        }
        instructionSP = strdup(instructionFP);

        //get number of args -> cmdArgsSize
        char * token = strtok(instructionFP, " ");
        char ** cmdArgs;
        int cmdArgsSize = 0;
        while(token != NULL){
            cmdArgsSize++;
            token = strtok(NULL, " ");
        }
        cmdArgs = (char **)malloc(sizeof(char *) * (cmdArgsSize + 1));
        if(cmdArgs == NULL){
            printf("Error mallocing cmdArgs\n");
            exit(1);
        }
        
        //store args in a char ** -> cmdArgs
        token = strtok(instructionSP, " ");
        int i;
        for(i = 0; i < cmdArgsSize; i++){
            cmdArgs[i] = (char *)malloc(sizeof(char)*strlen(token));
            if(cmdArgs[i] == NULL){
                printf("Error mallocing cmdArgs[%d]\n", i);
                exit(1);
            }
            strcpy(cmdArgs[i], token);
            token = strtok(NULL, " ");
        }
        
        //trim redirects
        int r1 = 0;
        int m1 = 0;
        int r2 = 0;
        int m2 = 0;
        int cmd1NumRedirs = 0;
        int cmd2NumRedirs = 0;
        bool hitPipe = false;
        char ** cmd1MinusRedirs = (char**)malloc(sizeof(char *) * (cmdArgsSize + 1));
        char ** cmd1Redirs = (char**)malloc(sizeof(char *) * (3 + 1));
        char ** cmd2MinusRedirs = (char**)malloc(sizeof(char *) * (cmdArgsSize + 1));
        char ** cmd2Redirs = (char**)malloc(sizeof(char *) * (3 + 1));
        if(cmd1MinusRedirs == NULL || cmd2MinusRedirs == NULL || cmd1Redirs == NULL || cmd2Redirs == NULL){
            printf("Error with malloc for cmdRedirs\n");
            exit(1);
        }
        //seperate redirects from command and args
        for(i = 0; i < cmdArgsSize; i++){
            
            //post pipe redirect trimming
            if(hitPipe == true){
                //collect redirects
                if(strcmp(cmdArgs[i], ">") == 0 || strcmp(cmdArgs[i], "<") == 0 || strcmp(cmdArgs[i], "2>") == 0){
                    cmd2Redirs[r2] = strdup(cmdArgs[i]);
                    //printf("cmdRedir2 %d: %s\n", r2, cmd2Redirs[r2]);
                    r2++;
                    //free(cmdArgs[i]);
                    i++;
                    
                    cmd2Redirs[r2] = strdup(cmdArgs[i]);
                    //printf("cmdRedir2 %d: %s\n", r2, cmd2Redirs[r2]);
                    r2++;
                    cmd2NumRedirs+=2;
                    //free(cmdArgs[i]);
                }
                //else add arg to cmdMinusRedirs
                else{
                    cmd2MinusRedirs[m2] = strdup(cmdArgs[i]);
                    //printf("arg2 %d: %s\n", m2, cmd2MinusRedirs[m2]);
                    m2++;
                    //free(cmdArgs[i]);
                }
            }
            
            //pre pipe redirect trimming
            else{
                //hit the pipe
                if(strcmp(cmdArgs[i], "|") == 0){
                    hitPipe = true;
                }
                //collect redirects
                else if(strcmp(cmdArgs[i], ">") == 0 || strcmp(cmdArgs[i], "<") == 0 || strcmp(cmdArgs[i], "2>") == 0){
                    cmd1Redirs[r1] = strdup(cmdArgs[i]);
                    //printf("cmdRedir1 %d: %s\n", r1, cmd1Redirs[r1]);
                    r1++;
                    //free(cmdArgs[i]);
                    i++;
                    
                    cmd1Redirs[r1] = strdup(cmdArgs[i]);
                    //printf("cmdRedir1 %d: %s\n", r1, cmd1Redirs[r1]);
                    r1++;
                    cmd1NumRedirs+=2;
                    //free(cmdArgs[i]);
                }
                //else add arg to cmdMinusRedirs
                else{
                    cmd1MinusRedirs[m1] = strdup(cmdArgs[i]);
                    //printf("arg1 %d: %s\n", m1, cmd1MinusRedirs[m1]);
                    m1++;
                    //free(cmdArgs[i]);
                }
            }
            cmd1MinusRedirs[m1] = NULL;
            cmd2MinusRedirs[m2] = NULL;
            
        }
        free(cmdArgs);
        
        if(hitPipe){
            pipeCommand(cmd1MinusRedirs, cmd1Redirs, cmd1NumRedirs, cmd2MinusRedirs, cmd2Redirs, cmd2NumRedirs, bgd);
        }
        else{
            doCommand(cmd1MinusRedirs, cmd1Redirs, cmd1NumRedirs, bgd);

        }
        
        //printf("We got here\n");
        
        
        
        
    }
    
    
}


