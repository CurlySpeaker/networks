typedef struct Peer {
    int is_alive;
    char name[1024];
    char ip[16];
    int port;
    char **filenames;
    int n_of_files;
    int sock_fd;
} Peer;


void init_peers(Peer *peers, int size);
int count_peers(Peer *peers, int size);
void list_peers(Peer *peers, int size);
int find_peer(Peer *peers, int size, char *ip, int port);
int add_peer(Peer *peers, int size);
int add_peer_req(Peer *peers, int size, char *name, char *ip, int port, char **filenames,int n_of_files);
int remove_peer(Peer *peers, int size);
