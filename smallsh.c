#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define MAX_COMMAND_LINE_LENGTH 2049 // 2048 characters plus null at the end
#define MAX_COMMAND_LINE_ARGUMENTS 512

int processIDs[MAX_COMMAND_LINE_ARGUMENTS];
int pidIndex = 0;
char exitStatus[256];

/* struct for command line elements */
struct commandElements
{
    char* commands[MAX_COMMAND_LINE_ARGUMENTS];
    char* inputFile;
    char* outputFile;
    // char* exitStatus;
    bool inputRedirect;
    bool outputRedirect;
    bool fg;    // false if & found at end of command line
    bool bg;    // true if & found at end of command line
    bool ignore;    // If command line is blank or a comment
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

    // printf("What you typed: %s\n", commandLine);
    // fflush(stdout);

    // Check if command line is a blank line or is a comment that
    // starts with '#'
    if(commandLine[0] == 0 || commandLine[0] == '#')
    {
        curCommand->ignore = true;
        return curCommand;  // return immediately as no other info needed
    }
    else
    {
        curCommand->ignore = false;
    }

    // Set if command will run in foreground or background
    setCommandPosition(commandLine, curCommand);

    // Initialize elements of curCommand struct
    curCommand->inputRedirect = false;
    curCommand->outputRedirect = false;
    // curCommand->exitStatus = calloc(256, sizeof(char));
    // strcpy(curCommand->exitStatus, "exit value 0");

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
    fflush(stdout);

    // Get command line until a newline is read
    fgets(commandLine, MAX_COMMAND_LINE_LENGTH, stdin);

    // Get pointer to first occurence of "$$" in commandLine
    // Reference: http://www.cplusplus.com/reference/cstring/strstr/
    char* chPtr = strstr(commandLine, "$$");
    char sPid[256]; // buffer to store int pid transformed to string

    // Go through commandLine and expand any instance of "$$" in a
    // command into the processID of smallsh itself
    // Also, transform processID into string
    while(chPtr != NULL)
    {
        sprintf(sPid, "%d", getpid());
        strncpy(chPtr, sPid, strlen("$$"));
        chPtr = strstr(commandLine, "$$");
    }

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
    fflush(stdout);

    if(curCommand->fg == false)
    {
        printf("Background Process.\n");
        fflush(stdout);
    }
    else
    {
        printf("Foreground Process.\n");
        fflush(stdout);
    }

    if(curCommand->inputRedirect == true)
    {
        printf("Input Redirect file: %s\n", curCommand->inputFile);
        fflush(stdout);
    }

    if(curCommand->outputRedirect == true)
    {
        printf("Output Redirect file: %s\n", curCommand->outputFile);
        fflush(stdout);
    }
    
