#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>
#include "peer.h"
#include <glob.h>


#define MAX_KNOWN_PEERS 30
#define MAX_BUF_SIZE 1024
#define MAX_THREADS 40
#define NAME "d.kalinin"
#define MAX_FILES 30


int release_thread(pthread_t th);

// Part for known peers
Peer known_peers[MAX_KNOWN_PEERS];

// Client
int client(){
    int sockfd;
    struct sockaddr_in serv_addr;
    int peer_id;

    // Print known peers for user
    list_peers(known_peers, MAX_KNOWN_PEERS);

    // Ask for peer to connect or abort
    printf("Enter peer id or -1 to abort:\n");
    scanf("%d", &peer_id);

    //Get peer and check that its alive
    if (peer_id == -1)
        return 0;
    if (peer_id >= MAX_KNOWN_PEERS || known_peers[peer_id].is_alive == 0)
    {
        printf("No such id in list!\n");
        return -1;
    }
    Peer tmp_peer = known_peers[peer_id];

    //Create socket
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        printf("Socket creation failed!\n");
        return -1;
    }

    //Create serv addr
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(tmp_peer.port);
    serv_addr.sin_addr.s_addr = inet_addr(tmp_peer.ip);

    //Connecting
    //printf("Connecting... to %s:%d\n", tmp_peer.ip, tmp_peer.port);
    if(connect(sockfd, (struct sockaddr *)&serv_addr,
                sizeof(struct sockaddr_in)) !=0)
    {
        printf("Connection failed!\n");
        return -1;
    }

    //Sending data
    int req = 0;
    int transfered_bytes = send(sockfd, &req, sizeof(int), 0);
    if(transfered_bytes == -1){
        printf("Peer rejected transfer!\n");
        return -1;
    }

    //Downloading
    char filename[MAX_BUF_SIZE];
    printf("Enter filename:\n");
    scanf("%s", filename);
    printf("Connected, sending %s to %s:%d ...\n", filename, tmp_peer.ip, tmp_peer.port);
    transfered_bytes = send(sockfd, filename, MAX_BUF_SIZE, 0);
    if(transfered_bytes == -1){
        printf("Data transfer failed!\n");
        return -1;
    }
    //printf("Succesfully transfered %d bytes to %s:%d \n", transfered_bytes, tmp_peer.ip, tmp_peer.port);

    //Getting amount of words
    int words;
    if(recv(sockfd, &words, sizeof(int), 0) == -1){
        printf("Failed to get response \n");
        return -1;
    }
    printf("Amount of words to be recieved: %d\n", words);
    char dir[MAX_BUF_SIZE];
    strcpy(dir, "data/");
    strcat(dir, filename);
    FILE *fp = fopen(dir, "w");

    //Receiving each word
    char buffer[MAX_BUF_SIZE];
    for(int i=0; i<words; i++){
        if(recv(sockfd, buffer, MAX_BUF_SIZE, 0) == -1)
            printf("Failed to get word \n");
        else fprintf(fp, "%s\n", buffer);
    }
    printf("Successfully got %d words\n", words);
    fclose(fp);

    //Close socket
    close(sockfd);
    return 0;
};

//--------------------------------- Find file in dir ---------------------------

int find_file(char *filename)
{
    DIR *dp;
    struct dirent *ep;     
    dp = opendir ("./data/");

    if (dp != NULL)
    {
        while (ep = readdir(dp))
        {
            if(strcmp(ep->d_name, filename)==0)
                return 0;
        }

        (void) closedir (dp);
    }
    else
        perror("Couldn't open the directory");

    return -1;
}

//--------------------------------- Parser part --------------------------------

