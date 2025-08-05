#ifndef STRUCTS_H
#define STRUCTS_H
// This client's client id
#define CLIENT_ID "-IT0001-"

/**
 * A linked list node structure for storing string values
 * 
 * @param next Pointer to the next node in the linked list
 * @param val String value stored in this node 
 */
typedef struct ll {
    struct ll *next;
    char *val;
} ll;

/**
 * Structure containing arguments needed for performing a BitTorrent handshake
 * 
 * @param server_addr Network address information for the remote peer
 * @param sockfd Socket file descriptor for the network connection
 * @param info_hash 20-byte SHA1 hash of the info dictionary from the torrent metainfo
 * @param peer_id 20-byte string used as unique identifier for the peer 
 */
typedef struct {
    struct sockaddr *server_addr;
    int sockfd;
    const char *info_hash;
    const char *peer_id;
} handshake_args_t;

#endif //STRUCTS_H
