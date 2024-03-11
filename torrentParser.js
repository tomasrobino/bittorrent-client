"use strict"

import fs from "fs";
import bencode from "bencode";
import crypto from "crypto";


export function open(path) {
    return bencode.decode(fs.readFileSync(path));
}

//Turns torrent's info property into a hash, to be used by the socket
export function infoHash(torrent) {
    //Encode into bencode torrent's info property
    const info = bencode.encode(torrent.info);
    //Creates hash
    //Uses SHA1 because it's the algotrithm used by Bittorrent
    return crypto.createHash("sha1").update(info).digest();
}

export function size(torrent) {
    let size;
    if (torrent.info.files) {
        size = BigInt(torrent.info.files.map(e => e.length).reduce((acc, val) => acc+val));
    } else size = BigInt(torrent.info.length);

    const buffer = Buffer.alloc(8);
    buffer.writeBigInt64BE(size);
    return buffer;
}