    for(i = 0; i < curCommand->numArguments; i++)
    {
        printf("Argument %d: %s\n", i+1, curCommand->commands[i]);
        fflush(stdout);
    }
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
void runExitCommand()
{
    int i;

    // printf("Is at run Exit command\n");
    // fflush(stdout);

    // Kill any other processes or jobs that shell has started
    // Idea: Go through list of processIDs from end, wait until each 
    // process is killed, then kill own process.
    for(i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        if(processIDs[i] != -1)
        {
            printf("Killing processID: %d\n", processIDs[i]);
            fflush(stdout);
            kill(processIDs[i], SIGTERM);
        }
    }

    // Status should ignore this command
    // Shell will be killed in main() by return EXIT_SUCCESS;
    // exit(0);
}

/*
*
*/
void runCdCommand(struct commandElements* curCommand)
{
    // printf("at cd\n");
    // fflush(stdout);
    
    char cwd[256];
    char path[256];

    if(curCommand->numArguments == 1)
    {
        // Then change to HOME directory
        getcwd(cwd, sizeof(cwd));
        // printf("Dir before cd: %s\n", cwd);
        // fflush(stdout);
        chdir(getenv("HOME"));
        getcwd(cwd, sizeof(cwd));
        // printf("Dir after cd: %s\n", cwd);
        // fflush(stdout);
    }
    // If there is an argument, then change to this
    // directory. Command should support both absolute
    // and relative paths.
    else
    {
        // Get path, which is argument after cd
        strcpy(path, curCommand->commands[1]);
        // printf("Path after cd: %s\n", path);
        // fflush(stdout);
        // Determine if path is absolute e.g. /bin/myfile
        // or relative e.g. mydir/myfile
        // If absolute, first char is '/', then chdir
        // with path
        char firstChar = path[0];
        if(firstChar == '/')
        {
            // printf("path is absolute\n");
            // fflush(stdout);
            getcwd(cwd, sizeof(cwd));
            // printf("Dir before cd: %s\n", cwd);
            // fflush(stdout);
            chdir(path);
            getcwd(cwd, sizeof(cwd));
            // printf("Dir after cd: %s\n", cwd);
            // fflush(stdout);
        }
        // If relative, first get cwd, add '/',
        // concatenate relative path, then chdir to path
        else
        {
            // printf("path is relative\n");
            // fflush(stdout);
            getcwd(cwd, sizeof(cwd));
            // printf("Dir before cd: %s\n", cwd);
            // fflush(stdout);
            strcat(cwd, "/");
            strcat(cwd, path);
            strcpy(path, cwd); // Did this as path is a better var name
            chdir(path);
            getcwd(cwd, sizeof(cwd));
            // printf("Dir after cd: %s\n", cwd);
            // fflush(stdout);
        }
    }
}

/*
*
*/
void removeFromPIDList(int pid)
{
    int i;

    for(i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        if(processIDs[i] == pid)
        {
            processIDs[i] = -1;
            break;
        }
    }
}

/*
*
*/
void addToPIDList(int pid)
{
    int i;

    for(i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        if(processIDs[i] == -1)
        {
            processIDs[i] = pid;
            break;
        }
    }
}

/*
*
*/
void runFGParent(pid_t spawnpid, struct commandElements* curCommand)
{
    int childExitStatus;
    char status[256];
    
    spawnpid = waitpid(spawnpid, &childExitStatus, 0);

    // printf("Parent's waiting is done as the child with pid %d exited\n", spawnpid);
    // fflush(stdout);
    
    if(WIFEXITED(childExitStatus))
    {
        // Change exit status to string
        sprintf(status, "%d", WEXITSTATUS(childExitStatus));

        // Then concatenate exit status and store in curCommand struct
        memset(exitStatus,0,strlen(exitStatus));
        strcpy(exitStatus, "exit value ");
        strcat(exitStatus, status);

        // printf("Child %d exited normally with %s\n", spawnpid, exitStatus);
        // fflush(stdout);
    }
    else
    {
        // Change termination signal to string
        sprintf(status, "%d", WTERMSIG(childExitStatus));
        // sprintf(status, "%d", errno);

        // Then concatenate exit status and store in curCommand struct
        memset(exitStatus, 0, strlen(exitStatus));
        strcpy(exitStatus, "terminated by signal ");
        strcat(exitStatus, status);

        // printf("Child %d exited abnormally %s\n", spawnpid, exitStatus);
        // fflush(stdout);
    }
}

/*
*
*/
void runFGChild(struct commandElements* curCommand)
{
    // Add to fgProcessIDs array
    pid_t childpid;
    int error;
    // int childExitStatus;
    char status[256];
    
    childpid = getpid();
    // addToFGList(childpid);
    // Child will use a function from the exec() family of functions
    // to run the command
    error = execvp(curCommand->commands[0], curCommand->commands);
    // perror("execvp");

    // If there is an error, then kill child
    if(error == -1)
    {
        // printf("Child %d exited abnormally %d\n", childpid, errno);
        // printf("Child %d exited abnormally %s\n", childpid, exitStatus);
        // fflush(stdout); 

        // Terminate this child
        kill(childpid, errno); 
    }
    // Remove from fgProcessIDs array
    // removeFromFGList(childpid);
    // exit(2);

    // Shell should use PATH variable to look for non-built in
    // commands
    // Shell should allow shell scripts to be executed

    // If a command fails because the shell could not find the
    // command to run, then the shell will print an error message and
    // set the exit status to 1

    // A child process must terminate after running a command (whether
    // the command is successful or it fails)
}

/*
*
*/
void runFGProcess(struct commandElements* curCommand)
{
    pid_t spawnpid = -5;

    spawnpid = fork();
    switch(spawnpid)
    {
        case -1:
            perror("fork() failed!");
            fflush(stderr);
            // exit(1);
            break;
        case 0:     // Child execution
            runFGChild(curCommand);
            break;
        default:    // Parent execution
            runFGParent(spawnpid, curCommand);
            // exit(0);
            break;
    }
}

/*
*
*/
void runBGParent(pid_t spawnpid, struct commandElements* curCommand)
{
    int childExitStatus;
    char status[256];
    
    printf("background pid is %d\n", spawnpid);
    fflush(stdout);

    spawnpid = waitpid(spawnpid, &childExitStatus, WNOHANG);

    // printf("Parent's waiting is done as the child with pid %d exited\n", spawnpid);
    // fflush(stdout);
    
    // if(WIFEXITED(childExitStatus))
    // {
    //     printf("background pid %d is done: exit value %d\n", spawnpid, WEXITSTATUS(childExitStatus));
    //     fflush(stdout);
    // }
    // else
    // {
    //     printf("background pid %d is done: terminated by signal %d\n", spawnpid, WTERMSIG(childExitStatus));
    //     fflush(stdout);
    // }

    // // take out of PID list
    // removeFromPIDList(spawnpid);
}

/*
*
*/
void runBGChild(struct commandElements* curCommand)
{
    // Add to fgProcessIDs array
    pid_t childpid;
    int error;
    // int childExitStatus;
    char status[256];
    
    // Print process id of background process when it begins
    // childpid = getpid();
    
    // fflush(stdout);

    // Child will use a function from the exec() family of functions
    // to run the command
    error = execvp(curCommand->commands[0], curCommand->commands);
    // perror("execvp bg");
    // fflush(stderr);

    // If there is an error, then kill child
    if(error == -1)
    {
        printf("BG Child %d error %d\n", childpid, errno);
        // printf("Child %d exited abnormally %s\n", childpid, exitStatus);
        fflush(stdout); 

        // Terminate this child
        // kill(childpid, errno); 
    }
    // Remove from fgProcessIDs array
    // removeFromFGList(childpid);
    // exit(2);

    // Shell should use PATH variable to look for non-built in
    // commands
    // Shell should allow shell scripts to be executed

    // If a command fails because the shell could not find the
    // command to run, then the shell will print an error message and
    // set the exit status to 1

    // A child process must terminate after running a command (whether
    // the command is successful or it fails)
}

/*
*   Commands ran as background processes. Shell will not wait for
*   these commands to complete. Parent must return command line
*   access and control to the user immediately after forking off the
*   child. The shell will print the process id of a background
*   process when it begins. When a background process terminates, a
*   message showing the process id and exit status will be printed.
*   This message must be printed just before the prompt for a new
*   command is displayed. If the user doesn't redirect the standard
*   input for a background command, then standard input should be
*   redirected to /dev/null. If the user doesn't redirect the
*   standard output for a background command, then standard output
*   should be redirected to /dev/null.
*/
void runBGProcess(struct commandElements* curCommand)
{
    pid_t spawnpid = -5;

    spawnpid = fork();

    // printf("runBGProcess bg spawnpid is: %d\n", spawnpid);
    // fflush(stdout);

    // Add to running list
    if(spawnpid != 0)
    {
        addToPIDList(spawnpid);
    }
    
    switch(spawnpid)
    {
        case -1:
            perror("fork() failed!");
            fflush(stderr);
            // exit(1);
            break;
        case 0:     // Child execution
            runBGChild(curCommand);
            break;
        default:    // Parent execution
            runBGParent(spawnpid, curCommand);
            // exit(0);
            break;
    }
}

/*
*   Run any other commands using fork(), exec(), and waitpid()
*   Foreground commands: any command without an & at the end. Shell
*   must wait for the completion of the command before prompting for
*   the next command. Do not return command line access until child
*   terminates.
*/
void runOtherCommands(struct commandElements* curCommand)
{
    // When non-built in command is received, fork off a child
    
    // First, determine if foreground/background command
    // If foreground
    if(curCommand->fg == true)
    {
        runFGProcess(curCommand);
    }
    else
    {
        // printf("Background: true\n");
        // fflush(stdout);
        runBGProcess(curCommand);
    }

    // Child will use a function from the exec() family of functions
    // to run the command

    // Shell should use PATH variable to look for non-built in
    // commands
    // Shell should allow shell scripts to be executed

    // If a command fails because the shell could not find the
    // command to run, then the shell will print an error message and
    // set the exit status to 1

    // A child process must terminate after running a command (whether
    // the command is successful or it fails)
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

    // // Go through and process all commands
    // for(i = 0; i < curCommand->numArguments; i++)
    // {
        // Check for built in commands 'exit', 'cd', and 'status'
        for(j = 0; j < numBuiltIns; j++)
        {
            // if(strcmp(curCommand->commands[i], builtInCommands[j]) == 0)
            if(strcmp(curCommand->commands[0], builtInCommands[j]) == 0)
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
                curCommand->fg = true;
                curCommand->bg = false;
                // If no argument after cd
                runCdCommand(curCommand); // Change index if needed
                // i = runCdCommand(curCommand, i); // Change index if needed
                break;
            case 3: // status command
                curCommand->fg = true;
                curCommand->bg = false;
                // Prints out either the exit status or the
                // terminating signal of the last foreground process
                // ran by the shell
                printf("%s\n", exitStatus);
                fflush(stdout);
                break;
            default: // none built in
                // printf("Non built in command\n");
                // fflush(stdout);
                // runOtherCommands(curCommand, i);
                runOtherCommands(curCommand);
                break;
        }
    // }
    return isExiting;
}

/*
*
*/
void initializePIDList()
{
    int i;

    for(i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        processIDs[i] = -1;
    }
}

/*
*
*/
void initializeExitStatus()
{
    strcpy(exitStatus, "exit value 0");
}

/*
*
*/
void checkBGProcesses()
{
    int i;
    int childExitStatus;
    pid_t spawnpid = 0;

    // printf("Checking BG processes\n");
    // fflush(stdout);

    for(i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        if(processIDs[i] != -1)
        {
            // printf("Checking BG processID: %d\n", processIDs[i]);
            // fflush(stdout);
            spawnpid = waitpid(processIDs[i], &childExitStatus, WNOHANG);
            
            // printf("Parent's waiting is done as the child with pid %d exited\n", spawnpid);
            // fflush(stdout);
            
            if(spawnpid != 0)
            {
                if(WIFEXITED(childExitStatus))
                {
                    printf("background pid %d is done: exit value %d\n", processIDs[i], WEXITSTATUS(childExitStatus));
                    fflush(stdout);
                }
                else
                {
                    printf("background pid %d is done: terminated by signal %d\n", processIDs[i], WTERMSIG(childExitStatus));
                    fflush(stdout);
                }

                // take out of PID list
                removeFromPIDList(spawnpid);
            }
        }
    }
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

    // Initialize global variables
    initializePIDList();
    initializeExitStatus();

    // Create linked list of commands
    printf("\n");
    fflush(stdout); /* FLUSH AFTER EVERY PRINT! */
    do
    {
        curCommand = getCommandLine();

        // Check if curCommand should not be ignored and commands ran
        if(!curCommand->ignore)
        {
            // printCommandElements(curCommand); // For testing only

            isExiting = runCommands(curCommand);
        }

        checkBGProcesses();
    }
    while(!isExiting);

    printf("\n");
    fflush(stdout);

    // Kill shell
    return EXIT_SUCCESS;
}