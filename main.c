#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <sys/types.h>
#include <unistd.h>

int execute();
char* readInput();
void display(char** args);
char** parse(int *cmdcount, char* line);
struct Command*  parseargv(char* command);
void printcommand(struct Command* command);

struct Command {
    char** args;
    int background;
    int input;
    int output;
};

int main() {
    printf("Welcome to my shell! \n");
    fflush(stdout);
    int status = 0;
    fflush(stdout);
    while (1) {
        printf("shell$  ");
        fflush(stdout);
        if (status == -1){
            printf("Goodbye ......");
            fflush(stdout);
            break;
        }
        status = execute();
        printf("\n");
    }
}

char* readInput() {
    size_t buffsize = 32;
    char* buffer = NULL;
    getline(&buffer, &buffsize, stdin);
    return buffer;
}


char** parse(int *cmdcount, char* line) {
    size_t maxsize = 32;
    char** commands = malloc(maxsize * sizeof(char*));

    char* token = strtok(line, "|\n");
    int i = 0;
    while (token != NULL) {
        commands[i] = token;
        i += 1;
        if (i >= maxsize) {
            maxsize += 32;
            commands = realloc(commands, maxsize * sizeof(char*));
        }
        token = strtok(NULL, "|\n");
    }

    *cmdcount = i;

    commands[i] = NULL;
    return commands;
}

struct Command* parseargv(char* command) {
    char* line = command;
    size_t maxsize = 32;
    char** args = malloc(maxsize * sizeof(char*));
    char* token = strtok(line, " \t\r\a\n");
    int i = 0;
    while (token != NULL) {
        args[i] = token;
        i += 1;
        if (i >= maxsize) {
            maxsize += 32;
            args = realloc(args, maxsize * sizeof(char*));
        }
        token = strtok(NULL, " \t\r\a\n");
    }

    struct Command* cmd = (struct Command*)malloc(sizeof(struct Command));
    cmd->input = -1;
    cmd->output = -1;
    cmd->background = -1;

    if (*args[i-1] == '&'){
        cmd->background = 1;
        args[i-1] = NULL;
    } else {
        args[i] = NULL;
    }

    int j = 0;
    while (args[j] != NULL) {
        if (*args[j] == '<' && args[j+1] != NULL) {
            FILE *fp1 = fopen(args[j+1], "r");
            if (fp1 == NULL) {
                perror("Error opening file : ");
                exit(1);
            }
            cmd->input  = fileno(fp1);
            args[j] = NULL;
            args[j+1] = NULL;
            j++;
        } else if (*args[j] == '>' && args[j+1] != NULL) {
            FILE *fp2 = fopen(args[j+1], "a+");
            cmd-> output = fileno(fp2);
            if (fp2 == NULL) {
                perror("Error opening file : ");
                exit(1);
            }
            args[j] = NULL;
            args[j+1] = NULL;
        }
        j++;
    }

    cmd->args = args;

    return cmd;
}

int execute() {
    char* line = readInput();
    int cmdcount;
    char** commands = parse(&cmdcount, line);
    //display(commands);
    int f_des_arr[cmdcount - 1][2];
    int pids[cmdcount];
    int hangflags[cmdcount];

    for (int j = 0; j < cmdcount - 1; j++){
        if (pipe(f_des_arr[j]) < 0){
            perror("Error creating pipe");
        }
    }

    int i = 0;
    while(i < cmdcount) {
        int pid = fork();
        pids[i] = pid;

        struct Command* cmd = parseargv(commands[i]);
        display(cmd->args);
        printcommand(cmd);

        if (pid < 0) {
            perror("Fork Error");
            exit(1);
        }

        else if (pid == 0) {

            if (cmd->input != -1){
                dup2(cmd->input, fileno(stdin));

            }

            if (cmd->output != -1) {
                dup2(cmd->output, fileno(stdout));
            }

            if (cmd->background == 1) {
                hangflags[i] = 1;
            }


            if (i != 0) {
                int dupin = dup2(f_des_arr[i-1][0], fileno(stdin));
                if (dupin < 0){
                    perror("Duping stdin failed");
                }
                close(f_des_arr[i-1][0]);
            }

            if (i != cmdcount - 1) {
                int dupout = dup2(f_des_arr[i][1], fileno(stdout));
                if (dupout < 0){
                    perror("Duping stdout Failed");
                }
            }

            for (int p =0; p < cmdcount - 1; p ++) {
                close(f_des_arr[p][0]);
                close(f_des_arr[p][1]);
            }

            //display(args);
            execvp(cmd->args[0],cmd->args);
            perror("Exec Error");
            free(cmd);
            exit(1);
        }

        i++;
    }

    for (int p =0; p < cmdcount - 1; p ++) {
        close(f_des_arr[p][0]);
        close(f_des_arr[p][1]);
    }


    for (int pr = 0 ; pr < cmdcount;  pr++){
        if (hangflags[pr] != 1){
            waitpid(pids[pr], 0, 0);
        }
    }

    return 4;
}


void printcommand(struct Command* command){
    printf("input %d output %d background %d\n", command->input, command->output, command->background);
}

void display(char** args){
    int i = 0;
    while (args[i] != NULL){
        printf("\narg %d %s ", i, args[i]);
        i++;
    }
    printf("\n");
}