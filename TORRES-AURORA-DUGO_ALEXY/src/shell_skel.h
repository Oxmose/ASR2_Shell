#ifndef DEF_SHELL_SKEL_H
#define DEF_SHELL_SKEL_H

// STD INCLUDES
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

// SYSTEM INCLUDES
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <glob.h>


// Define FALSE and TRUE values, makes the code more understandable.
#ifndef FALSE
    #define FALSE (0)
#endif
#ifndef TRUE
    #define TRUE (!FALSE)
#endif

// Max args count
#define ARG_MAX 10

// Output MACRO
#define CRITIC(OUT, CODE) do { \
    fputs("\033[31m", stderr); \
    fputs("[CRITIC] ", stderr); \
    fputs(OUT, stderr); \
    fputs("\n", stderr); \
    fputs(CODE, stderr); \
    fputs("\n", stderr); \
    fputs("\033[37m", stderr); \
    fflush(stderr); \
    exit(EXIT_FAILURE); \
}while(0);

#define ERROR(OUT, CODE) do { \
    fputs("\033[91m", stderr); \
    fputs("[ERROR] ", stderr); \
    fputs(OUT, stderr); \
    fputs("\n", stderr); \
    fputs(CODE, stderr); \
    fputs("\n", stderr); \
    fputs("\033[37m", stderr); \
    fflush(stderr); \
}while(0);

#define WARNING(OUT, CODE) do { \
    fputs("\033[93m", stdout); \
    fputs("[WARNING] ", stdout); \
    fputs(OUT, stdout); \
    fputs("\n", stdout); \
    fputs(CODE, stdout); \
    fputs("\n", stdout); \
    fputs("\033[37m", stdout); \
    fflush(stdout); \
}while(0);

#define INFO(OUT) do { \
    fputs(OUT, stdout); \
    fputs("\n", stdout); \
    fflush(stdout); \
}while(0);

/*
 *removed this define since it was already defined in dirent.h
 *#define PATH_MAX 1000
*/

// Prompt char
static char *prompt_str = "> ";

// User and hostname
static char user_machine_name[PATH_MAX];
static char username[PATH_MAX];

// Current directory
static char wd[PATH_MAX];

// Lock that tells us if we are waiting for a front process to end
static int wait_process = -1;

static int no_prompt = TRUE;

/*
 * ############################################################
 * #######   SIGNALS MANAGEMENT
 * ############################################################
 */

// Save old actions (just in case, may be deleted after)
struct sigaction old_sigchld_action;
struct sigaction old_sigint_action;
struct sigaction old_sigquit_action;

static void manage_signals();
static void reset_signals();
static void child_action(int signum, siginfo_t *siginfo, void *context);
static void sigint_action(int signum, siginfo_t *siginfo, void *context);
static void sigquit_action(int signum, siginfo_t *siginfo, void *context);

/*
 * ############################################################
 * #######   Builtins
 * ############################################################
 */

struct built_in_command {
    char *cmd;                  /* name */
    char *descr;                /* description */
    int (*function) (int, char **); /* function ptr */
}; // struct built_in_command

static int builtin_help(int argc, char **argv);
static int builtin_cd(int argc, char **argv);
static int builtin_pwd(int argc, char **argv);
static int builtin_exec(int argc, char **argv);
static int builtin_exit(int argc, char **argv) {exit(EXIT_SUCCESS);}
static struct built_in_command bltins[] = {
    {"cd", "Change working directory", builtin_cd},
    {"exec", "Exec command, replacing this shell with the exec'd process", builtin_exec},
    {"pwd", "Print working directory", builtin_pwd},
    {"exit", "Exit from shell()", builtin_exit},
    {"help", "List shell built-in commands", builtin_help},
    {NULL, NULL, NULL}
}; // static struct built_in_command bltins[]

/*
 * ############################################################
 * ####### GENERAL
 * ############################################################
 */

// UNUSED
// static int expand_arguments(char *command);

static void clean_path(char *wd);
static int is_empty(const char *command);
static void get_user_machine_name(char *name);
static int get_command(FILE * source, char *command);
static int expand_arguments2(char *command);
static char** get_background_commands(char *command, int *has_background, int *has_last, int *commands_count);
static void clean_command(char *command);
static int run_command(char **command, int *has_pipe, int pipefd1[2], int pipefd2[2], int *commandParity);


/*
 * ############################################################
 * ####### MAIN PROGRAMM
 * ############################################################
 */

void usage();
int main(int argc_l, char **argv_l);

#endif // DEF_SHELL_SKEL_H
