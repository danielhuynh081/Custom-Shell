
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
char ** create_raggedy(cmd_t* cmds);
void exec_pipelines(cmd_list_t * cmd);
void freehistory(char * history[]);
void handle_sigint(cmd_list_t cmd_list);

// 1. bye done
// 2. cd / cd dir done
// 3. cwd done
// 4. history done
// 5. echo done


//Main
int main(int argc, char * argv[]){
cmd_list_t *cmd_list = NULL;
    char str[MAX_STR_LEN] = {'\0'};
    char *ret_val = NULL;
    char *raw_cmd = NULL;
    char cwd[1024];
    char server[50];
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

	    //Control D
	    if (NULL == ret_val) {
		    //free history
		    freehistory(historylist);
		    free_list(cmd_list);
		    printf("\nExiting the shell.\n");
		    exit(EXIT_SUCCESS);
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
		    freehistory(historylist);
		    free_list(cmd_list);
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
	    free_list(cmd_list);
	    cmd_list = NULL;
    }
    //free history function
    freehistory(historylist);

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
				freehistory(historylist);
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
		    freehistory(historylist);
		    free_list(cmds);
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
	    exec_pipelines(cmds);
    }
}
char ** create_raggedy(cmd_t* cmds){
	param_t *param;
	char **args = (char **)malloc(4 * sizeof(char *));
	int i = 0;
	args[i++] = cmds->cmd;

	param = cmds->param_list;
	while (param) {
		args[i++] = param->param;
		param = param->next;
	}
	args[i] = NULL;
	return args;
}

void exec_pipelines(cmd_list_t * cmds){
	cmd_t * cmd = cmds->head;
	char ** args;
	int p_trail = -1; // no previous pipe exists, first pipe
	int pipe_fds[2];
	pid_t pid;
	if(!cmd){
		printf("error finding commands");
		freehistory(historylist);
		free_list(cmds);
		exit(EXIT_FAILURE);
	}
	while(cmd){
		if(cmd->next){
			if(pipe(pipe_fds) ==-1){
				printf("error creating pipe");
				freehistory(historylist);
				free_list(cmds);
				exit(EXIT_FAILURE);
			}
		}
		pid = fork();
		if(pid == -1){
			perror("Failure Forking");
			freehistory(historylist);
			free_list(cmds);
			exit(EXIT_FAILURE);
		}else if(pid ==0){
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
			if(p_trail != -1)// if not the first pipe
			{
				dup2(p_trail, STDIN_FILENO);
			}
			if(cmd->next != NULL){
				if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
					perror("dup2 failed for stdout");
					freehistory(historylist);
					free_list(cmds);
					exit(EXIT_FAILURE);
				}
			}
			if (p_trail != -1) close(p_trail);
			if (cmd->next != NULL) {
				close(pipe_fds[0]);
				close(pipe_fds[1]);
			}

			// Execute the command with a raggedy array
			args = create_raggedy(cmd);
			if(execvp(args[0], args)== -1) {
				perror("exec failed");
				freehistory(historylist);
				free_list(cmds);
				exit(EXIT_FAILURE);
			}

			// If execvp fails

		}
		// Parent process
		if (p_trail != -1) close(p_trail); // Close the old pipe read end
		if (cmd->next != NULL) {
			close(pipe_fds[1]); // Close the current pipe write end
			p_trail = pipe_fds[0]; // Update p_trail to the current pipe read end
		}

		cmd = cmd->next;

	}
	while(wait(NULL)>0);

}
void exec_external(cmd_t * cmd){
	pid_t pid = fork();
	if(pid == -1){
		perror("Failure Forking");
		freehistory(historylist);
		free_cmd(cmd);
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
			freehistory(historylist);
			free_cmd(cmd);
			exit(EXIT_FAILURE);
		}
	}else{
		wait(NULL);
	}

}
void freehistory(char * history[]){
	for(int i= 0; i< HIST; i++){
		free(history[i]);
		history[i] = NULL;
	}
}
void free_list(cmd_list_t *cmd_list) {
	param_t *next_param;
	cmd_t *current;
	param_t *param;
	if (cmd_list == NULL) return;  // Check for NULL to avoid invalid memory access

	current = cmd_list->head;
	param = current->param_list;
	while (current) {
		cmd_t *next = current->next;

		// Free dynamically allocated memory for cmd_t members
		if (current->raw_cmd) {
			free(current->raw_cmd);
		}
		if (current->cmd) {
			free(current->cmd);
		}
		if (current->input_file_name) {
			free(current->input_file_name);
		}
		if (current->output_file_name) {
			free(current->output_file_name);
		}

		// Free the param_list, if it exists
		param = current->param_list;
		while (param) {
			next_param = param->next;
			if (param->param) {
				free(param->param);  // Free the parameter string
			}
			free(param);  // Free the param_t structure
			param = next_param;
		}

		free(current);  // Free the cmd_t object itself
		current = next;
	}

	free(cmd_list);  // Free the cmd_list structure itself
}
	void
print_list(cmd_list_t *cmd_list)
{
	cmd_t *cmd = cmd_list->head;

	while (NULL != cmd) {
		print_cmd(cmd);
		cmd = cmd->next;
	}
}

	void
free_cmd (cmd_t *cmd)
{

	param_t * paramlist = cmd->param_list;
	param_t * temp;
	while(paramlist){
		temp = paramlist->next;
		free(paramlist->param);
		free(paramlist);
		paramlist = temp;
	}

	if(!cmd){
			freehistory(historylist);
		    free_cmd(cmd);
		exit(EXIT_FAILURE);
	}
	/*
	if(cmd->next){
		free_cmd(cmd->next);
	}
	*/
	free(cmd->raw_cmd);
	free(cmd->cmd);
	free(cmd->input_file_name);
	free(cmd->output_file_name);
	free(cmd);

	return;
}

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
			freehistory(historylist);
		exit(EXIT_SUCCESS);
	}
}
void addhistory(const char * newcmd, char * history[]){
	//free oldest command
	free(history[HIST-1]);

	memmove(&(history[1]), &(history[0]), (HIST-1) * sizeof(char* ));
	history[0] = strdup(newcmd);
}
