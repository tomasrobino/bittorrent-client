"use strict"

import fs from "fs";
import bencode from "bencode";

const torrent = bencode.decode(fs.readFileSync("example.torrent"));
const decoder = new TextDecoder();
//console.log(decoder.decode(torrent.announce));
const torrentUrl = new URL(decoder.decode(torrent.announce));