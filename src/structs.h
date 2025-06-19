#ifndef STRUCTS_H
#define STRUCTS_H
#include <stdint.h>

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
} address_t;

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

#endif //STRUCTS_H
