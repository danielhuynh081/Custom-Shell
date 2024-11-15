
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#include <fcntl.h>  // Required for open()
#include <sys/stat.h>
#include <sys/wait.h>
#include "cmd_parse.h"

//Define Macros
#define MAX_ARGS 16
#define HIST 16
#define MAX_CMD_LEN 256

//Define functions
char * historylist[HIST];
void addhistory(const char * newcmd, char * history[]);
void read_command( char * newinput);
void displayhistory(char * history[]);
void exec_external(cmd_t * cmd);
void echofunc(param_t * print, int argcount);
bool is_verbose;


// 1. bye done
// 2. cd / cd dir done
// 3. cwd done
// 4. history done
// 5. echo done


//Main
int main(int argc, char * argv[]){
    char str[MAX_STR_LEN] = {'\0'};
    char *ret_val = NULL;
    char *raw_cmd = NULL;
    char cwd[1024];
    char server[50];
    cmd_list_t *cmd_list = NULL;
    int cmd_count = 0;
    char prompt[10] = {'\0'};


    simple_argv(argc,argv);
    //signal check man page for 2nd param signal int handle function 


    for ( ; ; ) {
	    // Set up a cool user prompt.
	    // test to see of stdout is a terminal device (a tty)
	    // replace the cdw using slides
	    if (getcwd(cwd, sizeof(cwd)) == NULL) {
		    perror("getcwd() error");
		    exit(EXIT_FAILURE);
	    }
	    if(gethostname(server, sizeof(server)) == -1){
		    perror("host name error");
		    exit(EXIT_FAILURE);
	    }
	    if(isatty(fileno(stdout))){
		    printf(" PSUsh %s\n%s@%s # ", cwd, getenv("LOGNAME"), server);
		    fflush(stdout);
	    }
	    fputs(prompt, stdout);
	    memset(str, 0, MAX_STR_LEN);
	    ret_val = fgets(str, MAX_STR_LEN, stdin);

	    if (NULL == ret_val) {
	    printf("\nExiting the shell.\n");
	    exit(EXIT_SUCCESS);
		    break;
	    }

	    // STOMP on the pesky trailing newline returned from fgets().
	    if (str[strlen(str) - 1] == '\n') {
		    // replace the newline with a NULL
		    str[strlen(str) - 1] = '\0';
	    }
	    if (strlen(str) == 0) {
		    // An empty command line.
		    // Just jump back to the promt and fgets().
		    // Don't start telling me I'm going to get cooties by
		    // using continue.
		    continue;
	    }

	    if (strcmp(str, BYE_CMD) == 0)
	    {
		    //free history
		    printf("Exiting The Shell.\n");
		    exit(EXIT_SUCCESS);
	    }

	    // I put the update of the history of command in here.
	    addhistory(ret_val, historylist);

	    // Basic commands are pipe delimited.
	    // This is really for Stage 2.
	    raw_cmd = strtok(str, PIPE_DELIM);

	    cmd_list = (cmd_list_t *) calloc(1, sizeof(cmd_list_t));

	    // This block should probably be put into its own function.
	    cmd_count = 0;
	    while (raw_cmd != NULL ) {
		    cmd_t *cmd = (cmd_t *) calloc(1, sizeof(cmd_t));

		    cmd->raw_cmd = strdup(raw_cmd);
		    cmd->list_location = cmd_count++;

		    if (cmd_list->head == NULL) {
			    // An empty list.
			    cmd_list->tail = cmd_list->head = cmd;
		    }
		    else {
			    // Make this the last in the list of cmds
			    cmd_list->tail->next = cmd;
			    cmd_list->tail = cmd;
		    }
		    cmd_list->count++;

		    // Get the next raw command.
		    raw_cmd = strtok(NULL, PIPE_DELIM);
	    }
	    // Now that I have a linked list of the pipe delimited commands,
	    // go through each individual command.
	    parse_commands(cmd_list);

	    // This is a really good place to call a function to exec the
	    // the commands just parsed from the user's command line.
	    exec_commands(cmd_list);

	    // We (that includes you) need to free up all the stuff we just
	    // allocated from the heap. That linked list of linked lists looks
	    // like it will be nasty to free up, but just follow the memory.
//	    free_list(cmd_list);
	    cmd_list = NULL;
    }
    //free history function

    return(EXIT_SUCCESS);
}

