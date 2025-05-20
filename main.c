#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>

void printDir() {
    const int MAX_PATH = 1028;
    const char PURPLE_ANSI[] = "\033[0;95m";
    const char RESET_ANSI[] = "\033[0m";

    char cwd[MAX_PATH];
    getcwd(cwd, sizeof(cwd));
    printf("%s%s%s$ ", PURPLE_ANSI, cwd, RESET_ANSI);
}

char* getUserInput(const int MAX_CHARS) {
    char* userInput = (char*)malloc(MAX_CHARS * sizeof(char));
    if (userInput == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (fgets(userInput, MAX_CHARS, stdin) == NULL) {
        perror("fgets");
        free(userInput);
        return NULL;
    }
    
    userInput[strcspn(userInput, "\n")] = 0;
    
    return userInput;
}

char** parseInput(char* userInput, const int MAX_ARGS) {
    
    char** args = (char**)malloc((MAX_ARGS + 1) * sizeof(char*));
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char *inputCopy = strdup(userInput);
    if (inputCopy == NULL) {
        perror("strdup");
        free(args);
        exit(EXIT_FAILURE);
    }

    char *token = strtok(inputCopy, " ");
    int argCount = 0;
    while (token != NULL) {
        if (argCount >= MAX_ARGS) {
            fprintf(stderr, "Warning: Too many arguments.\n");
            break;
        }
        args[argCount] = strdup(token); // copy token into args
        if (args[argCount] == NULL) {
            perror("strdup");
            for (int i = 0; i < argCount; i++) {
                free(args[i]);
            }
            free(args);
            free(inputCopy);
            exit(EXIT_FAILURE);
        }
        argCount++;
        token = strtok(NULL, " ");
    }
    args[argCount] = NULL; // args must end with null value for execvp()
    free(inputCopy); // free the copy
    return args;
}

bool checkBuiltIns(char** args) {
    if (strcmp(args[0], "exit") == 0)
        exit(EXIT_SUCCESS);
    else if (strcmp(args[0], "cd") == 0) {
        chdir(args[1]);
        return true;
    }
    return false;
}

void handleRedirection(char** args) {
    int i = 0;
    while (args[i] != NULL) {
        printf("%d", i);
        if (strcmp(args[i], ">") == 0) {
            if (args[i] == NULL) {
                printf("No output file provided.");
                return;
            }
            FILE* filePtr = fopen(args[i+1], "w");
            if (filePtr == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            int fileDescriptor = fileno(filePtr);

            if (dup2(fileDescriptor, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            close(fileDescriptor);
            args[i] = NULL;
            return;
        }
        else if (strcmp(args[i], ">>") == 0) {
            if (args[i] == NULL) {
                printf("No output file provided.");
                return;
            }
            FILE* filePtr = fopen(args[i+1], "a");
            if (filePtr == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            int fileDescriptor = fileno(filePtr);

            if (dup2(fileDescriptor, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            close(fileDescriptor);
            args[i] = NULL;
            return;
        }
        else if (strcmp(args[i], "<") == 0) {
            if (args[i] == NULL) {
                printf("No input file provided.");
                return;
            }
            FILE* filePtr = fopen(args[i+1], "r");
            if (filePtr == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            int fileDescriptor = fileno(filePtr);

            if (dup2(fileDescriptor, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            close(fileDescriptor);
            args[i] = NULL;
            return;
        }

        i++;
    }
}

void executeArgs(char** args) {
    pid_t pid, w;
    int wstatus;

    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { // child process
        handleRedirection(args); // handles i/o files
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else { // parent process
        w = waitpid(pid, &wstatus, 0);

        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(wstatus)) {
            //printf("exited, wstatus=%d\n", WEXITSTATUS(wstatus));
        }
        else if (WIFSIGNALED(wstatus)) {
            printf("killed by signal %d\n", WTERMSIG(wstatus));
        }
        else if (WIFSTOPPED(wstatus)) {
            printf("stopped by signal %d\n", WSTOPSIG(wstatus));
        }
        else if (WIFCONTINUED(wstatus)) {
            printf("continued\n");
        }
    }
}

int main()
{
    const int MAX_CHARS = 50;
    const int MAX_ARGS = 20;

    while (true) {
        printDir();

        char* userInput = getUserInput(MAX_CHARS);
        if (userInput == NULL) continue;

        char** args = parseInput(userInput, MAX_ARGS);
        if (args == NULL) {
            free(userInput);
            continue;
        }
        
        if (checkBuiltIns(args)) {
            free(userInput);
            free(args);
            continue;
        }

        executeArgs(args);

        free(userInput);
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
        free(args);
    }

    return 0;
}