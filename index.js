"use strict"

import fs from "fs";
import bencode from "bencode";
import { getPeers } from "./tracker.js";

const torrent = bencode.decode(fs.readFileSync("example.torrent"));
getPeers(torrent);