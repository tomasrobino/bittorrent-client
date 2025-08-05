#ifndef BITTORRENT_CLIENT_MAGNET_H
#define BITTORRENT_CLIENT_MAGNET_H

/**
 * Enum representing different attributes of a magnet URI
 * For further documentation: https://en.wikipedia.org/wiki/Magnet_URI_scheme
 */
typedef enum {
    none = 20, // Represents no attribute
    xt, // Exact Topic - URN containing file hash
    dn, // Display Name - Filename to display to the user
    xl, // eXact Length - Size of file in bytes  
    tr, // TRacker URL - Address of BitTorrent tracker
    ws, // Web Seed - Direct download over HTTP/HTTPS
    as, // Acceptable Source - Direct download from web server
    xs, // eXact Source - P2P source or hub address
    kt, // Keyword Topic - Search keywords for P2P networks
    mt, // Manifest Topic - Link to metafile with magnets
    so // Select Only - Specific files to download from torrent
} Magnet_Attributes;

/**
 * Structure to store magnet URI data
 * Contains parsed attributes from magnet links following the Magnet URI scheme
 * For further documentation: https://en.wikipedia.org/wiki/Magnet_URI_scheme
 */
typedef struct {
    char *xt; //URN containing file hash
    char *dn; //A filename to display to the user
    long xl; //The file size, in bytes
    ll *tr; //Tracker URL linked list
    char *ws; //The payload data served over HTTP(S)
    char *as; //Refers to a direct download from a web server
    char *xs; //Either an HTTP (or HTTPS, FTP, FTPS, etc.) download source for the file pointed to by the Magnet link,
    //the address of a P2P source for the file or the address of a hub (in the case of DC++),
    //by which a client tries to connect directly, asking for the file and/or its sources
    char *kt; //Specifies a string of search keywords to search for in P2P networks, rather than a particular file
    char *mt; //Link to the metafile that contains a list of magnets
    char *so; //Lists specific files torrent clients should download,
    //indicated as individual or ranges (inclusive) of file indexes.
} magnet_data;

/**
 * Processes a magnet link string and extracts its attributes into a structured format.
 *
 * @param magnet A null-terminated string representing a magnet link, starting with "magnet:".
 *               The string should contain valid magnet link attributes separated by '&' or '?'.
 * @return A pointer to a dynamically allocated magnet_data structure containing the extracted
 *         attributes. The caller is responsible for freeing the returned structure using an
 *         appropriate function.
 */
magnet_data* process_magnet(const char* magnet);

/**
 * Frees the memory allocated for the `magnet_data` structure, including its dynamically allocated members.
 *
 * This function releases the memory for all fields within the `magnet_data` structure, including any
 * dynamically allocated strings, linked lists, or other resources. It then frees the `magnet_data` object itself.
 *
 * @param magnet_data Pointer to a `magnet_data` structure to be freed. If the pointer is `nullptr`, the function does nothing.
 */
void free_magnet_data(magnet_data* magnet_data);

#endif //BITTORRENT_CLIENT_MAGNET_H