void simple_argv(int argc, char *argv[] )
{
	int opt;

	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
			case 'h':
				// help
				// Show something helpful
				fprintf(stdout, "You must be out of your Vulcan mind if you think\n"
						"I'm going to put helpful things in here.\n\n");
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				// verbose option to anything
				// I have this such that I can have -v on the command line multiple
				// time to increase the verbosity of the output.
				is_verbose=true;
				if (is_verbose) {
					fprintf(stderr, "verbose: verbose option selected: %d\n"
							, is_verbose);
				}
				break;
			case '?':
				fprintf(stderr, "*** Unknown option used, ignoring. ***\n");
				break;
			default:
				fprintf(stderr, "*** Oops, something strange happened <%c> ... ignoring ...***\n", opt);
				break;
		}
	}
}


void 
exec_commands( cmd_list_t *cmds ) 
{
    cmd_t *cmd = cmds->head;

    if (1 == cmds->count) {
        if (!cmd->cmd) {
            // if it is an empty command, bail.
            return;
        }
        if (0 == strcmp(cmd->cmd, CD_CMD)) {
            if (0 == cmd->param_count) {
                // Just a "cd" on the command line without a target directory
                // need to cd to the HOME directory.
		    chdir(getenv("HOME"));
            }
            else {
                // try and cd to the target directory. It would be good to check
                // for errors here.
                if (0 == chdir(cmd->param_list->param)) {
                    // a happy chdir!  ;-)
                }
                else {
			perror( "\nerror finding directory\n");
			exit(EXIT_FAILURE);
                    // a sad chdir.  :-(
                }
            }
        }
        else if (0 == strcmp(cmd->cmd, CWD_CMD)) {
            char str[100];

            // Fetch the Current Working Wirectory (CWD).
            // aka - get country western dancing
            getcwd(str, 100); 
            printf(" " CWD_CMD ": %s\n", str);
        }
        else if (0 == strcmp(cmd->cmd, ECHO_CMD)) {
            // insert code here
            // insert code here
		echofunc(cmd->param_list, cmd->param_count);
            // Is that an echo?
        }
        else if (0 == strcmp(cmd->cmd, HISTORY_CMD)) {
		displayhistory(historylist);
        }
        else {
            // A single command to create and exec
            // If you really do things correctly, you don't need a special call
            // for a single command, as distinguished from multiple commands.
	    exec_external(cmd);
        }
    }
    else {
        // Other things???
        // More than one command on the command line. Who'da thunk it!
        // This really falls into Stage 2.
    }
}

