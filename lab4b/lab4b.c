//NAME: Samuel Shen
//EMAIL: sam.shen321@gmail.com
//ID: 405325252

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mraa.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

sig_atomic_t volatile run_flag = 1;
mraa_gpio_context button;
mraa_aio_context sensor;
struct tm* local;
time_t rawtime;

const int B = 4275;
const int R0 = 100000;
int period = 1;
char scale = 'F';
char* logfile = NULL;
int logfd = 0;

void closeSensors(){
    mraa_gpio_close(button);
    mraa_aio_close(sensor);
}

void buttonPress(){
    //outputs (and logs) a final sample with the time and the string SHUTDOWN (instead of a temperature).
    time(&rawtime);
    local = localtime(&rawtime);
    printf("%02d:%02d:%02d ",local->tm_hour, local->tm_min, local->tm_sec);
    printf("SHUTDOWN\n");
    if(logfile!=NULL){
        dprintf(logfd, "%02d:%02d:%02d ",local->tm_hour, local->tm_min, local->tm_sec);
	dprintf(logfd, "SHUTDOWN\n");
   
    }
    closeSensors();
    exit(0);
}

float readTemp(){
    float a = mraa_aio_read(sensor);
    float R = 1023.0/a-1.0;
    R = R0*R;
    float temp = 1.0/(logf(R/R0)/B+1/298.15)-273.15;
    if(scale=='F'){
        float temp_f = (temp*1.8)+32;
        return temp_f;
    }
    return temp;
}

void processCommand(char* inputbuff, int length){
    if(strncmp(inputbuff, "SCALE=F", 7)==0){
        scale = 'F';
    }else if(strncmp(inputbuff, "SCALE=C", 7)==0){
        scale = 'C';
    }else if(strncmp(inputbuff, "STOP", 4)==0){
        run_flag = 0;
    }else if(strncmp(inputbuff, "START", 5)==0){
        run_flag = 1;
    }else if(strncmp(inputbuff, "OFF", 3)==0){
        if(logfile!=NULL){
	    dprintf(logfd, "OFF\n");
	}
	buttonPress();
    }else if(strncmp(inputbuff, "PERIOD=", 7)==0){
        int num = atoi(inputbuff+7);
        period = num;
    }

    if(logfile!=NULL){
        write(logfd, inputbuff, length);
    }
}

int main(int argc, char** argv){
    int c;
    while(1){
        static struct option long_options[] = {
            {"period", required_argument, 0, 'p'},
            {"log", required_argument, 0, 'l'},
            {"scale", required_argument, 0, 's'},
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "p:l:s:", long_options, &option_index);

        if(c==-1){
            break;
        }

        switch(c){
        case 'p':
            period = atoi(optarg);
            break;
        case 'l':
            logfile = optarg;
            break;
        case 's':
            scale = optarg[0];
	    if(scale!='F' && scale!='C'){
	 	fprintf(stderr, "Unvalid argument for scale\n");
		exit(1);
	    }
            break;
        case '?':
            //unrecognized argument
            fprintf(stderr, "Unrecognized Argument. Usage: --period=# --scale=C or F --log=logfile\n");
            exit(1);
        default:
            abort();
        }
    }

    if(logfile!=NULL){
        logfd = creat(logfile, 0666);
    }

    //initialize
    button = mraa_gpio_init(60);
    if(button==NULL){
        fprintf(stderr, "Button init failed\n");
        closeSensors();
        exit(1);
    }

    sensor = mraa_aio_init(1);
    if(sensor==NULL){
        fprintf(stderr, "Sensor init failed\n");
        closeSensors();
        exit(1);
    }    

    //set dir
    if(mraa_gpio_dir(button, MRAA_GPIO_IN)!=MRAA_SUCCESS){
        fprintf(stderr, "Error with direction set\n");
        exit(1);
    }

    //link button to handler function
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &buttonPress, NULL);
    
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN|POLLHUP|POLLERR;
    
    char inputbuff[256];

    //commandPtr points to the index in the current command
    //validPtr points to the next valid position in the buffer
    int commandPtr = 0;
    int validPtr = 0;
    //run main program
    while(1){
        //check readability
	int r = poll(&fds[0], 1, 0);
	if(r==-1){
	    fprintf(stderr, "poll error\n");
	    exit(1);
	}
        if(fds[0].revents & POLLIN){
            int readstatus = read(0, inputbuff+validPtr, 256-validPtr);
            if(readstatus < 0){
                fprintf(stderr, "Read error\n");
                exit(1);
            }
            validPtr += readstatus;
            while(commandPtr < validPtr){
		//read until end of a command
                if(inputbuff[commandPtr]=='\n'){
                    //process command
                    processCommand(inputbuff, commandPtr+1);
                    char tempbuff[256];
		    
		    //erase old command and move everything after back to beginning of buffer
                    int numbytesleft = validPtr-commandPtr-1;
                    memcpy(tempbuff, inputbuff+commandPtr+1, numbytesleft);
                    memcpy(inputbuff, tempbuff, numbytesleft);
                    validPtr = numbytesleft;
                    commandPtr = 0;
                }
		commandPtr+=1;
            }

            if(validPtr==256){
                if(logfile!=NULL){
                    write(logfd, inputbuff, 256);
                }
                validPtr = 0;
                commandPtr = 0;
            }
            
        }
        if(run_flag){
            float temp = readTemp();
            time(&rawtime);
            local = localtime(&rawtime);
            printf("%02d:%02d:%02d ",local->tm_hour, local->tm_min, local->tm_sec);
            printf("%0.1f\n", temp);
	    if(logfile!=NULL){
	    	dprintf(logfd, "%02d:%02d:%02d ",local->tm_hour, local->tm_min, local->tm_sec);
		dprintf(logfd, "%0.1f\n", temp);
	    }
            sleep(period); 
        }

    }

    closeSensors();
    exit(0);
}
