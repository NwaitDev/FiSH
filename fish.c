#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "cmdline.h"

#define BUFLEN 1024

#define YES_NO(i) ((i) ? "Y" : "N")

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

/*
 Just a little reminder so that I don't switch files every ten seconds
 
struct cmd {
  char *args[MAX_ARGS + 1]; //+1 to have a NULL at the end if nargs = MAX_ARGS
  size_t n_args;
};

struct line {
  struct cmd cmds[MAX_CMDS];
  size_t n_cmds;
  bool redirect_input;
  char *file_input;
  bool redirect_output;
  char *file_output;
  bool background;
};
*/

int main() {
  struct line li;
  char buf[BUFLEN];
  
	int input;
	int output;
	
  line_init(&li);
  char cwd[BUFLEN];
  for (;;) {
  	input = 0;
  	output = 1;
  	getcwd(cwd,BUFLEN);
    printf("fish:%s> ",cwd);
    fgets(buf, BUFLEN, stdin);

    int err = line_parse(&li, buf);
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
  		//CD COMMAND
  		if(strcmp(li.cmds[0].args[0],"cd")==0){
  			char target_dir[100];
  			if(li.cmds[0].args[1]==NULL||*(li.cmds[0].args[1])=='~'){
  				//Gérer le cas où le premier caractère correspond à l'abreviation ~
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
  		
  		
  		
  		if(li.cmds[0].n_args>=1){
  			pid_t pid = fork();
  			if(pid==-1){
  				perror("fork");
  			}else{
  				if(pid==0){
  					dup2(input,0);
  					dup2(output,1);
  					execvp(li.cmds[0].args[0],li.cmds[0].args);
  					perror(li.cmds[0].args[0]);
  					exit(1);
 					}
 					
 					int wstatus;
 					wait(&wstatus);
 					if(WIFEXITED(wstatus) && wstatus!=0){
						fprintf(stderr,"abnormal termination : process exited with exit status %i\n",WEXITSTATUS(wstatus));
					}
					if(WIFSIGNALED(wstatus)){
						fprintf(stderr,"abnormal termination : execution sub process killed by signal %i\n",WTERMSIG(wstatus));
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

