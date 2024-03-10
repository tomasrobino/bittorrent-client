"use strict"

import dgram from "dgram";


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

function buildConReq() {
    return Buffer.from("placeholder");
}