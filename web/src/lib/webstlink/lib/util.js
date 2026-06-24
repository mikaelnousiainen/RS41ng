/* util.js
 * Common helper functions
 *
 * Copyright Devan Lai 2017
 *
 */
function hex_octet(b) {
    return b.toString(16).padStart(2, "0");
}

function hex_halfword(hw) {
    return hw.toString(16).padStart(4, "0");
}

function hex_word(w) {
    return w.toString(16).padStart(8, "0");
}

function hex_string(value, length) {
    return value.toString(16).padStart(length, "0");
}

function hex_octet_array(arr) {
    return Array.from(arr, hex_octet);
}

function async_sleep(seconds) {
    return new Promise(function(resolve, reject) {
        setTimeout(resolve, seconds * 1000);
    });
}

function async_timeout(seconds) {
    return new Promise(function(resolve, reject) {
        setTimeout(reject, seconds * 1000);
    });
}

export {
    hex_octet,
    hex_halfword,
    hex_word,
    hex_string,
    hex_octet_array,
    async_sleep,
    async_timeout,
};
