"use strict"

import fs from "fs";
import bencode from "bencode";


const torrent = bencode.decode(fs.readFileSync("example.torrent"));
console.log(torrent.announce.toString("utf-8"));