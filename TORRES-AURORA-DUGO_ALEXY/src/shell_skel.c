// STD INCLUDES
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

// SYSTEM INCLUDES
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <glob.h>

// HEADER
#include "shell_skel.h"

/*
 * ############################################################
 * #######   SIGNALS MANAGEMENT
 * ############################################################
 */

static void manage_signals()
{
    // Redirect SIGCHLD structure
    struct sigaction new_sigchld_action;
    new_sigchld_action.sa_sigaction = &child_action;
    new_sigchld_action.sa_flags = SA_SIGINFO;

    // Redirect SIGINT structure
    struct sigaction new_sigint_action;
    new_sigint_action.sa_sigaction = &sigint_action;
    new_sigint_action.sa_flags = SA_SIGINFO;

    // Redirect SIGQUIT structure
    struct sigaction new_sigquit_action;
    new_sigquit_action.sa_sigaction = &sigquit_action;
    new_sigquit_action.sa_flags = SA_SIGINFO;

    // Redirect signals structure
    if(sigaction(SIGCHLD, &new_sigchld_action, &old_sigchld_action) == -1)
    {
        CRITIC("Can't redirect SIGCHLD to custom handler.", strerror(errno));
    }
    if(sigaction(SIGINT, &new_sigint_action, &old_sigint_action) == -1)
    {
        CRITIC("Can't redirect SIGINT to custom handler.", strerror(errno));
    }
    if(sigaction(SIGQUIT, &new_sigquit_action, &old_sigquit_action) == -1)
    {
        CRITIC("Can't redirect SIGQUIT to custom handler.", strerror(errno));
    }

} // manage_signals()

static void reset_signals()
{
    // Reset the signals to their old action
    if(sigaction(SIGCHLD, &old_sigchld_action, NULL) == -1)
    {
        CRITIC("Can't redirect SIGCHLD to default handler.", strerror(errno));
    }
    if(sigaction(SIGINT, &old_sigint_action, NULL) == -1)
    {
        CRITIC("Can't redirect SIGINT to default handler.", strerror(errno));
    }
    if(sigaction(SIGQUIT, &old_sigquit_action, NULL) == -1)
    {
        CRITIC("Can't redirect SIGQUIT to default handler.", strerror(errno));
    }
} // reset_signals()

static void child_action(int signum, siginfo_t *siginfo, void *context)
{
    // If the dead child is not the front process
    // (Since front process death handling is done
    // in the main loop)
    if(siginfo->si_pid != wait_process)
    {
        int status;
        // Wait pid of child
        if(waitpid(siginfo->si_pid, &status, 0) == -1)
        {
            ERROR("Wait on pid.", strerror(errno));
        }
    }
} // child_action(int, iginfo_t*, void*)

static void sigint_action(int signum, siginfo_t *siginfo, void *context)
{
    if(no_prompt)
    {
        fputs("\n", stdout);
        char *white = "\033[0m";
        char *color = "\033[32m";

        fputs(color, stdout);
        fputs(user_machine_name, stdout);
        fputs(white, stdout);
        fputs(wd, stdout);
        fputs(prompt_str, stdout);
        fflush(stdin);
    }
} // sigint_action(int, iginfo_t*, void*)

static void sigquit_action(int signum, siginfo_t *siginfo, void *context)
{
    // On quit just got to a newline
    fputs("\n", stdout);
} // sigquit_action(int, iginfo_t*, void*)

/*
 * ############################################################
 * #######   Builtins
 * ############################################################
 */

static int builtin_pwd(int argc, char **argv)
{
    // If there are more than one argument, can't process that
    if(argc != 1)
    {
        ERROR("Usage is : pwd", "\n");
        return EXIT_FAILURE;
    }

    // Update wd value (in case it was modified by an other process)
    if(getcwd(wd, PATH_MAX) == NULL)
    {
        ERROR("Getting current directory", strerror(errno));
        return EXIT_FAILURE;
    }

    // Display current working directory
    printf("%s\n", wd);

    // Makes wd more "readable"
    clean_path(wd);

    return EXIT_SUCCESS;
} // int builtin_pwd(int, char**)

static int builtin_cd(int argc, char **argv)
{
    // If there are more than one argument, can't process that
    if(argc != 2)
    {
        ERROR("Usage is : cd <path>", "\n");
        return EXIT_FAILURE;
    }

    // Moving directory
    if(chdir(argv[1]) != 0)
    {
        ERROR("Moving directory", strerror(errno));
        return EXIT_FAILURE;
    }
    
    // Getting current directory
    if(getcwd(wd, PATH_MAX) == NULL)
    {
        ERROR("Getting current directory", strerror(errno));
        return EXIT_FAILURE;
    }

    // Makes wd more "readable"
    clean_path(wd);

    return EXIT_SUCCESS;
} // int builtin_cd(int, char**)

