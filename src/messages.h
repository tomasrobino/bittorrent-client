#ifndef MESSAGES_H
#define MESSAGES_H
#include <netinet/in.h>

// Handshake length
#define HANDSHAKE_LEN 68
// Bittorrent message size without payload (only length and id).
#define MESSAGE_MIN_SIZE 5
/**
 * Enumeration of BitTorrent protocol message types.
 * Each value represents a specific message ID that can be sent between peers:
 * CHOKE (0): Indicates the peer is choking the client
 * UNCHOKE (1): Indicates the peer is unchoking the client
 * INTERESTED (2): Client is interested in downloading from peer
 * NOT_INTERESTED (3): Client is not interested in downloading from peer
 * HAVE (4): Peer has successfully downloaded and verified a piece
 * BITFIELD (5): Represents pieces that peer has (sent right after handshake)
 * REQUEST (6): Request to download a piece from peer
 * PIECE (7): Contains the actual piece data being transferred
 * CANCEL (8): Cancels a previously requested piece
 * PORT (9): DHT port number the peer is listening on
 */
typedef enum {
    MSG_ERROR = -1,
    CHOKE,
    UNCHOKE,
    INTERESTED,
    NOT_INTERESTED,
    HAVE,
    BITFIELD,
    REQUEST,
    PIECE,
    CANCEL,
    PORT
} MESSAGE_ID;
/**
 * Structure representing a BitTorrent protocol message
 * @param length Length of the message in bytes, excluding the length field itself
 * @param id Message ID identifying the type of message (e.g. choke, interested)
 * @param payload Message payload data (can be NULL for messages without payload)
 */
typedef struct {
    uint32_t length;
    MESSAGE_ID id;
    unsigned char *payload;
} bittorrent_message_t;

typedef struct {
    unsigned int index;
    unsigned int begin;
    unsigned int length;
} request_t;

typedef struct {
    unsigned int index;
    unsigned int begin;
    unsigned char* block;
} piece_t;
/**
 * Converts a bitfield into its corresponding hexadecimal string representation.
 *
 * @param bitfield Pointer to the array representing the bitfield to convert.
 * @param byte_amount Number of bytes in the bitfield array.
 * @param hex_output Pointer to the output buffer where the resulting hexadecimal string should be stored.
 *                   The buffer must be large enough to hold 2 * byte_amount characters plus a null terminator.
 */
void bitfield_to_hex(const unsigned char *bitfield, unsigned int byte_amount, char *hex_output);

/**
 * Attempts to establish a connection to a peer using the specified socket and address.
 * @param sockfd The file descriptor of the socket to be used for the connection.
 * @param peer_addr Pointer to a sockaddr_in structure containing the peer's address.
 * @return Returns 1 if the connection is successfully initiated or is in progress;
 *         returns 0 and closes the socket if the connection fails with an error
 *         other than EINPROGRESS.
 */
int try_connect(int sockfd, const struct sockaddr_in* peer_addr);

/**
 * Sends a BitTorrent handshake message through the specified socket.
 * A handshake is required to identify and establish communication with the peer.
 *
 * @param sockfd The file descriptor of the socket used to communicate with the peer.
 * @param info_hash A 20-byte string representing the SHA1 hash of the torrent's info dictionary.
 * @param peer_id A 20-byte unique identifier for the peer initiating the handshake.
 * @return Returns the number of bytes successfully sent (68 bytes for a complete handshake);
 *         returns a negative value if an error occurs while sending the handshake.
 */
int send_handshake(int sockfd, const char* info_hash, const char* peer_id);

/**
 * Processes the response during a handshake interaction by validating the
 * info hash and extracting the peer's ID.
 * @param sockfd The file descriptor of the socket from which the response is received.
 * @param info_hash A pointer to the 20-byte info hash that is expected to match the
 *                  handshake response.
 * @return Returns a dynamically allocated 20-byte character array containing the
 *         peer's ID if the handshake is successful; returns nullptr if the validation
 *         fails or an error occurs.
 */
char* handshake_response(int sockfd, const char* info_hash);

/**
 * Processes two bitfields to identify pending bits by performing a bitwise
 * operation on the foreign bitfield and the client's bitfield.
 *
 * @param client_bitfield Pointer to the bitfield representing the client's bits.
 * @param foreign_bitfield Pointer to the bitfield to compare against the client's bitfield.
 * @param size The total number of bits in the bitfields.
 * @return A dynamically allocated pointer to the resulting bitfield containing
 *         the bits from the foreign bitfield that are not set in the client bitfield.
 *         The caller is responsible for freeing the allocated memory.
 */
unsigned char* process_bitfield(const unsigned char* client_bitfield, const unsigned char* foreign_bitfield, unsigned int size);

/**
 * Reads a BitTorrent message from a socket descriptor.
 *
 * This function attempts to read a BitTorrent message from the given socket.
 * It reads the message length, type, and optional payload if the length indicates
 * the presence of one, and updates the peer's timestamp with the current time.
 * In case of an error while receiving the message, it may return a null pointer.
 *
 * @param sockfd The socket file descriptor from which the message will be read.
 * @param peer_timestamp Pointer to a timestamp that will be updated with the current time when a message is successfully read.
 *
 * @return A pointer to a dynamically allocated `bittorrent_message_t` structure
 * containing the read message. Returns a null pointer if the read operation fails
 * or if an invalid message is received.
 */
bittorrent_message_t* read_message(int sockfd, time_t* peer_timestamp);
#endif //MESSAGES_H
