"use strict"

import { getPeers } from "./tracker.js";
import { open } from "./torrentParser.js"

const torrent = open("example.torrent");
getPeers(torrent);