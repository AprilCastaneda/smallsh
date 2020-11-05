#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_COMMAND_LINE_LENGTH 2049 // 2048 characters plus null at the end
#define MAX_COMMAND_LINE_ARGUMENTS 512

// Global variables
int processIDs[MAX_COMMAND_LINE_ARGUMENTS]; // Holds running bg processes
char exitStatus[256]; // Hold exitStatus
bool foregroundOnly = false; // determines if fg only mode

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
    // struct commandElements* next;
};

/* struct for handling SIGINT */
struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};

/*
*   Program that sets in struct if command will run in foreground or
*   background. This is determined by the '&' character, which, if it
*   appears at the end of the commandLine, then the command will run
*   in the background. Otherwise, command will run in the foreground.
*   Struct bool values for fg and bg are set accordingly.
*   Special case if global var foregroundOnly set, then fg is true.
*/
void setCommandPosition(char* commandLine, struct commandElements* curCommand)
{
    if(commandLine[strlen(commandLine) - 1] == '&')
    {
        if(foregroundOnly == false)
        {
            curCommand->fg = false;
            curCommand->bg = true;
        }
        else
        {
            curCommand->fg = true;
            curCommand->bg = false;
        }
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

    // For use with strtok_r
    char *saveptr;

    // Parse command into tokens
    char* token = strtok_r(commandLine, " ", &saveptr);
    int index = 0;
    int numArguments = 0;

    // Go through command line until all arguments parsed
    // If special symbols <, >, & found, process accordingly
    while(token != NULL)
    {
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
*   Finds instances of "$$" and replaces them with pid of shell.
*/
char* replaceString(char* commandLineCopy)
{
    // For use with strtok_r
    char* saveptr;
    char* tempLine = calloc(MAX_COMMAND_LINE_LENGTH, sizeof(char));
    char spid[256];
    int origLen = strlen(commandLineCopy);

    // Change shell pid to string
    sprintf(spid, "%d", getpid());

    // The first token is the title
    char* token = strtok_r(commandLineCopy, "$$", &saveptr);
    
    // If $$ is found, token length will be different than original
    if(strlen(token) != origLen)
    {
        // Store string without $$
        while(token != NULL)
        {
            strcpy(tempLine, token);
            strcat(tempLine, spid);
            token = strtok_r(NULL, "$$", &saveptr);
        }
        return tempLine;
    }
    else
    {
        return commandLineCopy;
    }    
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

    // Get index of newline char and overwrite it with 0
    commandLine[strcspn(commandLine, "\n")] = 0;

    // Pass copy of commandLine to be replaced if it's not null
    char* tempLine = calloc(MAX_COMMAND_LINE_LENGTH, sizeof(char));
    
    if(commandLine[0] != 0)
    {
        strcpy(tempLine, commandLine);
        strcpy(commandLine, replaceString(tempLine));
    }
    
    free(tempLine);

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
*   Handle SIGSTP by checking if global foregroundOnly is set. Toggle
*   value and print appropriate message.
*/
void handle_SIGTSTP(int signo)
{
    if(foregroundOnly)
    {
        foregroundOnly = false;
        write(STDOUT_FILENO, "\nExiting foreground-only mode\n", 31);
    }
    else
    {
        foregroundOnly = true;
        write(STDOUT_FILENO, "\nEntering foreground-only mode (& is now ignored)\n", 51);
    }
}

/*
*   When this command is run, shell kills any other processes or jobs
*   that shell has started before it terminates itself. 
*/
void runExitCommand()
{
    int i;

    // Kill any other processes or jobs that shell has started
    for(i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        if(processIDs[i] != -1)
        {
            kill(processIDs[i], SIGTERM);
        }
    }
    // Shell will be killed in main() by return EXIT_SUCCESS;
}

/*
*   Runs the built in command cd by changing either to HOME or an
*   assigned directory.
*/
void runCdCommand(struct commandElements* curCommand)
{
    char cwd[256];
    char path[256];

    // If no arguments after cd
    if(curCommand->numArguments == 1)
    {
        // Then change to HOME directory
        getcwd(cwd, sizeof(cwd));
        chdir(getenv("HOME"));
        getcwd(cwd, sizeof(cwd));
    }
    // If there is an argument, then change to this
    // directory. Command should support both absolute
    // and relative paths.
    else
    {
        // Get path, which is argument after cd
        strcpy(path, curCommand->commands[1]);
        // If absolute, first char is '/', then chdir
        // with path
        char firstChar = path[0];
        if(firstChar == '/')
        {
            getcwd(cwd, sizeof(cwd));
            chdir(path);
            getcwd(cwd, sizeof(cwd));
        }
        // If relative, first get cwd, add '/',
        // concatenate relative path, then chdir to path
        else
        {
            getcwd(cwd, sizeof(cwd));
            strcat(cwd, "/");
            strcat(cwd, path);
            strcpy(path, cwd); // Did this as path is a better var name
            chdir(path);
            getcwd(cwd, sizeof(cwd));
        }
    }
}

/*
*   Take out process ID from list of running ids
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
*   Add process id to list of running IDs
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
*   Run foreground parent process.
*/
void runFGParent(pid_t spawnpid, struct commandElements* curCommand)
{
    int childExitStatus;
    char status[256];
    
    // Change SIGINT to ignore
    SIGINT_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_action, NULL);

    spawnpid = waitpid(spawnpid, &childExitStatus, 0);

    // Get exit status
    if(WIFEXITED(childExitStatus))
    {
        SIGINT_action.sa_handler = SIG_IGN;
        sigaction(SIGINT, &SIGINT_action, NULL);
        
        // Change exit status to string
        sprintf(status, "%d", WEXITSTATUS(childExitStatus));

        // Then concatenate exit status and store in curCommand struct
        memset(exitStatus,0,strlen(exitStatus));
        strcpy(exitStatus, "exit value ");
        strcat(exitStatus, status);
    }
    else
    {
        SIGINT_action.sa_handler = SIG_IGN;
        sigaction(SIGINT, &SIGINT_action, NULL);

        // Change termination signal to string
        sprintf(status, "%d", WTERMSIG(childExitStatus));

        // Then concatenate exit status and store in curCommand struct
        memset(exitStatus, 0, strlen(exitStatus));
        strcpy(exitStatus, "terminated by signal ");
        strcat(exitStatus, status);

        // If terminated with 2, print to screen
        if(WTERMSIG(childExitStatus) == 2)
        {
            printf("%s\n", exitStatus);
            fflush(stdout);
        }
    }
}

/*
*   Redirect to file inputFile or /dev/null per boolean value
*/
void inputRedirect(struct commandElements* curCommand, bool redir)
{
    int result;
    int sourceFD;
    
    // If redirecting, then redirect to inputFile
    if(redir)
    {
        sourceFD = open(curCommand->inputFile, O_RDONLY);
    
        if(sourceFD == -1)
        {
            printf("cannot open %s for input\n", curCommand->inputFile);
            fflush(stdout);
            exit(1);
        }
        
        result = dup2(sourceFD, 0);
        if(result == -1)
        {
            perror("source dup2() fail\n");
            fflush(stderr);
            exit(2);
        }
    }
    else // Otherwise, redirect to /dev/null
    {
        sourceFD = open("/dev/null", O_RDONLY);
    
        if(sourceFD == -1)
        {
            printf("cannot open %s for input\n", curCommand->inputFile);
            fflush(stdout);
            exit(1);
        }

        result = dup2(sourceFD, 0);
        if(result == -1)
        {
            perror("source dup2() fail\n");
            fflush(stderr);
            exit(2);
        }
    }
}

/*
*   Redirect to file outputFile or /dev/null per bool value
*/
void outputRedirect(struct commandElements* curCommand, bool redir)
{
    int result;
    int targetFD;

    // If redirecting, then redirect to output file
    if(redir)
    {
        targetFD = open(curCommand->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if(targetFD == -1)
        {
            printf("cannot open %s for output\n", curCommand->outputFile);
            fflush(stdout);
            exit(1);
        }

        result = dup2(targetFD, 1);
        if(result == -1)
        {
            perror("target dup2() fail\n");
            fflush(stderr);
            exit(2);
        }
    }
    else // otherwise, redirect to /dev/null
    {
        targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if(targetFD == -1)
        {
            printf("cannot open %s for output\n", curCommand->outputFile);
            fflush(stdout);
            exit(1);
        }

        result = dup2(targetFD, 1);
        if(result == -1)
        {
            perror("target dup2() fail\n");
            fflush(stderr);
            exit(2);
        }
    }
}

/*
*   Run foreground child process
*/
void runFGChild(struct commandElements* curCommand)
{
    // Change SIGINT to default
    SIGINT_action.sa_handler = SIG_DFL;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Change SIGTSTP to ignore
    SIGTSTP_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL); 

    pid_t childpid;
    int error;
    char status[256];
    childpid = getpid();

    // Check if i/o redirect
    if(curCommand->inputRedirect == true) 
    {
        inputRedirect(curCommand, true);
    }
    if(curCommand->outputRedirect == true)
    {
        outputRedirect(curCommand, true);
    }

    // Child will use a function from the exec() family of functions
    // to run the command
    error = execvp(curCommand->commands[0], curCommand->commands);
    printf("%s: ", curCommand->commands[0]);
    fflush(stdout);
    perror("");
    fflush(stderr);

    // If there is an error, then kill child
    if(error == -1)
    {
        kill(childpid, errno); 
    }
}

/*
*   Run all foreground processes, parent and children
*/
void runFGProcess(struct commandElements* curCommand)
{
    pid_t spawnpid = -5;

    // Fork child
    spawnpid = fork();
    switch(spawnpid)
    {
        case -1:
            perror("fork() failed!");
            fflush(stderr);
            break;
        case 0:     // Child execution
            runFGChild(curCommand);
            break;
        default:    // Parent execution
            runFGParent(spawnpid, curCommand);
            break;
    }
}

/*
*   Run background parent process
*/
void runBGParent(pid_t spawnpid, struct commandElements* curCommand)
{
    int childExitStatus;
    char status[256];
    
    printf("background pid is %d\n", spawnpid);
    fflush(stdout);

    // Run in the background and do not wait for child process to finish
    spawnpid = waitpid(spawnpid, &childExitStatus, WNOHANG);
}

/*
*   Run background child process
*/
void runBGChild(struct commandElements* curCommand)
{
    // Change SIGTSTP to ignore
    SIGTSTP_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL); 

    pid_t childpid;
    int error;
    char status[256];

    // Check if i/o redirect
    if(curCommand->inputRedirect == true) 
    {
        inputRedirect(curCommand, true);
    }
    else
    {
        inputRedirect(curCommand, false);
    }

    if(curCommand->outputRedirect == true)
    {
        outputRedirect(curCommand, true);
    }
    else
    {
        outputRedirect(curCommand, false);
    }

    // Child will use a function from the exec() family of functions
    // to run the command
    error = execvp(curCommand->commands[0], curCommand->commands);

    // If there is an error, print error
    if(error == -1)
    {
        printf("BG Child %d error %d\n", childpid, errno);
        fflush(stdout); 
    }
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

    // Fork background child
    spawnpid = fork();

    // Add child to running list
    if(spawnpid != 0)
    {
        addToPIDList(spawnpid);
    }
    
    switch(spawnpid)
    {
        case -1:
            perror("fork() failed!");
            fflush(stderr);
            break;
        case 0:     // Child execution
            runBGChild(curCommand);
            break;
        default:    // Parent execution
            runBGParent(spawnpid, curCommand);
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
    // First, determine if foreground/background command
    // If foreground
    if(curCommand->fg == true)
    {
        runFGProcess(curCommand);
    }
    else
    {
        runBGProcess(curCommand);
    }
}

/*
*   Runs all commands whether they are built in or not.
*/
bool runCommands(struct commandElements* curCommand)
{
    bool isExiting = false;
    int i, j;
    int numBuiltIns = 3;
    int builtInNum = -1;
    int lastFgStatus = 0; 
    int lastFgSignal = 2; 
    char* builtInCommands[numBuiltIns];
    char* pathDir = calloc(256, sizeof(char));
    
    builtInCommands[0] = "exit";
    builtInCommands[1] = "cd";
    builtInCommands[2] = "status";

    // Check for built in commands 'exit', 'cd', and 'status'
    for(j = 0; j < numBuiltIns; j++)
    {
        // If command found
        if(strcmp(curCommand->commands[0], builtInCommands[j]) == 0)
        {
            builtInNum = j + 1;
            break;
        }
    }

    // Determine which command to run
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
            runOtherCommands(curCommand);
            break;
    }

    return isExiting;
}

/*
*   Initialize process ID list
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
*   Initialize first exitStatus
*/
void initializeExitStatus()
{
    strcpy(exitStatus, "exit value 0");
}

/*
*   Fill out SIGINT_action struct. Register SIG_IGN as the
*   signal handler.
*/
void initializeSIGINT()
{
    SIGINT_action.sa_handler = SIG_IGN; // ignore as a default
    sigfillset(&SIGINT_action.sa_mask); // block all catchable signals while handle_SIGINT is running
    SIGINT_action.sa_flags = 0; // no flags set
    sigaction(SIGINT, &SIGINT_action, NULL);
}

/*
*   Fill out SIGSTP_action struct. Register handle_SIGSTP as the
*   signal handler.
*/
void initializeSIGTSTP()
{
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask); 
    // block all catchable signals while handle_SIGSTP is running
    SIGTSTP_action.sa_flags = 0; // no flags set
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/*
*   Check all background processes and see which are running or not.
*/
void checkBGProcesses()
{
    int i;
    int childExitStatus;
    pid_t spawnpid = 0;

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
    initializeSIGINT();
    initializeSIGTSTP();

    printf("\n");
    fflush(stdout); 

    // Loop through shell
    do
    {
        curCommand = getCommandLine();

        // Check if curCommand should not be ignored and commands ran
        if(!curCommand->ignore)
        {
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