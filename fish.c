#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "util.h"
#include "cmdline.h"

#define BUFLEN 1024

#define YES_NO(i) ((i) ? "Y" : "N")

/**
	* Global variable that represents the
	* list of all the pids of the processes running background from FiSH
	*/
struct pid_list bg_pids;


/**
 * Prints how a child process terminated
 * 
 * @param child the pid of the terminated child
 * @param wstatus the termination status of the process
 */
void waitmessage(pid_t child, int wstatus){
	if(WIFEXITED(wstatus)){
		fprintf(stderr,"\n[%i] exited with exit status %i\n",child,WEXITSTATUS(wstatus));
	}
	if(WIFSIGNALED(wstatus)){
		fprintf(stderr,"\n[%i] killed by signal %i\n",child,WTERMSIG(wstatus));
	}
}

/**
 * Signal handler for SIGCHILD
 * this function determines which child process terminated and removes its 
 * pid from the global variable bg_pids 
 * so that the killed process is no more referenced
 * it also prints the terination status of the process 
 * on the standard error stream
 * 
 * @param signal the signal to be handled
 */
void zombie_killer(int signal){
	if(signal == SIGCHLD){
		int wstatus;
 		pid_t child = 0;
 		size_t i =0;
 		while(child==0 && i<bg_pids.size){
 			child = waitpid(bg_pids.data[i],&wstatus,WNOHANG);
 			++i;
 		}
 		if(child!=-1 && child!=0){
 			pid_list_remove(&bg_pids,child);
 			waitmessage(child, wstatus);
 		}
 		if(child==-1){
 			perror("waitpid");
 		}
	}
}

/**
	*	Prints the data of the line structure
	*
	* @param li the line of which to print data 
	*/
static void line_stats(struct line li){
	fprintf(stderr, "Command line:\n");
    fprintf(stderr, "\tNumber of commands: %zu\n", li.n_cmds);

    for (size_t i = 0; i < li.n_cmds; ++i) {
      fprintf(stderr, "\t\tCommand #%zu:\n", i);
      fprintf(stderr, "\t\t\tNumber of args: %zu\n", li.cmds[i].n_args);
      fprintf(stderr, "\t\t\tArgs:");
      for (size_t j = 0; j < li.cmds[i].n_args; ++j) {
        fprintf(stderr, " \"%s\"", li.cmds[i].args[j]);
      }
      fprintf(stderr, "\n");
    }

    fprintf(stderr, "\tRedirection of input: %s\n", YES_NO(li.redirect_input));
    if (li.redirect_input) {
      fprintf(stderr, "\t\tFilename: '%s'\n", li.file_input);
    }

    fprintf(stderr, "\tRedirection of output: %s\n", YES_NO(li.redirect_output));
    if (li.redirect_output) {
      fprintf(stderr, "\t\tFilename: '%s'\n", li.file_output);
    }

    fprintf(stderr, "\tBackground: %s\n", YES_NO(li.background));
}

/**
	*	function that changes the working 
	* directory for the adress given in argument
	*	this function handles empty adresses or adresses
	* starting with the ~ (home) symbol
	*
	* @param path the adress to which change working directory
	*/
void cd(char * path){
	char target_dir[100];
  if(path==NULL||*(path)=='~'){
  	//handling the case where the path starts with ~
  	char* home_dir = getenv("HOME");
  	strcpy(target_dir,home_dir);
  	int home_len = strlen(target_dir);
  	if(path!=NULL){
  		strcpy(target_dir+home_len,path+1);
  	}
  }else{
  	strcpy(target_dir,path);
  }	
 	int err = chdir(target_dir);
  if(err==-1){
  	perror("cd");
  }
}


/**
	* Main function of the FiSH program
	*
	*/
