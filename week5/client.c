// Client side implementation of UDP client-server model 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT	 5051
#define MAXLINE 1024
#define SERVER_IP "127.0.0.1"


struct client_data{
    char name[128];
    int age;
    char group[32];
};

struct client_data data;

// Driver code 
int main() {
	int sockfd;
	char buffer[MAXLINE];
	char *hello = "Hello from client";
	struct sockaddr_in	 servaddr;

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information 
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while(1){

        int n;

	    printf("Your name?\n");
	    scanf("%s", &data.name);
	    printf("Your age?\n");
	    scanf("%u", &data.age);
	    printf("Your group?\n");
	    scanf("%s", &data.group);

        sendto(sockfd, (void *) &data, sizeof(struct client_data),
             0, (const struct sockaddr *) &servaddr,
                sizeof(servaddr));
        printf("Data sent.\n");
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                    MSG_WAITALL, (struct sockaddr *) &servaddr,
                    MAXLINE);
        buffer[n] = '\0';
        printf("Server returned : %s\n", buffer);

        close(sockfd);
    }
	return 0;
} 

