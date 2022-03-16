/** Kyla Ramos
CSC 345-01
Project 1 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>

#define MAX_LINE 80 /* the maximum length command */
#define MAX_ARGS (MAX_LINE/2 + 1) /* maximum # of args */

int should_run = 1; // flag to determine when to exit program

int main(void) {
    /* declare chars */
    char *args[MAX_ARGS]; // command line arguments 
    char inputline[MAX_LINE]; // acts as command line input
    char *input; // holds the arguments from CL
    char *history = "."; // holds previous command for history
    char *token; // holds commands after being tokenized
    char cwd[MAX_LINE]; // holds current working directory
     
    const char s[2] =  " "; // used to compare in arg passing

    /* declare all ints*/
    int background_process = 0; // flag to determine a background process
    int pipe_flag = 0; // flag to determine a pipe is used
    int argc = 0; // arg counter
    int length = 0; // length of string of args for loop
    int file_in, file_out; // flags to determine if file is redirected in or out
    int file_read, file_write; // read/write vars for filles
    int fd[2]; // pipe var

    // loop for shell
    while (should_run) {
        // reset vars for next command 
        background_process = 0;   
        argc = 0;
        file_in = 0;
        file_out = 0;
        pipe_flag = 0;

        // prints cwd
        printf("osh:");        
        getcwd(cwd, sizeof(cwd));
        printf("%s", cwd);   
        printf(">"); 

        // gets user input & length from command line
        fgets(inputline, MAX_LINE, stdin);
        length = strlen(inputline);

        // allocate memory to take in arg input
        input = malloc(MAX_ARGS * sizeof(char));

        // remove newline char when passing arg
        if (inputline[length - 1] == '\n') {
            inputline[length - 1]  = '\0';
        }

        // checks for exit command
        if (strcmp(inputline, "exit") == 0) {
            should_run = 0;
            exit(EXIT_SUCCESS);
        }                  

        //check for !!/history command
        if (strcmp(inputline, "!!") == 0) {
            if (strcmp(history, ".") == 0) { 
                printf("No commands in history\n"); // if command hasnt changed
                continue;
            }
            input = history; 
            history = malloc((MAX_ARGS) * sizeof(char));
            memcpy(history, input, strlen(input));
            printf("%s\n", input); 
        } else if (inputline[0] != '\n' && inputline[0] != '\0') {
            input = inputline;
            history = malloc((MAX_ARGS) * sizeof(char));
            memcpy(history, input, length);
        }   

        // parses the input of different args into tokens 
        token = strtok(input, s);
        while (token != NULL) {
            args[argc] = token;

            // handles pipes
            if (strcmp(token, "|") == 0) {
                pipe_flag = argc;
            }   

            token = strtok(NULL, " "); // clear tokens
            argc++; 
        }
        // terminates array
        args[argc] = NULL;

        // checks for cd command
        if (strcmp(args[0], "cd") == 0) {
            chdir(args[1]);
            continue;         
        } 

        // handles background processes
        if (strcmp(args[argc-1], "&") == 0) {
            background_process = 1;
            args[argc-1] = '\0'; // clear last arg
        }

        // checks for file redirect (> or <) commands
        if (argc > 2) {
            if (!strcmp(args[argc - 2], ">"))  {
                file_out = 1;
            }  else if (!strcmp(args[argc - 2], "<")) {
                file_in = 1;
            }
        }

        pid_t pid = fork(); // fork child process
        if (pid < 0) {  // if fork fails
            printf("Fork failed");
            return 0;
        } else if (pid == 0) { // child process

            // command for input redirect (or <)
            if (file_in) {
                args[argc - 2] = NULL; // removes the <

                if ((file_read = open(args[argc - 1], O_RDONLY)) < 0) {
                    // handles error if file doesns't open
                    printf("Failed to open file\n");
                    exit(0);
                }

                // redirects the STDIN with dup2
                dup2(file_read, 0);
                close(file_read);

            // command for output redirect (or >)
            } else if (file_out) {
                args[argc - 2] = NULL; // 

                if ((file_out = creat(args[argc - 1], 0644)) < 0) {
                    // handles error if file doesns't open
                    printf("File error: File won't open\n");
                    exit(0);
                }

                // redirects the STDOUT with dup2 to be a file
                dup2(file_out, STDOUT_FILENO);
                close(file_out);               
            } 

            if (pipe_flag) { // if pipe is used, create pipe
                if (pipe(fd) < 0) {
                    printf("failed  to make pipe");
                    exit(1);
                }

                pid = fork(); // fork another process
                if (pid == 0) { // child #2 process for pipe
                    dup2(fd[1], STDOUT_FILENO);
                    args[pipe_flag] = '\0';
                    close(fd[0]);

                    // executes part before |
                    execvp(args[0], args);
                } else if (pid > 0) { //  parent process #2 for pipe
                    wait(NULL);

                    dup2(fd[0], STDIN_FILENO);
                    close(fd[1]);

                    // executes everything after the | 
                    execvp(args[pipe_flag + 1], &args[pipe_flag + 1]);
                } 
            } else {
                execvp(args[0], args); // executes nonpipe commands
            }
        } else if (pid > 0) { // total parent process          
            if (!background_process) {
                wait(NULL);  // wait unless & is used
            }
        }
    }
    free(input); // free memory allocated for input
    return 0;
}