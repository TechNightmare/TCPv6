#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 44444

void clearBuffer();

int main()
{       
        int fd, rc, n, check;
        struct sockaddr_in6 server;
        socklen_t len = sizeof(server);
        char *buffer = (char*)calloc(1485, sizeof(char));        
        
        memset(&server, 0, sizeof(server));

        fd = socket(AF_INET6, SOCK_STREAM, 0);
        if(fd == -1)
                perror("Error socket()");
        else
                printf("Descriptor: %i\n", fd);

        server.sin6_family = AF_INET6;                  // server init
        //server.sin6_addr = IN6ADDR_LOOPBACK_INIT;
        inet_pton(AF_INET6, "::1", &server.sin6_addr);  // server address
        server.sin6_port = htons(PORT);
        
        check = connect(fd, (struct sockaddr*) &server, len);
        if(check == -1)
            perror("Error at connect()\n");
        else
            printf("Client connected to server\n");
        
        fd_set rfds_orig, rfds_copy;
        FD_ZERO(&rfds_orig);
        FD_SET(fd, &rfds_orig);
        FD_SET(0, &rfds_orig);           // 0 is the keyboard file descriptor

        for(;;)                          // select between keyboard input and message from server 
        {
            int count = 0;               // number of written bytes
            rfds_copy = rfds_orig;

            rc = select(fd+1, &rfds_copy, 0, 0, 0);

            if(FD_ISSET(0, &rfds_copy))         // if keyboard input
            {
                scanf("%[^\n]s", buffer);       // read message
                clearBuffer();
                strcat(buffer, "\n");
                
                count = strlen(buffer);   
                write(fd, buffer, count+1);     // send to server
                memset(buffer, 0, (count+1) * sizeof(char));
            }

           
            if(FD_ISSET(fd, &rfds_copy))
            {
                n = read(fd, buffer, 1485 * sizeof(char));      // message from server
                printf("%s", buffer);
                memset(buffer, 0, n * sizeof(char));
            } 
        }

    return EXIT_SUCCESS;
}

void clearBuffer()      // clear input buffer
{
    char dummy;

    do
        scanf("%c", &dummy);
    while(dummy != '\n');
}
