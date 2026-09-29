/*
 * File: utf8.js
 * -------------
 * This file implements the JavaScript side of the UTF8 class.
 */

define([ "jslib",
         "java/lang" ],

function(jslib,
         java_lang) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var Class = java_lang.Class;
var RuntimeException = java_lang.RuntimeException;

/* UTF8.js */

var UTF8 = function() {
    /* Empty */
};

UTF8.encode = function(str) {
    var array = [ 0 ];
    str = unescape(encodeURIComponent(str));
    var n = str.length;
    var shift = 24;
    var offset = 0;
    for (var i = 0; i < n; i++) {
        array[offset] |= str.charCodeAt(i) << shift;
        shift -= 8;
        if (shift < 0) {
            shift = 24;
            offset++;
            array.push(0);
        }
    }
    return array;
};

UTF8.decode = function(array, origin) {
    var offset = origin || 0;
    var n = UTF8.getByteLength(array, offset);
    var str = "";
    var shift = 24;
    for (var i = 0; i < n; i++) {
        str += toStr((array[offset] >> shift) & 0xFF);
        shift -= 8;
        if (shift < 0) {
            shift = 24;
            offset++;
        }
    }
    return decodeURIComponent(escape(str));
};

UTF8.getByteLength = function(array, offset) {
    var shift = 24;
    for (var i = 0; true ; i++) {
        if (((array[offset] >> shift) & 0xFF) === 0) return i;
        shift -= 8;
        if (shift < 0) {
            shift = 24;
            offset++;
        }
    }
};

/* Exports */

return {
    UTF8 : UTF8
};

});
