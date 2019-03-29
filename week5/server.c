// Server side implementation of UDP client-server model 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT    5051
#define THREADS_COUNT 10


struct client_data{
    char name[128];
    int age;
    char group[32];
};

struct client_thread_data{
    int sockfd;
    int len;
    struct client_data data;
    struct sockaddr_in client_addr;
};


void *worker(void *data){
    char message[1024];
    struct client_thread_data *datum = (struct client_thread_data *)data;
    sprintf(message, "Your name is %s, your age is %d your group is %s", datum->data.name,
            datum->data.age, datum->data.group);

    printf("Thread %d \n %s \n",message, pthread_self());

    sendto(datum->sockfd, (char *) message, strlen(message),
        0, (struct sockaddr *) &datum->client_addr,
            sizeof(datum->client_addr));

};

// Driver code 
int main() {
    int sockfd;
    char *hello = "Hello from server";
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }


    pthread_t threads[THREADS_COUNT];
    int counter = 0;
        int n;
        struct client_thread_data *data = malloc(sizeof(struct client_thread_data));
        n = recvfrom(sockfd, (void *) &(data->data), sizeof(struct client_data),
                    MSG_WAITALL, (struct sockaddr *) &(data->client_addr),
                    &(data->len));
        data->sockfd = sockfd;
        pthread_create(&threads[counter], NULL, worker, (void *) data);

        pthread_join(&threads[counter], NULL);
        counter++;

    return 0;
}
