//
// Created by Tomas on 12/11/2025.
//

#ifndef BITTORRENT_CLIENT_MESSAGES_TYPES_H
#define BITTORRENT_CLIENT_MESSAGES_TYPES_H


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
 * @param id Message ID identifying the type of message (e.g., choke, interested)
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

#endif //BITTORRENT_CLIENT_MESSAGES_TYPES_H