"use strict"

import fs from "fs";
import bencode from "bencode";


export function open(path) {
    return bencode.decode(fs.readFileSync(path));
}