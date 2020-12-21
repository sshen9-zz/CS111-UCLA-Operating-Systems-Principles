/* NAME: Samuel Shen */
/* EMAIL: sam.shen321@gmail.com */
/* ID: 405325252 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

extern int errno;

void causeSegfault(){
  char* ptr = NULL;
  *ptr = 'a';
  return;
}
void handle_sigsegv(int sig){
  fprintf(stderr, "Caught signal %d", sig);
  fprintf(stderr, ": segmentation fault\n");
  exit(4);
}
int main(int argc, char **argv){
  int ifd = 0;
  int ofd = 1;
  
  int segfault = 0;
  int catch = 0;
  
  int c;
  while(1){
      static struct option long_options[] =
	{
	  //declare options here
	  {"input", required_argument, 0, 'i'},
	  {"output", required_argument, 0, 'o'},
	  {"segfault", no_argument, 0, 's'},
	  {"catch", no_argument, 0, 'c'},
	  {0,0,0,0}
	};

      int option_index = 0;
      c = getopt_long(argc, argv, "sci:o:", long_options, &option_index);

      if(c==-1){
	//end of options
	break;
      }

      switch(c){
      case 'i':
	ifd = open(optarg, O_RDONLY);
	if(ifd >=0 ){
	  close(0);
	  dup(ifd);
	  close(ifd);
	}else{
	  //open returns -1 on error
	  fprintf(stderr, "Error opening input file: %s, %s\n", optarg, strerror(errno));
	  exit(2);
	}
	break;
      case 'o':
	ofd = creat(optarg, 0666);
	if(ofd >= 0){
	  close(1);
	  dup(ofd);
	  close(ofd);
	}else{
	  fprintf(stderr, "Error creating output file: %s, %s\n", optarg, strerror(errno));
	  strerror(errno);
	  exit(3);
	}
	break;
	
      case 's':
	segfault = 1;
	break;
      case 'c':
	catch = 1;
	break;
      case '?':
	//unrecognized argument
	fprintf(stderr, "Unrecognized Argument; Correct Usage: --input filename, --output filename, --segfault, --catch\n");
	exit(1);
      default:
	abort();
      }
  }

  //1. already redirected files
  //2. register signal handler:
  if(catch){
    signal(SIGSEGV, handle_sigsegv);
  }

  //3. cause the segfault
  if(segfault){
    //cause segfault
    causeSegfault();
  }else{
    char buff[50];
    ssize_t status = 1;

    while(status>0){
      status = read(0, buff, 50);
      if(status==0){
	exit(0);
      }else if(status==-1){
	//error in the read
	fprintf(stderr, "Read Error\n");
	exit(0);
      }else{
	ssize_t w_status = write(1, buff, status);
	if(w_status==-1){
	  fprintf(stderr, "Write Error\n");
	  exit(0);
	}
      }
    }
  }
  
  
  exit(0);
  
}
