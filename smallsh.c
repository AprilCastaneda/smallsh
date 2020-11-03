#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_COMMAND_LINE_LENGTH 2049 // 2048 characters plus null at the end
#define MAX_COMMAND_LINE_ARGUMENTS 512

/* struct for command line elements */
struct commandElements
{
    char* commands[MAX_COMMAND_LINE_ARGUMENTS];
    char* inputFile;
    char* outputFile;
    int numArguments;
};

/*
*   Parses command line into elements in commandElements struct.
*/
struct commandElements* parseCommandLine(char* commandLine)
{
    struct commandElements *curCommand = malloc(sizeof(struct commandElements));

    // Get index of newline char and overwrite it with 0
    commandLine[strcspn(commandLine, "\n")] = 0;

    printf("You typed in: %s\n", commandLine);

    // For use with strtok_r
    char *saveptr;

    // The first token is the title
    char *token = strtok_r(commandLine, " ", &saveptr);
    int index = 0;
    int numArguments = 0;

    // Go through command line until all arguments parsed
    while(token != NULL)
    {
        printf("This word: %s\n", token);
        curCommand->commands[index] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(curCommand->commands[index], token);
        token = strtok_r(NULL, " ", &saveptr);
        index++;
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
    
    for(i = 0; i < curCommand->numArguments; i++)
    {
        printf("Argument %d: %s\n", i+1, curCommand->commands[i]);
    }
}

/*
*
*/
bool runCommands(struct commandElements* curCommand)
{
    bool exitShell = false;

    // See if shell is exiting
    int i;

    for(i = 0; i < curCommand->numArguments; i++)
    {
        if(strcmp(curCommand->commands[i], "exit") == 0)
        {
            exitShell = true;
            return exitShell;
        }        
    }

    return exitShell;
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
    bool exitShell = false;

    // Show shell prompt to user
    printf("\n");
    do
    {
        curCommand = getCommandLine();

        printCommandElements(curCommand);

        exitShell = runCommands(curCommand);
    }
    while(!exitShell);

    return EXIT_SUCCESS;
}