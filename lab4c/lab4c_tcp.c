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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netdb.h>

sig_atomic_t volatile run_flag = 1;
mraa_gpio_context button;
mraa_aio_context sensor;
struct tm* local;
time_t rawtime;
struct hostent* hostserver;

int PORT = 18000;
const int B = 4275;
const int R0 = 100000;
int period = 1;
char scale = 'F';
char* logfile = NULL;
int logfd = 0;
char id[13];
char* host;
int sock = 0;

void closeSensors(){
    mraa_gpio_close(button);
    mraa_aio_close(sensor);
}

void buttonPress(){
    //outputs (and logs) a final sample with the time and the string SHUTDOWN (instead of a temperature).
    time(&rawtime);
    local = localtime(&rawtime);
    char buf[50];
    sprintf(buf, "%02d:%02d:%02d SHUTDOWN\n", local->tm_hour, local->tm_min, local->tm_sec);
    if(send(sock, buf, strlen(buf), 0) == -1){
        fprintf(stderr, "Error sending to socket\n");
        exit(1);
    }
    
    if(logfile!=NULL){
        dprintf(logfd, "%02d:%02d:%02d ",local->tm_hour, local->tm_min, local->tm_sec);
        dprintf(logfd, "SHUTDOWN\n");
    }

    closeSensors();
    shutdown(sock, SHUT_RDWR);
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
	write(logfd, inputbuff, length);
    }else if(strncmp(inputbuff, "SCALE=C", 7)==0){
        scale = 'C';
	write(logfd, inputbuff, length);
    }else if(strncmp(inputbuff, "STOP", 4)==0){
        run_flag = 0;
	write(logfd, inputbuff, length);
    }else if(strncmp(inputbuff, "START", 5)==0){
        run_flag = 1;
	write(logfd, inputbuff, length);
    }else if(strncmp(inputbuff, "OFF", 3)==0){
        if(logfile!=NULL){
            dprintf(logfd, "OFF\n");
        }
        buttonPress();
	write(logfd, inputbuff, length);
    }else if(strncmp(inputbuff, "PERIOD=", 7)==0){
        int num = atoi(inputbuff+7);
        period = num;
	write(logfd, inputbuff, length);
    }

}

int main(int argc, char** argv){
    int c;
    while(1){
        static struct option long_options[] = {
            {"period", required_argument, 0, 'p'},
            {"log", required_argument, 0, 'l'},
            {"id", required_argument, 0, 'i'},
            {"host", required_argument, 0, 'h'},
            {"scale", required_argument, 0, 's'},
            //host
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "p:l:i:h:s:", long_options, &option_index);

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
        case 'i':
            if(strlen(optarg)!=9){
                fprintf(stderr, "ID should be 9 digits\n");
                exit(1);
            }
	    id[0] = 'I';
	    id[1] = 'D';
	    id[2] = '=';
	    strcpy(id+3, optarg);
	    id[12] = '\n';
            break;
        case 'h':
            host = optarg;
            break;
        case '?':
            //unrecognized argument                                                                     
            fprintf(stderr, "Unrecognized Argument. Usage: --period=# --scale=C or F --log=logfile\n");
            exit(1);
        default:
            abort();
        }
    }
	
    PORT = atoi(argv[optind]);
    if(PORT < 0){
	fprintf(stderr, "Invalid port\n");
	exit(1);
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
    
    struct sockaddr_in server_address;
    int address_length = sizeof(server_address);
    
    //creating socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0){
        fprintf(stderr, "Error while creating socket!\n");
        exit(1);
    }

    memset(&server_address, '0', address_length);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    
    hostserver = gethostbyname(host);
    if(hostserver==NULL){
        fprintf(stderr, "Error in gethostbyname\n");
        exit(2);
    }
    memcpy((char*)&server_address.sin_addr.s_addr, (char*)hostserver->h_addr, hostserver->h_length);

//    if(inet_pton(AF_INET, host, &server_address.sin_addr) <= 0){
//        fprintf(stderr, "Error converting host address\n");
//        exit(0);
//    }

    if(connect(sock, (struct sockaddr*) &server_address, address_length) < 0){
        fprintf(stderr, "Error while connectiong\n");
        exit(1);
    }

    //send ID
    if(send(sock, id, 13, 0) == -1){
        fprintf(stderr, "Sending ID failed\n");
        exit(1);
    }

    write(logfd, id, 13);

    //set up socket poll
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN|POLLHUP|POLLERR;

    char inputbuff[256];
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
            int readstatus = read(sock, inputbuff+validPtr, 256-validPtr);
            if(readstatus < 0){
                fprintf(stderr, "Read error\n");
                exit(1);
            }
            validPtr += readstatus;
            while(commandPtr < validPtr){
                if(inputbuff[commandPtr]=='\n'){
                    //process command
                    processCommand(inputbuff, commandPtr+1);
                    char tempbuff[256];
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
            char msgbuf[50];
            sprintf(msgbuf, "%02d:%02d:%02d %0.1f\n",local->tm_hour, local->tm_min, local->tm_sec, temp);

            //printf("%s\n", msgbuf);
            if(send(sock, msgbuf, strlen(msgbuf), 0) == -1){
                fprintf(stderr, "Error sending\n");
                exit(1);
            }

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