int main() {
	//sets umask to zero so that the 
	//newly created files have default permissions
	//doesn't seem to be working
	umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	
	//initializing the variables
  struct line li;
  line_init(&li);
  
  pid_list_create(&bg_pids);
  
  char buf[BUFLEN];
  int err;
	int input;
	int output;
	char cwd[BUFLEN];
  
  //initializing actions in case of BG process termination
  struct sigaction sa;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = zombie_killer;
	err=sigaction(SIGCHLD,&sa,NULL);
  
  //blocking SIGINT for FiSH
  sigset_t toblock, oldset;
  sigaddset(&toblock,SIGINT);
  //sigaddset(&toblock,SIGQUIT);
	err = sigprocmask(SIG_BLOCK,&toblock,&oldset);
	if(err==-1){
		perror("sigprocmask blockset");
		return 1;
	}
	
	//starting to prompt
  for (;;) {
  	input = 0;
  	output = 1;
  	
  	//printing the current directory
  	getcwd(cwd,BUFLEN);
    printf("fish:%s> ",cwd);
    
    //getting the command(s)
    fgets(buf, BUFLEN, stdin);
    err = line_parse(&li, buf);
    if (err==-1) { 
      //the command line entered by the user isn't valid
      line_reset(&li);
      continue;
    }
		/*debugging tool*/
		if(false){
			line_stats(li);
		}
		
		//EMPTY COMMAND
		if(li.n_cmds==0){
			line_reset(&li);
			continue;
		}
		
		//EXIT COMMAND
  	if(li.n_cmds!=0 && strcmp(li.cmds[0].args[0],"exit")==0){
  		line_reset(&li);
  		break;
  	}
  	
  	//Handling redirections
  	if(li.redirect_input){
  		input = open(li.file_input,O_RDONLY);
  		if(input==-1){
  			perror("redirection of input");
  			line_reset(&li);
  			continue;
  		}
  	}
  	if(li.redirect_output){
  		output=open(li.file_output,O_WRONLY|O_CREAT|O_TRUNC);
  		if(output==-1){
  			perror("redirection of output");
  			line_reset(&li);
  			if(input!=0){
  				close(input);
  			}
  			continue;
  		}
  	}else{
  		if(li.background){
  			output=open("/dev/null",O_WRONLY);
  			if(output==-1){
  				perror("redirection of output");
  				line_reset(&li);
  				if(input!=0){
  					close(input);
  				}
  			}
  		}
  	}
  	
  	if(li.n_cmds==1){
  	
  		//CD COMMAND
  		if(strcmp(li.cmds[0].args[0],"cd")==0){
  			cd(li.cmds[0].args[1]);
  			//reseting and going to the next line
  			line_reset(&li);
  			continue;
  		}
  		
  		//executing the command if this isn't an internal command 
  		// if the command is FG and doesn't have pipes
  		if(li.cmds[0].n_args>=1 && !li.background){
  			pid_t pid = fork();
  			if(pid==-1){
  				perror("fork");
  			}else{
  				if(pid==0){
  					//removing the SIGNAL mask so that the program can be stopped
  					err = sigprocmask(SIG_SETMASK,&oldset,NULL);
						if(err==-1){
							perror("sigprocmask reset in child");
							line_reset(&li);
							exit(1);
						}
						//redirecting to the required streams
  					dup2(input,0);
  					dup2(output,1);
  					execvp(li.cmds[0].args[0],li.cmds[0].args);
  					perror(li.cmds[0].args[0]);
  					exit(1);
 					}
 					//closing the files if there has been a redirection
					if(input!=0){
						close(input);
					}
					if(output!=1){
						close(output);
					}
					//waiting for the end of the process
 					int wstatus;
 					pid_t child = waitpid(pid,&wstatus,0);
 					if(false){
 						waitmessage(child,wstatus);
 					}
  			}
  	 	}//end of the 1 foreground process treatement
  	 	// if the command is BG and doesn't have pipes
  		if(li.cmds[0].n_args>=1 && li.background){
  			pid_t pid = fork();
  			if(pid==-1){
  				perror("fork");
  			}else{
  				if(pid==0){
  					dup2(input,0);
  					dup2(output,1);
  					execvp(li.cmds[0].args[0],li.cmds[0].args);
  					perror(li.cmds[0].args[0]);
  					line_reset(&li);
  					exit(1);
 					}
 					pid_list_add(&bg_pids,pid);
 					if(input!=0){
						close(input);
					}
					if(output!=1){
						close(output);
					}
  			}
  	 	}//end of the 1 background process treatement
		}//end of the 1 command treatement
		else{
  		//executing the command if this isn't an internal command 
  		// if the command is FG and requires pipes
			if(!li.background){
				//initializing the pipes
				int tubes[li.n_cmds-1][2];
				
				//preparing to list of all the future processes to kill
				struct pid_list pipe_processes;
				pid_list_create(&pipe_processes);
				
				for(size_t i=0;i<li.n_cmds;++i){
					pipe(tubes[i]);
					pid_t newpid = fork();
					if(newpid == -1){
						perror("fork");
						break;
					}
					if(newpid==0){
						//removing the SIGNAL mask so that the program can be stopped
						err = sigprocmask(SIG_SETMASK,&oldset,NULL);
						if(err==-1){
							perror("sigprocmask reset in child");
							line_reset(&li);
							exit(1);
						}
						
						if(i==0){
							//first process needs to read the input stream
							dup2(input, 0);
						}else{
							//other processes need to read in the pipe of the previous process
							dup2(tubes[i-1][0],0);
						}
						if(i==li.n_cmds-1){
							//last process needs to write in the output stream
							dup2(output, 1);
						}else{
							//other processes need to write in their pipe
							dup2(tubes[i][1],1);
						}
						//closing useless file descriptors
						close(tubes[i][0]);
						close(tubes[i][1]);
						execvp(li.cmds[i].args[0],li.cmds[i].args);
						perror(li.cmds[i].args[0]);
						exit(1);
					}
					//adding the new child to the list of processes to kill
					pid_list_add(&pipe_processes, newpid);
					//closing useless file descriptors 
					if(i!=0){
						close(tubes[i-1][0]);
					}
					close(tubes[i][1]);
				}
				close(tubes[li.n_cmds-2][0]);
				if(input!=0){
					close(input);
				}
				if(output!=1){
					close(output);
				}
				//killing the piped processes of the list one by one before continuing the loop
				for(size_t i = 0; i<li.n_cmds;++i){
					int wstatus;
					pid_t child = waitpid(pipe_processes.data[i],&wstatus,0);
 					if(false){
 						waitmessage(child,wstatus);
 					}
				}
				pid_list_destroy(&pipe_processes);
				
			}//end of the foreground piped processes treatement
			else{
				//if the command is BG and requires pipes
				int tubes[li.n_cmds-1][2];
				for(size_t i=0;i<li.n_cmds;++i){
					pipe(tubes[i]);
					pid_t newpid = fork();
					if(newpid == -1){
						perror("fork");
						break;
					}
					if(newpid==0){
						if(i==0){
							dup2(input, 0);
						}else{
							dup2(tubes[i-1][0],0);
						}
						if(i==li.n_cmds-1){
							dup2(output, 1);
						}else{
							dup2(tubes[i][1],1);
						}
						close(tubes[i][0]);
						close(tubes[i][1]);
						execvp(li.cmds[i].args[0],li.cmds[i].args);
						perror(li.cmds[i].args[0]);
						exit(1);
					}
					pid_list_add(&bg_pids, newpid);
					if(i!=0){
						close(tubes[i-1][0]);
					}
					close(tubes[i][1]);
				}
				close(tubes[li.n_cmds-2][0]);
				if(input!=0){
					close(input);
				}
				if(output!=1){
					close(output);
				}
			}//end of the background piped processes treatement
		}//end of the piped commands treatement
  	//pid_list_print(&bg_pids);
    line_reset(&li);
  }//end of the prompt loop
  pid_list_destroy(&bg_pids);
  return 0;
}

