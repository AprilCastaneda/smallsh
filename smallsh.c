#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMAND_LINE_LENGTH 2049 // 2048 characters plus null at the end
#define MAX_COMMAND_LINE_ARGUMENTS 512

int processIDs[MAX_COMMAND_LINE_ARGUMENTS];
int pidIndex = 0;

/* struct for command line elements */
struct commandElements
{
    char* commands[MAX_COMMAND_LINE_ARGUMENTS];
    char* inputFile;
    char* outputFile;
    bool inputRedirect;
    bool outputRedirect;
    bool fg;    // false if & found at end of command line
    bool bg;    // true if & found at end of command line
    int pid;
    int numArguments;
    struct commandElements* next;
};

/*
*   Program that sets in struct if command will run in foreground or
*   background. This is determined by the '&' character, which, if it
*   appears at the end of the commandLine, then the command will run
*   in the background. Otherwise, command will run in the foreground.
*   Struct bool values for fg and bg are set accordingly.
*/
void setCommandPosition(char* commandLine, struct commandElements* curCommand)
{
    if(commandLine[strlen(commandLine) - 1] == '&')
    {
        // printf("& is at eol\n");
        curCommand->fg = false;
        curCommand->bg = true;

        // Overwrite '&' as it has already been used to determine
        // command position
        commandLine[strlen(commandLine) - 1] = 0;
    }
    else
    {
        // printf("& is at midline\n");
        curCommand->fg = true;
        curCommand->bg = false;
    }
}

/*
*   Parses command line into elements in commandElements struct.
*/
struct commandElements* parseCommandLine(char* commandLine)
{
    struct commandElements *curCommand = malloc(sizeof(struct commandElements));

    // Get index of newline char and overwrite it with 0
    commandLine[strcspn(commandLine, "\n")] = 0;

    // printf("You typed in: %s\n", commandLine);

    // Set if command will run in foreground or background
    setCommandPosition(commandLine, curCommand);

    // Initialize elements of curCommand struct
    curCommand->inputRedirect = false;
    curCommand->outputRedirect = false;

    // For use with strtok_r
    char *saveptr;

    // The first token is the title
    char* token = strtok_r(commandLine, " ", &saveptr);
    int index = 0;
    int numArguments = 0;

    // Go through command line until all arguments parsed
    // If special symbols <, >, & found, process accordingly
    while(token != NULL)
    {
        // printf("This word: %s\n", token);

        // Check if token is a special symbol
        switch(token[0])
        {
            case '<':   // If input redirect, then set inputFile
                token = strtok_r(NULL, " ", &saveptr);
                curCommand->inputFile = calloc(strlen(token) + 1, sizeof(char));
                strcpy(curCommand->inputFile, token);
                curCommand->inputRedirect = true;
                token = strtok_r(NULL, " ", &saveptr);
                break;
            case '>':   // If output redirect, then set outputFile
                token = strtok_r(NULL, " ", &saveptr);
                curCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
                strcpy(curCommand->outputFile, token);
                curCommand->outputRedirect = true;
                token = strtok_r(NULL, " ", &saveptr);
                break;
            default:    // Otherwise, store as command arguments
                curCommand->commands[index] = calloc(strlen(token) + 1, sizeof(char));
                strcpy(curCommand->commands[index], token);
                token = strtok_r(NULL, " ", &saveptr);
                index++;
        }
    }

    numArguments = index;
    curCommand->numArguments = numArguments;

    return curCommand;
}

/*
*   Get command line elements and parse elements into commandElements
*   struct.
*/
struct commandElements* getCommandLine()
{
    char* commandLine = calloc(MAX_COMMAND_LINE_LENGTH, sizeof(char));

    // Print shell prompt character
    printf(": ");

    // Get command line until a newline is read
    fgets(commandLine, MAX_COMMAND_LINE_LENGTH, stdin);

    // Parse command line into struct
    return parseCommandLine(commandLine);
} 

/*
*   Print data for the commandElements struct. For testing purposes.
*/
void printCommandElements(struct commandElements* curCommand)
{
    int i;

    printf("%d arguments\n", curCommand->numArguments);

    if(curCommand->fg == false)
    {
        printf("Background Process.\n");
    }
    else
    {
        printf("Foreground Process.\n");
    }

    if(curCommand->inputRedirect == true)
    {
        printf("Input Redirect file: %s\n", curCommand->inputFile);
    }

    if(curCommand->outputRedirect == true)
    {
        printf("Output Redirect file: %s\n", curCommand->outputFile);
    }
    
    for(i = 0; i < curCommand->numArguments; i++)
    {
        printf("Argument %d: %s\n", i+1, curCommand->commands[i]);
    }
}

/*
*
*/
// bool checkExit(char* command)
// {
//     bool isExiting = false;

//     // Check if argument wants to exit
//     if(strcmp(command, "exit") == 0)
//     {
//         isExiting = true;
//     }

//     return isExiting;
// }

