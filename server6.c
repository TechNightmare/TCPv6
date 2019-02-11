#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>

#define PORT 44444
#define MAX_CLIENTS 42          // just relevant for listen()

int main()
{       
        int lfd;                // listen file descriptor
        int nfd;                // file descriptor of new client
        int n;                  // number of received characters
        char buffer [1486] ;    // read buffer
        struct sockaddr_in6 server;
        
        socklen_t len = sizeof(server);
        memset(buffer, 0, sizeof(buffer));      // clear reserved memory
        memset(&server, 0, sizeof(server));

        lfd = socket(AF_INET6, SOCK_STREAM, 0);
        if(lfd == -1)
                perror("Error socket()");
        else
                printf("Descriptor: %i\n", lfd);

        server.sin6_family = AF_INET6;         // ready for IPv6
        server.sin6_addr = in6addr_any;
        server.sin6_port = htons(PORT);
        
        int enable = 1;
        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)        // reuse port after closing server
            perror("setsockopt(SO_REUSEPORT) failed");
        if(bind(lfd,(struct sockaddr*) &server, len) == -1)                             // bind server to address
            perror("Error bind()\n");
        if(listen(lfd, MAX_CLIENTS) == -1)                                              // start listening
            perror("Error listen()\n");

        int fdmax = lfd;                        // biggest file descriptor currently
        fd_set rfds_copy, rfds_orig;            // file descriptor sets to handle multiple clients
        FD_ZERO(&rfds_orig);
        FD_ZERO(&rfds_copy);
        FD_SET(lfd, &rfds_orig);
        
        while(1)
        {
            int temp;                               // for current client number
            char extbuf[1500] = "Client ";          
            char clnumber[4];                       // give incoming clients a number
            
            memset(buffer, 0, sizeof(buffer));
            rfds_copy = rfds_orig;
            select(fdmax+1, &rfds_copy, 0, 0, 0);   // synchronous multiplexing with select()  
            
            if(FD_ISSET(lfd, &rfds_copy))           // new incoming connection
            {
                nfd = accept(lfd, (struct sockaddr*) &server, &len);    
                FD_SET(nfd, &rfds_orig);
                printf("Client with FD: %d connected\n", nfd);
                if(nfd > fdmax)                     // set new biggest file descriptor
                    fdmax = nfd;
            }
            else                                    // client sent a message read it and share with the others
            {
                for(int i = 4; i <= fdmax; ++i)     // client file descriptors start at 4
                {
                    temp = i;
                    if(FD_ISSET(i, &rfds_copy))     
                    {    
                        sprintf(clnumber, "%d", i-3);           // give sending client his number
                        strcat(extbuf, clnumber);
                        strcat(extbuf, ": ");

                        n = read(i, buffer, sizeof(buffer));    // read message from client
                        
			            strcat(extbuf, buffer);                 // merge clientnumber and message
                        if(!n)                                  // close connection to client
                        {
                            close(i);                           
                            FD_CLR(i, &rfds_orig);
                        }
                        else                                    // send message to all connected clients                        
                             for(int i = 4; i <= fdmax; ++i)
                                if(FD_ISSET(i, &rfds_orig))
                                    if(i != temp)               // exclude the sending client
                                        write(i, &extbuf, (15+n) * sizeof(char));           // 15 Bytes for "Client xxxxxx:"                                                                                                  
                    }
                }
            }
        }

    return EXIT_SUCCESS;
}    