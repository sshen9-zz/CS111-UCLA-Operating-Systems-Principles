 /* NAME: Samuel Shen */
/* EMAIL: sam.shen321@gmail.com */
/* ID: 405325252 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <signal.h>
#include <sys/wait.h>

extern int errno;

void reset(const struct termios* init_termios_p, int code){
  if(tcsetattr(0, TCSANOW, init_termios_p)==-1){
    fprintf(stderr, "Error changing settings back: %s\r\n", strerror(errno));
    exit(1);
  }
  exit(code);
}

void shellerr(int sig, int stat){
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", sig, stat);
}

void getShellExitStatus(pid_t p, int exitstatus, const struct termios* init_termios_p){
  if(waitpid(p, &exitstatus, WUNTRACED|WNOHANG|WCONTINUED)==-1){
    fprintf(stderr, "Error with waitpid\r\n");
    reset(init_termios_p, 1);
  }
	  
  int stat = WEXITSTATUS(exitstatus);
  int sig = exitstatus&0x00FF;
  shellerr(sig, stat);

  return;
}

int main(int argc, char* argv[]){
  //get options
  int c;
  char* program = NULL;
  while(1){
    static struct option long_options[] = {
      {"shell", required_argument, 0, 's'},
      {0,0,0,0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "s:", long_options, &option_index);

    if(c==-1){
      //end of options
      break;
    }

    switch(c){
    case 's':
      program = optarg;
      break;
    case '?':
      //unrecognized argument
      fprintf(stderr, "Unrecognized Argument; Correct Usage: --shell program\n");
      exit(1);
    default:
      abort();
    }
  }


  int parent_to_child[2];
  int child_to_parent[2];

  char map[2];
  map[0] = '\r';
  map[1] = '\n';


  
  if(pipe(parent_to_child)==-1){
    fprintf(stderr, "Pipe parent to child failed\n");
    exit(1);
  }
  if(pipe(child_to_parent)==-1){
    fprintf(stderr, "Pipe child to parent failed\n");
    exit(1);
  }

  pid_t p;
  if(program!=NULL){
    //used --shell option
    p = fork();
    if(p<0){
      fprintf(stderr, "Fork Failed\n");
      exit(1);
    }else if(p==0){
      //child process
      //read from parent to child[0] and write to child to parent[1];

      close(parent_to_child[1]);
      close(child_to_parent[0]);
	    
      //redirect stdin
      close(0);
      dup(parent_to_child[0]);
      close(parent_to_child[0]);

      //redirect stdout
      close(1);
      close(2);
      dup(child_to_parent[1]);
      dup(child_to_parent[1]);
      close(child_to_parent[1]);
      
      if(execl(program, program, (char*) NULL)==-1){
	fprintf(stderr, "Error in executing bash\r\n");
	close(1);
	close(2);
	exit(1);
      }
      exit(0);
    }
  }

  //this is the parent process now


  close(parent_to_child[0]);
  close(child_to_parent[1]);
  
  struct termios init_termios, modified_termios;
  struct termios* init_termios_p = &init_termios;
  struct termios* modified_termios_p = &modified_termios;
  
  if(tcgetattr(0, init_termios_p)==-1 || tcgetattr(0, modified_termios_p)==-1){
    fprintf(stderr, "Error getting initial keyboard termios info: %s\n", strerror(errno));
    exit(1);
  }

  //modify settings
  modified_termios.c_iflag = ISTRIP;
  modified_termios.c_oflag = 0;
  modified_termios.c_lflag = 0;

  //set the new settings
  if(tcsetattr(0, TCSANOW, modified_termios_p)==-1){
    fprintf(stderr, "Error with set attr: %s\n", strerror(errno));
    exit(1);
  }


  /* //read ascii input from keyboard into buffer */
  char buff[256];  
  char outputbuff[256];
  
  //  char test[1];
  //test[0] = '*';
  
  int eof = 0;
  ssize_t readstatus;
  ssize_t writestatus;

  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[1].fd = child_to_parent[0];
  fds[0].events = POLLIN|POLLHUP|POLLERR;
  fds[1].events = POLLIN|POLLHUP|POLLERR;


  int exitstatus = 0;
  int shell_status = 0;
  
  if(program!=NULL){
    shell_status = 1;
  }
  
  while(eof==0){
    if(poll(fds, 2, 0)==-1){
      fprintf(stderr, "Poll Error: %s\r\n", strerror(errno));
      reset(init_termios_p, 1);
    }
      

    if(fds[0].revents & POLLIN){
      //check for readability for keyboard first
      readstatus = read(0, buff, 256);
      if(readstatus<0){
	reset(init_termios_p, 1);
      }
      
      for(int i=0; i<readstatus; i++){
	if(buff[i]==0x04){
	  //eof
	  eof = 1;
	  close(parent_to_child[1]);
	  
	  if(shell_status==1){
	    shell_status = 0;
	    getShellExitStatus(p, exitstatus, init_termios_p);
	  }
	  break;
	  
	}else if(buff[i]==0x03){
	  if(shell_status){
	    if(kill(p, SIGINT)==-1){
	      fprintf(stderr, "Kill Error\r\n");
	      reset(init_termios_p, 1);
	    };
	  }
	  
	}else if(buff[i]==0x0D || buff[i]==0x0A){
	  writestatus = write(1, map, 2);
	  if(writestatus<0){
	    reset(init_termios_p, 1);
	  }

	  if(shell_status){
	    //only write the lf to the shell
	    writestatus = write(parent_to_child[1], map+1, 1);	    
	  }

	}else{
	  writestatus = write(1, buff+i, 1);
	  if(writestatus<0){
	    reset(init_termios_p, 1);
	  }
	  
	  if(shell_status){
	    writestatus = write(parent_to_child[1], buff+i, 1);
	  }

	}
      }
    }				     

    //read in from shell pipe
    if(shell_status && fds[1].revents & POLLIN){
      readstatus = read(child_to_parent[0], outputbuff, 256);
      for(int i=0; i<readstatus; i++){
	if(outputbuff[i]==0x0A){
	  write(1, map, 2);
	}
	else if(outputbuff[i]==0x04){
	  if(shell_status==1){
	    shell_status = 0;
	    getShellExitStatus(p, exitstatus, init_termios_p);
	  }
	  
	}
	else{
	  write(1, outputbuff+i, 1);
	}
      }
    }

        
    if(fds[1].revents&POLLHUP||fds[1].revents&POLLERR){
      if(shell_status==1){
	shell_status = 0;
	//get exit status from shell
	getShellExitStatus(p, exitstatus, init_termios_p);
      }
    }
    
  }

  reset(init_termios_p, 0);
}