int parse_peer(char buffer[MAX_BUF_SIZE], char *name, char *ip, int *port, char **filenames, int *n_of_files, int is_main)
{
    char *iter;

    //Parse name
    iter = strtok(buffer, ":");
    if (iter == NULL || strlen(iter) > 1024){
        printf("Invalid name or no name!\n");
        return -1;
    }
    strcpy(name, iter);

    //Parse ip
    iter = strtok(NULL, ":");
    if (iter == NULL || strlen(iter) > 15){
        printf("Invalid ip!\n");
        return -1;
    }
    strcpy(ip, iter);       

    //Parse port
    iter = strtok(NULL, ":");
    if (iter == NULL || strlen(iter) > 5){
        printf("Invalid port!\n");
        return -1;
    }
    *port = atoi(iter);

    //Parse filenames
    //Check if main peer
    if(is_main == 0)
        return 0;

    iter = strtok(NULL, " ");
    if (iter == NULL){
        printf("Invalid array of files!\n");
        return -1;
    }
    //Remove []
    //iter[strlen(iter)-1] = 0;
    //iter += 1;

    char *filename;
    int array_len = 0;
    filename = strtok(iter, ",");
    while(filename){
        strcpy(filenames[array_len], filename);
        array_len += 1;
        filename = strtok(NULL, ",");
    }
    *n_of_files = array_len;

    return 0;
}

//--------------------------------- Serv part --------------------------------

struct client_thread_data{
    int sockfd;
    int len;
    struct sockaddr_in client_addr;
};


void *data_worker(void *request_data){
    struct client_thread_data *data = (struct client_thread_data *)request_data;
    int request_flag;

    if(recv(data->sockfd, &request_flag, sizeof(int), 0) == -1)
        printf("Data transfer failed\n");

    if(request_flag != 0 && request_flag != 1) {
        printf("Invalid flag %d!\n", request_flag);
        release_thread(pthread_self());
        close(data->sockfd);
        return NULL;
    }


    //Take sync
    if(request_flag == 1){
        char peer_buf[MAX_BUF_SIZE];
        char *name = malloc(MAX_BUF_SIZE);
        char *ip = malloc(MAX_BUF_SIZE);
        int *port = malloc(sizeof(int));
        char **filenames = malloc(MAX_FILES);
        for(int i=0;i<MAX_FILES;i++)
            filenames[i] = malloc(255);
        int *n_of_files = malloc(sizeof(int));

        if(recv(data->sockfd, peer_buf, MAX_BUF_SIZE, 0) == -1)
            printf("Peer data ransfer failed\n");
        if(parse_peer(peer_buf, name, ip, port, filenames, n_of_files, 1)==-1){
            printf("Failed to parse main peer!\n");
            release_thread(pthread_self());
            close(data->sockfd);
            return NULL;
        }

        if(add_peer_req(known_peers, MAX_KNOWN_PEERS, name, ip, *port, filenames, *n_of_files) == -1){
            printf("Failed to add peer!\n");
            release_thread(pthread_self());
            close(data->sockfd);
            return NULL;
        }

        // Now getting other peers
        int n_of_peers;
        if(recv(data->sockfd, &n_of_peers, sizeof(int), 0) == -1)
        {
            printf("Failed to get peers\n");
            release_thread(pthread_self());
            close(data->sockfd);
            return NULL;
        }

        for(int i=0; i<n_of_peers; i++)
        {
            if(recv(data->sockfd, peer_buf, MAX_BUF_SIZE, 0) == -1)
                printf("Slave peer data ransfer failed\n");
            //printf("Peer %s\n", peer_buf);
            if(parse_peer(peer_buf, name, ip, port, filenames, 0, 0) == -1){
                printf("Failed to parse slave peer!\n");
                release_thread(pthread_self());
                close(data->sockfd);
                return NULL;
            }
            if(add_peer_req(known_peers, MAX_KNOWN_PEERS, name, ip, *port, filenames, 0) == -1){
                printf("Failed to add slave peer!\n");
                release_thread(pthread_self());
                close(data->sockfd);
                return NULL;
            }
        }
    }

    if((int)request_flag == 0){
        char filename_buf[MAX_BUF_SIZE];
        if(recv(data->sockfd, filename_buf, MAX_BUF_SIZE, 0) == -1)
            printf("Filename transfer failed\n");

        /*
        if(find_file(filename_buf)!=0){
            printf("Failed to find file!\n");
            release_thread(pthread_self());
            close(data->sockfd);
            return NULL;
        }
        */

        char *filename = filename_buf;
        char dir[255];
        sprintf(dir, "./data/%s", filename);
        FILE *fp = fopen(dir, "r");

        if(fp == NULL)
        {
            printf("No requested file %s\n", dir);
            release_thread(pthread_self());
            close(data->sockfd);
            return NULL;
        }

        int count = 0;
        char x[MAX_BUF_SIZE];

        while(fscanf(fp, "%1023s", x)==1)
            count++;
        send(data->sockfd, &count, sizeof(int), 0);
        fclose(fp);

        fp = fopen(dir, "r");
        while(fscanf(fp, " %1023s", x)==1) {
            x[MAX_BUF_SIZE-1] = '\0';
            send(data->sockfd, x, sizeof(x), 0);
        }
        fclose(fp);
        release_thread(pthread_self());
        close(data->sockfd);
        return 0;

    }
    release_thread(pthread_self());
    return 0;
}


