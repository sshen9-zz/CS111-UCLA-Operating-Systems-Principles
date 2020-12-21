//NAME: Samuel Shen
//EMAIL: sam.shen321@gmail.com
//ID: 405325252

//Server Code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <errno.h>
#include <sys/poll.h>
#include <signal.h>
#include <zlib.h>

extern int errno;

void end_streams(z_stream* out, z_stream* in){
    if(inflateEnd(out)==-1){
        fprintf(stderr, "Inflate end error\n");
        exit(1);
    }
    
    if(deflateEnd(in)==-1){
        fprintf(stderr, "Deflate end error\n");
        exit(1);
    }
}


void getShellExitStatus(pid_t p, int exitstatus){
    if(waitpid(p, &exitstatus, WUNTRACED|WNOHANG|WCONTINUED)==-1){
        fprintf(stderr, "Error with waitpid\n");
        exit(1);
    }

    int stat = WEXITSTATUS(exitstatus);
    int sig = exitstatus&0x00FF;
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", sig, stat);
}

int main(int argc, char* argv[]){
    //get options
    int c;
    int PORT = -1;
    char* program = NULL;
    int compress = 0;
    
    while(1){
        static struct option long_options[] = {
            {"port", required_argument, 0, 'p'},
            {"shell", required_argument, 0, 's'},
            {"compress", no_argument, 0, 'c'},
            {0,0,0,0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "cp:s:", long_options, &option_index);

        if(c==-1){
            //end of options
            break;
        }

        switch(c){
        case 'p':
            PORT = atoi(optarg);
            break;
        case 's':
            program = optarg;
            break;
        case 'c':
            compress = 1;
            break;
        case '?':
            //unrecognized argument                                                                                
            fprintf(stderr, "Unrecognized Argument. Usage: --port=# --shell=name --compress\n");
            exit(1);
        default:
            abort();
        }
    }

    if(PORT==-1){
        fprintf(stderr, "Error in Port; Use --port=#\n");
        exit(1);
    }

    if(program==NULL){
        fprintf(stderr, "Error in shell; use --shell=program_name\n");
        exit(1);
    }
    
    struct sockaddr_in address;
    int sockfd, new_socket, address_length = sizeof(address);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        printf("Error while creating socket!\n");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(sockfd, (struct sockaddr*) &address, address_length) < 0){
        printf("Error while binding socket!\n");
        return -1;
    }

    if(listen(sockfd, 5)<0){
        printf("Error while listening!\n");
        return -1;
    }

    new_socket = accept(sockfd, (struct sockaddr*) &address, (socklen_t*) &address_length);

    if(new_socket < 0){
        printf("Error while accepting!\n");
        return -1;
    }


    //SET UP COMPRESSION
    z_stream out_stream; //decompress from client, send to shell
    z_stream in_stream;  //compress from shell, send to client
    
    if(compress != 0){
        out_stream.zalloc = Z_NULL;
        out_stream.zfree = Z_NULL;
        out_stream.opaque = Z_NULL;

        int inflate_return = inflateInit(&out_stream);
        if(inflate_return != Z_OK){
            fprintf(stderr, "Unable to create decompression outstream\n");
            exit(1);
        }

        in_stream.zalloc = Z_NULL;
        in_stream.zalloc = Z_NULL;
        in_stream.zalloc = Z_NULL;

        int deflate_return = deflateInit(&in_stream, Z_DEFAULT_COMPRESSION);
        if(deflate_return != Z_OK){
            fprintf(stderr, "Unable to create compression instream\n");
            exit(1);
        }
    }

    

    

    //FORK PROCESS
    int parent_to_child[2];
    int child_to_parent[2];

    if(pipe(parent_to_child)==-1){
        fprintf(stderr, "Pipe parent to child failed\n");
        exit(1);
    }
    if(pipe(child_to_parent)==-1){
        fprintf(stderr, "Pipe child to parent failed\n");
        exit(1);
    }

    pid_t p;
    p = fork();
    if(p < 0){
        fprintf(stderr, "Fork Failed\n");
        exit(1);
    }else if(p==0){
        //Child
        
        //read from parent to child[0] and write to child to parent[1];                      
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        //redirect stdin                                                                     
        close(0);
        dup(parent_to_child[0]);
        close(parent_to_child[0]);

        //redirect stdout and stderr
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


    //Parent

    close(parent_to_child[0]);
    close(child_to_parent[1]);
    
    char inputbuff[256];
    char outputbuff[256];
    
    char map[2];
    map[0] = 0x0D;
    map[1] = 0x0A;
    
    ssize_t readstatus;
    int exitstatus = 0;

    //Poll 0: monitor status of shell
    //Poll 1: monitor the input socket
    
    struct pollfd fds[2];
    fds[0].fd = child_to_parent[0];
    fds[0].events = POLLIN|POLLHUP|POLLERR;
    fds[1].fd = new_socket;
    fds[1].events = POLLIN|POLLHUP|POLLERR;
        


    
    int eof = 0;
    while(eof==0){
        if(poll(fds, 2, 0)==-1){
            //shell process?
            fprintf(stderr, "Poll Error: %s\n", strerror(errno));
            close(parent_to_child[1]);
            end_streams(&out_stream, &in_stream);
            exit(1);
        }

        
        //Check Readability of Socket
        if(fds[1].revents & POLLIN){
            readstatus = read(new_socket, inputbuff, 256);
            if(readstatus <= 0){
                close(parent_to_child[1]);
                getShellExitStatus(p, exitstatus);
                fprintf(stderr, "Client exited, connection terminated. Closed Shell\n");
                shutdown(new_socket, SHUT_RDWR);
                end_streams(&out_stream, &in_stream);
                exit(0);
            }


            if(compress==1){
                //if compress, inflate/decompress here and send to shell
                char decomp_buff[500];

                out_stream.total_in = 0;
                out_stream.total_out = 0;
                
                out_stream.avail_in = (uInt)readstatus;
                out_stream.next_in = (Bytef*)inputbuff;
                out_stream.avail_out = 500;
                out_stream.next_out = (Bytef*)decomp_buff;

                inflate(&out_stream, Z_SYNC_FLUSH);
                int size = out_stream.total_out;
                
                //                write(1, decomp_buff, out_stream.total_out);
                    
                //Loop Through Socket Input
                for(int i=0; i<size; i++){
                    if(decomp_buff[i]==0x03){
                        //Ctrl C
                        if(kill(p, SIGINT)==-1){
                            fprintf(stderr, "Kill Error\n");
                            getShellExitStatus(p, exitstatus);
                            shutdown(new_socket, SHUT_RDWR);
                            end_streams(&out_stream, &in_stream);
                            exit(1);
                        }
                        getShellExitStatus(p, exitstatus);
                        shutdown(new_socket, SHUT_RDWR);
                        end_streams(&out_stream, &in_stream);
                        exit(0);
                
                    }else if(decomp_buff[i]==0x04){
                        //EOF
                        eof = 1;
                        close(parent_to_child[1]);
                        getShellExitStatus(p, exitstatus);
                        end_streams(&out_stream, &in_stream);
                        shutdown(new_socket, SHUT_RDWR);
                        exit(0);
                
                    }else if(decomp_buff[i]==0x0D || decomp_buff[i]==0x0A){
                        if(write(parent_to_child[1], map+1, 1)==-1){
                            close(parent_to_child[1]);
                            shutdown(new_socket, SHUT_RDWR);
                            end_streams(&out_stream, &in_stream);
                            exit(1);
                        }
                    }else{
                        if(write(parent_to_child[1], decomp_buff+i, 1)==-1){
                            close(parent_to_child[1]);
                            shutdown(new_socket, SHUT_RDWR);
                            end_streams(&out_stream, &in_stream);
                            exit(1);
                        }
                    }
                }



            }else{
                //Loop Through Socket Input
                for(int i=0; i<readstatus; i++){
                    if(inputbuff[i]==0x03){
                        //Ctrl C
                        if(kill(p, SIGINT)==-1){
                            fprintf(stderr, "Kill Error\n");
                            exit(1);
                        }
                        getShellExitStatus(p, exitstatus);
                        shutdown(new_socket, SHUT_RDWR);
                        end_streams(&out_stream, &in_stream);
                        exit(0);
                
                    }else if(inputbuff[i]==0x04){
                        //EOF
                        eof = 1;
                        close(parent_to_child[1]);
                        getShellExitStatus(p, exitstatus);
                        shutdown(new_socket, SHUT_RDWR);
                        end_streams(&out_stream, &in_stream);
                        exit(0);
                
                    }else if(inputbuff[i]==0x0D || inputbuff[i]==0x0A){
                        if(write(parent_to_child[1], map+1, 1)==-1){
                            close(parent_to_child[1]);
                            shutdown(new_socket, SHUT_RDWR);
                            end_streams(&out_stream, &in_stream);
                            exit(1);
                        }
                    }else{
                        if(write(parent_to_child[1], inputbuff+i, 1)==-1){
                            close(parent_to_child[1]);
                            shutdown(new_socket, SHUT_RDWR);
                            end_streams(&out_stream, &in_stream);
                            exit(1);
                        }
                    }
                }

                //end else block
            }
                    
        }

        //Read from shell pipe, send back to socket
        if(fds[0].revents & POLLIN){
            readstatus = read(child_to_parent[0], outputbuff, 256);

            //if compress, compress/deflate here and send back to client
            if(compress==1){
                char comp_buff[256];
                in_stream.total_in = 0;
                in_stream.total_out = 0;

                in_stream.avail_in = (uInt)readstatus;
                in_stream.next_in = (Bytef*)outputbuff;
                in_stream.avail_out = 256;
                in_stream.next_out = (Bytef*)comp_buff;

                deflate(&in_stream, Z_SYNC_FLUSH);
                
                if(send(new_socket, comp_buff, in_stream.total_out, 0)==-1){
                    fprintf(stderr, "Error sending compressed output to client\n");
                    close(parent_to_child[1]);
                    getShellExitStatus(p, exitstatus);
                    shutdown(new_socket, SHUT_RDWR);
                    end_streams(&out_stream, &in_stream);
                    exit(1);
                }
                //endif compression block
            }else{
                for(int i=0; i<readstatus; i++){
                    if(outputbuff[i]==0x04){
                        //EOF from Shell
                        close(parent_to_child[1]);
                        getShellExitStatus(p, exitstatus);
                        shutdown(new_socket, SHUT_RDWR);
                        end_streams(&out_stream, &in_stream);
                        exit(0);
                    }else if(outputbuff[i]==0x0A){
                        send(new_socket, map, 2, 0);
                    }else{
                        send(new_socket, outputbuff+i, 1, 0);
                    }
                }
            }
                        
        }


        //Check for pollerr in the shell
        if(fds[0].revents&POLLHUP||fds[0].revents&POLLERR){
            close(parent_to_child[1]);
            getShellExitStatus(p, exitstatus);
            close(new_socket);
            end_streams(&out_stream, &in_stream);
            exit(1);
        }

    }

    
}