static int builtin_exec(int argc, char **argv)
{
    // Used to redirect output
    char file[PATH_MAX];
    int redir = 0;

    // Get the right array of arguments to send
    char *args[argc];
    int i;    
    for(i = 1; i < argc; ++i)
    {
        // Search for the redirection sign
        if(strcmp(argv[i], ">") == 0 && i < argc + 1)
        {
            strcpy(file, argv[i + 1]);
            redir = 1;
            break;
        }

        // Add arguments
        args[i - 1] = malloc(sizeof(char) * strlen(argv[i]));
        strcpy(args[i - 1], argv[i]);
    }  
 
    args[argc - 1] = NULL;

    if(redir == 1)
    {
        // Flush before redir
        fflush(stdout);

        // File will be written atthe exit of the programm
        int fd_out = open(file, O_RDWR|O_CREAT|O_APPEND, 0660);
        if(fd_out == -1)
        {
            ERROR("Can't open redirection file.", strerror(errno));
            return EXIT_FAILURE;
        }
        if(dup2(fd_out, fileno(stdout)) == -1)
        {
            ERROR("dup2 : can't redirect stdout to file.", strerror(errno));
            return EXIT_FAILURE;
        }
    }
    
    // Reset signals to default
    reset_signals();

    // Execute process
    execvp(args[0], args);

    WARNING("Wrong command", strerror(errno));

    // Make signal normal again
    manage_signals();

    return EXIT_FAILURE;
} // int builtin_exec(int, char**)

static int builtin_help(int argc, char **argv)
{
    struct built_in_command *x;

    printf("\nBuilt-in commands:\n");
    printf("-------------------\n");
    for (x = bltins; x->cmd; x++) {
        if (x->descr==NULL)
            continue;
        printf("%s\t%s\n", x->cmd, x->descr);
    }
    printf("\n");
    return EXIT_SUCCESS;
} // int builtin_help(int, char**)


/*
 * ############################################################
 * ####### GENERAL
 * ############################################################
 */

static void clean_path(char *wd)
{
    char homePath[PATH_MAX] = "/home/";
    strcat(homePath, username);

    // Search for the home directory pattern to replace it whith ~
    char resulting_path[PATH_MAX];
    char partial[PATH_MAX];
    unsigned int i;
    for(i = 0; i < strlen(wd) && i < PATH_MAX - 1; ++i)
    {
        partial[i] = wd[i];
        partial[i + 1] = '\0';
        if(strcmp(partial, homePath) == 0)
        {
            resulting_path[0] = '~';
            unsigned int j;
            unsigned int p = 1;
            for(j = i + 1; j < strlen(wd); ++j)
            {
                resulting_path[p] = wd[j];
                ++p;
            }
            resulting_path[p] = '\0';
            strcpy(wd, resulting_path); 
            break;
        }
    }   
} // clean_path(char*)

static void get_user_machine_name(char *name)
{
    char *result;
    char hostname[1024];
    uid_t uid = geteuid();
    // Get the user informations
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        result = pw->pw_name;
        strcpy(username, result);
        strcat(result, "@");
        size_t len = 1024;
        // Get the hostname
        if(gethostname(hostname, len) == 0)
        {
            strcat(result, hostname);
            strcat(result, ":");
            strcpy(name, result);
        }
    
    }
} // get_user_machine_name(char*)

static int get_command(FILE * source, char *command)
{
    if (source == stdin) {
        char *white = "\033[0m";
        char *color = "\033[32m";

        fputs(color, stdout);
        fputs(user_machine_name, stdout);
        fputs(white, stdout);
        fputs(wd, stdout);
        fputs(prompt_str, stdout);
    }
    
    no_prompt = TRUE;
    if (!fgets(command, BUFSIZ - 2, source)) {        
        if (source == stdin && errno != 4)
        {
            printf("\n");
            return 1;
        }
        // Manage parralels process, to be cleaned, this is not a good way
        // TODO ASK in practical
        no_prompt = TRUE;
        while(!fgets(command, BUFSIZ - 2, source) && errno == 4);
    }
    no_prompt = FALSE;
    return EXIT_SUCCESS;
} // int get_command(FILE*, char*)

