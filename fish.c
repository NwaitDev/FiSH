#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "fish.h"
#include "cmdline.h"

#define BUFLEN 1024

#define YES_NO(i) ((i) ? "Y" : "N")

struct pid_list bg_pids;

void zombie_killer(int signal){
	if(signal == SIGCHLD){
		int wstatus;
 		pid_t child = 0;
 		size_t i =0;
 		while(child==0 && i<bg_pids.size){
 			child = waitpid(bg_pids.data[i++],&wstatus,WNOHANG);
 		}
 		if(child!=-1 && child!=0){
 			pid_list_remove(bg_pids,child);
 			if(WIFEXITED(wstatus)){
				fprintf(stderr,"BG : process %i exited with exit status %i\n",child,WEXITSTATUS(wstatus));
			}
			if(WIFSIGNALED(wstatus)){
				fprintf(stderr,"BG : process %i killed by signal %i\n",child,WTERMSIG(wstatus));
			}
 		}
 		if(child==-1){
 			perror("waitpid");
 		}
	}
}

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


int main() {
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
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = zombie_killer;
	err=sigaction(SIGCHLD,&sa,NULL);
  
  //blocking SIGINT for FiSH
  sigset_t toblock, oldset;
  sigaddset(&toblock,SIGINT);
  sigaddset(&toblock,SIGQUIT);
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
    if (err) { 
      //the command line entered by the user isn't valid
      line_reset(&li);
      continue;
    }
		
		/*debugging tool*/
		if(true){
			line_stats(li);
		}
		
		//EXIT COMMAND
  	if(li.n_cmds!=0 && strcmp(li.cmds[0].args[0],"exit")==0){
  		line_reset(&li);
  		break;
  	}
  	
  	
  	if(li.n_cmds==1){
  		////////////////////////////////////////////////
  		//CD COMMAND
  		if(strcmp(li.cmds[0].args[0],"cd")==0){
  			char target_dir[100];
  			if(li.cmds[0].args[1]==NULL||*(li.cmds[0].args[1])=='~'){
  				
  				//handling the case where the path starts with ~
  				char* home_dir = getenv("HOME");
  				strcpy(target_dir,home_dir);
  				int home_len = strlen(target_dir);
  				if(li.cmds[0].args[1]!=NULL){
  					strcpy(target_dir+home_len,li.cmds[0].args[1]+1);
  				}
  			}else{
  				strcpy(target_dir,li.cmds[0].args[1]);
  			}	
 	  		err = chdir(target_dir);
  	 		if(err==-1){
  	 			perror("cd");
  			}
  			//reseting and going to the next line
  			line_reset(&li);
  			continue;
  		}
  		////////////////////////////////////////////////
  		
  		/*REDIRECTION PART*/
  		if(li.redirect_input){
  			input = open(li.file_input,O_RDONLY);
  			if(input==-1){
  				perror("open");
  				line_reset(&li);
  				continue;
  			}
  		}
  		if(li.redirect_output){
  			output=open(li.file_output,O_WRONLY|O_CREAT|O_TRUNC);
  			if(output==-1){
  				perror("open");
  				line_reset(&li);
  				continue;
  			}
  		}
  		////////////////////////////////////////////////
  		
  		//executing the command if this isn't an internal command 
  		// if the command is FG
  		if(li.cmds[0].n_args>=1 && !li.background){
  			pid_t pid = fork();
  			if(pid==-1){
  				perror("fork");
  			}else{
  				if(pid==0){
  					//removing the SIGINT mask so that the program can be stopped
  					err = sigprocmask(SIG_SETMASK,&oldset,NULL);
						if(err==-1){
							perror("sigprocmask reset in child");
							line_reset(&li);
							exit(1);
						}
  					dup2(input,0);
  					dup2(output,1);
  					execvp(li.cmds[0].args[0],li.cmds[0].args);
  					perror(li.cmds[0].args[0]);
  					line_reset(&li);
  					exit(1);
 					}
					
 					int wstatus;
 					pid_t child = wait(&wstatus);
 					if(WIFEXITED(wstatus)){
						fprintf(stderr,"FG : process %i exited with exit status %i\n",child,WEXITSTATUS(wstatus));
					}
					if(WIFSIGNALED(wstatus)){
						fprintf(stderr,"FG : execution process killed by signal %i\n",WTERMSIG(wstatus));
					}
  			}
  			// if the command is BG
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
  				}
  	 		}
  	 	}
		}
		if(li.redirect_input){
  			close(input);
  	}
  	if(li.redirect_output){
  		close(output);
  	}
    line_reset(&li);
  }
  
  return 0;
}

