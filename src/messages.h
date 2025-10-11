#ifndef MESSAGES_H
#define MESSAGES_H
#include <netinet/in.h>

#include "downloading.h"
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

typedef int8_t MESSAGE_ID_t;
/**
 * Structure representing a BitTorrent protocol message
 * @param length Length of the message in bytes, excluding the length field itself
 * @param id Message ID identifying the type of message (e.g. choke, interested)
 * @param payload Message payload data (can be NULL for messages without payload)
 */
typedef struct {
    uint32_t length;
    MESSAGE_ID_t id;
    unsigned char *payload;
} bittorrent_message_t;

typedef struct {
    uint32_t index;
    uint32_t begin;
    uint32_t length;
} request_t;

typedef struct {
    uint32_t index;
    uint32_t begin;
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
void bitfield_to_hex(const unsigned char *bitfield, uint32_t byte_amount, char *hex_output);

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
int32_t try_connect(int32_t sockfd, const struct sockaddr_in* peer_addr, LOG_CODE log_code);

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
int32_t send_handshake(int32_t sockfd, const unsigned char *info_hash, const unsigned char *peer_id, LOG_CODE log_code);

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
unsigned char* process_bitfield(const unsigned char* client_bitfield, const unsigned char* foreign_bitfield, uint32_t size);

/**
 * @brief Reads the length of a bittorrent message from the given buffer and updates the peer's last activity timestamp.
 *
 * This function reads the `length` field from the provided buffer and interprets it in network byte order. It updates
 * the `peer_timestamp` to the current system time. If the message length is `0`, which signifies a keep-alive message,
 * the function cleans up allocated memory for the message and returns `false`. For other messages, the function
 * returns `true`, indicating further processing is required.
 *
 * Preconditions:
 * - The `buffer` must point to a valid memory location containing at least the `length` of a bittorrent message.
 * - The `peer_timestamp` pointer must not be null.
 *
 * Parameters:
 * @param buffer           A pointer to an array of `unsigned char` storing the raw message data.
 * @param peer_timestamp   A pointer to a `time_t` variable to record the peer's last activity timestamp.
 *
 * Return:
 * @return `true` if a non keep-alive message was read successfully, or `false` if a keep-alive message was detected.
 */
bool read_message_length(const unsigned char buffer[], time_t* peer_timestamp);

/**
 * @brief Handles the "HAVE" message from a peer in the BitTorrent protocol.
 *
 * The HAVE message indicates that the peer has successfully downloaded a specific piece.
 * This function updates the peer's bitfield to reflect the announced piece and checks
 * whether the client is interested in this piece. If the client has not yet downloaded
 * the piece, the `am_interested` flag for the peer is set to true.
 *
 * If the peer sends a HAVE message without first sending a BITFIELD message, a bitfield
 * will be allocated and initialized to track the peer's pieces.
 *
 * @param peer Pointer to the peer_t structure representing the connected peer.
 * @param payload Pointer to the message's payload, which contains the piece index.
 *        The piece index is represented as a 4-byte integer in network byte order.
 * @param client_bitfield Pointer to the client's bitfield, which represents the pieces
 *        the client has already downloaded.
 * @param bitfield_byte_size The size of the bitfield in bytes, used to correctly allocate
 *        memory for the peer's bitfield if necessary.
 * @param log_code Logging level used to determine whether to output verbose log messages.
 *        The log levels are defined as LOG_NO, LOG_ERR, LOG_SUMM, and LOG_FULL.
 */
void handle_have(peer_t *peer, const unsigned char *payload, const unsigned char *client_bitfield,
                 uint32_t bitfield_byte_size, LOG_CODE log_code);

/**
 * @brief Processes the BITFIELD message received from a peer.
 *
 * This function handles the BITFIELD message containing information about
 * which pieces the peer has. It updates the peer's bitfield and status
 * accordingly. If the payload is null, it initializes the peer's bitfield
 * to zero. Additionally, it checks if the peer has any pieces that
 * the client is missing, setting the status `am_interested` if necessary.
 * Logging is done based on the provided log code.
 *
 * @param peer A pointer to the peer_t structure representing the peer that sent the BITFIELD.
 * @param payload A pointer to the received BITFIELD payload from the peer.
 *                 This contains the pieces the peer currently has.
 * @param client_bitfield The bitfield of the client, representing the pieces it already has.
 * @param bitfield_byte_size The size of the bitfield in bytes.
 * @param log_code The log code enum (LOG_CODE) indicating the level of logging to perform.
 */
void handle_bitfield(peer_t *peer, const unsigned char *payload, const unsigned char *client_bitfield,
                     uint32_t bitfield_byte_size, LOG_CODE log_code);

/**
 * Handles an incoming request message from a peer. The request asks for a specific block of data
 * and, if the requested block is available, this function sends it back to the requesting peer.
 *
 * @param peer The peer sending the request, represented by its connection and status information.
 * @param payload The payload of the request message, containing details of the piece index, offset, and length.
 * @param log_code The logging level to determine the verbosity of log messages.
 */
void handle_request(const peer_t* peer, unsigned char* payload, LOG_CODE log_code);

/**
 * Broadcasts a "HAVE" message to all connected peers to indicate possession of a specific piece.
 *
 * @param peer_array An array of peers representing the current peer connections.
 * @param peer_count The total number of peers in the peer array.
 * @param piece_index The index of the piece that has been acquired.
 * @param log_code The level of logging to perform during the broadcast operation.
 */
void broadcast_have(const peer_t* peer_array, uint32_t peer_count, uint32_t piece_index, LOG_CODE log_code);

/**
 * Processes a received 'piece' message from a peer, updates internal state
 * such as the bitfield and block tracker, and manages the overall progress
 * of file downloading.
 *
 * @param peer The peer from which the 'piece' message was received.
 * @param payload The raw payload of the received 'piece' message to be processed.
 * @param metainfo The torrent metadata including piece and file details.
 * @param client_bitfield The bitfield indicating which pieces are already downloaded by the client.
 * @param block_tracker Tracker indicating the state of downloaded blocks for all pieces.
 * @param blocks_per_piece The number of blocks in a single piece.
 * @param peer_array Array of peer structures representing all connected peers.
 * @param peer_count The count of currently connected peers.
 * @param left_ptr Pointer to the remaining byte count to be downloaded. Modified when a block is fully processed.
 * @param log_code Indicates the logging level for messages and errors.
 */
void handle_piece(const peer_t *peer, unsigned char *payload, metainfo_t metainfo, unsigned char *client_bitfield,
                  unsigned char *block_tracker, uint32_t blocks_per_piece, const peer_t *peer_array, uint32_t peer_count,
                  uint64_t *left_ptr, LOG_CODE log_code);

#endif //MESSAGES_H
