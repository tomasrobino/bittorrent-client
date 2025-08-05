#ifndef BITTORRENT_CLIENT_MAGNET_H
#define BITTORRENT_CLIENT_MAGNET_H
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

magnet_data* process_magnet(const char* magnet);

void free_magnet_data(magnet_data* magnet_data);

#endif //BITTORRENT_CLIENT_MAGNET_H