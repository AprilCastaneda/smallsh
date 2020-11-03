#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_COMMAND_LINE_LENGTH 2049 // 2048 characters plus null at the end
#define MAX_COMMAND_LINE_ARGUMENTS 512
#define NUM_SPECIAL_SYMBOLS 3

const char* specialSymbols[NUM_SPECIAL_SYMBOLS] = {"<", ">", "&"};

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
    bool endOfLine;
    int numArguments;
};

/*
*   Check if token is a special symbol <, >, or &.
*/
bool checkSpecialSymbol(char* token)
{
    int i;
    bool isSpecialSymbol = false;

    // Check if special symbol
    for(i = 0; i < NUM_SPECIAL_SYMBOLS; i++)
    {
        if(strcmp(token, specialSymbols[i]) == 0)
        {
            isSpecialSymbol = true;
            return isSpecialSymbol;
        }
    }

    return isSpecialSymbol;
}

/*
*   Does not have to support i/o redirections, does not have to set any
*   exit status, if used as bg process with & - ignore option and run
*   command in foreground anyway, i.e. don't display an error.
*   When this command is run, shell kills any other processes or jobs
*   that shell has started before it terminates itself. 
*   Built in commands do not count as foreground processes for the
*   status command, i.e., status should ignore this command.
*/
bool processExit()
{
    return true;
}

/*
*
*/
char* processBG(char* token, struct commandElements* curCommand)
{
    char* tempptr;
    char* temptoken = strtok_r(token, " ", &tempptr);

    if(temptoken == NULL)
    {
        curCommand->fg = false;
        curCommand->bg = true;
        curCommand->endOfLine = true;
    }

    return temptoken;
}

/*
*
*/
char* processOutputRedirect(char* token, char* saveptr, struct commandElements* curCommand)
{
    char* temptoken = strtok_r(token, " ", &saveptr);

    curCommand->outputFile = calloc(strlen(temptoken) + 1, sizeof(char));
    strcpy(curCommand->outputFile, temptoken);
    curCommand->outputRedirect = true;

    return temptoken;
}

/*
*
*/
char* processInputRedirect(char* token, char* saveptr, struct commandElements* curCommand)
{
    char* temptoken = strtok_r(token, " ", &saveptr);

    curCommand->inputFile = calloc(strlen(temptoken) + 1, sizeof(char));
    strcpy(curCommand->inputFile, temptoken);
    curCommand->inputRedirect = true;

    return temptoken;
}

/*
*
*/
void processSpecialToken(char specialChar, char* token, char* saveptr, struct commandElements* curCommand)
{
    bool isSpecialSymbol = false;
    bool endOfLine = false;
    char firstChar;
    char *temptoken;

    switch(specialChar)
    {
        case '<':
            temptoken = processInputRedirect(token, saveptr, curCommand);
            break;
        case '>': 
            temptoken = processOutputRedirect(token, saveptr, curCommand);
            break;
        case '&': 
            temptoken = processBG(token, curCommand);
            break;
        default:
            printf("Error. Not a special char.\n");
    }

    if(!curCommand->endOfLine)
    {
        temptoken = strtok_r(NULL, " ", &saveptr);

        if(temptoken != NULL)
        {
            isSpecialSymbol = checkSpecialSymbol(temptoken);

            if(isSpecialSymbol)
            {
                firstChar = token[0];
                processSpecialToken(firstChar, temptoken, saveptr, curCommand);
            }
        }
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

    printf("You typed in: %s\n", commandLine);

    // Initialize some elements of curCommand
    curCommand->inputRedirect = false;
    curCommand->outputRedirect = false;
    curCommand->fg = true;
    curCommand->bg = false;
    curCommand->endOfLine = false;

    // For use with strtok_r
    char *saveptr;

    // The first token is the title
    char *token = strtok_r(commandLine, " ", &saveptr);
    // char *temptoken;
    char specialChar;
    int index = 0;
    int numArguments = 0;
    bool isSpecialSymbol = false;
    // bool endOfLine = false;
    bool continueParse = true;

    // Go through command line until all arguments parsed
    // If special symbols <, >, & found, process accordingly
    while(token != NULL && continueParse)
    {
        printf("This word: %s\n", token);

        // Check if token is a special symbol
        isSpecialSymbol = checkSpecialSymbol(token);

        // If special symbol found, process differently
        if(isSpecialSymbol)
        {
            specialChar = token[0];

            if(specialChar == '&')
            {
                token = processBG(token, curCommand);

                if(curCommand->endOfLine)
                {
                    continueParse = false;
                }
                else
                {
                    curCommand->commands[index] = calloc(2, sizeof(char)); // specialChar + '/0'
                    strcpy(curCommand->commands[index], token);
                    index++;
                    token = strtok_r(NULL, " ", &saveptr); 
                }
            }
            else
            {
                processSpecialToken(specialChar, token, saveptr, curCommand);
                continueParse = false;
            }
        }
        else
        {
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
bool checkExit(char* command)
{
    // Check if argument wants to exit
    if(strcmp(command, "exit") == 0)
    {
        return processExit();
    }

    return false;
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
        // Check if any argument wants to exit
        exitShell = checkExit(curCommand->commands[i]);

        if(exitShell)
        {
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