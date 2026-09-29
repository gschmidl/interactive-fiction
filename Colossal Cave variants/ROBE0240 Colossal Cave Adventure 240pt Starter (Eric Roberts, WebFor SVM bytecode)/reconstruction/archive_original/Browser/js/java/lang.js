/*
 * File: lang.js
 * -------------
 * This module defines replacements for the classes in the java.lang
 * package.
 */

define([ "jslib" ],

function(jslib) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;

/* Class.js (must be first) */

var Class = function(name, c) {
    this.name = name;
    this.proto = c.prototype;
    Class.classes[name] = this;
};

Class.classes = { };
Class.prototype.$class = new Class("Class", Class);

Class.forName = function(name) {
    name = name.substring(name.lastIndexOf(".") + 1);
    var c = Class.classes[name];
    if (!c) throw new RuntimeException("No class named " + name);
    return c;
};

Class.prototype.getName = function() {
    return this.name;
};

Class.prototype.toString = function() {
    return this.name;
};

Class.prototype.getMethod = function(name, types) {
    for (var p = this.proto; p; p = p.__proto__) {
        if (p[name] && typeof(p[name]) === "function") {
            return new Method(name, p[name]);
        }
    }
    throw new RuntimeException("No method named " + name);
};

/* Method.js */

var Method = function(name, fn) {
    this.name = name;
    this.fn = fn;
};

Method.prototype.$class = new Class("Method", Method);

Method.prototype.getName = function() {
    return this.name;
};

Method.prototype.invoke = function(target, args) {
    return this.fn.apply(target, args);
};

Method.prototype.toString = function() {
    return this.name + "()";
};

/* Integer.js */

var Integer = function() {
    throw new RuntimeException("Can't call Integer constructor");
};

Integer.prototype.$class = new Class("Integer", Integer);

Integer.parseInt = function(str, radix) {
    var start = 0;
    for (var i = 0; i < str.length; i++) {
        var ch = str.charAt(i);
        if (ch === '+' && i === 0) {
            start = 1;
        } else if (Character.digit(ch) === -1 && (ch !== '-' || i > 0)) {
            throw new RuntimeException('"' + str + '" is not a legal integer');
        }
    }
    return parseInt(str.substring(start), radix);
};

Integer.toString = function(number, radix) {
    return number.toString(radix);
};

Integer.toHexString = function(number) {
    if (number < 0) number += 0x100000000;
    return number.toString(16);
};

/* Double.js */

var Double = function(n) {
    throw new RuntimeException("Can't call Double constructor");
};

Double.prototype.$class = new Class("Double", Double);

Double.parseDouble = function(str) {
    if (str.length > 0 && str.charAt(0) === '+') str = str.substring(1);
    var result = Number(str);
    if (isNaN(result)) {
        throw new RuntimeException('"' + str + '" is not a legal number');
    }
    return result;
};

Double.isNaN = function(num) {
    return isNaN(result)
};

Double.isInfinite = function(num) {
    return !isNaN(result) && !isFinite(result);
};

Double.toString = function(number) {
    return number.toString();
};

Double.NaN = NaN;
Double.POSITIVE_INFINITY = Infinity;
Double.NEGATIVE_INFINITY = -Infinity;

/* Character.js */

/*
 * Implementation notes
 * --------------------
 * These static methods simulate a few of the more important
 * functions in Java's Character class.  For convenience and
 * brevity, these facilities are also defined as top-level
 * functions using their names from the C++ <cctype> library.
 */

var Character = function() {
    throw new RuntimeException("Can't call Character constructor");
};

Character.prototype.$class = new Class("Character", Character);

Character.digit = function(ch, radix) {
    var result = parseInt(toStr(ch), radix);
    return isNaN(result) ? -1 : result;
};

Character.forDigit = function(digit, radix) {
    return digit.toString(radix);
};

Character.isDigit = function(ch) {
    return /^[0-9]$/.test(toStr(ch));
};

Character.isJavaIdentifierPart = function(ch) {
    return /^[A-Za-z0-9_]$/.test(toStr(ch));
};

Character.isJavaIdentifierStart = function(ch) {
    return /^[A-Za-z_]$/.test(toStr(ch));
};

Character.isLetter = function(ch) {
    return /^[A-Za-z]$/.test(toStr(ch));
};

Character.isLetterOrDigit = function(ch) {
    return /^[A-Za-z0-9]$/.test(toStr(ch));
};

Character.isLowerCase = function(ch) {
    return /^[a-z]$/.test(toStr(ch));
};

Character.isUpperCase = function(ch) {
    return /^[A-Z]$/.test(toStr(ch));
};

Character.isWhitespace = function(ch) {
    return /^\s$/.test(toStr(ch));
};

Character.toUpperCase = function(ch) {
    if (typeof(ch) === "string") return ch.toUpperCase();
    return toInt(toStr(ch).toUpperCase());
};

Character.toLowerCase = function(ch) {
    if (typeof(ch) === "string") return ch.toLowerCase();
    return toInt(toStr(ch).toLowerCase());
};

var isalnum = Character.isLetterOrDigit;
var isalpha = Character.isLetter;
var isdigit = Character.isDigit;
var islower = Character.isLowerCase;
var isspace = Character.isWhitespace;
var isupper = Character.isUpperCase;
var tolower = Character.toLowerCase;
var toupper = Character.toUpperCase;

var isxdigit = function(ch) {
    if (typeof(ch) === "number") ch = String.fromCharCode(ch);
    return /^[A-Fa-f0-9]$/.test(ch);
};

/* RuntimeException.js */

var RuntimeException = function(str) {
    if (!(this instanceof RuntimeException)) return new RuntimeException(str);
    this.message = str;
};

RuntimeException.prototype.$class = new Class("RuntimeException",
                                              RuntimeException);

RuntimeException.prototype.getMessage = function() {
    return this.message;
};

RuntimeException.prototype.toString = function() {
    return this.message;
};

RuntimeException.patchMessage = function(ex) {
    return ex.message || ex.toString();
};

/* System.js */

var System = function() {
    throw new RuntimeException("Can't call System constructor");
};
System.prototype.$class = new Class("System", System);

System.currentTimeMillis = function() {
    return new Date().getTime();
};

System.exit = function() {
    /* Ignored in JavaScript */
};

return {
    Character : Character,
    Class : Class,
    Double : Double,
    Integer : Integer,
    Method : Method,
    RuntimeException : RuntimeException,
    System : System
};

});
