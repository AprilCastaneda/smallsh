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
};

/*
*   Parses command line into elements in commandElements struct.
*/
void parseCommandLine(char* commandLine)
{
    // Get index of newline char and overwrite it with 0
    commandLine[strcspn(commandLine, "\n")] = 0;

    printf("You typed in: %s\n", commandLine);
}

/*
*   Get command line elements and parse elements into commandElements
*   struct.
*/
void getCommandLine()
{
    char* commandLine = calloc(MAX_COMMAND_LINE_LENGTH, sizeof(char));

    // Print shell prompt character
    printf(": ");

    // Get command line until a newline is read
    fgets(commandLine, MAX_COMMAND_LINE_LENGTH, stdin);

    // Parse command line into struct
    parseCommandLine(commandLine);
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
    bool exitShell = false;

    // Show shell prompt to user
    printf("\n");
    // do
    // {
        getCommandLine();

    //     exitShell = parseCommandLine(commandLine);
    // }
    // while(!exitShell);

    return EXIT_SUCCESS;
}