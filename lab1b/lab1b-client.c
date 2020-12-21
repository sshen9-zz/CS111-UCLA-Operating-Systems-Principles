//NAME: Samuel Shen
//EMAIL: sam.shen321@gmail.com
//ID: 405325252

//Client Code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <termios.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ulimit.h>
#include <zlib.h>

extern int errno;

void reset(const struct termios* init_termios_p, int exitcode, int sock, z_stream* defstrm, z_stream* infstrm){
    shutdown(sock, SHUT_RDWR);
    if(tcsetattr(0, TCSANOW, init_termios_p)==-1){
        fprintf(stderr, "Error changing settings back: %s\r\n", strerror(errno));
        exit(1);
    }

    if(inflateEnd(infstrm)==-1){
        fprintf(stderr, "Inflate end error\r\n");
        exit(1);
    }

    if(deflateEnd(defstrm)==-1){
        fprintf(stderr, "Deflate end error\r\n");
        exit(1);
    }

    exit(exitcode);
}

int main(int argc, char** argv){
    //get options
    int c;
    int PORT = -1;
    char* logfile = NULL;
    int compress = 0;
    
    while(1){
        static struct option long_options[] = {
            {"port", required_argument, 0, 'p'},
            {"log", required_argument, 0, 'l'},
            {"compress", no_argument, 0, 'c'},
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "cp:l:", long_options, &option_index);

        if(c==-1){
            break;
        }

        switch(c){
        case 'p':
            PORT = atoi(optarg);
            break;
        case 'l':
            logfile = optarg;
            break;
        case 'c':
            compress = 1;
            break;
        case '?':
            //unrecognized argument                                                  
            fprintf(stderr, "Unrecognized Argument. Usage: --port=# --log=filename --compress\n");
            exit(1);
        default:
            abort();
        }
    }

    
    if(PORT==-1){
        fprintf(stderr, "Must specify port using --port=#\n");
        exit(1);
    }

    int logfd = 0;
    if(logfile!=NULL){
        logfd = creat(logfile, 0666);
        ulimit(UL_SETFSIZE, 10000);
    }

    //Read from keyboard, echo to display, send the input over to the server, then get
    //the output from the server and echo it back to the display
    
    struct sockaddr_in server_address;
    int sock = 0, address_length = sizeof(server_address);
    char buffer[256];
    char outputbuff[256];

    //Creating socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0){
        printf("Error while creating socket!\n");
        return -1;
    }

    memset(&server_address, '0', address_length);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0){
        printf("Invalid Address\n");
        return -1;
    }

    if(connect(sock, (struct sockaddr*) &server_address, address_length) < 0){
        printf("Error while connecting!\n");
        return -1;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[1].fd = sock;
    fds[0].events = POLLIN|POLLHUP|POLLERR;
    fds[1].events = POLLIN|POLLHUP|POLLERR;
    
    
    //SET TERMINAL SETTINGS:
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

    //SET UP COMPRESSION
    z_stream out_stream;
    z_stream in_stream;

    if(compress != 0){
        out_stream.zalloc = Z_NULL;
        out_stream.zfree = Z_NULL;
        out_stream.opaque = Z_NULL;

        int deflate_return = deflateInit(&out_stream, Z_DEFAULT_COMPRESSION);
        if(deflate_return != Z_OK){
            fprintf(stderr, "Unable to create compression outstream\r\n");
            reset(init_termios_p, 1, sock, &out_stream, &in_stream);
        }

        in_stream.zalloc = Z_NULL;
        in_stream.zfree = Z_NULL;
        in_stream.opaque = Z_NULL;

        int inflate_return = inflateInit(&in_stream);
        if(inflate_return != Z_OK){
            fprintf(stderr, "Unable to create decompression instream\r\n");
            reset(init_termios_p, 1, sock, &out_stream, &in_stream);
        }
                      
    }
    


    char map[2];
    map[0] = '\r';
    map[1] = '\n';
    
    ssize_t readstatus;
    while(1){
        if(poll(fds, 2, 0)==-1){
            fprintf(stderr, "Poll Error: %s\n", strerror(errno));
            reset(init_termios_p, 1, sock, &out_stream, &in_stream);
        }

        //NEED TO READ INTO
        
        //Check Readability of Keyboard
        if(fds[0].revents & POLLIN){
            
            //Read from Keyboard into buffer
            readstatus = read(0, buffer, 256);
            if(readstatus <= 0){
                fprintf(stderr, "Error with Read(): %s\n", strerror(errno));
                reset(init_termios_p, 1, sock, &out_stream, &in_stream);
            }


            //if compress, compress and send it over
            if(compress==1){
                char compressed_buff[256];
                int numbytes = readstatus;

                out_stream.total_out = 0;
                out_stream.total_in = 0;
                
                out_stream.avail_in = numbytes;
                out_stream.next_in = (Bytef*)buffer;
                out_stream.avail_out = 256;
                out_stream.next_out = (Bytef*)compressed_buff;

                deflate(&out_stream, Z_SYNC_FLUSH);

                //Write to LogFile
                if(logfile!=NULL){
                    char msgbuff[300];
                    numbytes = out_stream.total_out;
                    int res = snprintf(msgbuff, 300, "SENT %d bytes: ", numbytes);
                    write(logfd, msgbuff, res);
                    write(logfd, compressed_buff, out_stream.total_out);
                    char* end = "\r\n";
                    write(logfd, end, 2);
                }

                
                if(send(sock, compressed_buff, out_stream.total_out, 0)==-1){
                    fprintf(stderr, "Error with compressed send to socket: %s\n", strerror(errno));
                    reset(init_termios_p, 1, sock, &out_stream, &in_stream);
                }
            }else{
                //if no compress, send regularly

                //Write to LogFile
                if(logfile!=NULL && readstatus!=0){
                    char msgbuff[300];
                    int numbytes = readstatus;
                    int res = snprintf(msgbuff, 300, "SENT %d bytes: ", numbytes);
                    write(logfd, msgbuff, res);
                    write(logfd, buffer, readstatus);
                    char* end = "\r\n";
                    write(logfd, end, 2);
                }

                if(send(sock, buffer, readstatus, 0)==-1){
                    fprintf(stderr, "Error sending to socket: %s\n", strerror(errno));
                    reset(init_termios_p, 1, sock, &out_stream, &in_stream);
                }
            }

            for(int i=0; i<readstatus; i++){
                if(buffer[i]=='\r' || buffer[i]=='\n'){
                    if(write(1, map, 2)==-1){
                        fprintf(stderr, "Error writing to display: %s\n", strerror(errno));
                        reset(init_termios_p, 1, sock, &out_stream, &in_stream);
                    }
                }else{
                    if(write(1, buffer+i, 1)==-1){
                        fprintf(stderr, "Error writing to display: %s\n", strerror(errno));
                        reset(init_termios_p, 1, sock, &out_stream, &in_stream);
                    }
                }
            }
            
        }


        //Check Readability of Socket
        if(fds[1].revents & POLLIN){
            //Read Output from Socket
            readstatus = read(sock, outputbuff, 256);

            if(readstatus <= 0){
                if(readstatus!=0){
                    fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
                }
                reset(init_termios_p, 1, sock, &out_stream, &in_stream);
            }

            //Write to Logfile
            if(logfile!=NULL){
                int numbytes = readstatus;
                char msgbuff[300];
                int res = snprintf(msgbuff, 300, "RECEIVED %d bytes: ", numbytes);
                write(logfd, msgbuff, res);
                write(logfd, outputbuff, readstatus);
                char* end = "\r\n";
                write(logfd, end, 2);
            }

            
            //if compress, decompress here
            if(compress==1){
                char decomp_buff[500];

                in_stream.total_out = 0;
                in_stream.total_in = 0;
                
                in_stream.avail_in = (uInt)readstatus;
                in_stream.next_in = (Bytef*)outputbuff;
                in_stream.avail_out = 500;
                in_stream.next_out = (Bytef*)decomp_buff;

                inflate(&in_stream, Z_SYNC_FLUSH);
                int size = in_stream.total_out;

                for(int i=0; i<size; i++){
                    if(decomp_buff[i]==0x0A){
                        write(1, map, 2);
                    }else if(decomp_buff[i]==0x04){
                        //questionable
                        reset(init_termios_p, 0, sock, &out_stream, &in_stream);
                    }else{
                        write(1, decomp_buff+i, 1);
                    }
                }
                
            }else{

                //Write back to display
                if(write(1, outputbuff, readstatus)==-1){
                    fprintf(stderr, "Error writing output from socket back to display: %s\n", strerror(errno));
                    reset(init_termios_p, 1, sock, &out_stream, &in_stream);
                }
                
            }
            
        }
        
    }

    
}
