#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LINE_LENGTH 2048
#define MAX_COMMAND_LINE_ARGUMENTS 512

/*
*   C shell that implements a subset of features such as providing a
*   prompt for running commands, handling blank lines and comments,
*   providing expansion for the variable $$, executing three commands
*   exit, cd, and status via code built into the shell, executing
*   other commands by creating new processes using a function from
*   the exec family of functions, supporting input and output
*   redirection, supporting running commands in foreground and
*   background processes, and implementing custom handlers for two
*   signals, SIGINT and SIGTSTP.
*/
int main()
{
    char commandLine[MAX_COMMAND_LINE_LENGTH];

    // Show shell prompt to user
    printf("\n");
    do
    {
        = getShellCommand();
        choice = getChoice(FIRST_MENU_MAX_CHOICES);

        switch(choice)
        {
            case 1:
                showSecondMenu(SECOND_MENU_MAX_CHOICES);
                break;
            case 2:
                break;
            default:
                printf("You entered an incorrect choice. Try again.\n\n");
        }
    }
    while(choice != 2);

    return EXIT_SUCCESS;
}