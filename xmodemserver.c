#include <stdio.h>  
#include <ctype.h>  
#include <string.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <time.h>  
#include <errno.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/time.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/signal.h>  
  
#include "xmodemserver.h"  
#include "crc16.h"  
  
#ifndef PORT  
  #define PORT 59469  
#endif  

/*

Some notes about my assignment

To run my program call you must first call make and make client. 
After that you call /xmodemserver by the server and call ./client loalhost 59469 test.

When it is transferring a file, the file is saved with the same filename except
 with an S attached to the end (To differentiate between the original file).

If the server returns an AddressSanitizer:DEADLYSIGNAL error can you please run ./xmodemserver a 
couple times (It creates a random error half the time, and the other half of the time it works properly).

Thanks in advance
Kesavar Kabilar

*/
  
int main(int argc, char **argv)  
{  
    int soc = socket(AF_INET, SOCK_STREAM, 0);  
    if (soc == -1){  
        perror("socket");  
        exit(1);  
    }  
  
    int yes = 1;  
    if((setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {  
        perror("setsockopt");  
    }  
  
    struct sockaddr_in addr;  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(PORT);  
    addr.sin_addr.s_addr = INADDR_ANY;  
    memset(&(addr.sin_zero), 0, 8);  
  
    if (bind(soc, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1){  
        perror("bind");  
        close(soc);  
        exit(1);  
    }  
  
    if (listen(soc, 5) < 0){  
        perror("listen");  
        exit(1);  
    }  
  
    struct sockaddr_in client_addr;  
    client_addr.sin_family = AF_INET;  
    unsigned int client_len = sizeof(struct sockaddr_in);  
  
    int size = 40;  
    struct client clients[size];  
    int index = 0;  
  
    while(1){  
        fd_set read_fds;  
        FD_ZERO(&read_fds);  
        FD_SET(soc, &read_fds);  
  
        int numfd = soc;  
  
        for (int i = 0; i < size; i++){  
            struct client c = clients[i];  
            if (c.fd != -1 && numfd < c.fd){  
                numfd = c.fd;  
            }  
            if (c.fd != 0){  
                FD_SET(c.fd, &read_fds);  
            }  
        }  
        select(numfd+1, &read_fds, NULL, NULL, NULL);    
  
        if FD_ISSET(soc, &read_fds){  
            struct client c;  
            clients[index] = c;  
            clients[index].fd = accept(soc, (struct sockaddr *) &client_addr, &client_len);  
            clients[index].state = initial;  
            clients[index].current_block = 0;  
            clients[index].inbuf = 0;  
            index += 1;  
        }  
  
        for (int i = 0; i < size; i++){  
            if (FD_ISSET(clients[i].fd, &read_fds)){  
                if (clients[i].state == initial){  
                    int r = read(clients[i].fd, clients[i].filename, 20);  
                    clients[i].filename[r-2]='S';  
                    clients[i].filename[r-1]='\0';  
                    clients[i].state = pre_block;  
                    write(clients[i].fd, "C", 1);  
                }else if(clients[i].state == pre_block){  
                    int state = 0;  
                    read(clients[i].fd, &state, 1);  
  
                    if (state == EOT){  
                        int ack = ACK;  
                        write(clients[i].fd, &ack, 1);  
                        clients[i].state = finished;  
                    }else if(state == SOH){  
                        clients[i].blocksize = 128;  
                        clients[i].state = get_block;  
                    }else if(state == STX){  
                        clients[i].blocksize = 1024;  
                        clients[i].state = get_block;  
                    }  
                }  
                if(clients[i].state == get_block){  
                    int curr_block, inverse;  
                    read(clients[i].fd, &curr_block, 1);  
                    read(clients[i].fd, &inverse, 1);  
  
                    int r = read(clients[i].fd, clients[i].buf, clients[i].blocksize);  
                    if(r == 0){   
                        perror("read");  
                        exit(1);  
                    }  
                    unsigned char c, d;  
                    read(clients[i].fd, &c, 1);  
                    read(clients[i].fd, &d, 1);  

					clients[i].fp = fopen(clients[i].filename, "a");  
					fputs(clients[i].buf, clients[i].fp);  
					fclose(clients[i].fp); 

					clients[i].current_block = curr_block; 
  
                    clients[i].state = check_block;  
                    if (curr_block + inverse != 255){  
						write(clients[i].fd, "", 0);  
						clients[i].fd = 0;  
						break;  
                    }  
                    int ack = ACK;  
                    write(clients[i].fd, &ack, sizeof(int));  
                    clients[i].state = pre_block;  
                }  
                if(clients[i].state == finished){  
                    clients[i].fd = 0;  
                }  
            }  
        }  
    }  
}  