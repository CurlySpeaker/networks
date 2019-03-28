#include <string.h>
#include <stdio.h>
#include "peer.h"

void init_peers(Peer *peers, int size)
{
    memset(peers, 0, sizeof(Peer)*size);
}


int count_peers(Peer *peers, int size)
{
    Peer tmp_peer;
    int count = 0;
    for(int i=0; i<size; i++)
    {
        tmp_peer = peers[i];
        if(tmp_peer.is_alive == 1)
            count++;
    }
    return count;
}


int find_peer(Peer *peers, int size, char *ip, int port)
{
    Peer tmp_peer;
    for(int i=0; i<size; i++)
    {
        tmp_peer = peers[i];
        if(tmp_peer.is_alive == 1 && strcmp(tmp_peer.ip,ip) == 0 && tmp_peer.port == port)
            return i;
    }
    return -1;
}


void list_peers(Peer *peers, int size)
{
    Peer tmp_peer;
    printf("/----------------------------\\\n");
    printf("|--------LIST OF PEERS-------|\n");
    printf("|----------------------------|\n");
    for(int i=0; i<size; i++)
    {
        tmp_peer = peers[i];
        if(tmp_peer.is_alive == 1){
            printf("ID: %d \n name: %s \n ip: %s \n port: %d \n",
                    i, tmp_peer.name, tmp_peer.ip, tmp_peer.port); 
            printf("|----------FILES-------------|\n");
            for(int j=0; j<tmp_peer.n_of_files; j++)
                printf("%d : %s \n", j, tmp_peer.filenames[j]);
            printf("|----------------------------|\n");
        }
    }

}


int add_peer(Peer *peers, int size)
{
    Peer *tmp_peer;
    int id = -1;
    for(int i=0; i<size; i++)
        if(peers[i].is_alive==0){
            tmp_peer = &peers[i];
            id = i;
            break;
        }
    if(id == -1)
    {
        printf("Reached the limit of peers \n");
        return -1;
    }
    printf("Enter peer data:\n");
    printf("Enter name:\n");
    scanf("%s", tmp_peer->name);
    printf("Enter ip:\n");
    scanf("%s", tmp_peer->ip);
    printf("Enter port:\n");
    scanf("%d", &tmp_peer->port);
    tmp_peer->is_alive = 1;
    printf("Succesfully created peer with ID: %d \n", id);
    return 0;
}


int add_peer_req(Peer *peers, int size, char *name, char *ip, int port, char **filenames,int n_of_files)
{

    Peer *tmp_peer;
    int id = -1;
    if((id = find_peer(peers, size, ip, port)) != -1)
    {
        peers[id].filenames = filenames;
        if(n_of_files > 0)
            peers[id].n_of_files = n_of_files;
        return 0;
    }

    for(int i=0; i<size; i++)
        if(peers[i].is_alive==0){
            tmp_peer = &peers[i];
            id = i;
            break;
        }
    if(id == -1)
    {
        printf("Reached the limit of peers \n");
        return -1;
    }
    strcpy(tmp_peer->name, name);
    strcpy(tmp_peer->ip, ip);
    tmp_peer->port = port;
    peers[id].filenames = filenames;
    if(n_of_files >0)
        tmp_peer->n_of_files = n_of_files;
    tmp_peer->is_alive = 1;
    printf("Succesfully created peer with ID: %d \n", id);
    return 0;
}


int remove_peer(Peer *peers, int size)
{
    int id;
    printf("Enter peer id:\n");
    scanf("%d", &id);
    if(id<0 || id>=size)
    {
        printf("No peer with such ID \n");
        return -1;
    }
    peers[id].is_alive = 0;
    return 0;
}