void exec_external(cmd_t * cmd){
	pid_t pid = fork();


	if(pid == -1){
		perror("Failure Forking");
		exit(EXIT_FAILURE);
	}else if(pid ==0){
		param_t *param = cmd->param_list;
		int i = 1; 	
		char *args[MAX_ARGS];
		args[0] = cmd->cmd;
		while (param != NULL) {
			args[i] = param->param;
			param = param->next;
			i++;
		}

		args[i] = NULL;
		if(cmd->output_dest == REDIRECT_FILE){
			int fdout;
			fdout = open(cmd->output_file_name,  O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
			if(fdout<0){
				fprintf(stderr,"******* redir out failed %d *******\n", errno);
				exit(7);
			}
			dup2(fdout, STDOUT_FILENO);
			close(fdout);
		}
		if(cmd->input_src == REDIRECT_FILE){
			int fdin;
			fdin = open(cmd->input_file_name,  O_RDONLY);
			if(fdin<0){
				fprintf(stderr,"******* redir in failed %d *******\n", errno);
				exit(7);
			}
			dup2(fdin, STDIN_FILENO);
			close(fdin);
		}
		// Execute the external command
		if (execvp(cmd->cmd, args) == -1) {
			perror("Exec failed");
			exit(EXIT_FAILURE);
		}
	}else{
		wait(NULL);
	}

}
/*

void
free_list(cmd_list_t *cmd_list)
{
    // Proof left to the student.
    // You thought I was going to do this for you! HA! You get
    // the enjoyment of doing it for yourself.
    return;
}
*/
void
print_list(cmd_list_t *cmd_list)
{
    cmd_t *cmd = cmd_list->head;

    while (NULL != cmd) {
        print_cmd(cmd);
        cmd = cmd->next;
    }
}
/*
void
free_cmd (cmd_t *cmd)
{
    // Proof left to the student.
    // Yep, on yer own.
    // I beleive in you.
    return;
}
*/
// Oooooo, this is nice. Show the fully parsed command line in a nice
// easy to read and digest format.
void
print_cmd(cmd_t *cmd)
{
    param_t *param = NULL;
    int pcount = 1;

    fprintf(stderr,"raw text: +%s+\n", cmd->raw_cmd);
    fprintf(stderr,"\tbase command: +%s+\n", cmd->cmd);
    fprintf(stderr,"\tparam count: %d\n", cmd->param_count);
    param = cmd->param_list;

    while (NULL != param) {
        fprintf(stderr,"\t\tparam %d: %s\n", pcount, param->param);
        param = param->next;
        pcount++;
    }

    fprintf(stderr,"\tinput source: %s\n"
            , (cmd->input_src == REDIRECT_FILE ? "redirect file" :
               (cmd->input_src == REDIRECT_PIPE ? "redirect pipe" : "redirect none")));
    fprintf(stderr,"\toutput dest:  %s\n"
            , (cmd->output_dest == REDIRECT_FILE ? "redirect file" :
               (cmd->output_dest == REDIRECT_PIPE ? "redirect pipe" : "redirect none")));
    fprintf(stderr,"\tinput file name:  %s\n"
            , (NULL == cmd->input_file_name ? "<na>" : cmd->input_file_name));
    fprintf(stderr,"\toutput file name: %s\n"
            , (NULL == cmd->output_file_name ? "<na>" : cmd->output_file_name));
    fprintf(stderr,"\tlocation in list of commands: %d\n", cmd->list_location);
    fprintf(stderr,"\n");
}

// Remember how I told you that use of alloca() is
// dangerous? You can trust me. I'm a professional.
// And, if you mention this in class, I'll deny it
// ever happened. What happens in stralloca stays in
// stralloca.
#define stralloca(_R,_S) {(_R) = alloca(strlen(_S) + 1); strcpy(_R,_S);}

void
parse_commands(cmd_list_t *cmd_list)
{
    cmd_t *cmd = cmd_list->head;
    char *arg;
    char *raw;

    while (cmd) {
        // Because I'm going to be calling strtok() on the string, which does
        // alter the string, I want to make a copy of it. That's why I strdup()
        // it.
        // Given that command lines should not be tooooo long, this might
        // be a reasonable place to try out alloca(), to replace the strdup()
        // used below. It would reduce heap fragmentation.
        //raw = strdup(cmd->raw_cmd);

        // Following my comments and trying out alloca() in here. I feel the rush
        // of excitement from the pending doom of alloca(), from a macro even.
        // It's like double exciting.
        stralloca(raw, cmd->raw_cmd);

        arg = strtok(raw, SPACE_DELIM);
        if (NULL == arg) {
            // The way I've done this is like ya'know way UGLY.
            // Please, look away.
            // If the first command from the command line is empty,
            // ignore it and move to the next command.
            // No need free with alloca memory.
            //free(raw);
            cmd = cmd->next;
            // I guess I could put everything below in an else block.
            continue;
        }
        // I put something in here to strip out the single quotes if
        // they are the first/last characters in arg.
        if (arg[0] == '\'') {
            arg++;
        }
        if (arg[strlen(arg) - 1] == '\'') {
            arg[strlen(arg) - 1] = '\0';
        }
        cmd->cmd = strdup(arg);
        // Initialize these to the default values.
        cmd->input_src = REDIRECT_NONE;
        cmd->output_dest = REDIRECT_NONE;

        while ((arg = strtok(NULL, SPACE_DELIM)) != NULL) {
            if (strcmp(arg, REDIR_IN) == 0) {
                // redirect stdin

                //
                // If the input_src is something other than REDIRECT_NONE, then
                // this is an improper command.
                //

                // If this is anything other than the FIRST cmd in the list,
                // then this is an error.

                cmd->input_file_name = strdup(strtok(NULL, SPACE_DELIM));
                cmd->input_src = REDIRECT_FILE;
            }
            else if (strcmp(arg, REDIR_OUT) == 0) {
                // redirect stdout
                       
                //
                // If the output_dest is something other than REDIRECT_NONE, then
                // this is an improper command.
                //

                // If this is anything other than the LAST cmd in the list,
                // then this is an error.

                cmd->output_file_name = strdup(strtok(NULL, SPACE_DELIM));
                cmd->output_dest = REDIRECT_FILE;
            }
            else {
                // add next param
                param_t *param = (param_t *) calloc(1, sizeof(param_t));
                param_t *cparam = cmd->param_list;

                cmd->param_count++;
                // Put something in here to strip out the single quotes if
                // they are the first/last characters in arg.
                if (arg[0] == '\'') {
                    arg++;
                }
                if (arg[strlen(arg) - 1] == '\'') {
                    arg[strlen(arg) - 1] = '\0';
                }
                param->param = strdup(arg);
                if (NULL == cparam) {
                    cmd->param_list = param;
                }
                else {
                    // I should put a tail pointer on this.
                    while (cparam->next != NULL) {
                        cparam = cparam->next;
                    }
                    cparam->next = param;
                }
            }
        }
        // This could overwite some bogus file redirection.
        if (cmd->list_location > 0) {
            cmd->input_src = REDIRECT_PIPE;
        }
        if (cmd->list_location < (cmd_list->count - 1)) {
            cmd->output_dest = REDIRECT_PIPE;
        }

        // No need free with alloca memory.
        //free(raw);
        cmd = cmd->next;
    }

    if (is_verbose > 0) {
        print_list(cmd_list);
    }
}


//Define Variables	
//	cmd_list_t * commandList=NULL;
/*
   cmd_t * newCommand = NULL;
   char * history[HIST] ={'\0'};
   char * arglist[MAX_ARGS]={'\0'};
   char input[MAX_CMD_LEN];
   char* cmd = NULL;
   char* rawcmd = NULL;
   char * arg = NULL;
   int argcount=0;
   struct passwd * pw = getpwuid(getuid());

   memset(history, 0, sizeof(history));
   newCommand = calloc(1, sizeof(cmd_t));
//	commandList = calloc(1,sizeof(cmd_list_t));
while(1){
if (getcwd(cwd, sizeof(cwd)) == NULL) {
perror("getcwd() error");
exit(EXIT_FAILURE);
}
if(gethostname(server, sizeof(server)) == -1){
perror("host name error");
exit(EXIT_FAILURE);
}
if(isatty(fileno(stdout))){
printf(" PSUsh %s\n%s@%s # \n", cwd, pw->pw_name, server);
fflush(stdout);
}
//cwd Command

//Read User Input
read_command(input);
input[strcspn(input, "\n")] = 0;

//raw command
rawcmd=strdup(input);
//make sure you're reading the input correctly and then make a loop to iterate the inputlist
//inside the loop initialize a cmd_t struct, similar to list use calloc to allocate size for cmd_t

//Separate Command
cmd = strtok(input, " ");

//parse command
if(cmd){
newCommand->cmd = strdup(cmd);
newCommand->raw_cmd = strdup(rawcmd);
}
newCommand->param_count =0;
newCommand->param_list = calloc(1, sizeof(param_t));

while((cmd = strtok(NULL, " ")) != NULL){
newCommand->param_list[newCommand->param_count].param = strdup(cmd)
newCommand->param_count++;
}
printf("Command : %s\n", newCommand->cmd);
printf("Raw Command : %s\n", newCommand->raw_cmd);
printf("params : ");
for(int i=0; i< newCommand->paran_count; i++){
printf("%s ", newCommand->param_list[i].param);
}
printf("Raw Command : %s\n", newCommand->cmd);



//Store Argument List

//cd command
if(cmd && strcmp(cmd, "cd") ==0){
}	
//echo command
else if(cmd && strcmp(cmd, "echo") ==0){
	echofunc(arglist, argcount);
}	
//Bye command
else if(cmd && strcmp(cmd, "bye") ==0){
	printf("Exiting The Shell.\n");
	exit(EXIT_SUCCESS);
}
else if(cmd && strcmp(cmd, "history") ==0){
	displayhistory(history);
}
//Extneral commands
else if(cmd){
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork failed");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		arglist[0]=  cmd;
		argcount++;

	}

}

printf("\n");
}


exit(EXIT_SUCCESS);
*/


void echofunc(param_t * print, int argcount){
	for(int i= 1; i < argcount; i++){
		printf("%s ", print->param);
		print =print->next;

	}
}

void displayhistory(char * history[]){
	for ( int i =14; i > -1; i--){
		if(history[i]){
			printf("%d:%s\n", i+1, history[i]);
		}
	}

}
void read_command(char * newinput){
	printf("%s> ", PROMPT_STR); // Display shell prompt
	if (fgets(newinput, MAX_CMD_LEN, stdin) != NULL) {
		newinput[strcspn(newinput, "\n")] = '\0';
	} else {
		// Handle EOF (Ctrl+D) or read error
		printf("\nExiting the shell.\n");
		newinput[0] = '\0'; // Set input to empty so we can exit gracefully
		exit(EXIT_SUCCESS);
	}
}
void addhistory(const char * newcmd, char * history[]){
	//free oldest command
	free(history[HIST-1]);

	//for (int i = HIST -2; i <=0; i--){
	//	history[i+1] = history[i];
	//}
	memmove(&(history[1]), &(history[0]), (HIST-1) * sizeof(char* ));
	history[0] = strdup(newcmd);
}