/*static int expand_arguments(char *command)
{
    char *saved_ptr = NULL;
    char resulting_command[PATH_MAX];
    char *token = strtok_r(command, " ", &saved_ptr);
    char *next_tokens = strtok_r(NULL, "", &saved_ptr);
    resulting_command[0] = '\0';
    do
    {       
        if(token[strlen(token) - 1] == '\n')
            token[strlen(token) - 1] = '\0';

        // If token is not *
        if(strcmp(token, "*") != 0)
        {
            strcat(resulting_command, token);
            strcat(resulting_command," ");
        }
        else
        {
            DIR *current_dir = opendir(wd);
            struct dirent *entry;

            while((entry = readdir(current_dir)))
            {
                if(entry->d_name[0] != '.')
                {
                    strcat(resulting_command, entry->d_name);
                    strcat(resulting_command, " ");
                }
            }   
        }

        if(next_tokens != NULL)
        {
            token = strtok_r(next_tokens, " ", &saved_ptr);
            next_tokens = strtok_r(NULL, "", &saved_ptr);
        }
        else
            token = NULL;

    } while(token != NULL);
    
    strcpy(command, resulting_command);

    return TRUE;
} // int expand_arguments(char*)*/

static int expand_arguments2(char *command)
{
    char *saved_ptr = NULL;
    char resulting_command[PATH_MAX];
    char *token = strtok_r(command, " ", &saved_ptr);
    char *next_tokens = strtok_r(NULL, "", &saved_ptr);
    resulting_command[0] = '\0';
    
    // Browse the command    
    do
    {       
        if(token[strlen(token) - 1] == '\n')
            token[strlen(token) - 1] = '\0';

        // If token is not *
        if(strcmp(token, "*") != 0)
        {
            strcat(resulting_command, token);
            strcat(resulting_command," ");
        }
        else
        {
            // Get pattern replacement
            glob_t glob_result;
            int ret;
            if((ret = glob("*", GLOB_TILDE, NULL, &glob_result)) != 0)
            {
                ERROR("Glob pattern math.", "Can't continue");
                return FALSE;
            }  
            unsigned int i;
            for(i = 0; i < glob_result.gl_pathc; ++i)
            {
                strcat(resulting_command, glob_result.gl_pathv[i]);
                strcat(resulting_command, " ");
            }
            globfree(&glob_result);
        }

        // Manage end of the command
        if(next_tokens != NULL)
        {
            token = strtok_r(next_tokens, " ", &saved_ptr);
            next_tokens = strtok_r(NULL, "", &saved_ptr);
        }
        else
            token = NULL;

    } while(token != NULL);
    
    strcpy(command, resulting_command);
    return TRUE;
} // int expand_arguments2(char*)

static char** get_background_commands(char *command, int *has_background, int *has_last, int *commands_count)
{
    char **background = malloc(sizeof(char*));
    int background_mem = 1;

    *commands_count = 0;

    strcat(command, " ");

    char *saved_ptr = NULL;
    char *cur_command = strtok_r(command, "&", &saved_ptr);
    char *next_commands = strtok_r(NULL, "", &saved_ptr);

    // Manage commang, we didn"t find any &
    if(next_commands == NULL)
    {
        *commands_count = 1;
        // If the last, so is a front command
        *has_last = TRUE;
        *has_background = FALSE;
        background[0] = command;
        return background;
    }

    *has_background = TRUE;
    *has_last = FALSE;

    background[0] = malloc(sizeof(char) * (strlen(cur_command) + 1));
    strcpy(background[0], cur_command);
    background[0][strlen(cur_command)] = '\0';
    ++*commands_count;
    
    int i = 1;
    // Browse the command
    do
    {
        // f this is the last command and should not be processed in background
        if(strcmp(next_commands, " ") == 0)
        {
            *has_last = FALSE;
            break;
        }

        *has_last = TRUE;
        // Dynamic array management
        if(i > background_mem)
        {
            background_mem *= 2;
            background = realloc(background, sizeof(char*) * background_mem);            
        }

        cur_command = strtok_r(next_commands, "&", &saved_ptr);
        next_commands = strtok_r(NULL, "", &saved_ptr);

        // Add the current read command to background commands
        background[i] = malloc(sizeof(char) * (strlen(cur_command) + 1));
        strcpy(background[i], cur_command);
        background[i][strlen(cur_command)] = '\0';
        
        ++i;
        ++*commands_count;
    }while(next_commands != NULL);
    
    return background;
} // char** get_background_commands(char*, int*, int*, int*)

