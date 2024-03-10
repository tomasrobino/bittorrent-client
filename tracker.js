"use strict"

import dgram from "dgram";
import crypto from "crypto";


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