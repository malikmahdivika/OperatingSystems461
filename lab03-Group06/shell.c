#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"

#define BUFLEN 1024

//To Do: This base file has been provided to help you start the lab, you'll need to heavily modify it to implement all of the features

int main() {
    char buffer[1024];
    char* firstCommand;
    char** parsedArgs;
    char** args1 = malloc(BUFLEN * sizeof(char*));
    char** args2 = malloc(BUFLEN * sizeof(char*));
    char newline;
    int pipe_fd[2];

    printf("Welcome to the Group06 shell! Enter commands, enter 'quit' to exit\n");
    do {
        //Print the terminal prompt and get input
        printf("$ ");
        char *input = fgets(buffer, sizeof(buffer), stdin);
        if(!input)
        {
            fprintf(stderr, "Error reading input\n");
            return -1;
        }
        // allocate for piping
        args1 = malloc(BUFLEN * sizeof(char*));
        args2 = malloc(BUFLEN * sizeof(char*));

        // Find first command to pass to execve()
        firstCommand = (char*) malloc(BUFLEN * sizeof(char));
        size_t firstLength = firstword(firstCommand, input, BUFLEN);        
        // trim newline
        firstCommand[strcspn(firstCommand, "\n")] = '\0';

        // Clean and parse the input string for arguments
        parsedArgs = malloc(BUFLEN * sizeof(char*));
        size_t argslength = trimstring(parsedArgs, input, BUFLEN);
        
        // Pipe check must be done before fork()
        int pipeposition = findpipe(input, BUFLEN);
        
        // printf("pipeposition: %d\n", pipeposition);
        // printf("first command: %s\n", firstCommand);

        // printf("argument(s):\n");
        // for (size_t i = 0; i < argslength; i++) {
        //     printf("    arg[%zu]: %s\n", i, parsedArgs[i]);
        // }

        // printf("input string modified?: %s\n", input); 

        if ( strcmp(firstCommand, "quit") == 0 ) {
            printf("Bye!!\n");
            return 0;
        } else if (pipeposition > -1) {
            // set up pipeline file descriptors.
            pipe(pipe_fd);

            pid_t p1 = fork();
            if (p1 == 0) { // fork of first child
                // extract args from left side of pipe
                char leftex[BUFLEN];
                args1 = malloc(BUFLEN * sizeof(char*));
                strncpy(leftex, input, pipeposition);
                leftex[pipeposition] = '\0';
                trimstring(args1, leftex, BUFLEN);
                
                // redirect stdout, close read end
                close(pipe_fd[0]);
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);

                // begin first child process (filtering for absolute/relative/PATH modes of execution)
                if (*args1[0] == '/') {
                    // "absolute" path for commands starting with '/'
                    execve(args1[0], args1, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                } else if (strchr(*args1, '/') != NULL) {
                    // "relative" path execution finds other '/' occurrences
                    execve(args1[0], args1, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                }
                 else {
                    const char* pathCommand = accessCheck(args1[0], BUFLEN);
                    execve(pathCommand, args1, NULL);
                    perror("execve cmd1");      // execve returns on failure; cmd1 specifier.
                    exit(EXIT_FAILURE);
                    return 0;
                }
            }
            pid_t p2 = fork(); 
            if (p2 == 0) { // fork of second child
                // extract args from left side of pipe
                char rightex[BUFLEN];
                strcpy(rightex, input + pipeposition + 1);
                rightex[pipeposition] = '\0';
                trimstring(args2, rightex, BUFLEN);

                // redirect stdin, close write end
                close(pipe_fd[1]);
                dup2(pipe_fd[0], STDIN_FILENO);
                close(pipe_fd[0]);

                const char* pathCommand = accessCheck(args2[0], BUFLEN);
                if (pathCommand == NULL) {
                    printf("Command not found or access to executable denied: %s\n", args1[0]);
                }

                if (*args2[0] == '/') {
                    // "absolute" path for commands starting with '/'
                    execve(args2[0], args2, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                } else if (strchr(*args2, '/') != NULL) {
                    // "relative" path execution finds other '/' occurrences
                    execve(args2[0], args2, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                }
                 else {
                    const char* pathCommand = accessCheck(args2[0], BUFLEN);
                    execve(pathCommand, args2, NULL);
                    perror("execve cmd2");       // execve returns on failure; cmd2 specifier.
                    exit(EXIT_FAILURE);
                    return 0;
                }
            }
            // Parent must wait for both processes (also close both unused pipe ends)
            if (p1 != 0 && p2 != 0) {
                close(pipe_fd[0]);
                close(pipe_fd[1]);

                waitpid(p1, NULL, 0);
                waitpid(p2, NULL, 0);
            }
        } else {
	    // Here goes the magic!
            if (fork() != 0) {
                wait(NULL); 
            } else {
                if (*firstCommand == '/') {
                    // "absolute" path for commands starting with '/'
                    execve(firstCommand, parsedArgs, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                } else if (strchr(firstCommand, '/') != NULL) {
                    // "relative" path execution finds other '/' occurrences
                    execve(firstCommand, parsedArgs, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                } else {
                    const char* pathCommand = accessCheck(firstCommand, BUFLEN);
                    execve(pathCommand, parsedArgs, NULL);
                    perror("execve");   // execve returns on failure.
                    exit(EXIT_FAILURE);
                    return 0;
                }
            }
        }

        //Remember to free any memory you allocate!
        free(firstCommand);
        free(parsedArgs);
        free(args1);
        free(args2);
    } while ( 1 );

    return 0;
}
