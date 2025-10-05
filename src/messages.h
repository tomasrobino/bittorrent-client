#ifndef MESSAGES_H
#define MESSAGES_H
#include <netinet/in.h>

#include "util.h"

// Handshake length
#define HANDSHAKE_LEN 68
// Bittorrent message length's size
#define MESSAGE_LENGTH_SIZE 4
// Bittorrent message size without payload (only length and id).
#define MESSAGE_LENGTH_AND_ID_SIZE 5
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
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return Returns 1 if the connection is successfully initiated or is in progress;
 *         returns 0 and closes the socket if the connection fails with an error
 *         other than EINPROGRESS.
 */
int try_connect(int sockfd, const struct sockaddr_in* peer_addr, LOG_CODE log_code);

/**
 * Sends a BitTorrent handshake message through the specified socket.
 * A handshake is required to identify and establish communication with the peer.
 *
 * @param sockfd The file descriptor of the socket used to communicate with the peer.
 * @param info_hash A 20-byte string representing the SHA1 hash of the torrent's info dictionary.
 * @param peer_id A 20-byte unique identifier for the peer initiating the handshake.
 * @param log_code Controls the verbosity of logging output. Can be LOG_NO (no logging),
 *                 LOG_ERR (error logging), LOG_SUMM (summary logging), or
 *                 LOG_FULL (detailed logging).
 * @return Returns the number of bytes successfully sent (68 bytes for a complete handshake);
 *         returns a negative value if an error occurs while sending the handshake.
 */
int send_handshake(int sockfd, const unsigned char *info_hash, const unsigned char *peer_id, LOG_CODE log_code);

/**
 * Validates the handshake response from a peer to ensure it complies with the expected
 * BitTorrent protocol format and matches the provided info hash.
 *
 * @param info_hash A pointer to a 20-byte info hash that uniquely identifies the torrent.
 * @param buffer A pointer to the handshake buffer received from the peer, which includes
 * the protocol identifier, reserved bytes, and the info hash.
 * @return True if the handshake is valid (protocol matches "BitTorrent protocol" and
 * info hashes are identical), otherwise false.
 */
bool check_handshake(const unsigned char* info_hash, const unsigned char* buffer);

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

bool read_message_length(const unsigned char buffer[], time_t* peer_timestamp);

bittorrent_message_t* read_message_body(bittorrent_message_t* message, const unsigned char buffer[], time_t* peer_timestamp);

#endif //MESSAGES_H
