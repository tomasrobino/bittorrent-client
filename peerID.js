"use strict"

import crypto from "crypto";


//It is only generated once
let id = null;
export function generateID() {
    if (!id) {
        //Generates 20 random bytes
        id = crypto.randomBytes(20);
        //Overwrites some, leaving the rest random
        Buffer.from("-ZZ0100-").copy(id, 0);
    }
    return id;
}