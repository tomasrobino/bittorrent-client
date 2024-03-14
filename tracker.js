"use strict"

import dgram from "dgram";
import crypto from "crypto";
import { infoHash } from "./torrentParser.js";
import { generateID } from "./peerID.js";


export function getPeers(torrent, cb) {
    const decoder = new TextDecoder();
    const url = decoder.decode(torrent.announce);
    const socket = dgram.createSocket("udp4");
    sendUDP(socket, buildConReq(), url);

    socket.on("message", msg => {
        console.log(msg);
    });
}


//Sends a connect request
function sendUDP(socket, msg, rawURL, cb=()=>{console.log("connect request sent")}) {
    const torrentURL = new URL(rawURL);
    if (torrentURL.port === "") torrentURL.port = "161";
    socket.send(msg, 0, msg.length, torrentURL.port, torrentURL.host, cb);
}


/*
Connect request format is the following:

Offset  Size            Name            Value
0       64-bit integer  protocol_id     0x41727101980 // magic constant
8       32-bit integer  action          0 // connect
12      32-bit integer  transaction_id
16

*/
function buildConReq() {
    const buffer = Buffer.alloc(16);
    //Write protocol_id
    //Must be Big-Endian due to protocol
    //Split because precise 64-bit integers aren't supported
    buffer.writeUint32BE(0x417, 0);
    buffer.writeUint32BE(0x27101980, 4);
    //Write action
    buffer.writeUint32BE(0, 8);
    //Write transaction_id
    crypto.randomBytes(4).copy(buffer, 12);

    return buffer;
}


/*
Connect response format is the following:

Offset  Size            Name            Value
0       32-bit integer  action          0 // connect
4       32-bit integer  transaction_id
8       64-bit integer  connection_id
16

*/
function parseConRes(res) {
    return {
        action: res.readUInt32BE(0),
        transaction_id: res.readUInt32BE(4),
        //Slice because of imprecise 64-bit ints
        connection_id: res.slice(8)
    }
}

/*
Announce request format is the following:

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
84      32-bit integer  IP address      0 // default
88      32-bit integer  key             ? // random
92      32-bit integer  num_want        -1 // default
96      16-bit integer  port            ? // should be between 6681 and 6685
98

*/
function buildAnnounceReq(connection_id, torrent) {
    const buffer = Buffer.allocUnsafe(98);
    connection_id.copy(buffer);
    //action
    buffer.writeUint32BE(1, 8);
    //transaction_id
    crypto.randomBytes(4).copy(buffer, 12);
    infoHash(torrent).copy(buffer, 16);
    generateID().copy(buffer, 36);
    //Amount already downloaded, for now leave it at 0
    Buffer.alloc(8).copy(buffer, 56)
    //Amount left to download
    size(torrent).copy(buffer, 64);
    //Uploaded so far, for now leave it at 0
    Buffer.alloc(8).copy(buffer, 72);
    //event, 0 is none
    buffer.writeUint32BE(0, 80);
    //ip, 0 is default
    buffer.writeUint32BE(0, 84);
    //key, random
    crypto.randomBytes(4).copy(buffer, 88);
    //num_want, -1 is default
    buffer.writeInt32BE(-1, 92);
    //port, between 6681 and 6685
    buffer.writeUint16BE(6685, 96);


    return buffer;
}

/*
Connect response format is the following:

Offset      Size            Name            Value
0           32-bit integer  action          1 // announce
4           32-bit integer  transaction_id
8           32-bit integer  interval
12          32-bit integer  leechers
16          32-bit integer  seeders
20 + 6 * n  32-bit integer  IP address
24 + 6 * n  16-bit integer  TCP port
20 + 6 * N

*/
function parseAnnounceRes(res) {
    function extract(buffer, step) {
        let group = [];
        for (let  i=0; i<buffer.length;i+=step) {
            group.push({
                ip: buffer.subarray(i, i+4),
                port: buffer.subarray(i+4, i+step)
            });
        }
        return group;
    }

    return {
        action: res.readUInt32BE(0),
        transaction_id: res.readUInt32BE(4),
        interval: res.readUInt32BE(8),
        leechers: res.readUInt32BE(12),
        seeders: res.readUInt32BE(16),
        peers: extract(res.subarray(20), 6)
    }
}