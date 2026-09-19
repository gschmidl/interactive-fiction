/*
 * File: jslib.js
 * --------------
 * This module defines several top-level functions that are used widely
 * in the Stanford javascript development library.  To use them in an
 * application, add "jslib" as a requirejs dependency and then call the
 * functions as top-level entries in jslib, as in
 *
 *    jslib.startsWith(str, prefix)
 */

define(function() {

var isalpha = function(ch) {
    return /^[A-Za-z]$/.test(toStr(ch));
};

var isdigit = function(ch) {
    return /^[0-9]$/.test(toStr(ch));
};

var isalnum = function(ch) {
    return /^[A-Za-z0-9]$/.test(toStr(ch));
};

var islower = function(ch) {
    return /^[a-z]$/.test(toStr(ch));
};

var isupper = function(ch) {
    return /^[A-Z]$/.test(toStr(ch));
};

var isspace = function(ch) {
    return /^\s$/.test(toStr(ch));
};

var toupper = function(ch) {
    return toInt(toStr(ch).toUpperCase());
}

var tolower = function(ch) {
    return toInt(toStr(ch).toLowerCase());
}

var isxdigit = function(ch) {
    if (typeof(ch) === "number") ch = String.fromCharCode(ch);
    return /^[A-Fa-f0-9]$/.test(ch);
};

/* String functions */

var startsWith = function(str, prefix) {
    if (str.length < prefix.length) return false;
    return str.substring(0, prefix.length) === prefix;
};

var endsWith = function(str, suffix) {
    if (str.length < suffix.length) return false;
    return str.substring(str.length - suffix.length) === suffix;
};

var trim = function(str) {
    return str.replace(/^\s\s*/, "").replace(/\s\s*$/, "");
};

var equals = function(s1, s2) {
    return s1 === s2;
}

var equalsIgnoreCase = function(s1, s2) {
    return s1.toUpperCase() === s2.toUpperCase();
}

var quoteHTML = function(str) {
    var result = "";
    for (var i = 0; i < str.length; i++) {
        var ch = str.charAt(i);
        switch (ch) {
          case ' ': ch = "&nbsp;"; break;
          case '<': ch = "&lt;"; break;
          case '>': ch = "&gt;"; break;
          case '&': ch = "&amp;"; break;
          case '\n':
            if (i === 0 || str.charAt(i - 1) !== '\r') {
                ch = "<br>";
            }
            break;
          case '\r':
            if (i === 0 || str.charAt(i - 1) !== '\n') {
                ch = "<br>";
            }
            break;
        }
        result += ch;
    }
    return result;
};

var toStr = function(arg) {
    if ((typeof arg) === "string") return arg;
    if (arg < 0) return "";
    return String.fromCharCode(arg);
};

var toInt = function(arg) {
    if ((typeof arg) === "string") {
        return (arg && arg.length) ? arg.charCodeAt(0) : 0;
    } else {
        return truncate(arg);
    }
};

var digit = function(ch, base) {
    if (typeof(ch) === "number") ch = String.fromCharCode(ch);
    ch = ch.toUpperCase();
    var alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    var k = alphabet.substr(0, base).indexOf(ch);
    if (k === -1) throw new Error("digit: Illegal digit " + ch);
    return k;
};

/* Arithmetic functions */

var truncate = function(value) {
    return (value < 0) ? Math.ceil(value) : Math.floor(value);
};

var round = function(value) {
    return Math.round(value);
};

/*
 * Function: newArray
 * Usage: jslib.newArray(n);
 *        jslib.newArray(n, m);
 * ----------------------------
 * Creates a new array of size n or nxm, as appropriate.  All elements are
 * initialized to 0.
 */

var newArray = function(n, m) {
    var array = [];
    if (m === undefined) {
        for (var i = 0; i < n; i++) {
            array[i] = 0;
        }
    } else {
        for (var i = 0; i < n; i++) {
            array[i] = [];
            for (var j = 0; j < n; j++) {
                array[i][j] = 0;
            }
        }
    }
    return array;
};

/*
 * Function: inheritPrototype
 * Usage: class.prototype = jslib.inheritPrototype(superclass, str);
 * -----------------------------------------------------------------
 * Simulates inheritance using the Javascript prototype chain.  The
 * string str is used to print error messages about the inheritance
 * relationship.
 */

var inheritPrototype = function(superclass, str) {
    var dummy = function() { };
    if (typeof(superclass) !== "function") {
        alert("Missing superclass: " + str);
    }
    dummy.prototype = superclass.prototype;
    return new dummy();
};

/*
 * Function: toString
 * Usage: str = jslib.toString(obj);
 * ---------------------------------
 * Converts obj to a string.
 */

var toString = function(obj) {
    var str = "{";
    for (var key in obj) {
        if (str.length > 1) str += ", ";
        str += key + ":" + obj[key];
    }
    return str + "}";
};

/*
 * Function: clone
 * Usage: obj = jslib.clone(obj);
 * ---------------------------------
 * Creates a shallow copy of obj.
 */

var clone = function(obj) {
    result = { };
    for (var key in obj) {
        result[key] = obj[key];
    }
    return result;
};

/*
 * Function: addListener
 * Usage: jslib.addListener(element, trigger, fn);
 * -----------------------------------------------
 * Sets fn as the event listener for the specified element and trigger
 * event.  The trigger argument indicates the event name, without the
 * "on" prefix required on older browsers.
 */

var addListener = function(element, trigger, fn) {
    if (!element) alert(arguments.callee.caller.toString());
    if (element.addEventListener) {
        element.addEventListener(trigger, fn, false);
    } else if (element.attachEvent) {
        element.attachEvent("on" + trigger, fn);
    } else {
        element["on" + trigger] = fn;
    }
};

/*
 * Function: getEventTarget
 * Usage: target = jslib.getEventTarget(e);
 * ----------------------------------------
 * Returns the target of the event.
 */

var getEventTarget = function(e) {
    if (!e) e = window.event;
    var target = e.target || e.srcElement;
    if (target.nodeType === 3) target = target.parentNode;
    return target;
};

/*
 * Function: getLocalCoordinates
 * Usage: pt = jslib.getLocalCoordinates(spt, element);
 * ----------------------------------------------------
 * Converts the point <code>spt</code> (which can also be an event) to
 * local coordinates relative to the specified element.
 */

var getLocalCoordinates = function(spt, element) {
    var px = 0;
    var py = 0;
    if (spt.pageX) {
        px = spt.pageX;
        py = spt.pageY;
    } else if (spt.clientX) {
        px = spt.clientX + document.body.scrollLeft +
                           document.documentElement.scrollLeft;
        py = spt.clientY + document.body.scrollTop +
                           document.documentElement.scrollTop;
    } else if (spt.x) {
        px = spt.x;
        py = spt.y;
    }
    var cx = 0;
    var cy = 0;
    while (element) {
        cx += element.offsetLeft;
        cy += element.offsetTop;
        element = element.offsetParent;
    }
    return { x : px - cx, y : py - cy };
};

/*
 * Function: getScreenCoordinates
 * Usage: spt = jslib.getScreenCoordinates(pt, element);
 * -----------------------------------------------------
 * Returns the screen coordinates of the specified point.
 */

var getScreenCoordinates = function(pt, element) {
    var cx = pt.x;
    var cy = pt.y;
    while (element) {
        cx += element.offsetLeft;
        cy += element.offsetTop;
        element = element.offsetParent;
    }
    return { x : cx, y : cy };
};

/*
 * Function: getCSSStyle
 * Usage: style = jslib.getCSSStyle(key);
 * --------------------------------------
 * Returns the CSS style for the specified key.
 */

var getCSSStyle = function(key) {
    var styleSheets = document.styleSheets;
    for (var i = 0; i < styleSheets.length; i++) {
        var rules = document.styleSheets[i].rules
                 || document.styleSheets[i].cssRules;
        for (var j = 0; j < rules.length; j++) {
            if (rules[j].selectorText === key) {
                if (rules[j].cssText) return rules[j].cssText;
                return rules[j].style.cssText;
            }
        }
    }
};

/*
 * Function: parseFont
 * Usage: font = jslib.parseFont(str);
 * -----------------------------------
 * Parses a string into an object whose fields specify the font.
 */

var parseFont = function(str) {
    if (str.family) return str;
    var parts = str.split("-");
    var family = parts[0];
    var size = 12;
    var style = "";
    switch (parts.length) {
      case 1:
        break;
      case 2:
        if (this.isalpha(parts[1].charAt(0))) {
          style = parts[1].toLowerCase();
        } else {
          size = parseInt(parts[1]);
        }
        break;
      case 3:
        style = parts[1].toLowerCase();
        size = parseInt(parts[2]);
        break;
    }
    return {
        family: family,
        size: size,
        isBold: style.indexOf("bold") !== -1,
        isItalic: style.indexOf("italic") !== -1,
    };
};

/*
 * Implementation notes: printStackTrace
 * -------------------------------------
 * This implementation is adapted from Eric Wendelin's public-domain source
 * at http://www.eriwen.com/javascript/js-stack-trace/.
 */

var printStackTrace = function(ex) {
    var callstack = [];
    var isCallstackPopulated = false;
    if (!ex) {
        try {
            this.fail();
        } catch (e) {
            ex = e;
        }
    }
    if (ex.stack) { /* Firefox */
        var lines = ex.stack.split('\n');
        var len = lines.length;
        for (var i = 0; i < len; i++) {
            if (lines[i].match(/^\s*[A-Za-z0-9\-_\$]+\(/)) {
                callstack.push(lines[i]);
            }
        }
        callstack.shift();
        isCallstackPopulated = true;
    } else if (window.opera && ex.message) { /* Opera */
        var lines = ex.message.split('\n');
        var len = lines.length;
        for (var i = 0; i < len; i++) {
            if (lines[i].match(/^\s*[A-Za-z0-9\-_\$]+\(/)) {
                var entry = lines[i];
                /* Append next line also since it has the file info */
                if (lines[i + 1]) {
                    entry += ' at ' + lines[i + 1];
                    i++;
                }
                callstack.push(entry);
            }
        }
        callstack.shift();
        isCallstackPopulated = true;
    }
    if (!isCallstackPopulated) { /* IE and Safari */
        var currentFunction = arguments.callee.caller;
        var lastFunction = null;
        while (currentFunction && currentFunction !== lastFunction) {
            var fn = currentFunction.toString();
            var fname = fn.substring(fn.indexOf("function") + 8,
                                     fn.indexOf(" ")) || 'anonymous';
            callstack.push(fname);
            lastFunction = currentFunction;
            currentFunction = currentFunction.caller;
        }
    }
    alert(callstack.join("\n"));
};

return {
    startsWith : startsWith,
    endsWith : endsWith,
    trim : trim,
    equals : equals,
    equalsIgnoreCase : equalsIgnoreCase,
    isdigit : isdigit,
    isalnum : isalnum,
    isxdigit : isxdigit,
    isalpha : isalpha,
    islower : islower,
    isupper : isupper,
    isspace : isspace,
    toupper : toupper,
    tolower : tolower,
    quoteHTML : quoteHTML,
    toStr : toStr,
    toInt : toInt,
    digit : digit,
    truncate : truncate,
    round : round,
    newArray : newArray,
    inheritPrototype : inheritPrototype,
    toString : toString,
    clone : clone,
    addListener : addListener,
    getEventTarget : getEventTarget,
    getLocalCoordinates : getLocalCoordinates,
    getScreenCoordinates : getScreenCoordinates,
    getCSSStyle : getCSSStyle,
    parseFont : parseFont,
    printStackTrace : printStackTrace
};

});