/*
*   Does not have to support i/o redirections, does not have to set any
*   exit status, if used as bg process with & - ignore option and run
*   command in foreground anyway, i.e. don't display an error.
*   When this command is run, shell kills any other processes or jobs
*   that shell has started before it terminates itself. 
*   Built in commands do not count as foreground processes for the
*   status command, i.e., status should ignore this command.
*/
void runExitCommand()
{
    int i;
    int arraySize = pidIndex;

    printf("Is at run Exit command\n");

    // Kill any other processes or jobs that shell has started
    // Idea: Go through list of processIDs from end, wait until each 
    // process is killed, then kill own process.
    for(i = 0; i < arraySize; i++)
    {
        printf("Kill processID: %d\n", processIDs[i]);
        kill(processIDs[i], SIGTERM);
    }

    // Status should ignore this command
    // Shell will be killed in main() by return EXIT_SUCCESS;
}

/*
*
*/
int runCdCommand(struct commandElements* curCommand, int i)
{
    printf("at cd\n");
    fflush(stdout);
    char cwd[256];
    char path[256];

    if(i == curCommand->numArguments - 1)
    {
        // Then change to HOME directory
        getcwd(cwd, sizeof(cwd));
        printf("Dir before cd: %s\n", cwd);
        fflush(stdout);
        chdir(getenv("HOME"));
        getcwd(cwd, sizeof(cwd));
        printf("Dir after cd: %s\n", cwd);
        fflush(stdout);
    }
    // If there is an argument, then change to this
    // directory. Command should support both absolute
    // and relative paths.
    else
    {
        // Get path, which is argument after cd
        i++;
        strcpy(path, curCommand->commands[i]);
        printf("Path after cd: %s\n", path);
        fflush(stdout);
        // Determine if path is absolute e.g. /bin/myfile
        // or relative e.g. mydir/myfile
        // If absolute, first char is '/', then chdir
        // with path
        char firstChar = path[0];
        if(firstChar == '/')
        {
            printf("path is absolute\n");
            fflush(stdout);
            getcwd(cwd, sizeof(cwd));
            printf("Dir before cd: %s\n", cwd);
            fflush(stdout);
            chdir(path);
            getcwd(cwd, sizeof(cwd));
            printf("Dir after cd: %s\n", cwd);
        }
        // If relative, first get cwd, add '/',
        // concatenate relative path, then chdir to path
        else
        {
            printf("path is relative\n");
            fflush(stdout);
            getcwd(cwd, sizeof(cwd));
            printf("Dir before cd: %s\n", cwd);
            fflush(stdout);
            strcat(cwd, "/");
            strcat(cwd, path);
            strcpy(path, cwd); // Did this as path is a better var name
            chdir(path);
            getcwd(cwd, sizeof(cwd));
            printf("Dir after cd: %s\n", cwd);
        }
    }

    return i;
}

/*
*
*/
bool runCommands(struct commandElements* curCommand)
{
    bool isExiting = false;
    int i, j;
    int numBuiltIns = 3;
    int builtInNum = -1;
    int lastFgStatus = 0; /* REVISIT THIS */ // Exit status or terminating
    int lastFgSignal = 2; /* REVISIT THIS */ // signal of the last foreground process ran by shell
    char* builtInCommands[numBuiltIns];
    char* pathDir = calloc(256, sizeof(char));
    // char* changeDirectory[MAX_COMMAND_LINE_LENGTH];
    
    builtInCommands[0] = "exit";
    builtInCommands[1] = "cd";
    builtInCommands[2] = "status";

    // Go through and process all commands
    for(i = 0; i < curCommand->numArguments; i++)
    {
        // Check for built in commands 'exit', 'cd', and 'status'
        for(j = 0; j < numBuiltIns; j++)
        {
            if(strcmp(curCommand->commands[i], builtInCommands[j]) == 0)
            {
                builtInNum = j + 1;
                break;
            }
        }
        
        switch(builtInNum)
        {
            case 1: // exit command
                curCommand->fg = true;
                curCommand->bg = false;
                runExitCommand();
                isExiting = true;
                return isExiting;   // return immediately to exit
                break;
            case 2: // cd command
                // If no argument after cd
                i = runCdCommand(curCommand, i); // Change index if needed
                break;
            case 3: // status command
                // Prints out either the exit status or the
                // terminating signal of the last foreground process
                // ran by the shell
                // If exit status
                printf("exit value %d\n", lastFgStatus);
                fflush(stdout);
                // or if terminating signal
                printf("terminated by signal %d\n", lastFgSignal);
                fflush(stdout);
                break;
            default: // none built in
                printf("Default\n");
        }
    }
    return isExiting;
}

/*
*   C shell that implements a subset of features such as providing a
*   prompt for running commands, handling blank lines and comments,
*   providing expansion for the variable $$, executing three commands
*   exit, cd, and status via code built into the shell, executing
*   other commands by creating new processes using a function from
*   the exec() family of functions, supporting input and output
*   redirection, supporting running commands in foreground and
*   background processes, and implementing custom handlers for two
*   signals, SIGINT and SIGTSTP.
*/
int main()
{
    struct commandElements* curCommand;
    bool isExiting = false;

    // Create linked list of commands
    printf("\n");
    fflush(stdout); /* FLUSH AFTER EVERY PRINT! */
    do
    {
        curCommand = getCommandLine();

        printCommandElements(curCommand); // For testing only

        isExiting = runCommands(curCommand);
    }
    while(!isExiting);

    // Kill shell
    return EXIT_SUCCESS;
}