static void clean_command(char *command)
{
    // Remove \n and ' '
    while(!isalnum(command[0]))
        ++command;
    size_t last = strlen(command) - 1;
    while(command[last] == '\n' || command[last] == ' ')
        command[last--] = '\0';
} // clean_command(char*)

static int is_empty(const char *command)
{
    int alpha = TRUE;
    unsigned int i;
    for(i = 0; i < strlen(command); ++i)
    {
        if(isalnum(command[i]))
        {
            alpha = FALSE;
            break;
        }
    }
    return alpha;
} // static int is_empty(const char*)

static int run_command(char **command, int *has_pipe, int pipefd1[2], int pipefd2[2], int *commandParity)
{
    char *saved_ptr, *saved_ptr2 = NULL;
    char *cur_command = strtok_r(command[0], "|", &saved_ptr);
    char *next_commands = strtok_r(NULL, "", &saved_ptr);

    //Manage pipe
    int position = 2;

    // Here, pipe created but no next command, we are in the last command
    if(next_commands == NULL && *has_pipe)
    {
        *has_pipe = FALSE;
        position = 1;
        ++*commandParity;
        *commandParity %= 2;
    }
    // Here no pipe created but command has pipe, we are in the first commmand
    else if(next_commands != NULL && !*has_pipe)
    {
        *has_pipe = TRUE;
        position = -1;
        *commandParity = 0;
    }
    // Here, a pipe was created and we are not in the last of first command
    else if(next_commands != NULL && *has_pipe)
    {   
        position = 0;
        ++*commandParity;
        *commandParity %= 2;
    }
    
    if(position == 2)
        *commandParity = 0;
    
    *command = next_commands;

    // Clean the command
    clean_command(cur_command);

    // We use this to manage & symbols
    int has_background;
    int has_last;
    int commands_count;
    char **background_commands = get_background_commands(cur_command, &has_background, &has_last, &commands_count);
    int is_front = FALSE;
    int c;

    for(c = 0; c < commands_count; ++c)
    {
        // Manage last command
        if((has_last && c == commands_count - 1) || !has_background)
        {
            is_front= TRUE;
        }

        // Get the command name
        char *token = strtok_r(background_commands[c], " ", &saved_ptr2);

        if(token == NULL)
        {
            return EXIT_SUCCESS;
        }

        if(token[strlen(token) - 1] == '\n')
            token[strlen(token) - 1]= '\0';

        // Allocate array (dynamically resized
        int parametersCount = 2;
        int parametersRemaining = 1;
        int argc = 0;
        char **parameters = malloc(sizeof(char*) * parametersCount);
        parameters[argc++] = token;

        // Get the command arguments
        char *next_tokens = strtok_r(NULL, "", &saved_ptr2);
        while(next_tokens != NULL)
        {
            if(next_tokens[0] == ' ')
                break;

            if(parametersRemaining == 0)
            {
                parametersRemaining = parametersCount;
                parametersCount *= 2;
                parameters = realloc(parameters, sizeof(char*) * parametersCount);
            }
            char *next_token = strtok_r(next_tokens, " ", &saved_ptr2);
            if(next_token[strlen(next_token) - 1] == '\n')
                next_token[strlen(next_token) - 1]= '\0';

            parameters[argc++] = next_token;
            --parametersRemaining;

            next_tokens = strtok_r(NULL, "", &saved_ptr2);
        }

        int i;
        // Clean args
        char **full_params = malloc(sizeof(char*) * (argc + 1));
        full_params[argc] = NULL;
        full_params[0] = malloc(PATH_MAX * sizeof(char));
        for(i = 1; i < argc; ++i)
        {
            full_params[i] = malloc(PATH_MAX * sizeof(char));
            strcpy(full_params[i], parameters[i]);
        }
        strcpy(full_params[0], token);

        // Manage pipes
        // If first process
        if(*has_pipe && position == -1)
        {
            if(pipe(pipefd1) == -1)
            {
                CRITIC("FAILED TO CREATE PIPE", strerror(errno));
            }
        }
        else if(*has_pipe || position == 1)
        {
            if(*commandParity)
            {
                if(pipe(pipefd2) == -1)
                {
                    CRITIC("FAILED TO CREATE PIPE", strerror(errno));
                }
            }
            else
            {
                if(pipe(pipefd1) == -1)
                {
                    CRITIC("FAILED TO CREATE PIPE", strerror(errno));
                }
            }
        }

        // Search for built in function
        for(i = 0; bltins[i].cmd; ++i)
        {     
            if(strcmp(token, bltins[i].cmd) == 0)
            {
                if(!is_front)
                {
                    pid_t process = fork();
                    if(!process)
                    {
                        // We are in the forked branch                    
                        bltins[i].function(argc, full_params);
                        exit(EXIT_SUCCESS);
                    }
                    else
                    {
                        return EXIT_SUCCESS;
                    }
                }
                else
                {
                   
                    return bltins[i].function(argc, parameters);  
                }                  
            }
        }

        /*
        * If we are here, it mean the command is not a builtin one.
        */
        
        pid_t process = fork();
        if(process == -1)
        {
            ERROR("Can't fork process.", strerror(errno));
            return EXIT_FAILURE;
        }
        wait_process = process;
        if(!process)
        {
            // We are in the forked branch
            // Reset signals to default
            reset_signals();

            // Search for redirection
            // Used to redirect output
            char file[PATH_MAX];
            int redir = 0;

            char *args[argc];

            int i;    
            for(i = 0; i < argc; ++i)
            {
                // Search for the redirection sign
                if(strcmp(full_params[i], ">") == 0 && i < argc + 1)
                {
                    strcpy(file, full_params[i + 1]);
                    redir = 1;
                    args[i] = NULL;
                    break;
                }
                args[i] = malloc(sizeof(char) * strlen(full_params[i]));
                strcpy(args[i], full_params[i]);
            }

            if(sizeof(args) > (size_t)argc)
            {
                args[argc] = NULL;
            }

            if(redir == TRUE)
            {
                // Flush before redir
                fflush(stdout);

                // File will be written atthe exit of the programm
                int fd_out = open(file, O_RDWR|O_CREAT|O_APPEND, 0660);
                if(fd_out == -1)
                {
                    ERROR("Can't open redirection file.", strerror(errno));
                    return EXIT_FAILURE;
                }
                if(dup2(fd_out, fileno(stdout)) == -1)
                {
                    ERROR("dup2 : can't redirect stdout to file.", strerror(errno));
                    return EXIT_FAILURE;
                }
            }

            // Manage PIPE
            // If first process)
            if(*has_pipe && position == -1)
            {
                if(dup2(pipefd1[1], fileno(stdout)) == -1)
                {
                    CRITIC("dup2 : can't redirect stdout to pipe.", strerror(errno));
                }

                if(close(pipefd1[0])  == -1)
                {
                    ERROR("Can't close pipe read end.1", strerror(errno));
                }             
            }
            else if(*has_pipe && position == 0)
            {
                if(*commandParity)
                {
                    if(dup2(pipefd1[0], fileno(stdin)) == -1)
                    {
                        CRITIC("dup2 : can't redirect stdin to pipe.", strerror(errno));
                    }
                    if(dup2(pipefd2[1], fileno(stdout)) == -1)
                    {
                        CRITIC("dup2 : can't redirect stdout to pipe.", strerror(errno));
                    }

                    if(close(pipefd1[1]) == -1)
                    {
                        ERROR("Can't close pipe write end.", strerror(errno));
                    } 
                }
                else
                {
                    if(dup2(pipefd2[0], fileno(stdin)) == -1)
                    {
                        CRITIC("dup2 : can't redirect stdin to pipe.", strerror(errno));
                    }
                    if(dup2(pipefd1[1], fileno(stdout)) == -1)
                    {
                        CRITIC("dup2 : can't redirect stdout to pipe.", strerror(errno));
                    }
                    

                    if(close(pipefd1[0]) == -1)
                    {
                        ERROR("Can't close pipe read end.2", strerror(errno));
                    }  
                    if(close(pipefd2[1]) == -1)
                    {
                        ERROR("Can't close pipe write end.", strerror(errno));
                    }  
                }
            }
            else if(position == 1)
            {
                if(*commandParity)
                {
                    if(dup2(pipefd1[0], fileno(stdin)) == -1)
                    {
                        CRITIC("dup2 : can't redirect stdin to pipe.", strerror(errno));
                    }

                    if(close(pipefd1[1]) == -1)
                    {
                        ERROR("Can't close pipe write end.", strerror(errno));
                    }  
                }
                else
                {
                    if(dup2(pipefd2[0], fileno(stdin)) == -1)
                    {
                        CRITIC("dup2 : can't redirect stdin to pipe.", strerror(errno));
                    }

                    if(close(pipefd2[1]) == -1)
                    {
                        ERROR("Can't close pipe write end.", strerror(errno));
                    }  
                }
            }

            // Execute
            execvp(token, args);

            WARNING("Wrong command", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else
        {
            // Free memory
            for(i = 0; i < argc + 1; ++i)
                free(full_params[i]);

            free(full_params);

            if(position == -1)
            {
                if(close(pipefd1[1]) == -1)
                {
                    ERROR("Can't close pipe write end.", strerror(errno));
                }  
            }
            else if(position == 0)
            {
                if(*commandParity)
                {
                    if(close(pipefd1[0]) == -1)
                    {
                        ERROR("Can't close pipe read end.4", strerror(errno));
                    } 
                    if(close(pipefd2[1]) == -1)
                    {
                        ERROR("Can't close pipe write end.", strerror(errno));
                    }  
                }
                else
                {
                    if(close(pipefd1[1]) == -1)
                    {
                        ERROR("Can't close pipe write end.", strerror(errno));
                    } 
                    if(close(pipefd2[0]) == -1)
                    {
                        ERROR("Can't close pipe read end.5", strerror(errno));
                    } 
                }
            }            
            else if(position == 1)
            {
                if(close(pipefd1[0]) == -1)
                {
                    ERROR("Can't close pipe read end.6", strerror(errno));
                } 
                if(close(pipefd1[1]) == -1)
                {
                    ERROR("Can't close pipe write end.", strerror(errno));
                }
            }
            if(is_front)
            {                 
                return process;
            }
        }
    }
    return EXIT_SUCCESS;
} // int run_command(char**, int*, int[2], int[2], int*)


/*
 * ############################################################
 * ####### MAIN PROGRAMM
 * ############################################################
 */


void usage()
{
    fprintf(stderr, "Usage: [options] ]\n" );
    fprintf(stderr, "\tOptions: \n" );
    fprintf(stderr, "\t\t-h  \t : help\n" );
    fprintf(stderr, "\t\t-i  \t : interactive (default)\n" );
} // usage()

int main(int argc_l, char **argv_l)
{
    int opt, interactive=FALSE;

    FILE *input = stdin;
    interactive=TRUE;
    int script = FALSE;

    

    while ((opt = getopt(argc_l, argv_l, "ihc:")) > 0) {
        switch (opt) {
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            case 'c':
                input = fopen(argv_l[2], "r");
                if(input == NULL)
                {
                    ERROR("Error while opening script file", "\n");
                    input = stdin;
                    break;
                }
                script = TRUE;
                interactive=FALSE;
                break;
            case 'i':
                input = stdin;
                interactive=TRUE;
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

    if (interactive==TRUE) {
        printf( "\n\n\033[34mHello from ASR2 Shell\n");
        printf( "Enter 'help' for a list of built-in commands.\033[37m\n\n");
    }

    // INIT PROMT INFO AND CURRENT PATH
    if(getcwd(wd, PATH_MAX) == NULL)
    {
        ERROR("Can't get current directory", strerror(errno));
    }

    get_user_machine_name(user_machine_name);

    clean_path(wd);

    char *command;
    char *next_command = NULL;
    command = (char *) calloc(BUFSIZ, sizeof(char));

    // Signals management
    manage_signals();

    // Pipe management
    int has_pipe = FALSE;
    int pipefd1[2];
    int pipefd2[2];
    int commandParity = 0;

    while (TRUE) {

        if(script && fgetc(input) == EOF)
        {
            input = stdin;
            script = FALSE;
        }
        else if(script)
            fseek(input, -1, SEEK_CUR);

        if (!next_command) {

            int value;
            if ((value = get_command(input, command)))
            {
                ERROR("Command management", strerror(errno));
                break;
            }
            next_command = command;
        }

        if (is_empty(next_command) || ! expand_arguments2(next_command)) {
            free(command);
            command = (char *) calloc(BUFSIZ, sizeof(char));
            next_command = NULL;
            continue;
        }

        // if run_command started a new process with fork (0 is built in, 1 is failure)
        pid_t process = run_command(&next_command, &has_pipe, pipefd1, pipefd2, &commandParity);
        if(process > 1)
        {
            int status;
            // Wait pid of child
            int ret;
            do
	    {
		waitpid(process, &status, 0);
            }while(!WIFEXITED(status) && !WIFSIGNALED(status));
            wait_process = -1;
        }

        if(next_command == NULL){
            free(command);
            command = (char *) calloc(BUFSIZ, sizeof(char));
        }         
    }
    free(command);
    return EXIT_SUCCESS;
} // int main(int, char**)