int serv_sockfd;

// Thread manager
pthread_t threads[MAX_THREADS];
int thread_controller[MAX_THREADS];

int get_free_thread(){
    for (int i=0; i < MAX_THREADS; i++)
        if(thread_controller[i]==1)
        {
            thread_controller[i]=0;
            return i;
        }
    return -1;
}

int release_thread(pthread_t th){
    for (int i=0; i < MAX_THREADS; i++)
        if(thread_controller[i]==0 && threads[i]==th){
            thread_controller[i] = 1;
            threads[i] = 0;
            return 0;
        }
return -1;
}

// Server
void *server_p(void *server_data){
    Peer *server;
    server = (Peer *)server_data;

    //Create server socket
    if((serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) <0)
    {
        printf("Socket creation failed!\n");
        return NULL;
    }

    //Prepare serv_addr
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server->ip);
    serv_addr.sin_port = htons(server->port);

    //Bind socket
    if(bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) != 0){
        printf("Socket bind failed!\n");
        return NULL;
    }

    //Make socket to listen TCP connections
    if(listen(serv_sockfd, 30) != 0){
        printf("Listen failed!\n");
        return NULL;
    }

    //Accepting for clients
    int connection_fd;
    struct sockaddr_in client_addr;
    struct client_thread_data *client;
    socklen_t client_addr_len;
    int thread_index = 0;
    while(1)
    {
        sleep(1);
        if ((connection_fd = accept(serv_sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1){
            printf("Connection failed!\n");
            continue;
        }
        thread_index = get_free_thread();
        if(thread_index == -1){
            printf("Max threads reached\n");
            continue;
        }

        client = malloc(sizeof(struct client_thread_data));
        client->sockfd = connection_fd;
        client->client_addr = client_addr;
        client->len = client_addr_len;
        pthread_create(&threads[thread_index], NULL, data_worker, (void *)client);
    }

}

//--------------------------------- Synchronizer part --------------------------------
void get_files(char *buffer)
{
    const char  * pattern = "./data/*.txt";
    char filename[255];
    glob_t pglob;
    glob(pattern, GLOB_ERR, NULL, &pglob);
    for(int i=0; i < (int)pglob.gl_pathc; i++)
    {
        strcpy(filename, pglob.gl_pathv[i]);
        memmove(filename, filename + 7, strlen(filename) - 5);
        sprintf(&buffer[strlen(buffer)],"%s,",filename);
    }
}
//--------------------------------- Synchronizer part --------------------------------
void *synchronizer(void *server_data){
    int sockfd;
    struct sockaddr_in serv_addr;
    Peer *server;
    server = (Peer *)server_data;

    Peer tmp_peer;
    while(1){
        sleep(5);
        for(int i=0;i<MAX_KNOWN_PEERS;i++){
            tmp_peer = known_peers[i];
            if(tmp_peer.is_alive){
                //Create socket
                if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
                    printf("Socket creation failed in sync!\n");
                    return NULL;
                }

                //Create serv addr
                memset(&serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(tmp_peer.port);
                serv_addr.sin_addr.s_addr = inet_addr(tmp_peer.ip);

                //Connecting
                if(connect(sockfd, (struct sockaddr *)&serv_addr,
                            sizeof(struct sockaddr_in)) !=0){
                    printf("Connection failed!\n");
                    continue;
                }

                //Send 0
                int request = 1;
                int transfered_bytes = send(sockfd, &request, sizeof(int), 0);
                if(transfered_bytes == -1){
                    printf("Failed to send SYN\n");
                    continue;
                }

                char my_info[MAX_BUF_SIZE];
                char *my_files = malloc(MAX_BUF_SIZE);
                get_files(my_files);
                sprintf(my_info, "%s:%s:%d:%s", NAME, server->ip, server->port, my_files);

                transfered_bytes = send(sockfd, &my_info, MAX_BUF_SIZE, 0);
                if(transfered_bytes == -1){
                    printf("Failed to send my info\n");
                    continue;
                }

                //Send Number of peers

                int n_of_peers = count_peers(known_peers, MAX_KNOWN_PEERS) - 1;
                transfered_bytes = send(sockfd, &n_of_peers, sizeof(int), 0);
                if(transfered_bytes == -1){
                    printf("Failed to send number of files\n");
                    continue;
                }

                Peer to_send_peer;
                char peer_info[MAX_BUF_SIZE];
                for(int j=0; j<MAX_KNOWN_PEERS; j++){
                    to_send_peer = known_peers[j];
                    if(to_send_peer.is_alive == 0 || i == j)
                        continue;
                    sprintf(peer_info, "%s:%s:%d", to_send_peer.name, to_send_peer.ip, to_send_peer.port);
                    transfered_bytes = send(sockfd, &peer_info, MAX_BUF_SIZE, 0);
                    if(transfered_bytes == -1){
                        printf("Failed to send peer info\n");
                        continue;
                    }
                }

                close(sockfd);
            }
        }
    }

    return 0;
};

//--------------------------------- Main ---------------------------------------------
int main(int argc, char **argv){
    char command[256];
    pthread_t server_th;
    pthread_t synchronizer_th;
    Peer *server;

    for(int i=0;i<MAX_THREADS;i++)
        thread_controller[i]=1;

    //Create server
    server = malloc(sizeof(Peer));
    strcpy(server->ip, argv[1]);
    server->port = strtoul(argv[2], (char **)NULL, 10);

    //Run server in separate thread
    if(pthread_create(&server_th, NULL, server_p, (void *)server) !=0)
        printf("Server creation failed!\n");

    //Run server in separate thread
    if(pthread_create(&synchronizer_th, NULL, synchronizer, (void *)server) !=0)
        printf("Synchronizer creation failed!\n");


    printf("To see available commands print help:\n");
    while (1) {
        printf("Command:\n");
        scanf("%s", command);
        if (strcmp(command, "help") == 0){
            printf("Available commands: \n");
            printf("list - list all peers \n");
            printf("new - add new peer \n");
            printf("remove - remove peer \n");
            printf("ping - ping peer \n");
        }
        if (strcmp(command, "ping") == 0) 
            client();
        if (strcmp(command, "remove") == 0) 
            remove_peer(known_peers, MAX_KNOWN_PEERS);
        if (strcmp(command, "new") == 0) 
            add_peer(known_peers, MAX_KNOWN_PEERS);
        if (strcmp(command, "list") == 0) 
            list_peers(known_peers, MAX_KNOWN_PEERS);
        if (strcmp(command, "quit") == 0){
            pthread_join(server_th, NULL);
            break;
        }
    }
    return 0;

}
