/*
 * File: tokenscanner.js
 * ---------------------
 * This file implements the TokenScanner in JavaScript.
 */

/* Standard header for requirejs */

define([ "jslib",
         "java/lang" ],

function(jslib,
         java_lang) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var Integer = jslib.Integer;
var Double = jslib.Double;
var Character = jslib.Character;
var ElementList = jslib.ElementList;
var RuntimeException = jslib.RuntimeException;
var Class = java_lang.Class;
var RuntimeException = java_lang.RuntimeException;

/*
 * Class: TokenScanner
 * -------------------
 * This class is used to divide a string into individual logical units
 * called <i>tokens</i>.  By default, the <code>TokenScanner</code>
 * recognizes two kinds of tokens, as follows:
 *
 * 1. Strings of consecutive letters and digits representing words
 * 2. One-character strings representing punctuation or separators
 *
 * The use of the <code>TokenScanner</code> class is illustrated by
 * the following pattern, which reads the tokens in the string variable
 * <code>input</code>:
 *
 *<pre>
 *    var scanner = new TokenScanner();
 *    scanner.setInput(input);
 *    while (scanner.hasMoreTokens()) {
 *       var token = scanner.nextToken();
 *       . . . process the token . . .
 *    }
 *</pre>
 *
 * The <code>TokenScanner</code> class exports several additional methods
 * that give clients more control over its behavior.  Those methods are
 * described individually in the documentation.
 */

/*
 * Constructor: TokenScanner
 * Usage: scanner = new TokenScanner(str);
 * ---------------------------------------
 * Creates a <code>TokenScanner</code> object that takes its input from
 * the string <code>str</code>.
 */

var TokenScanner = function(str) {
    if (!(this instanceof TokenScanner)) return new TokenScanner(str);
    this.ignoreWhitespaceFlag = false;
    this.ignoreCommentsFlag = false;
    this.scanNumbersFlag = false;
    this.scanStringsFlag = false;
    this.savedCharacters = [ ];
    this.savedTokens = [ ];
    this.operators = { };
    this.wordChars = "";
    if (!str) str = "";
    this.buffer = str;
    this.length = str.length;
    this.cp = 0;
    this.start = -1;
    this.savedCharacters = [ ];
    this.savedTokens = [ ];
};

TokenScanner.prototype.$class = new Class("TokenScanner", TokenScanner);

/* Constants */

TokenScanner.EOF = -1;
TokenScanner.SEPARATOR = 0;
TokenScanner.WORD = 1;
TokenScanner.NUMBER = 2;
TokenScanner.STRING = 3;
TokenScanner.OPERATOR = 4;

TokenScanner.MAX_TO_STRING_LENGTH = 20;

/* Import library methods */

TokenScanner.prototype.isdigit = jslib.isdigit;
TokenScanner.prototype.isalpha = jslib.isalpha;
TokenScanner.prototype.isalnum = jslib.isalnum;
TokenScanner.prototype.isspace = jslib.isspace;
TokenScanner.prototype.isxdigit = jslib.isxdigit;
TokenScanner.prototype.startsWith = jslib.startsWith;

/*
 * Method: setInput
 * Usage: scanner.setInput(str);
 * -----------------------------
 * Sets the token stream for this scanner to the specified string.
 * Any previous token stream is discarded.
 */

TokenScanner.prototype.setInput = function(str) {
    this.buffer = str;
    this.length = str.length;
    this.cp = 0;
    this.start = -1;
    this.savedCharacters = [ ];
    this.savedTokens = [ ];
};

/*
 * Method: hasMoreTokens
 * Usage: if (scanner.hasMoreTokens()) . . .
 * -----------------------------------------
 * Returns <code>true</code> if there are additional tokens for this
 * scanner to read.
 */

TokenScanner.prototype.hasMoreTokens = function() {
    var token = this.nextToken();
    this.saveToken(token);
    return token !== "";
};

/*
 * Method: nextToken
 * Usage: token = scanner.nextToken();
 * -----------------------------------
 * Returns the next token from this scanner.  If <code>nextToken</code>
 * is called when no tokens are available, it returns the empty string.
 */

TokenScanner.prototype.nextToken = function() {
    if (this.savedTokens.length !== 0) {
        return this.savedTokens.pop();
    }
    while (true) {
        if (this.ignoreWhitespaceFlag) this.skipSpaces();
        this.start = this.cp;
        var ch = this.getChar();
        if (ch === "") return "";
        if (ch === '/' && this.ignoreCommentsFlag) {
            ch = this.getChar();
            if (ch === '/') {
                while (true) {
                    ch = this.getChar();
                    if (ch === '\n' || ch === '\r' || ch === "") break;
                }
                continue;
            } else if (ch === '*') {
                var prev = "";
                while (true) {
                    ch = this.getChar();
                    if (ch === "" || (prev === '*' && ch === '/')) break;
                    prev = ch;
                }
                continue;
            }
            this.saveChar(ch);
            ch = '/';
        }
        if ((ch === '"' || ch === '\'') && this.scanStringsFlag) {
            this.saveChar(ch);
            return this.scanString();
        }
        if (this.isdigit(ch) && this.scanNumbersFlag) {
            this.saveChar(ch);
            return this.scanNumber();
        }
        if (this.isWordCharacter(ch)) {
            this.saveChar(ch);
            return this.scanWord();
        }
        var op = ch;
        while (this.isOperatorPrefix(op)) {
            ch = this.getChar();
            if (ch === "") break;
            op += ch;
        }
        while (op.length > 1 && !this.isOperator(op)) {
            this.saveChar(op[op.length - 1]);
            op = op.substring(0, op.length - 1);
        }
        return op;
    }
};

/*
 * Method: saveToken
 * Usage: scanner.saveToken(token);
 * --------------------------------
 * Adds the specified token to this scanner's input stream so that
 * when <code>nextToken</code> is called, the scanner will return the
 * saved token without reading any additional characters from the
 * token stream.
 */

TokenScanner.prototype.saveToken = function(token) {
    this.savedTokens.push(token);
    this.start = -1;
};

/*
 * Method: verifyToken
 * Usage: scanner.verifyToken(expected);
 * -------------------------------------
 * Reads the next token and makes sure it matches the string
 * <code>expected</code>.  If it does not, <code>verifyToken</code>
 * throws an error.
 */

TokenScanner.prototype.verifyToken = function(expected) {
    var token = this.nextToken();
    if (token !== expected) {
        var msg = "";
        if (token === "") {
            msg = "Missing " + expected;
        } else {
            msg = "Found " + token + " when expecting " + expected;
        }
        throw new RuntimeException(msg);
    }
};

/*
 * Method: ignoreWhitespace()
 * Usage: scanner.ignoreWhitespace();
 * ----------------------------------
 * Tells the scanner to ignore whitespace character.  By default,
 * the <code>nextToken</code> method treats whitespace characters
 * (typically spaces and tabs) just like any other punctuation mark
 * and returns them as single-character tokens.
 * Calling 
 *
 *<pre>
 *    scanner.ignoreWhitespace();
 *</pre>
 *
 * changes this behavior so that the scanner ignore whitespace characters.
 */

TokenScanner.prototype.ignoreWhitespace = function() {
    this.ignoreWhitespaceFlag = true;
};

/*
 * Method: ignoreComments
 * Usage: scanner.ignoreComments(flag);
 * ------------------------------------
 * Tells the scanner to ignore comments.  The scanner package recognizes
 * both the slash-star and slash-slash comment format from the C-based
 * family of languages.  Calling
 *
 *<pre>
 *    scanner.ignoreComments();
 *</pre>
 *
 * sets the parser to ignore comments.
 */

TokenScanner.prototype.ignoreComments = function() {
    this.ignoreCommentsFlag = true;
};

/*
 * Method: scanNumbers
 * Usage: scanner.scanNumbers();
 * -----------------------------
 * Controls how the scanner treats tokens that begin with a digit.  By
 * default, the <code>nextToken</code> method treats numbers and letters
 * identically and therefore does not provide any special processing for
 * numbers.  Calling
 *
 *<pre>
 *    scanner.scanNumbers();
 *</pre>
 *
 * changes this behavior so that <code>nextToken</code> returns the
 * longest substring that can be interpreted as a real number.
 */

TokenScanner.prototype.scanNumbers = function() {
    this.scanNumbersFlag = true;
};

/*
 * Method: scanStrings
 * Usage: scanner.scanStrings();
 * -----------------------------
 * Controls how the scanner treats tokens enclosed in quotation marks.  By
 * default, quotation marks (either single or double) are treated just like
 * any other punctuation character.  Calling
 *
 *<pre>
 *    scanner.scanStrings();
 *</pre>
 *
 * changes this assumption so that <code>nextToken</code> returns a single
 * token consisting of all characters through the matching quotation mark.
 * The quotation marks are returned as part of the scanned token so that
 * clients can differentiate strings from other token types.
 */

TokenScanner.prototype.scanStrings = function() {
    this.scanStringsFlag = true;
};

/*
 * Method: addWordCharacters
 * Usage: scanner.addWordCharacters(str);
 * --------------------------------------
 * Adds the characters in <code>str</code> to the set of characters
 * that are acceptable in an identifier.  For example, calling
 * <code>addWordCharacters("_")</code> adds the underscore
 * to the set of characters that are legal in an identifier.
 */

TokenScanner.prototype.addWordCharacters = function(str) {
    this.wordChars += str;
};

/*
 * Method: addOperator
 * Usage: scanner.addOperator(op);
 * -------------------------------
 * Defines a new multicharacter operator.  Whenever you call
 * <code>nextToken</code> when the input stream contains operator
 * characters, the scanner returns the longest possible operator
 * string that can be read at that point.
 */

TokenScanner.prototype.addOperator = function(op) {
    this.operators[op] = true;
};

/*
 * Method: setTokenStart
 * Usage: scanner.setTokenStart(start);
 * ------------------------------------
 * Sets the starting position of the current token.  This method is
 * required only for subclasses that reimplement <code>nextToken</code>.
 */

TokenScanner.prototype.setTokenStart = function(start) {
    this.start = start;
}

/*
 * Method: getTokenStart
 * Usage: start = scanner.getTokenStart();
 * ---------------------------------------
 * Returns the starting position of the last token read.  Calling
 * <code>saveToken</code> invalidates this value.
 */

TokenScanner.prototype.getTokenStart = function() {
    return this.start;
}

/*
 * Method: getTokenEnd
 * Usage: end = scanner.getTokenEnd();
 * -----------------------------------
 * Returns the ending position of the last token read.
 */

TokenScanner.prototype.getTokenEnd = function() {
    return this.cp;
}

/*
 * Method: getPosition
 * Usage: pos = scanner.getPosition();
 * -----------------------------------
 * Returns the current position of the scanner in the input stream.
 * If <code>saveToken</code> has been called, this position corresponds
 * to the beginning of the saved token.  If <code>saveToken</code> is
 * called more than once, the position is unavailable.
 */

TokenScanner.prototype.getPosition = function() {
    switch (this.savedTokens.length) {
      case 0: return this.cp;
      case 1: return this.cp - this.savedTokens[0].length;
    }
    throw new RuntimeException("Internal error: getPosition after two saves");
};

/*
 * Method: isValidIdentifier
 * Usage: if (scanner.isValidIdentifier(token)) . . .
 * --------------------------------------------------
 * Returns <code>true</code> if the token is a valid identifier.
 */

TokenScanner.prototype.isValidIdentifier = function(token) {
    if (token.length === 0) return false;
    var ch = token.charAt(0);
    if (!this.isWordCharacter(ch) || this.isdigit(ch)) return false;
    for (var i = 1; i < token.length; i++) {
        if (!this.isWordCharacter(token.charAt(i))) return false;
    }
    return true;
};

/*
 * Method: isWordCharacter
 * Usage: if (scanner.isWordCharacter(ch)) . . .
 * ---------------------------------------------
 * Returns <code>true</code> if the character is valid in a word.
 */

TokenScanner.prototype.isWordCharacter = function(ch) {
    return this.isalnum(ch) || this.wordChars.indexOf(ch) !== -1;
};

/*
 * Method: getTokenType
 * Usage: type = scanner.getTokenType(token);
 * ------------------------------------------
 * Returns the type of this token, which is one of the following
 * enumerated type constants:
 *
 *<pre>
 *    TokenScanner.EOF
 *    TokenScanner.SEPARATOR
 *    TokenScanner.WORD
 *    TokenScanner.NUMBER
 *    TokenScanner.STRING
 *    TokenScanner.OPERATOR
 *</pre> 
 */

TokenScanner.prototype.getTokenType = function(token) {
    if (token === "") return TokenScanner.EOF;
    var ch = token.charAt(0);
    if (this.isspace(ch)) return TokenScanner.SEPARATOR;
    if (ch === '"' || ch === "'") return TokenScanner.STRING;
    if (this.isdigit(ch)) return TokenScanner.NUMBER;
    if (this.isWordCharacter(ch)) return TokenScanner.WORD;
    return TokenScanner.OPERATOR;
};

/*
 * Method: toString
 * Usage: str = scanner.toString();
 * --------------------------------
 * Returns a printable representation of this scanner.
 */

TokenScanner.prototype.toString = function() {
    var str = typeof(this);
    if (this.buffer.length < TokenScanner.MAX_TO_STRING_LENGTH) {
        str += "(\"" + this.buffer + "\")";
    } else {
        str += "(" + this.buffer.length + " chars)";
    }
    return str;
};

/*
 * Function: getStringValue
 * Usage: str = scanner.getStringValue(token);
 * -------------------------------------------
 * Returns the actual string value corresponding to a string token.
 */

TokenScanner.prototype.getStringValue = function(token) {
    return eval(token);
};

/*
 * Function: getNumberValue
 * Usage: n = TokenScanner.getNumberValue(token);
 * ----------------------------------------------
 * Returns the numeric value corresponding to a number token.
 */

TokenScanner.getNumberValue = function(token) {
    return eval(token);
};

/* Private methods */

TokenScanner.prototype.getChar = function() {
    if (this.savedCharacters.length === 0) {
        return (this.cp >= this.length) ? "" : this.buffer.charAt(this.cp++);
    } else {
        this.cp++;
        return this.savedCharacters.pop();
    }
};

TokenScanner.prototype.saveChar = function(ch) {
    this.cp--;
    this.savedCharacters.push(ch);
};

TokenScanner.prototype.skipSpaces = function() {
    while (true) {
        var ch = this.getChar();
        if (ch === "") return;
        if (!this.isspace(ch) || this.operators[ch] === true) {
            this.saveChar(ch);
            return;
        }
    }
};

TokenScanner.prototype.scanWord = function() {
    var token = "";
    while (true) {
        var ch = this.getChar();
        if (ch === "") break;
        if (!this.isWordCharacter(ch)) {
            this.saveChar(ch);
            break;
        }
        token += ch;
    }
    return token;
};

TokenScanner.INITIAL_STATE = 0;
TokenScanner.BEFORE_DECIMAL_POINT = 1;
TokenScanner.AFTER_DECIMAL_POINT = 2;
TokenScanner.STARTING_EXPONENT = 3;
TokenScanner.FOUND_EXPONENT_SIGN = 4;
TokenScanner.SCANNING_EXPONENT = 5;
TokenScanner.LEADING_ZERO = 6;
TokenScanner.SCANNING_HEX = 7;
TokenScanner.FINAL_STATE = 8;

TokenScanner.prototype.scanNumber = function() {
    var token = "";
    var state = TokenScanner.INITIAL_STATE;
    while (state !== TokenScanner.FINAL_STATE) {
        var ch = this.getChar();
        var xch = 'e';
        switch (state) {
          case TokenScanner.INITIAL_STATE:
            if (ch === '0') {
                state = TokenScanner.LEADING_ZERO;
            } else {
                state = TokenScanner.BEFORE_DECIMAL_POINT;
            }
            break;
          case TokenScanner.BEFORE_DECIMAL_POINT:
            if (ch === '.') {
                state = TokenScanner.AFTER_DECIMAL_POINT;
            } else if (ch === 'E' || ch === 'e') {
                state = TokenScanner.STARTING_EXPONENT;
                xch = ch;
            } else if (!this.isdigit(ch)) {
                this.saveChar(ch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          case TokenScanner.AFTER_DECIMAL_POINT:
            if (ch === 'E' || ch === 'e') {
                state = TokenScanner.STARTING_EXPONENT;
                xch = ch;
            } else if (!this.isdigit(ch)) {
                this.saveChar(ch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          case TokenScanner.STARTING_EXPONENT:
            if (ch === '+' || ch === '-') {
                state = TokenScanner.FOUND_EXPONENT_SIGN;
            } else if (this.isdigit(ch)) {
                state = TokenScanner.SCANNING_EXPONENT;
            } else {
                this.saveChar(ch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          case TokenScanner.FOUND_EXPONENT_SIGN:
            if (this.isdigit(ch)) {
                state = TokenScanner.SCANNING_EXPONENT;
            } else {
                this.saveChar(ch);
                this.saveChar(xch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          case TokenScanner.SCANNING_EXPONENT:
            if (!this.isdigit(ch)) {
                this.saveChar(ch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          case TokenScanner.LEADING_ZERO:
            if (ch === 'x' || ch === 'X') {
                state = TokenScanner.SCANNING_HEX;
            } else if (ch === '.') {
                state = TokenScanner.AFTER_DECIMAL_POINT;
            } else if (ch === 'E' || ch === 'e') {
                state = TokenScanner.STARTING_EXPONENT;
                xch = ch;
            } else if (!this.isdigit(ch)) {
                this.saveChar(ch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          case TokenScanner.SCANNING_HEX:
            if (!this.isxdigit(ch)) {
                this.saveChar(ch);
                state = TokenScanner.FINAL_STATE;
            }
            break;
          default:
            state = TokenScanner.FINAL_STATE;
            break;
        }
        if (state !== TokenScanner.FINAL_STATE) {
            token += ch;
        }
    }
    return token;
};

TokenScanner.prototype.scanString = function() {
    var token = "";
    var delim = this.getChar();
    token += delim;
    while (true) {
        var ch = this.getChar();
        if (ch === "") throw new RuntimeException("Unterminated string");
        if (ch === delim) break;
        if (ch === '\\') {
            token += this.scanEscapeCharacter();
        } else {
            token += ch;
        }
    }
    return token + delim;
};

TokenScanner.prototype.scanEscapeCharacter = function() {
    var str = "\\";
    var ch = this.getChar();
    str += ch;
    if (this.isdigit(ch) || ch === 'x' || ch === 'u') {
        var hex = !this.isdigit(ch);
        while (true) {
            ch = this.getChar();
            if ((hex) ? !this.isxdigit(ch) : !this.isdigit(ch)) break;
            str += ch;
        }
        this.saveChar(ch);
    }
    return str;
};

TokenScanner.prototype.isOperator = function(op) {
    return this.operators[op] === true;
};

TokenScanner.prototype.isOperatorPrefix = function(op) {
    for (var str in this.operators) {
        if (this.startsWith(str, op)) return true;
    }
    return false;
};

/* Exports */

return {
    TokenScanner : TokenScanner
};

});
