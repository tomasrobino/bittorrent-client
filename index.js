"use strict"

import fs from "fs";
import bencode from "bencode";
import dgram from "dgram";

const torrent = bencode.decode(fs.readFileSync("example.torrent"));
const decoder = new TextDecoder();
//console.log(decoder.decode(torrent.announce));
const torrentUrl = new URL(decoder.decode(torrent.announce));
if (torrentUrl.port === "") torrentUrl.port = "161";
const socket = dgram.createSocket("udp4");
console.log(torrentUrl)

const msg = Buffer.from("hi!");
socket.send(msg, 0, msg.length, torrentUrl.port, torrentUrl.host, () => {});

socket.on("message", msg => {
    console.log(msg);
});