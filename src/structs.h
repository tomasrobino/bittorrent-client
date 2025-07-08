#ifndef STRUCTS_H
#define STRUCTS_H
#include <stdint.h>
#define CLIENT_ID "-IT0001-"
#define ANNOUNCE_REQUEST_SIZE 98

typedef enum {
    none = 20,
    xt,
    dn,
    xl,
    tr,
    ws,
    as,
    xs,
    kt,
    mt,
    so
} Magnet_Attributes;

typedef enum {
    HTTP,
    HTTPS,
    UDP
} Protocols;

typedef struct ll {
    struct ll *next;
    char *val;
} ll;

typedef struct {
    char* xt; //URN containing file hash
    char* dn; //A filename to display to the user
    long xl;  //The file size, in bytes
    ll* tr; //Tracker URL linked list
    char* ws; //The payload data served over HTTP(S)
    char* as; //Refers to a direct download from a web server
    char* xs; //Either an HTTP (or HTTPS, FTP, FTPS, etc.) download source for the file pointed to by the Magnet link,
    //the address of a P2P source for the file or the address of a hub (in the case of DC++),
    //by which a client tries to connect directly, asking for the file and/or its sources
    char* kt; //Specifies a string of search keywords to search for in P2P networks, rather than a particular file
    char* mt; //Link to the metafile that contains a list of magnets
    char* so; //Lists specific files torrent clients should download,
    //indicated as individual or ranges (inclusive) of file indexes.
} magnet_data;

typedef struct files_ll {
    struct files_ll* next;
    long length;
    ll* path;
} files_ll;

typedef struct announce_list_ll {
    struct announce_list_ll* next;
    ll* list;
} announce_list_ll;

typedef struct {
    files_ll* files;
    long length;
    char* name;
    unsigned int piece_length;
    unsigned int piece_number;
    char* pieces;
    bool priv;
} info_t;

typedef struct {
    char* announce;
    announce_list_ll* announce_list;
    char* comment;
    char* created_by;
    unsigned int creation_date;
    char* encoding;
    info_t* info;
} metainfo_t;

typedef struct {
    char* host;
    char* port;
    Protocols protocol;
    int ip_version;
} address_t;

typedef struct {
    address_t* split_addr;
    char* ip;
    int sockfd;
    struct sockaddr* server_addr;
} connection_data_t;

typedef struct {
    uint64_t protocol_id; // 0x41727101980 magic constant
    uint32_t action; // 0 connect
    uint32_t transaction_id; // random
} connect_request_t;

typedef struct {
    uint32_t action; // 0 connect
    uint32_t transaction_id; // same as sent
    uint64_t connection_id; // assigned by tracker
} connect_response_t;


/*
Announce request

Offset  Size    Name    Value
0       64-bit integer  connection_id
8       32-bit integer  action          1 // announce
12      32-bit integer  transaction_id
16      20-byte string  info_hash
36      20-byte string  peer_id
56      64-bit integer  downloaded
64      64-bit integer  left
72      64-bit integer  uploaded
80      32-bit integer  event           0 // 0: none; 1: completed; 2: started; 3: stopped
84      32-bit integer  IP address      0 // default. If using IPv6, should always be 0
88      32-bit integer  key
92      32-bit integer  num_want        -1 // default
96      16-bit integer  port
98
*/
typedef struct {
    uint64_t connection_id;
    uint32_t action;
    uint32_t transaction_id;
    char info_hash[20];
    char peer_id[20];
    uint64_t downloaded;
    uint64_t left;
    uint64_t uploaded;
    uint32_t event;
    uint32_t ip;
    uint32_t key;
    uint32_t num_want;
    uint16_t port;
} announce_request_t;

// Represents a peer as returned in an announce response
typedef struct peer_ll {
    char* ip;
    uint16_t port;
    struct peer_ll* next;
} peer_ll;


/*
IPv4 announce response

Offset      Size            Name            Value
0           32-bit integer  action          1 // announce
4           32-bit integer  transaction_id
8           32-bit integer  interval
12          32-bit integer  leechers
16          32-bit integer  seeders
20 + 6 * n  32-bit integer  IP address
24 + 6 * n  16-bit integer  TCP port
20 + 6 * N

IPv6 has instead a stride size of 18, 16 of which belong to "IP address" and 2 to "TCP port"
*/
typedef struct {
    uint32_t action;
    uint32_t transaction_id;
    uint32_t interval;
    uint32_t leechers;
    uint32_t seeders;
    peer_ll* peer_list;
} announce_response_t;

#endif //STRUCTS_H
