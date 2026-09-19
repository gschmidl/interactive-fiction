/*
 * File: java2js.js
 * ----------------
 * This file implements the java2js package, which includes a small
 * number of classes to bridge certain differences between Java and
 * JavaScript.
 */

/* Standard header for requirejs */

define([ "jslib",
     "java/awt",
     "java/lang",
     "java/util",
     "javax/swing" ],

function(jslib,
     java_awt,
     java_lang,
     java_util,
     javax_swing) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var addListener = jslib.addListener;
var quoteHTML = jslib.quoteHTML;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var ActionEvent = java_awt.ActionEvent;
var BorderLayout = java_awt.BorderLayout;
var Color = java_awt.Color;
var Component = java_awt.Component;
var Dimension = java_awt.Dimension;
var FlowLayout = java_awt.FlowLayout;
var Font = java_awt.Font;
var FontMetrics = java_awt.FontMetrics;
var Graphics = java_awt.Graphics;
var MouseEvent = java_awt.MouseEvent;
var Point = java_awt.Point;
var Rectangle = java_awt.Rectangle;
var Class = java_lang.Class;
var RuntimeException = java_lang.RuntimeException;
var ArrayList = java_util.ArrayList;
var BorderFactory = javax_swing.BorderFactory;
var JButton = javax_swing.JButton;
var JComponent = javax_swing.JComponent;
var JLabel = javax_swing.JLabel;
var JList = javax_swing.JList;
var JPanel = javax_swing.JPanel;
var JScrollPane = javax_swing.JScrollPane;
var JScrollBar = javax_swing.JScrollBar;
var JTextField = javax_swing.JTextField;

var JSColor = function() {
    /* Empty */
};

JSColor.decode = function(name) {
    if (jslib.startsWith(name, "#")) {
        return new Color(parseInt(name.substring(1), 16));
    } else if (jslib.startsWith(name, "0x")) {
        return new Color(parseInt(name.substring(2), 16));
    } else {
        if (JSColor.jsColors === null) JSColor.initJSColors();
        var color = JSColor.jsColors[name.toLowerCase()];
        if (color === undefined) {
            throw new RuntimeException("No color named " + name);
        }
        return color;
    }
};

JSColor.initJSColors = function() {
    JSColor.jsColors = { };
    JSColor.jsColors["aliceblue"] = new Color(0xF0F8FF);
    JSColor.jsColors["antiquewhite"] = new Color(0xFAEBD7);
    JSColor.jsColors["aqua"] = new Color(0x00FFFF);
    JSColor.jsColors["aquamarine"] = new Color(0x7FFFD4);
    JSColor.jsColors["azure"] = new Color(0xF0FFFF);
    JSColor.jsColors["beige"] = new Color(0xF5F5DC);
    JSColor.jsColors["bisque"] = new Color(0xFFE4C4);
    JSColor.jsColors["black"] = new Color(0x000000);
    JSColor.jsColors["blanchedalmond"] = new Color(0xFFEBCD);
    JSColor.jsColors["blue"] = new Color(0x0000FF);
    JSColor.jsColors["blueviolet"] = new Color(0x8A2BE2);
    JSColor.jsColors["brown"] = new Color(0xA52A2A);
    JSColor.jsColors["burlywood"] = new Color(0xDEB887);
    JSColor.jsColors["cadetblue"] = new Color(0x5F9EA0);
    JSColor.jsColors["chartreuse"] = new Color(0x7FFF00);
    JSColor.jsColors["chocolate"] = new Color(0xD2691E);
    JSColor.jsColors["coral"] = new Color(0xFF7F50);
    JSColor.jsColors["cornflowerblue"] = new Color(0x6495ED);
    JSColor.jsColors["cornsilk"] = new Color(0xFFF8DC);
    JSColor.jsColors["crimson"] = new Color(0xDC143C);
    JSColor.jsColors["cyan"] = new Color(0x00FFFF);
    JSColor.jsColors["darkblue"] = new Color(0x00008B);
    JSColor.jsColors["darkcyan"] = new Color(0x008B8B);
    JSColor.jsColors["darkgoldenrod"] = new Color(0xB8860B);
    JSColor.jsColors["darkgray"] = new Color(0xA9A9A9);
    JSColor.jsColors["darkgrey"] = new Color(0xA9A9A9);
    JSColor.jsColors["darkgreen"] = new Color(0x006400);
    JSColor.jsColors["darkkhaki"] = new Color(0xBDB76B);
    JSColor.jsColors["darkmagenta"] = new Color(0x8B008B);
    JSColor.jsColors["darkolivegreen"] = new Color(0x556B2F);
    JSColor.jsColors["darkorange"] = new Color(0xFF8C00);
    JSColor.jsColors["darkorchid"] = new Color(0x9932CC);
    JSColor.jsColors["darkred"] = new Color(0x8B0000);
    JSColor.jsColors["darksalmon"] = new Color(0xE9967A);
    JSColor.jsColors["darkseagreen"] = new Color(0x8FBC8F);
    JSColor.jsColors["darkslateblue"] = new Color(0x483D8B);
    JSColor.jsColors["darkslategray"] = new Color(0x2F4F4F);
    JSColor.jsColors["darkslategrey"] = new Color(0x2F4F4F);
    JSColor.jsColors["darkturquoise"] = new Color(0x00CED1);
    JSColor.jsColors["darkviolet"] = new Color(0x9400D3);
    JSColor.jsColors["deeppink"] = new Color(0xFF1493);
    JSColor.jsColors["deepskyblue"] = new Color(0x00BFFF);
    JSColor.jsColors["dimgray"] = new Color(0x696969);
    JSColor.jsColors["dimgrey"] = new Color(0x696969);
    JSColor.jsColors["dodgerblue"] = new Color(0x1E90FF);
    JSColor.jsColors["firebrick"] = new Color(0xB22222);
    JSColor.jsColors["floralwhite"] = new Color(0xFFFAF0);
    JSColor.jsColors["forestgreen"] = new Color(0x228B22);
    JSColor.jsColors["fuchsia"] = new Color(0xFF00FF);
    JSColor.jsColors["gainsboro"] = new Color(0xDCDCDC);
    JSColor.jsColors["ghostwhite"] = new Color(0xF8F8FF);
    JSColor.jsColors["gold"] = new Color(0xFFD700);
    JSColor.jsColors["goldenrod"] = new Color(0xDAA520);
    JSColor.jsColors["gray"] = new Color(0x808080);
    JSColor.jsColors["grey"] = new Color(0x808080);
    JSColor.jsColors["green"] = new Color(0x008000);
    JSColor.jsColors["greenyellow"] = new Color(0xADFF2F);
    JSColor.jsColors["honeydew"] = new Color(0xF0FFF0);
    JSColor.jsColors["hotpink"] = new Color(0xFF69B4);
    JSColor.jsColors["indianred"] = new Color(0xCD5C5C);
    JSColor.jsColors["indigo"] = new Color(0x4B0082);
    JSColor.jsColors["ivory"] = new Color(0xFFFFF0);
    JSColor.jsColors["khaki"] = new Color(0xF0E68C);
    JSColor.jsColors["lavender"] = new Color(0xE6E6FA);
    JSColor.jsColors["lavenderblush"] = new Color(0xFFF0F5);
    JSColor.jsColors["lawngreen"] = new Color(0x7CFC00);
    JSColor.jsColors["lemonchiffon"] = new Color(0xFFFACD);
    JSColor.jsColors["lightblue"] = new Color(0xADD8E6);
    JSColor.jsColors["lightcoral"] = new Color(0xF08080);
    JSColor.jsColors["lightcyan"] = new Color(0xE0FFFF);
    JSColor.jsColors["lightgoldenrodyellow"] = new Color(0xFAFAD2);
    JSColor.jsColors["lightgray"] = new Color(0xD3D3D3);
    JSColor.jsColors["lightgrey"] = new Color(0xD3D3D3);
    JSColor.jsColors["lightgreen"] = new Color(0x90EE90);
    JSColor.jsColors["lightpink"] = new Color(0xFFB6C1);
    JSColor.jsColors["lightsalmon"] = new Color(0xFFA07A);
    JSColor.jsColors["lightseagreen"] = new Color(0x20B2AA);
    JSColor.jsColors["lightskyblue"] = new Color(0x87CEFA);
    JSColor.jsColors["lightslategray"] = new Color(0x778899);
    JSColor.jsColors["lightslategrey"] = new Color(0x778899);
    JSColor.jsColors["lightsteelblue"] = new Color(0xB0C4DE);
    JSColor.jsColors["lightyellow"] = new Color(0xFFFFE0);
    JSColor.jsColors["lime"] = new Color(0x00FF00);
    JSColor.jsColors["limegreen"] = new Color(0x32CD32);
    JSColor.jsColors["linen"] = new Color(0xFAF0E6);
    JSColor.jsColors["magenta"] = new Color(0xFF00FF);
    JSColor.jsColors["maroon"] = new Color(0x800000);
    JSColor.jsColors["mediumaquamarine"] = new Color(0x66CDAA);
    JSColor.jsColors["mediumblue"] = new Color(0x0000CD);
    JSColor.jsColors["mediumorchid"] = new Color(0xBA55D3);
    JSColor.jsColors["mediumpurple"] = new Color(0x9370DB);
    JSColor.jsColors["mediumseagreen"] = new Color(0x3CB371);
    JSColor.jsColors["mediumslateblue"] = new Color(0x7B68EE);
    JSColor.jsColors["mediumspringgreen"] = new Color(0x00FA9A);
    JSColor.jsColors["mediumturquoise"] = new Color(0x48D1CC);
    JSColor.jsColors["mediumvioletred"] = new Color(0xC71585);
    JSColor.jsColors["midnightblue"] = new Color(0x191970);
    JSColor.jsColors["mintcream"] = new Color(0xF5FFFA);
    JSColor.jsColors["mistyrose"] = new Color(0xFFE4E1);
    JSColor.jsColors["moccasin"] = new Color(0xFFE4B5);
    JSColor.jsColors["navajowhite"] = new Color(0xFFDEAD);
    JSColor.jsColors["navy"] = new Color(0x000080);
    JSColor.jsColors["oldlace"] = new Color(0xFDF5E6);
    JSColor.jsColors["olive"] = new Color(0x808000);
    JSColor.jsColors["olivedrab"] = new Color(0x6B8E23);
    JSColor.jsColors["orange"] = new Color(0xFFA500);
    JSColor.jsColors["orangered"] = new Color(0xFF4500);
    JSColor.jsColors["orchid"] = new Color(0xDA70D6);
    JSColor.jsColors["palegoldenrod"] = new Color(0xEEE8AA);
    JSColor.jsColors["palegreen"] = new Color(0x98FB98);
    JSColor.jsColors["paleturquoise"] = new Color(0xAFEEEE);
    JSColor.jsColors["palevioletred"] = new Color(0xDB7093);
    JSColor.jsColors["papayawhip"] = new Color(0xFFEFD5);
    JSColor.jsColors["peachpuff"] = new Color(0xFFDAB9);
    JSColor.jsColors["peru"] = new Color(0xCD853F);
    JSColor.jsColors["pink"] = new Color(0xFFC0CB);
    JSColor.jsColors["plum"] = new Color(0xDDA0DD);
    JSColor.jsColors["powderblue"] = new Color(0xB0E0E6);
    JSColor.jsColors["purple"] = new Color(0x800080);
    JSColor.jsColors["rebeccapurple"] = new Color(0x663399);
    JSColor.jsColors["red"] = new Color(0xFF0000);
    JSColor.jsColors["rosybrown"] = new Color(0xBC8F8F);
    JSColor.jsColors["royalblue"] = new Color(0x4169E1);
    JSColor.jsColors["saddlebrown"] = new Color(0x8B4513);
    JSColor.jsColors["salmon"] = new Color(0xFA8072);
    JSColor.jsColors["sandybrown"] = new Color(0xF4A460);
    JSColor.jsColors["seagreen"] = new Color(0x2E8B57);
    JSColor.jsColors["seashell"] = new Color(0xFFF5EE);
    JSColor.jsColors["sienna"] = new Color(0xA0522D);
    JSColor.jsColors["silver"] = new Color(0xC0C0C0);
    JSColor.jsColors["skyblue"] = new Color(0x87CEEB);
    JSColor.jsColors["slateblue"] = new Color(0x6A5ACD);
    JSColor.jsColors["slategray"] = new Color(0x708090);
    JSColor.jsColors["slategrey"] = new Color(0x708090);
    JSColor.jsColors["snow"] = new Color(0xFFFAFA);
    JSColor.jsColors["springgreen"] = new Color(0x00FF7F);
    JSColor.jsColors["steelblue"] = new Color(0x4682B4);
    JSColor.jsColors["tan"] = new Color(0xD2B48C);
    JSColor.jsColors["teal"] = new Color(0x008080);
    JSColor.jsColors["thistle"] = new Color(0xD8BFD8);
    JSColor.jsColors["tomato"] = new Color(0xFF6347);
    JSColor.jsColors["turquoise"] = new Color(0x40E0D0);
    JSColor.jsColors["violet"] = new Color(0xEE82EE);
    JSColor.jsColors["wheat"] = new Color(0xF5DEB3);
    JSColor.jsColors["white"] = new Color(0xFFFFFF);
    JSColor.jsColors["whitesmoke"] = new Color(0xF5F5F5);
    JSColor.jsColors["yellow"] = new Color(0xFFFF00);
    JSColor.jsColors["yellowgreen"] = new Color(0x9ACD32);
};

JSColor.jsColors = null;

/* JSFont */

var JSFont = function() {
    /* Empty */
};

JSFont.decode = function(str) {
    return Font(str);
};

/* JSElementList */

var JSElementList = function(collection) {
    if (!(this instanceof JSElementList)) {
        return new JSElementList(collection);
    }
    ArrayList.call(this);
    var array = collection.toArray ? collection.toArray() : collection;
    for (var i = 0; i < array.length; i++) {
        this.add(array[i]);
    }
};

JSElementList.prototype =
    jslib.inheritPrototype(ArrayList, "JSElementList extends ArrayList");
JSElementList.prototype.constructor = JSElementList;
JSElementList.prototype.$class = new Class("JSElementList", JSElementList);

/* JSErrorEvent */

var JSErrorEvent = function(source, msg, ex) {
    if (!(this instanceof JSErrorEvent)) {
        return new JSErrorEvent(source, msg, ex);
    }
    ActionEvent.call(this, source, ActionEvent.ACTION_PERFORMED, msg);
    this.exception = (ex === undefined) ? null : ex;
};

JSErrorEvent.prototype =
    jslib.inheritPrototype(ActionEvent, "JSErrorEvent extends ActionEvent");
JSErrorEvent.prototype.constructor = JSErrorEvent;
JSErrorEvent.prototype.$class = new Class("JSErrorEvent", JSErrorEvent);

JSErrorEvent.prototype.getException = function() {
    return this.exception;
};

/* JSEvent */

var JSEvent = function() {
    if (!(this instanceof JSEvent)) return new JSEvent();
    /* Empty */
};

JSEvent.createActionEvent = function(source, command, modifiers) {
    return new ActionEvent(source, ActionEvent.ACTION_PERFORMED, command,
                           (modifiers === undefined) ? null : modifiers);
};

JSEvent.dispatch = function(listener, e) {
    listener.actionPerformed(e);
};

JSEvent.dispatchList = function(listeners, e) {
    var n = listeners.elements.length;
    for (var i = 0; i < n; i++) {
        listeners.elements[i].actionPerformed(e);
    }
};

JSEvent.isErrorEvent = function(e) {
    return e.exception !== undefined;
}

JSEvent.setHeadlessTimer = function(flag) {
    /* Ignored in JavaScript */
};

JSEvent.getHeadlessTimer = function() {
    return false;
};

/* JSCanvas */

var JSCanvas = function() {
    if (!(this instanceof JSCanvas)) return new JSCanvas();
    JComponent.call(this, document.createElement("canvas"));
    this.canvas = this.element;
    this.canvas.contentEditable = true;
    this.canvas.overflow = "hidden";
    this.canvas.style.outlineWidth = "0px";
    this.element = this.canvas;
    this.sf = 1;
};

JSCanvas.prototype =
    jslib.inheritPrototype(JComponent, "JSCanvas extends JComponent");
JSCanvas.prototype.constructor = JSCanvas;
JSCanvas.prototype.$class = new Class("JSCanvas", JSCanvas);

JSCanvas.prototype.toString = function() {
    return "JSCanvas #" + this.uid;
};

JSCanvas.prototype.getSize = function() {
    return new Dimension(this.width, this.height);
};

JSCanvas.prototype.setScaleFactor = function(sf) {
    this.sf = sf;
};

JSCanvas.prototype.getScaleFactor = function() {
    return this.sf;
};

JSCanvas.prototype.repaint = function() {
    var g = this.getGraphics();
    if (g && g.ctx) this.paint(g);
};

JSCanvas.prototype.paint = function(g) {
    g.ctx.save();
    if (this.isOpaque() && this.bg) {
        g.ctx.fillStyle = this.bg.getColorTag();
        g.ctx.fillRect(0, 0, this.element.width, this.element.height);
    }
    g.ctx.scale(this.sf, this.sf);
    g.ctx.fillStyle = g.ctx.strokeStyle = this.fg.getColorTag();
    g.ctx.font = this.font.getFontTag();
    this.paintComponent(g);
    g.ctx.restore();
};

JSCanvas.prototype.paintComponent = function(g) {
    /* Empty */
};

JSCanvas.getEnclosingFrame = function(comp) {
    return comp;
};

/* JSCookie */

var JSCookie = function() {
    if (!(this instanceof JSCookie)) return new JSCookie();
    throw new RuntimeException("JSCookie cannot be instantiated");
};

JSCookie.localMap = null;

JSCookie.set = function(name, value, days) {
    if (JSCookie.localMap !== null) {
        JSCookie.localMap[name] = value;
    } else {
        var str = name + "=" + encodeURIComponent(value);
        if (days !== undefined) {
            var date = new Date(Date.now() + days * 24 * 60 * 60 * 1000);
            str += "; expires=" + date.toUTCString();
        }
        document.cookie = str;
        if (document.cookie === "") {
            JSCookie.localMap = { };
            JSCookie.localMap[name] = value;
        }
    }
};

JSCookie.remove = function(name) {
    if (JSCookie.localMap !== null) {
        delete JSCookie.localMap[name];
    } else {
        document.cookie = name + "=; expires=" + new Date(0).toUTCString();
    }
};

JSCookie.get = function(name) {
    value = null;
    if (JSCookie.localMap === null) {
        var cookies = document.cookie.split(";");
        for (var i = 0; i < cookies.length; i++) {
            var binding = cookies[i];
            var equals = binding.indexOf("=");
            if (equals === -1) {
                throw new RuntimeException("Invalid cookie string");
            }
            var key = jslib.trim(binding.substring(0, equals));
            value = jslib.trim(binding.substring(equals + 1));
            if (key === name) {
                value = decodeURIComponent(value);
                break;
            }
        }
    } else {
        value = JSCookie.localMap[name];
    }
    if (value === "@") {
        value = JSCookie.libraryFileMap[name];
    }
    return value;
};

JSCookie.defineLibraryFile = function(name, value) {
    if (!JSCookie.libraryFileMap) {
        JSCookie.libraryFileMap = { };
    }
    JSCookie.libraryFileMap[name] = value;
}

JSCookie.getNames = function() {
    var names = [];
    if (JSCookie.localMap !== null) {
        for (var name in JSCookie.localMap) {
            names.push(name);
        }
    } else if (document.cookie !== "") {
        var cookies = document.cookie.split(";");
        for (var i = 0; i < cookies.length; i++) {
            var binding = cookies[i];
            var equals = binding.indexOf("=");
            if (equals === -1) {
                throw new RuntimeException("Invalid cookie string");
            }
            names.push(jslib.trim(binding.substring(0, equals)));
        }
    }
    return names;
};

/* JSFile */

var JSFile = function(dir, file) {
    if (!(this instanceof JSFile)) return new JSFile(dir, file);
    if (file !== undefined) {
        this.path = (dir === "") ? file.name : dir + "/" + file.name;
        this.file = file;
    } else if (typeof(dir) === "string") {
        this.path = dir;
        this.file = null;
    } else {
        this.file = dir;
        this.path = dir.name;
    }
};

JSFile.prototype.$class = new Class("JSFile", JSFile);

JSFile.prototype.getPath = function() {
    return this.path;
};

JSFile.prototype.getName = function() {
    return JSFile.getTail(this.path);
};

JSFile.prototype.getProtocol = function() {
    return this.path.substring(0, this.path.indexOf(":") + 1);
};

JSFile.prototype.getPathname = function() {
    return this.path.substring(this.path.indexOf(":") + 1);
};

JSFile.prototype.getAbsolutePath = function() {
    return this.path;
};

JSFile.prototype.read = function(listener) {
    var protocol = this.getProtocol();
    if (window.FileReader && this.file && protocol === "") {
        this.readUsingFileReader(this.file, listener);
    } else {
        var path = this.getPathname();
        if (JSFile.isLocalFileSystem()) {
            var s1 = path.indexOf("/");
            var root = path.substring(0, s1);
            var s2 = path.indexOf("/", s1 + 1);
            if (s2 === -1) {
                path = path.substring(s1 + 1);
            } else {
                root = path.substring(s1 + 1, s2);
                path = path.substring(s2 + 1);
            }
            var source = this;
            var callback = function(fs) {
                var e = JSEvent.createActionEvent(source, fs[path]);
                JSEvent.dispatch(listener, e);
            };
            JSFile.callLocalFileSystem(root, callback);
        } else {
            var cmd = "readFile.pl?path=" + encodeURIComponent(path);
            JSFile.sendCGIRequest(cmd, listener, this);
        }
    }
};

JSFile.prototype.readDirectory = function(listener) {
    var protocol = this.getProtocol();
    if (window.FileReader && this.file && protocol === "") {
        throw new RuntimeException("Not implemented");
    } else if (JSFile.isLocalFileSystem()) {
        throw new RuntimeException("Not implemented");
    } else {
        var path = this.getPathname();
        var cmd = "readDirectory.pl?path=" + encodeURIComponent(path);
        JSFile.sendCGIRequest(cmd, listener, this);
    }
};

JSFile.prototype.readDirectoryTree = function(listener) {
    var protocol = this.getProtocol();
    if (window.FileReader && this.file && protocol === "") {
        throw new RuntimeException("Not implemented");
    } else {
        var path = this.getPathname();
        if (JSFile.isLocalFileSystem()) {
            var root = JSFile.getTail(path);
            var source = this;
            var callback = function(fs) {
                var e = JSEvent.createActionEvent(source, fs["."]);
                JSEvent.dispatch(listener, e);
            };
            JSFile.callLocalFileSystem(root, callback);
        } else {
            var cmd = "readTree.pl?path=" + encodeURIComponent(path);
            JSFile.sendCGIRequest(cmd, listener, this);
        }
    }
};

JSFile.prototype.readUsingFileReader = function(file, listener) {
    var reader = new FileReader();
    var fileLoaded = function(e) {
        e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                            e.target.result);
        listener.actionPerformed(e);
    }
    reader.onload = fileLoaded;
    reader.readAsText(file);
};

JSFile.prototype.write = function(text, listener) {
    var protocol = this.getProtocol();
    if (window.FileWriter && this.file && protocol === "") {
        this.writeUsingFileWriter(this.file, listener, text);
    } else if (JSFile.isLocalFileSystem()) {
        throw new RuntimeException("Not implemented");
    } else {
        var path = this.getPathname();
        var cmd = "writeFile.pl?path=" + encodeURIComponent(path) +
                  "&text=" + encodeURIComponent(text);
        JSFile.sendCGIRequest(cmd, listener, this);
    }
};

JSFile.prototype.writeUsingFileWriter = function(file, listener, text) {
    throw new RuntimeException("Not yet implemented");
};

JSFile.prototype.delete = function(listener) {
    if (JSFile.isLocalFileSystem()) {
        throw new RuntimeException("Not implemented");
    }
    var protocol = this.getProtocol();
    var path = this.getPathname();
    var cmd = "deleteFile.pl?path=" + encodeURIComponent(path);
    JSFile.sendCGIRequest(cmd, listener, this);
};

JSFile.login = function(listener) {
    if (JSFile.isLocalFileSystem()) {
        var e = JSEvent.createActionEvent("login", "library");
        listener.actionPerformed(e);
    } else {
        var cmd = "login.pl";
        JSFile.sendCGIRequest(cmd, listener);
    }
};

JSFile.prototype.toString = function() {
    return this.path;
};

JSFile.isLocalFileSystem = function() {
    return window && window.location && window.location.href &&
           jslib.startsWith(window.location.href, "file:");
};

JSFile.sendCGIRequest = function(cmd, listener, source) {
    var server = JSFile.CGIServer;
    JSFile.sendHttpRequest(server + "/" + cmd, listener, source);
};

JSFile.sendHttpRequest = function(url, listener, source) {
    var req = new XMLHttpRequest();
    if (!source) source = "JSFile";
    req.ok = false;
    req.onreadystatechange = function() {
        if (req.readyState === 2) {
            req.ok = true;
        } else if (req.readyState === 4) {
            var text = req.responseText;
            if (jslib.startsWith(text, "OK\n")) {
                text = text.substring(3);
            } else if (jslib.startsWith(text, "Error:")) {
                req.ok = false;
            }
            var e = null;
            if (req.ok) {
                e = new ActionEvent(source, ActionEvent.ACTION_PERFORMED,
                                    text);
            } else {
                e = new JSErrorEvent(source, "Can't open " + url);
            }
            if (e !== null && listener !== null) {
                listener.actionPerformed(e);
            }
        }
    };
    req.open("GET", url, true);
    req.send();
};

JSFile.setCGIServer = function(url) {
    JSFile.CGIServer = url;
};

JSFile.getCGIServer = function() {
    return JSFile.CGIServer;
};

JSFile.getHead = function(path) {
    var slashIndex = -1;
    for (var i = 0; i < path.length; i++) {
        switch (path.charCodeAt(i)) {
             case toInt('/'): case toInt('\\'): slashIndex = i; break;
        }
    }
    if (slashIndex === -1) {
        return "";
    } else if (slashIndex === 0) {
        return path.substring(0, 1);
    } else {
        return path.substring(0, slashIndex);
    }
};

JSFile.getTail = function(path) {
    var slashIndex = -1;
    for (var i = 0; i < path.length; i++) {
        switch (path.charCodeAt(i)) {
             case toInt('/'): case toInt('\\'): slashIndex = i; break;
        }
    }
    if (slashIndex === -1) return path;
    return path.substring(slashIndex + 1);
};

JSFile.getRoot = function(path) {
    var dotIndex = -1;
    for (var i = 0; i < path.length; i++) {
        switch (path.charCodeAt(i)) {
             case toInt('.'): dotIndex = i; break;
             case toInt('/'): case toInt('\\'): dotIndex = -1; break;
        }
    }
    if (dotIndex === -1) return path;
    return path.substring(0, dotIndex);
};

JSFile.getExtension = function(path) {
    var dotIndex = -1;
    for (var i = 0; i < path.length; i++) {
        switch (path.charCodeAt(i)) {
             case toInt('.'): dotIndex = i; break;
             case toInt('/'): case toInt('\\'): dotIndex = -1; break;
        }
    }
    if (dotIndex === -1) return "";
    return path.substring(dotIndex);
};

JSFile.parseHTMLDirectory = function(html) {
    var files = new ArrayList();
    var start = html.indexOf("Parent Directory");
    if (start === -1) return null;
    while (true) {
        start = html.indexOf("<td><a href=\"", start);
        if (start === -1) break;
        start += "<td><a href=\"".length;
        var finish = html.indexOf("\"", start);
        if (finish === -1) return null;
        files.add(html.substring(start, finish));
    }
    var n = files.size();
    var array = jslib.newArray(n);
    for (var i = 0; i < n; i++) {
        array[i] = files.get(i);
    }
    return array;
};

JSFile.readClientFile = function(filename, callback, parent) {
    var dialog = document.getElementById("readClientFile");
    if (!dialog) {
        dialog = document.createElement("input");
        dialog.setAttribute("style",
                            "visibility:hidden;" +
                            "position:fixed;" +
                            "bottom:-10;");
        dialog.setAttribute("type", "file");
        dialog.setAttribute("id", "readClientFile");
        dialog.addEventListener("change", fileSelected, false);
        document.body.appendChild(dialog);
    }
    dialog.click();

    function fileSelected(e) {
        var file = e.target.files[0];
        var rd = new FileReader();
        rd.onload = fireLoadListener;
        rd.readAsText(file);
        function fireLoadListener() {
            var e = JSEvent.createActionEvent("readClientFile", rd.result);
            if (callback.actionPerformed) {
                callback.actionPerformed(e);
            } else {
                callback(e);
            }
        }
    }

};

JSFile.writeClientFile = function(filename, text, callback, parent) {
    var dialog = JSDownloadDialog(parent, "Download");
    dialog.setDownloadCallback(download);
    dialog.setCancelCallback(cancel);
    dialog.centerOnParent();
    dialog.setFilename(JSFile.getTail(filename));
    dialog.setVisible(true);
    dialog.fileField.requestFocus();

    function cancel(dialog) {
        dialog.setVisible(false);
        var e = JSEvent.createActionEvent("writeClientFile", "Cancel");
        if (callback.actionPerformed) {
            callback.actionPerformed(e);
        } else {
            callback(e);
        }
    }

    function download(dialog) {
        dialog.setVisible(false);
        let anchor = document.createElement("a");
        anchor.setAttribute("href", "data:text/plain;charset=utf-8," +
                                    encodeURIComponent(text));
        anchor.setAttribute("download", dialog.getFilename());
        if (document.createEvent) {
            let event = document.createEvent("MouseEvents");
            event.initEvent("click", true, true);
            anchor.dispatchEvent(event);
        } else {
            anchor.click();
        }
        var e = JSEvent.createActionEvent("writeClientFile", "OK");
        if (callback.actionPerformed) {
            callback.actionPerformed(e);
        } else {
            callback(e);
        }
    }

};

JSFile.callLocalFileSystem = function(root, callback) {
    if (JSFile.localFileSystems[root]) {
        callback(JSFile.localFileSystems[root]);
    } else {
        var def = function(fs) {
            JSFile.localFileSystems[root] = fs;
            callback(fs);
        };
        require([ "jsfs/" + root ], def);
    }
};

JSFile.localFileSystems = { };

JSFile.DEFAULT_CGI_SERVER = ".";

/* JSFileChooser */

var JSFileChooser = function(target) {
    if (!(this instanceof JSFileChooser)) return new JSFileChooser(target);
    var chooser = this;
    var loadListener = {
        actionPerformed:
            function(e) {
                if (!(e instanceof JSErrorEvent)) {
                    chooser.path = e.getActionCommand();
                    e.actionCommand = "ApproveSelection";
                }
                chooser.fireLoadListeners(e);
            }
    };
    var saveListener = {
        actionPerformed:
            function(e) {
                if (!(e instanceof JSErrorEvent)) {
                    chooser.path = e.getActionCommand();
                    e.actionCommand = "ApproveSelection";
                }
                chooser.fireSaveListeners(e);
            }
    };
    this.loadDialog = new JSLoadDialog(target);
    this.loadDialog.addActionListener(loadListener);
    this.saveDialog = new JSSaveDialog(target);
    this.saveDialog.addActionListener(saveListener);
    this.loadListeners = [];
    this.saveListeners = [];
};

JSFileChooser.prototype.$class = new Class("JSFileChooser", JSFileChooser);

JSFileChooser.prototype.setCurrentDirectory = function(dir) {
    this.loadDialog.setDirectory(new JSFile(dir));
};

JSFileChooser.prototype.showOpenDialog = function() {
    this.showLoadDialog();
};

JSFileChooser.prototype.showLoadDialog = function() {
    this.loadDialog.centerOnParent();
    this.loadDialog.setVisible(true);
};

JSFileChooser.prototype.showSaveDialog = function() {
    this.saveDialog.centerOnParent();
    this.saveDialog.setVisible(true);
};

JSFileChooser.prototype.getPath = function() {
    return this.path;
};

JSFileChooser.prototype.addLoadListener = function(listener) {
    this.loadListeners.push(listener);
};

JSFileChooser.prototype.removeLoadListener = function(listener) {
    var index = this.loadListeners.indexOf(listener);
    if (index >= 0) this.loadListeners.splice(index, 1);
};

JSFileChooser.prototype.fireLoadListeners = function(e) {
    for (var i = 0; i < this.loadListeners.length; i++) {
        this.loadListeners[i].actionPerformed(e);
    }
};

JSFileChooser.prototype.addSaveListener = function(listener) {
    this.saveListeners.push(listener);
};

JSFileChooser.prototype.removeSaveListener = function(listener) {
    var index = this.saveListeners.indexOf(listener);
    if (index >= 0) this.saveListeners.splice(index, 1);
};

JSFileChooser.prototype.fireSaveListeners = function(e) {
    for (var i = 0; i < this.saveListeners.length; i++) {
        this.saveListeners[i].actionPerformed(e);
    }
};

JSFileChooser.prototype.setDialogTitle = function(str) {
    this.loadDialog.setTitle(str);
    this.saveDialog.setTitle(str);
};

JSFileChooser.prototype.getDialogTitle = function() {
    return this.loadDialog.getTitle();
};

/* JSFileInteractor */

var JSFileInteractor = function(target) {
    if (!(this instanceof JSFileInteractor)) {
        return new JSFileInteractor(target);
    }
    var chooser = this;
    this.target = target;
    this.prefix = null;
    this.filename = null;
    this.loadListeners = [];
    this.saveListeners = [];
};

JSFileInteractor.prototype.$class =
    new Class("JSFileInteractor", JSFileInteractor);

JSFileInteractor.prototype.setCookiePrefix = function(prefix) {
    this.prefix = prefix;
};

JSFileInteractor.prototype.getCookiePrefix = function() {
    return this.prefix;
};

JSFileInteractor.prototype.setCurrentDirectory = function(dir) {
    /* Ignored in the browser */
};

JSFileInteractor.prototype.loadFile = function() {
    if (this.prefix === null) {
        this.loadHTML5File();
    } else {
        this.loadCookieFile();
    }
};

JSFileInteractor.prototype.saveFile = function(text) {
    if (this.prefix === null) {
        this.saveHTML5File(text);
    } else {
        this.saveCookieFile(text);
    }
};

JSFileInteractor.prototype.getPath = function() {
    return this.path;
};

JSFileInteractor.prototype.setPath = function(path) {
    this.path = path;
};

JSFileInteractor.prototype.defineLibraryFile = function(name, value) {
    if (this.prefix == null) {
        throw new RuntimeException("Library files exist only as cookies.");
    }
    JSCookie.defineLibraryFile(this.prefix + name, value);
    JSCookie.set(this.prefix + name, "@", 1);
}

JSFileInteractor.prototype.addLoadListener = function(listener) {
    this.loadListeners.push(listener);
};

JSFileInteractor.prototype.removeLoadListener = function(listener) {
    var index = this.loadListeners.indexOf(listener);
    if (index >= 0) this.loadListeners.splice(index, 1);
};

JSFileInteractor.prototype.fireLoadListeners = function(e) {
    for (var i = 0; i < this.loadListeners.length; i++) {
        this.loadListeners[i].actionPerformed(e);
    }
};

JSFileInteractor.prototype.addSaveListener = function(listener) {
    this.saveListeners.push(listener);
};

JSFileInteractor.prototype.removeSaveListener = function(listener) {
    var index = this.saveListeners.indexOf(listener);
    if (index >= 0) this.saveListeners.splice(index, 1);
};

JSFileInteractor.prototype.fireSaveListeners = function(e) {
    for (var i = 0; i < this.saveListeners.length; i++) {
        this.saveListeners[i].actionPerformed(e);
    }
};

/* JSFileInteractor - HTML5 implementation */

JSFileInteractor.prototype.loadHTML5File = function() {
    var chooser = this;
    var dialog = document.getElementById("HTMLDialog");
    if (!dialog) {
        dialog = document.createElement("input");
        dialog.setAttribute("style",
                            "visibility:hidden;" +
                            "position:fixed;" +
                            "bottom:-10;");
        dialog.setAttribute("type", "file");
        dialog.setAttribute("id", "HTMLDialog");
        dialog.addEventListener("change", fileSelected, false);
        document.body.appendChild(dialog);
    }
    dialog.click();

    function fileSelected(e) {
        var file = e.target.files[0];
        chooser.path = file.name;
        var rd = new FileReader();
        rd.onload = fireLoadListeners;
        rd.readAsText(file);
        function fireLoadListeners() {
            var e = JSEvent.createActionEvent(chooser, rd.result);
            chooser.fireLoadListeners(e);
        }
    }
};

JSFileInteractor.prototype.saveHTML5File = function(text) {
    var chooser = this;
    var dialog = JSDownloadDialog(this.target, "Download");
    dialog.setDownloadCallback(download);
    dialog.setCancelCallback(cancel);
    dialog.centerOnParent();
    dialog.setVisible(true);
    dialog.fileField.requestFocus();

    function cancel(dialog) {
        var e = JSEvent.createActionEvent(chooser, "Cancel");
        chooser.fireSaveListeners(e);
    }

    function download(dialog) {
        let anchor = document.createElement("a");
        anchor.setAttribute("href", "data:text/plain;charset=utf-8," +
                                    encodeURIComponent(text));
        anchor.setAttribute("download", dialog.getFilename());
        chooser.path = dialog.getFilename();
        if (document.createEvent) {
            let event = document.createEvent("MouseEvents");
            event.initEvent("click", true, true);
            anchor.dispatchEvent(event);
        } else {
            anchor.click();
        }
        var e = JSEvent.createActionEvent(chooser, "OK");
        chooser.fireSaveListeners(e);
        if (chooser.path === null) {
            dialog.setFilename("");
        } else {
            dialog.setFilename(JSFile.getTail(chooser.path));
        }
    }

}

/* JSFileInteractor - Cookie implementation */

JSFileInteractor.prototype.loadCookieFile = function() {
    var chooser = this;
    dialog = JSLoadCookieDialog(this);
    dialog.setFileList(readCookieDirectory(chooser.prefix));
    dialog.centerOnParent();
    dialog.setVisible(true);

    function readCookieDirectory(prefix) {
        var list = [ ];
        var cookies = JSCookie.getNames();
        for (var i = 0; i < cookies.length; i++) {
            var name = cookies[i];
            if (name.startsWith(prefix)) {
                list.push(name.substring(prefix.length));
            }
        }
        list.sort();
        return list;
    }
};

JSFileInteractor.prototype.saveCookieFile = function(text) {
    var chooser = this;
    var dialog = JSSaveCookieDialog(this.target, "Save");
    dialog.setSaveCallback(saveFile);
    dialog.setDownloadCallback(downloadFile);
    dialog.setCancelCallback(cancel);
    dialog.centerOnParent();
    if (chooser.getPath()) {
        dialog.setFilename(chooser.getPath());
    }
    dialog.setVisible(true);
    dialog.fileField.requestFocus();

    function cancel(dialog) {
        var e = JSEvent.createActionEvent(chooser, "Cancel");
        chooser.fireSaveListeners(e);
    }

    function saveFile(dialog) {
        var filename = dialog.getFilename();
        if (filename) {
            chooser.setPath(filename);
            JSCookie.set(chooser.prefix + filename, text);
        }
        var e = JSEvent.createActionEvent(chooser, "OK");
        chooser.fireSaveListeners(e);
    }

    function downloadFile(dialog) {
        let anchor = document.createElement("a");
        anchor.setAttribute("href", "data:text/plain;charset=utf-8," +
                                    encodeURIComponent(text));
        anchor.setAttribute("download", dialog.getFilename());
        chooser.path = dialog.getFilename();
        if (document.createEvent) {
            let event = document.createEvent("MouseEvents");
            event.initEvent("click", true, true);
            anchor.dispatchEvent(event);
        } else {
            anchor.click();
        }
        var e = JSEvent.createActionEvent(chooser, "OK");
        chooser.fireSaveListeners(e);
        if (chooser.path === null) {
            dialog.setFilename("");
        } else {
            dialog.setFilename(JSFile.getTail(chooser.path));
        }
    }

};

/* JSImage */

var JSImage = function(src) {
    if (!(this instanceof JSImage)) return new JSImage(src);
    var jsi = this;
    var image = new Image();
    jsi.image = image;
    if (typeof(src) === "string") {
        image.src = src;
        if (src.indexOf(JSImage.DATA_HEADER) === 0) {
            var hlen = JSImage.DATA_HEADER.length;
            var header = window.atob(src.substring(hlen, hlen + 32));
            jsi.width = JSImage.getInt(header, 16);
            jsi.height = JSImage.getInt(header, 20);
            image.alt = "Image-" + image.width + "x" + image.height;
        } else {
            jsi.width = 0;
            jsi.height = 0;
            image.alt = src;
        }
    } else {
        var height = src.length;
        var width = src[0].length;
        var canvas = document.createElement("canvas");
        canvas.width = width;
        canvas.height = height;
        var ctx = canvas.getContext("2d");
        var data = ctx.createImageData(width, height);
        var k = 0;
        for (var i = 0; i < height; i++) {
            for (var j = 0; j < width; j++) {
                var pixel = src[i][j];
                data.data[k++] = (pixel >> 16) & 0xFF;
                data.data[k++] = (pixel >> 8) & 0xFF;
                data.data[k++] = pixel & 0xFF;
                data.data[k++] = (pixel >> 24) & 0xFF;
            }
        }
        ctx.putImageData(data, 0, 0);
        jsi.width = width;
        jsi.height = height;
        image.src = canvas.toDataURL("image/png");
        image.alt = "Image-" + width + "x" + height;
        ctx.drawImage(image, 0, 0);
    }
};

JSImage.prototype.$class = new Class("JSImage", JSImage);

JSImage.prototype.getWidth = function() {
    return this.width;
};

JSImage.prototype.getHeight = function() {
    return this.height;
};

JSImage.prototype.getImage = function() {
    return this.image;
};

JSImage.isComplete = function(image) {
    return image.complete;
};

JSImage.addActionListener = function(jsi, listener) {
    if (jsi.listeners === undefined) jsi.listeners = [ ];
    jsi.listeners.push(listener);
    jsi.image.onload = function() {
        jsi.width = jsi.image.width;
        jsi.height = jsi.image.height;
        var e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED, jsi);
        var n = jsi.listeners.length;
        for (var i = 0; i < n; i++) {
            jsi.listeners[i].actionPerformed(e);
        }
    };
};

JSImage.createPixelArray = function(width, height) {
    var array = [ ];
    for (var i = 0; i < height; i++) {
        var row = [ ];
        for (var j = 0; j < width; j++) {
            row.push(0);
        }
        array.push(row);
    }
    return array;
};

JSImage.getInt = function(bytes, start) {
    var n = bytes.charCodeAt(start);
    n = n << 8 | bytes.charCodeAt(start + 1);
    n = n << 8 | bytes.charCodeAt(start + 2);
    n = n << 8 | bytes.charCodeAt(start + 3);
    return n;
};

JSImage.DATA_HEADER = "data:image/png;base64,";

/* JSLineReader */

var JSLineReader = function(arg) {
    if (!(this instanceof JSLineReader)) return new JSLineReader(arg);
    if (typeof(arg) === "string") {
        this.lines = JSPlatform.splitLines(arg);
        this.nLines = this.lines.length;
        if (this.nLines > 0 && this.lines[this.nLines - 1] === "") {
            this.nLines--;
        }
    } else {
        this.lines = arg;
        this.nLines = arg.length;
    }
    this.index = 0;
};

JSLineReader.prototype.nextLine = function() {
    if (this.index >= this.nLines) return null;
    return this.lines[this.index++];
};

JSLineReader.prototype.saveLine = function() {
    if (this.index > 0) this.index--;
};

/* JSPanel */

var JSPanel = function() {
    if (!(this instanceof JSPanel)) return new JSPanel();
    JComponent.call(this);
    this.element.style.position = "relative";
    this.layout = new FlowLayout();
};

JSPanel.prototype =
    jslib.inheritPrototype(JComponent, "JSPanel extends JComponent");
JSPanel.prototype.constructor = JSPanel;
JSPanel.prototype.$class = new Class("JSPanel", JSPanel);

/* JSFrame */

var JSFrame = function(contents, title) {
    if (!(this instanceof JSFrame)) return new JSFrame(contents, title);
    JSPanel.call(this);
    this.setLayout(new BorderLayout());
    this.setBorder(BorderFactory.createLineBorder(JSFrame.BORDER_COLOR));
    this.add(contents, BorderLayout.CENTER);
    this.tb = new JSTitleBar(title);
    this.add(this.tb, BorderLayout.NORTH);
    contents.addFocusListener(this.tb);
    this.tb.addMouseListener(new JSTitleBarListener(contents));
};

JSFrame.prototype =
    jslib.inheritPrototype(JSPanel, "JSFrame extends JSPanel");
JSFrame.prototype.constructor = JSFrame;
JSFrame.prototype.$class = new Class("JSFrame", JSFrame);

JSFrame.prototype.setTitle = function(title) {
    this.tb.setTitle(title);
};

JSFrame.prototype.getTitle = function() {
    return this.tb.getTitle();
};

JSFrame.BORDER_COLOR = new Color(0x666666);

var JSTitleBarListener = function(target) {
    if (!(this instanceof JSTitleBarListener)) {
        return new JSTitleBarListener(target);
    }
    this.target = target;
};

JSTitleBarListener.prototype.mouseClicked = function(e) {
    this.target.requestFocus();
};

JSTitleBarListener.prototype.mouseEntered = function(e) {
    /* Empty */
};

JSTitleBarListener.prototype.mouseExited = function(e) {
    /* Empty */
};

JSTitleBarListener.prototype.mousePressed = function(e) {
    /* Empty */
};

JSTitleBarListener.prototype.mouseReleased = function(e) {
    /* Empty */
};

/* JSPackage.js */

var JSPackage = function() {
    if (!(this instanceof JSPackage)) return new JSPackage();
    /* Empty */
};

JSPackage.prototype.$class = new Class("JSPackage", JSPackage);

JSPackage.prototype.init = function(arg) {
    throw new RuntimeException("Each package must override init");
};

JSPackage.load = function(packages, arg) {
    throw new RuntimeException("Illegal load from JavaScript");
};

JSPackage.require = function(packages, arg, listener) {
    if (typeof(packages) === "string") packages = [ packages ];
    var n = packages.length;
    for (var i = 0; i < n; i++) {
        packages[i] = packages[i].replace(/[.]/g, "/");
    }
    var callback =
        function() {
            var n = arguments.length;
            for (var i = 0; i < n; i++) {
                var name = packages[i];
                var suffix = name.substring(name.lastIndexOf("/") + 1);
                var pkg = new arguments[i]["Package_" + suffix]();
                pkg.init(arg);
            }
            var e = JSEvent.createActionEvent(arg, "OK");
            JSEvent.dispatch(listener, e);
        };
    require(packages, callback);
};

/* JSPlatform.js */

var JSPlatform = function() {
    if (!(this instanceof JSPlatform)) return new JSPlatform();
    /* Empty */
};

JSPlatform.prototype.$class = new Class("JSPlatform", JSPlatform);

JSPlatform.exit = function(status) {
    /* Ignored */
};

JSPlatform.isJavaScript = function() {
    return true;
};

JSPlatform.elementExists = function(id) {
    return document.getElementById(id) !== null;
};

JSPlatform.splitLines = function(text) {
    return text.split(/\r?\n|\r/);
};

JSPlatform.joinLines = function(lines) {
    if (lines.length === 0) {
        return "";
    }
    return lines.join("\n") + "\n";
};

JSPlatform.printStackTrace = function(ex) {
    jslib.printStackTrace(ex);
};

JSPlatform.getScrollBarWidth = function() {
    return JScrollBar.getScrollBarWidth();
};

JSPlatform.isInteger = function(obj) {
    return typeof(obj) === "number" &&
           isFinite(obj) && Math.floor(obj) === obj;
}

JSPlatform.isNumber = function(obj) {
    return typeof(obj) === "number";
}

JSPlatform.isBoolean = function(obj) {
    return typeof(obj) === "boolean";
}

JSPlatform.isString = function(obj) {
    return typeof(obj) === "string";
}

/* JSProgram */

var JSProgram = function() {
    if (!(this instanceof JSProgram)) return new JSProgram();
    JComponent.call(this);
    this.controlRow = null;
    this.defaultUser = null;
    this.prefix = "";
    JSFile.setCGIServer(JSFile.DEFAULT_CGI_SERVER);
};

JSProgram.prototype =
    jslib.inheritPrototype(JComponent, "JSProgram extends JComponent");
JSProgram.prototype.constructor = JSProgram;
JSProgram.prototype.$class = new Class("JSProgram", JSProgram);

JSProgram.prototype.setUID = function(uid) {
    this.uid = uid;
};

JSProgram.prototype.getUID = function() {
    return this.uid;
};

JSProgram.prototype.setPrefix = function(user) {
    this.prefix = user;
};

JSProgram.prototype.getPrefix = function() {
    return this.prefix;
};

JSProgram.prototype.setDefaultUser = function(user) {
    this.defaultUser = user;
};

JSProgram.prototype.getDefaultUser = function() {
    return this.defaultUser;
};

JSProgram.prototype.createProgramFrame = function() {
    /* Empty */
};

JSProgram.prototype.getFrame = function() {
    return this;
};

JSProgram.prototype.setLayout = function(layout) {
    this.layout = null;
    this.layoutManager = layout;
};

JSProgram.prototype.getLayout = function() {
    return this.layoutManager;
};

JSProgram.prototype.setMenuBar = function(mbar) {
    // Fill in
};

JSProgram.prototype.add = function(c, label) {
    var div = null;
    var newlyCreated = false;
    if (typeof(label) === "string") {
        div = document.getElementById(label);
        if (!div) {
            newlyCreated = true;
            div = document.createElement("div");
            div.id = label;
            document.body.appendChild(div);
        }
    } else {
        div = label.element;
    }
    div.appendChild(c.element);
    c.div = div;
    if (c.install) c.install(c, div);
    if (typeof(label) === "string") {
        var style = window.getComputedStyle(div);
        this.width = parseInt(style.width);
        this.height = parseInt(style.height);
        if (c.setSize) c.setSize(this.width, this.height);
        if (c.layout) c.layout.layoutContainer(c);
    }
    if (newlyCreated) {
        if (label === "console") {
            c.setSize(0.95 * window.innerWidth, 0.95 * window.innerHeight);
            c.element.style.overflow = "auto";
        } else {
            c.element.style.overflow = "none";
            c.element.style.border = "solid 1px black";
        }
    }
    if (c.repaint) c.repaint();
};

JSProgram.prototype.getSize = function() {
    var app = document.getElementById(this.prefix + "application");
    if (!app) app = document.getElementsByTagName("body")[0];
    return new Dimension(app.clientWidth, app.clientHeight);
};

JSProgram.prototype.setTitle = function(title) {
    this.title = title;
};

JSProgram.prototype.getTitle = function() {
    return this.title;
};

JSProgram.prototype.addControl = function(button) {
    if (this.controlRow === null) {
        var controls = document.getElementById(this.prefix + "controls");
        this.controlRow = document.createElement("tr");
        var table = document.createElement("table");
        table.appendChild(this.controlRow);
        controls.appendChild(table);
    }
    var td = document.createElement("td");
    td.appendChild(button.element);
    this.controlRow.appendChild(td);
    if (button.repaint) button.repaint();
};

JSProgram.prototype.setVisible = function(flag) {
    /* Ignored */
};

JSProgram.prototype.run = function() {
    /* Empty */
};

JSProgram.prototype.start = function() {
    this.startAfterDelay(JSProgram.DEFAULT_DELAY);
};

JSProgram.prototype.startAfterDelay = function(milliseconds) {
    var pgm = this;
    var callback = function() { pgm.run(); };
    setTimeout(callback, milliseconds || JSProgram.DEFAULT_DELAY);
};

JSProgram.prototype.startAfterLogin = function() {
    this.startAfterSetup(null);
};

JSProgram.prototype.startAfterSetup = function(path) {
    var pgm = this;
    if (jslib.startsWith(window.location.href, "file:")) {
        pgm.setUID("eroberts");
        pgm.run();
    } else {
        var listener = {
            actionPerformed : function(e) {
                if (e instanceof JSErrorEvent) {
                    // Fill in
                } else {
                    pgm.setUID(e.getActionCommand().trim());
                    pgm.run();
                }
            }
        };
        var cmd = "login.pl";
        if (path) cmd += "?path=" + encodeURIComponent(path);
        JSFile.sendCGIRequest(cmd, listener, this);
    }
};

JSProgram.prototype.toString = function() {
    return "JSProgram";
};

JSProgram.isJavaScript = function() {
    return true;
};

JSProgram.alert = function(value) {
    alert(value);
};

JSProgram.exit = function(status) {
    JSPlatform.exit(status);
};

JSProgram.DEFAULT_DELAY = 100;

/* JSProgramLayout.js */

var JSProgramLayout = function() {
    if (!(this instanceof JSProgramLayout)) return new JSProgramLayout();
    /* Not used on the JavaScript side */
}

/* JSScrollPane.js */

var JSScrollPane = function(view, vspPolicy, hspPolicy) {
    if (!(this instanceof JSScrollPane)) {
        return new JSScrollPane(view, vspPolicy, hspPolicy);
    }
    JScrollPane.call(this, view, vspPolicy, hspPolicy);
};

JSScrollPane.prototype =
    jslib.inheritPrototype(JScrollPane, "JSScrollPane extends JScrollPane");
JSScrollPane.prototype.constructor = JSScrollPane;
JSScrollPane.prototype.$class = new Class("JSScrollPane", JSScrollPane);

JSScrollPane.prototype.getViewPosition = function() {
    return new Point(this.element.scrollLeft, this.element.scrollTop);
};

JSScrollPane.prototype.scrollRectToVisible = function(r) {
    var div = this.element;
    var x = r.x - div.scrollLeft;
    if (x < 0 || x >= div.scrollWidth - r.width) {
        changed = true;
        div.scrollLeft =
            Math.max(0, Math.min(r.x, div.scrollWidth - div.clientWidth));
    }
    var y = r.y - div.scrollTop;
    if (y < 0 || y >= div.scrollHeight - r.height) {
        div.scrollTop =
            Math.max(0, Math.min(r.y, div.scrollHeight - div.clientHeight));
    }
}

JSScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS =
    JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS;
JSScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED =
    JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED;
JSScrollPane.HORIZONTAL_SCROLLBAR_NEVER =
    JScrollPane.HORIZONTAL_SCROLLBAR_NEVER;
JSScrollPane.VERTICAL_SCROLLBAR_ALWAYS =
    JScrollPane.VERTICAL_SCROLLBAR_ALWAYS;
JSScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED =
    JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED;
JSScrollPane.VERTICAL_SCROLLBAR_NEVER =
    JScrollPane.VERTICAL_SCROLLBAR_NEVER;

/* JSTitleBar.js */

var JSTitleBar = function(title) {
    if (!(this instanceof JSTitleBar)) return new JSTitleBar(title);
    JSCanvas.call(this);
    this.title = title;
    this.setFont(JSTitleBar.FONT);
    this.componentHasFocus = false;
};

JSTitleBar.prototype =
    jslib.inheritPrototype(JSCanvas, "JSTitleBar extends JSCanvas");
JSTitleBar.prototype.constructor = JSTitleBar;
JSTitleBar.prototype.$class = new Class("JSTitleBar", JSTitleBar);

JSTitleBar.prototype.setTitle = function(title) {
    this.title = title;
    this.repaint();
};

JSTitleBar.prototype.getTitle = function() {
    return this.title;
};

JSTitleBar.prototype.getPreferredSize = function() {
    return this.getMinimumSize();
};

JSTitleBar.prototype.getMinimumSize = function() {
    var fm = this.getFontMetrics(this.getFont());
    return new Dimension(fm.stringWidth(this.title), JSTitleBar.HEIGHT);
};

JSTitleBar.prototype.focusGained = function(e) {
    this.componentHasFocus = true;
    this.repaint();
};

JSTitleBar.prototype.focusLost = function(e) {
    this.componentHasFocus = false;
    this.repaint();
};

JSTitleBar.prototype.repaint = function() {
    var g = this.getGraphics();
    if (!g.ctx) return;
    g.ctx.save();
    var c1 = (this.componentHasFocus) ? JSTitleBar.TOP_FOCUSED
                                      : JSTitleBar.TOP_BLURRED;
    var c2 = (this.componentHasFocus) ? JSTitleBar.BOTTOM_FOCUSED
                                      : JSTitleBar.BOTTOM_BLURRED;
    var lg = g.ctx.createLinearGradient(0, 0, 0, this.height);
    lg.addColorStop(0, c1.getColorTag());
    lg.addColorStop(1, c2.getColorTag());
    g.ctx.fillStyle = lg;
    g.ctx.fillRect(0, 0, this.width, this.height);
    g.ctx.fillStyle = this.fg.getColorTag();
    g.ctx.font = this.font.getFontTag();
    var fm = g.getFontMetrics();
    var x = (this.width - fm.stringWidth(this.title)) / 2;
    var y = (this.height + fm.getAscent()) / 2 + JSTitleBar.TITLE_DY;
    g.ctx.fillText(this.title, x, y);
    g.ctx.restore();
};

JSTitleBar.HEIGHT = 20;
JSTitleBar.FONT = Font.decode("System-12");
JSTitleBar.TOP_FOCUSED = new Color(0xCCCCCC);
JSTitleBar.BOTTOM_FOCUSED = new Color(0x999999);
JSTitleBar.TOP_BLURRED = new Color(0xEEEEEE);
JSTitleBar.BOTTOM_BLURRED = new Color(0xBBBBBB);
JSTitleBar.TITLE_DY = -1;

/* JSDialog.js */

var JSDialog = function(target) {
    if (!(this instanceof JSDialog)) return new JSDialog(target);
    JComponent.call(this);
    this.listeners = [];
    this.target = target;
    this.body = document.getElementsByTagName("body")[0];
    this.glassPane = document.createElement("div");
    this.glassPane.style = "position:static; width:100%; height:100%; " +
                           "background-color:rgba(0,0,0,0);" +
                           "overflow:hidden; z-index:10;";
    this.glassPane.appendChild(this.element);
    this.visible = false;
};

JSDialog.prototype =
    jslib.inheritPrototype(JComponent, "JSDialog extends JComponent");
JSDialog.prototype.constructor = JSDialog;
JSDialog.prototype.$class = new Class("JSDialog", JSDialog);

JSDialog.prototype.execute = function(cmd) {
    var e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED, cmd);
    this.fireActionListeners(e);
    this.setVisible(false);
};

JSDialog.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
};

JSDialog.prototype.fireActionListeners = function(e) {
    for (var i = 0; i < this.listeners.length; i++) {
        this.listeners[i].actionPerformed(e);
    }
};

JSDialog.prototype.getTarget = function() {
    return this.target;
};

JSDialog.prototype.getTargetCoordinates = function(pt) {
    var x = pt.x;
    var y = pt.y;
    var c = this.target.element;
    while (c && c !== this.body) {
        x -= c.offsetLeft;
        y -= c.offsetTop;
        c = c.offsetParent;
    }
    return new Point(x, y);
};

JSDialog.prototype.getWindowCoordinates = function(pt) {
    var x = pt.x;
    var y = pt.y;
    var c = this.target.element;
    while (c && c !== this.body) {
        x += c.offsetLeft;
        y += c.offsetTop;
        c = c.offsetParent;
    }
    return new Point(x, y);
};

JSDialog.prototype.setTitle = function(title) {
    /* Empty */
};

JSDialog.prototype.setVisible = function(flag) {
    if (flag !== this.visible) {
        this.visible = flag;
        if (flag) {
            this.body.appendChild(this.glassPane);
        } else {
            this.body.removeChild(this.glassPane);
        }
    }
};

JSDialog.prototype.setLocation = function(x, y) {
    if (typeof(x) === "number") {
        this.location = new Point(x, y);
    } else {
        this.location = x;
    }
};

JSDialog.prototype.centerOnParent = function() {
    var psize = this.target.getSize();
    var dsize = this.getSize();
    var x = (psize.width - dsize.width) / 2;
    var y = (psize.height - dsize.height) / 2;
    this.setLocation(x, y);
//    Used to be -- Why did this change?
//    var pt = jslib.getScreenCoordinates({ x: x, y: y }, this.target.element);
//    this.setLocation(pt.x, pt.y);
};

/* JSErrorDialog.js */

var JSErrorDialog = function(target) {
    if (!(this instanceof JSErrorDialog)) return new JSErrorDialog(target);
    JSDialog.call(this, target);
    var element = this.element;
    element.innerHTML =
        "<table class=JSErrorTable cellPadding=0 cellSpacing=0 border=0 " +
        "bgcolor=" + JSErrorDialog.ERROR_BACKGROUND.getColorTag() + ">" +
        "<tr><td><img src='" + JSErrorDialog.BUG_IMAGE + "' width=44></td>" +
        "<td valign=middle><div class=JSErrorMessage></div></td>" +
        "<td>&nbsp;</td></tr></table>";
    this.errorTable = element.getElementsByClassName("JSErrorTable")[0];
    this.errorMessage = element.getElementsByClassName("JSErrorMessage")[0];
    var listener = new JSErrorDialogListener(this);
    this.addMouseListener(listener);
    this.maxWidth = JSErrorDialog.DEFAULT_MAX_WIDTH;
};

JSErrorDialog.prototype =
    jslib.inheritPrototype(JSDialog, "JSErrorDialog extends JSDialog");
JSErrorDialog.prototype.constructor = JSErrorDialog;
JSErrorDialog.prototype.$class = new Class("JSErrorDialog", JSErrorDialog);

JSErrorDialog.prototype.setErrorMessage = function(msg) {
    this.msg = msg;
};

JSErrorDialog.prototype.setMaxWidth = function(width) {
    this.maxWidth = width;
};

JSErrorDialog.prototype.getMaxWidth = function() {
    return this.maxWidth;
};

// This original version of the JSErrorDialog mechanism that added the
// dialog to the glass pane doesn't work for most applications, because
// the dialog doesn't scroll with the page.  This rewrite adds it to
// the target element (or the parent of the target element if the target
// is a canvas) and then centers the dialog in that space.  At some point,
// this whole mechanism needs to be rethought.

JSErrorDialog.prototype.setVisible = function(flag) {
    if (flag !== this.visible) {
        this.visible = flag;
        if (flag) {
            this.dialogParent = this.target.element;
            if (this.dialogParent.nodeName === "CANVAS") {
                this.dialogParent = this.dialogParent.offsetParent;
                this.dialogParent.style.position = "relative";
            }
            this.dialogParent.appendChild(this.element);
            this.errorMessage.innerHTML = this.msg;
            var tw = this.dialogParent.offsetWidth;
            var th = this.dialogParent.offsetHeight;
	    var ew = this.element.offsetWidth;
	    var eh = this.element.offsetHeight;
            var x = (tw - ew) / 2;
            var y = (th - eh) / 2;
            this.errorTable.style.left = x + "px";
            this.errorTable.style.top = y + "px";
            this.errorTable.style.position = "absolute";
        } else {
            if (this.dialogParent) {
                this.dialogParent.removeChild(this.element);
            }
        }
    }
};

JSErrorDialog.ERROR_BACKGROUND = new Color(0xFFCC33);
JSErrorDialog.DEFAULT_MAX_WIDTH = 500;
JSErrorDialog.BUG_IMAGE =
"data:image/png;base64," +
"iVBORw0KGgoAAAANSUhEUgAAACwAAAAsCAYAAAAehFoBAAADGUlEQVR42u2Z" +
"PU7DQBCFLSRKEJR0iAPQUNJyBSRKCkSHRMcJkOhoEBIVBVS5AlfIFbgCV1j0" +
"YT3lZbxO1ontCMFKKyfrn/0882b2x9U0peo31eof+C8APzykdHeX0mSS0mDA" +
"5+cpUels1WcAWVV1vb5O6ehoIOCzs5SoANPZ/n53cO4X6NdX+ik8ZxBgOgKQ" +
"8vlZu5M2IEruPzmZWVbPQQ78H9TCXj4+apcCUworC1M4DgaMRXGf3AosBdcC" +
"1OZaSYjKy3H/y0t9n9oGCzq3ktzq0LFzrvF7vOgchugdGItgqdPTWeeylAII" +
"XWNll4fDUhVoHLm2RA4rAUsGz8/zAKqSh6zGS+ie3HWSCdf1CqxA4zcPp0PP" +
"ozl5IAs/x38NEFiWZygt9j7SeVDQofQWoZAB0NRo2ah1SaFLDu8sh9zvnDRy" +
"FferKL0tS4Nra5hO/ahAXASKBfEK13nO7SKFlYEVHByxDpaWBNqAFWAOW5oV" +
"esnDx8dNKIHngD0/qw1rjwa8tzcf+QIF4vGxmRmQgwJsHdi1Jj/eOdYD7PKy" +
"mco4720HBxsGJvhkve3tvH6Vb6OENgK8rPqop8wwapag7u6WwWoK6mmPNg/A" +
"rtCdYdXR1tY83OFhE5hgQw4+rdRLeFuXwWMlWHWA5TREO4BPzJm5eTvXx7Yu" +
"maMzbNusynUKlKRAu6c0rE6J84ySyXsxsFYRi6xwfz8/wcGKEYpnxAHEay/A" +
"PHyZxhyMa7WgzGmakpuW9gJM58uWLYLFeq+vzYEjahoPtFn46mqNJRIgJTsx" +
"vJDr7+ammUE0S4vZIaZAzpFtFklv4YysdAXtXqDz9/eZFXkZzdaAavOAtD+d" +
"1t5qM1Yv+2L+cNe7a7VNBi4ZeYJj24DS+8Ze1PzbWw20s1M+Mmr5lJPGOFuk" +
"1cwLT0813MVF/d8zjDZXJJ9cwI8G7J0LLLe8cuDcjG5wWLQbg9i3s3I5mRfS" +
"wmBUYEBjp9pYcdjb22a6axusBgWOQQOsSyOmRO0mMcxv/JNBLq9GqZTsEW/0" +
"20b0gBasv+IrUu+bgf+fvf4K8Ddxu3rpl8K1pQAAAABJRU5ErkJggg==";

var JSErrorDialogListener = function(dialog) {
    if (!(this instanceof JSErrorDialogListener)) {
        return new JSErrorDialogListener(dialog);
    }
    this.dialog = dialog;
};

JSErrorDialogListener.prototype.mouseClicked = function(e) {
    this.dialog.setVisible(false);
};

JSErrorDialogListener.prototype.mouseEntered = function(e) {
    /* Empty */
};

JSErrorDialogListener.prototype.mouseExited = function(e) {
    /* Empty */
};

JSErrorDialogListener.prototype.mousePressed = function(e) {
    /* Empty */
};

JSErrorDialogListener.prototype.mouseReleased = function(e) {
    /* Empty */
};

/* JSFileDialog.js */

var JSFileDialog = function(target) {
    if (!(this instanceof JSFileDialog)) return new JSFileDialog(target);
    JSDialog.call(this, target);
    this.fileList = new JSDirectoryTree(this);
    this.setLayout(new BorderLayout());
    this.titleBar = new JSTitleBar("File Dialog");
    this.add(this.titleBar, BorderLayout.NORTH);
    this.add(this.fileList, BorderLayout.CENTER);
    this.setSize(JSFileDialog.DIALOG_WIDTH, JSFileDialog.DIALOG_HEIGHT);
    this.element.style.backgroundColor = "white";
    this.element.style.border = "solid 1px #666666";
};

JSFileDialog.prototype =
    jslib.inheritPrototype(JSDialog, "JSFileDialog extends JSDialog");
JSFileDialog.prototype.constructor = JSFileDialog;
JSFileDialog.prototype.$class = new Class("JSFileDialog", JSFileDialog);

JSFileDialog.prototype.setTitle = function(title) {
    this.titleBar.setTitle(title);
};

JSFileDialog.prototype.getTitle = function() {
    return this.titleBar.getTitle();
};

JSFileDialog.prototype.setDirectory = function(dir) {
    this.dir = dir;
    this.ready = false;
    this.waiting = false;
    this.selectedFile = null;
    this.directoryReader = new JSDirectoryReader(this, dir);
};

JSFileDialog.prototype.getDirectory = function() {
    return this.dir;
};

JSFileDialog.prototype.getTree = function() {
    return this.fileList;
};

JSFileDialog.prototype.setVisible = function(flag) {
    if (flag !== this.visible) {
        this.visible = flag;
        if (flag) {
            this.selectedFile = null;
            this.fileList.select(-1, null);
            if (this.ready) {
                this.body.appendChild(this.glassPane);
                var origin = this.getWindowCoordinates(new Point(0, 0));
                this.element.style.left = (this.location.x + origin.x) + "px";
                this.element.style.top = (this.location.y + origin.y) + "px";
                this.element.style.position = "absolute";
                this.element.style.zIndex = 11;
                this.titleBar.repaint();
            } else {
                this.waiting = true;
            }
        } else {
            this.body.removeChild(this.glassPane);
        }
    }
};

JSFileDialog.prototype.getSelectedFile = function() {
    if (this.selectedFile === null) {
        var filename = this.fileList.getSelectedValue();
        if (filename !== null) {
            this.selectedFile = this.dir.getPath() + "/" + filename;
        }
    }
    return this.selectedFile;
};

JSFileDialog.prototype.setSelectedFile = function(value) {
    this.selectedFile = value;
};

JSFileDialog.prototype.setControlPanel = function(panel) {
    this.controlPanel = panel;
    this.add(this.controlPanel, BorderLayout.SOUTH);
    this.setSize(JSFileDialog.DIALOG_WIDTH, JSFileDialog.DIALOG_HEIGHT);
};

JSFileDialog.prototype.getControlPanel = function() {
    return this.controlPanel;
};

JSFileDialog.prototype.signalError = function(msg) {
    // Fill in
};

JSFileDialog.DIALOG_WIDTH = 400;
JSFileDialog.DIALOG_HEIGHT = 250;

/* JSLoadDialog */

var JSLoadDialog = function(target) {
    if (!(this instanceof JSLoadDialog)) return new JSLoadDialog(target);
    JSFileDialog.call(this, target);
    this.setTitle("Load File");
    this.controlListener = new JSLoadDialogListener(this);
    this.setControlPanel(this.createDefaultPanel());
//   this.getTree().addActionListener(this.controlListener);
//   this.getTree().setActionCommand("Load");
};

JSLoadDialog.prototype =
    jslib.inheritPrototype(JSFileDialog, "JSLoadDialog extends JSFileDialog");
JSLoadDialog.prototype.constructor = JSLoadDialog;
JSLoadDialog.prototype.$class = new Class("JSLoadDialog", JSLoadDialog);

JSLoadDialog.prototype.createDefaultPanel = function () {
    var tree = this.getTree();
    var panel = new JSPanel();
    panel.setLayout(new FlowLayout(FlowLayout.RIGHT));
    var cancelButton = new JButton("Cancel");
    var loadButton = new JButton("Load");
    this.approveButton = loadButton;
    cancelButton.addActionListener(this.controlListener);
    loadButton.addActionListener(this.controlListener);
    panel.add(cancelButton);
    panel.add(loadButton);
    return panel;
};

JSLoadDialog.prototype.getControlListener = function() {
    return this.controlListener;
};

var JSLoadDialogListener = function(dialog) {
    this.dialog = dialog;
};

JSLoadDialogListener.prototype.actionPerformed = function(e) {
    var cmd = e.getActionCommand();
    if (cmd === "Cancel") {
        this.dialog.setVisible(false);
        this.dialog.execute("");
    } else if (cmd === "Load") {
        cmd = this.dialog.getSelectedFile();
        this.dialog.execute((cmd === null) ? "" : cmd);
    }
};

/* JSSaveDialog */

var JSSaveDialog = function(target) {
    if (!(this instanceof JSSaveDialog)) return new JSSaveDialog(target);
    JSFileDialog.call(this, target);
    this.setTitle("Save File");
    this.controlListener = new JSSaveDialogListener(this);
    this.setControlPanel(this.createDefaultPanel());
//   this.getTree().addActionListener(this.controlListener);
//   this.getTree().setActionCommand("Save");
};

JSSaveDialog.prototype =
    jslib.inheritPrototype(JSFileDialog, "JSSaveDialog extends JSFileDialog");
JSSaveDialog.prototype.constructor = JSSaveDialog;
JSSaveDialog.prototype.$class = new Class("JSSaveDialog", JSSaveDialog);

JSSaveDialog.prototype.createDefaultPanel = function () {
    var panel = new JSPanel();
    this.fileField = new JTextField();
    this.fileField.setActionCommand("Save");
    this.fileField.addActionListener(this.controlListener);
    this.fileField.setPreferredSize(100, 20);
    var filePanel = new JSPanel();
    filePanel.setLayout(new BorderLayout());
    filePanel.add(this.fileField, BorderLayout.CENTER);
    filePanel.add(new JLabel("  "), BorderLayout.EAST);
    var buttons = new JSPanel();
    panel.setLayout(new BorderLayout());
    panel.setPreferredSize(100, 28);
    buttons.setLayout(new FlowLayout(FlowLayout.RIGHT));
    var cancelButton = new JButton("Cancel");
    var saveButton = new JButton("Save");
    cancelButton.addActionListener(this.controlListener);
    saveButton.addActionListener(this.controlListener);
    buttons.add(cancelButton);
    buttons.add(saveButton);
    panel.add(filePanel, BorderLayout.CENTER);
    panel.add(buttons, BorderLayout.EAST);
    return panel;
};

JSSaveDialog.prototype.getControlListener = function() {
    return this.controlListener;
};

JSSaveDialog.prototype.getSelectedFile = function () {
    var name = this.fileField.getText();
    if (name === null || name === "") {
        name = this.fileList.getSelectedValue();
    }
    return this.getDirectory().getPath() + "/" + name;
}

var JSSaveDialogListener = function(dialog) {
    this.dialog = dialog;
};

JSSaveDialogListener.prototype.actionPerformed = function(e) {
    var cmd = e.getActionCommand();
    if (cmd === "Cancel") {
        this.dialog.setVisible(false);
        this.dialog.execute("");
    } else if (cmd === "Save") {
        cmd = this.dialog.getSelectedFile();
        this.dialog.execute((cmd === null) ? "" : cmd);
    }
};

/* JSDownloadDialog */

var JSDownloadDialog = function(target, title) {
    if (!(this instanceof JSDownloadDialog)) {
        return new JSDownloadDialog(target, title);
    }
    JSDialog.call(this, target);
    if (title === undefined) title = "Download";
    this.setLayout(new BorderLayout());
    this.titleBar = new JSTitleBar(title);
    this.downloadCallback = function(dialog) { };
    this.element.style.backgroundColor = "white";
    this.element.style.border = "solid 1px #666666";
    this.add(this.titleBar, BorderLayout.NORTH);
    this.add(this.createDefaultPanel(), BorderLayout.CENTER);
    this.setSize(JSDownloadDialog.DIALOG_WIDTH,
                 JSDownloadDialog.DIALOG_HEIGHT);
};

JSDownloadDialog.prototype =
    jslib.inheritPrototype(JSDialog, "JSDownloadDialog extends JSDialog");
JSDownloadDialog.prototype.constructor = JSDownloadDialog;
JSDownloadDialog.prototype.$class =
    new Class("JSDownloadDialog", JSDownloadDialog);

JSDownloadDialog.DIALOG_WIDTH = 240;
JSDownloadDialog.DIALOG_HEIGHT = 90;

JSDownloadDialog.prototype.setVisible = function(flag) {
    if (flag !== this.visible) {
        this.visible = flag;
        if (flag) {
            this.body.appendChild(this.glassPane);
            var origin = this.getWindowCoordinates(new Point(0, 0));
            this.element.style.left = (this.location.x + origin.x) + "px";
            this.element.style.top = (this.location.y + origin.y) + "px";
            this.element.style.position = "absolute";
            this.element.style.zIndex = 11;
            this.titleBar.repaint();
        } else {
            this.body.removeChild(this.glassPane);
        }
    }
};

JSDownloadDialog.prototype.createDefaultPanel = function() {
    var dialog = this;
    var panel = new JSPanel();
    var filePanel = new JSPanel();
    filePanel.setLayout(new FlowLayout(FlowLayout.CENTER));
    this.fileField = new JTextField();
    this.fileField.setActionCommand("Save");
    this.fileField.addActionListener(dialogAction);
    this.fileField.setPreferredSize(150, 24);
    this.fileField.element.style.font = "16px 'Helvetica Neue','Sans-Serif'";
    filePanel.add(this.fileField);
    var buttons = new JSPanel();
    panel.setLayout(new BorderLayout());
    buttons.setLayout(new FlowLayout(FlowLayout.CENTER));
    var cancelButton = new JButton("Cancel");
    var saveButton = new JButton("Save");
    cancelButton.addActionListener(dialogAction);
    cancelButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    cancelButton.setPreferredSize(75, 24);
    saveButton.addActionListener(dialogAction);
    saveButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    saveButton.setPreferredSize(72, 24);
    buttons.add(cancelButton);
    buttons.add(saveButton);
    panel.add(filePanel, BorderLayout.CENTER);
    panel.add(buttons, BorderLayout.SOUTH);
    return panel;

    function dialogAction(e) {
        if (e == "Save") {
            dialog.downloadCallback(dialog);
        } else {
            dialog.cancelCallback(dialog);
        }
        dialog.setVisible(false);
    }

};

JSDownloadDialog.prototype.setCancelCallback = function(fn) {
    this.cancelCallback = fn;
};

JSDownloadDialog.prototype.setDownloadCallback = function(fn) {
    this.downloadCallback = fn;
};

JSDownloadDialog.prototype.getFilename = function() {
    return this.fileField.getText();
}

JSDownloadDialog.prototype.setFilename = function(filename) {
    this.fileField.setText(filename);
}

var JSDirectoryReader = function(dialog, dir) {
    this.dialog = dialog;
    dir.readDirectoryTree(this);
};

JSDirectoryReader.prototype.actionPerformed = function(e) {
    var cmd = jslib.trim(e.getActionCommand());
    var dialog = this.dialog;
    if (e instanceof JSErrorEvent) {
        dialog.signalError(cmd);
    } else {
        var fileList = this.dialog.getTree();
        fileList.parseDirectoryTree(cmd);
        fileList.update();
        dialog.ready = true;
        if (dialog.waiting) dialog.setVisible(true);
    }
};

var JSDirectoryTree = function(dialog) {
    JComponent.call(this);
    this.listeners = [];
    this.actionCommand = "";
    this.dialog = dialog;
    this.selectedIndex = -1;
    this.expanded = false;
    this.element.style.overflowX = "auto";
    this.element.style.overflowY = "auto";
    this.element.style.backgroundColor = "white";
};

JSDirectoryTree.prototype =
    jslib.inheritPrototype(JComponent, "JSDirectoryTree extends JComponent");
JSDirectoryTree.prototype.constructor = JSDirectoryTree;
JSDirectoryTree.prototype.$class = new Class("JSDirectoryTree",
                                             JSDirectoryTree);

JSDirectoryTree.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
};

JSDirectoryTree.prototype.fireActionListeners = function() {
    var e = null;
    for (var i in this.listeners) {
        var listener = this.listeners[i];
        if (typeof(listener) === "function") {
            listener(this.actionCommand);
        } else {
            if (!e) e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                                        this.actionCommand);
            listener.actionPerformed(e);
        }
    }
};

JSDirectoryTree.prototype.setActionCommand = function(cmd) {
    this.actionCommand = cmd;
};

JSDirectoryTree.prototype.getActionCommand = function() {
    return this.actionCommand;
};

JSDirectoryTree.prototype.getSelectedIndex = function() {
    return this.selectedIndex;
};

JSDirectoryTree.prototype.getSelectedValue = function() {
    var file = null;
    if (this.selectedIndex !== -1) {
        file = this.array[this.selectedIndex];
        if (this.selectedIndex > this.boundary) {
            file = "examples/" + file;
        }
    }
    return file;
};

JSDirectoryTree.prototype.add = function(name) {
    this.files.push(name);
    this.update();
};

// This implementation is a stub that allows only one subdirectory

JSDirectoryTree.prototype.parseDirectoryTree = function(text) {
    var lines = JSPlatform.splitLines(text);
    var line = "";
    var n = lines.length;
    this.examples = null;
    this.files = [];
    for (var i = 0; i < n; i++) {
        line = lines[i];
        if (jslib.endsWith(line, "/")) {
            this.examples = [];
        } else if (jslib.startsWith(line, ":")) {
            this.examples.push(line.substring(1));
        } else {
            this.files.push(line);
        }
    }
};

JSDirectoryTree.prototype.update = function() {
    var table = document.createElement("table");
    table.style.border = "none";
    table.style.backgroundColor = "white";
    var n = this.files.length;
    this.files.sort();
    this.array = [];
    for (var i = 0; i < n; i++) {
        this.array.push(this.files[i]);
    }
    if (this.examples !== null) {
        this.boundary = n++;
        this.array.push("examples");
        if (this.expanded) {
            this.examples.sort();
            for (var i = 0; i < this.examples.length; i++) {
                this.array.push(this.examples[i]);
            }
            n += this.examples.length;
        }
    }
    for (var i = 0; i < n; i++) {
        table.appendChild(this.createTableEntry(i));
    }
    while (this.element.firstChild) {
        this.element.removeChild(this.element.firstChild);
    }
    this.element.appendChild(table);
};

JSDirectoryTree.prototype.select = function(k, td) {
    if (this.selectedIndex !== -1) {
        this.selectedEntry.style.backgroundColor = "white";
    }
    if (k === this.boundary) {
        this.expanded = !this.expanded;
        this.update();
    } else {
        this.selectedIndex = k;
        this.selectedEntry = td;
        if (td !== null) {
            td.style.backgroundColor = JSDirectoryTree.SELECTED_COLOR;
        }
    }
};

JSDirectoryTree.prototype.createTableEntry = function(k) {
    var list = this;
    var tr = document.createElement("tr");
    var td = document.createElement("td");
    td.style.width = JSLoadDialog.DIALOG_WIDTH + "px";
    td.style.backgroundColor = "white";
    td.style.fontFamily = "arial","helvetica","sans-serif";
    td.style.fontSize = "0.9em";
    var mousedown = function(e) {
        if (k === list.boundary) {
            list.expanded = !list.expanded;
            list.update();
        } else {
            list.select(k, td);
        }
    };
    var dblclick = function(e) {
        if (k !== list.boundary) list.fireActionListeners();
    };
    jslib.addListener(td, "mousedown", mousedown);
    jslib.addListener(td, "dblclick", dblclick);
    var leader = JSDirectoryTree.SPACER;
    var icon = JSDirectoryTree.FILE_ICON;
    if (k === list.boundary) {
        icon = JSDirectoryTree.FOLDER_CLOSED;
        if (list.expanded) {
            leader = JSDirectoryTree.TRIANGLE_DOWN;
        } else {
            leader = JSDirectoryTree.TRIANGLE_RIGHT;
        }
    }
    var html = "<img src='" + leader + "' alt='folder' />";
    if (k > list.boundary) {
        html += "<img src='" + leader + "' alt='folder' />";
    }
    html += "<img src='" + icon + "' alt='folder' />" +
            "<span style='vertical-align: 10%;'>&nbsp;" +
            list.array[k] + "</span>";
    td.innerHTML = html;
    tr.appendChild(td);
    return tr;
};

JSDirectoryTree.SELECTED_COLOR = "Highlight";
JSDirectoryTree.FILE_ICON =
    "data:image/png;base64," +
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAASCAYAAABb0P4QAAABKElEQVR42q2S" +
    "zYuCQByG+/c9eBC6dQ6zLp1NNKUPhD168VIgUYiglGKR2Lu8A7sH3Z0xdgce" +
    "kJ/PPMyIow9g9J/0BlEUYb/f/8rbQW46n88/Mp/PldHeYLfb4XK54Hg89phO" +
    "pyiKAnTeCqZpiiRJesxmM9zvd2m0N9hut8iyDKfTqYdlWWjb9jtKVxncbDbI" +
    "81xcu8tisYCmaQJd10FXGQyCANfrVZyyC08Zx7HgcDiArjLo+z6qqhJX6sKT" +
    "f1GWJegqg+v1Wnyj2+0mhQ5dZdDzPDyfT9R1LYUOXWXQdV00TYPH4yGFDl1l" +
    "0HEc8Wtwgww6dJXB1WoFrtfrJYWLrjJo2/bgIN3BwSFrUDAMQyyXS5imiclk" +
    "gvF4DMMwBHzmjO/o0FUG/8onyCmD8X0UFykAAAAASUVORK5CYII=";

JSDirectoryTree.FOLDER_CLOSED =
    "data:image/png;base64," +
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAAQCAYAAAAWGF8bAAACQ0lEQVR42q2R" +
    "/07TUBiGuU+8Aw0xCqgx6PihCUYEndvIQAMCY3RjY8gQ2MRNNge00E22ruva" +
    "rrVdBhrRyAW8nNN2i6RLTIh/PHnP+XLeJ19yejig539y5bLzhYdnbuMKdHZt" +
    "IRUs5iSLBSfp7FpC72oKd30RzGyxmN3mOvT7IxYDDoMO9xzo7FU0BZfwwXQE" +
    "RfMCRaPNb5SaF+C//QGr/QLbOEdBPUdW+YGM9B27tTOkhVOkqz+JeKW7MCue" +
    "IlNpOhhW7pZN7HzVkSrp2OJ1rPMNJI5UxFkF0QMZsUOdbMx0EQZW8J5vYoOU" +
    "NilFHUlLoGHtqIE418AqKyN6qCBCRCv7MsL7EkIFrfuG9wMMYryJBKeSDRqW" +
    "ZI2jIhWrhypiB4otKchYJoTydSzlyecR4aCvi5AOI6wJhpSYA5u2IFSwy/bv" +
    "1zC/J2LuM4FmTkW/L+wW9vuWMZvT8G7PLsw7SUtvSXk2I+BNpoqZTwKCfxHI" +
    "qhjwhtzCO95l+HclTH+sWAQc/OkKfOkyXqdOCGULb6p9P8HLtIjbpOsS3ppa" +
    "wvi2gIkPJbxos1nChMPzZAnjyaKVFtaMx7OtCmi3q3B4XcBYgsNo4hjDhNF1" +
    "knGScfs8QnhKeEIYo2f6Ling5mQ34eQiHkaP4WFyeMTkMUSwMkwynO/MPA6P" +
    "nRyKHYN2XcK+qQX0jgTROxrEjX/Q22bEpm9ywS1sts7QarVgmiZ0XYeqqpBl" +
    "GZIkQRRFCIKAarWKWq2Ger0ORVGgaRoMw4BumB3hJRx/xEMGKdVJAAAAAElF" +
    "TkSuQmCC";

JSDirectoryTree.FOLDER_OPEN =
    "data:image/png;base64," +
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAASCAYAAABb0P4QAAACR0lEQVR42rWU" +
    "cU8SYRzHeZ/2DmqulVqtWYjWZsu0CHBKTVMRAUFMDIUIEkK9k4OEA4674647" +
    "htayli/g2/PcHa7GbW0u//js+zy/3fez3/Zs52ABx//EcaXC3U8cnAtbf0Fn" +
    "lxZSwXJeMFiyks4uJXSvp3DbE8Z8koF/h71gyBs2GLYYsbhjQWcvIin0Ce/N" +
    "hlHWz1HWevxEpXMO7ssvMMoPMO0zFOUz5KRvyApfkWmeIs2fIF3/TsRr9sJc" +
    "4wTZWsdCMzJT1bH7WUWqoiLJqdjk2ogfyYgxEiIHIqKHKtk4ZCP0reEt18EW" +
    "KW1TyioShkDBxlEbMbaNdUZE5FBCmIjW9kUE9wUEior9hnd9IUQ5HXFWJhu0" +
    "DckGS0Uy1g9lRA8kU1IUsUoIFFpYKZDHI8IRj42QDsOMjhAphQ5MeoJA0Syb" +
    "r9/E4l4DCx8JNPMyhjzBfuGQZxX+vII3e2Zh0Upaek3K/iyPV9k65j/wmPsD" +
    "X07GsDvQL7zlXoU3I2D2fc3AZ+FN1+BJV/EydUyoGrhTvfsxnqcbuEm6fcIb" +
    "MyuY3OEx9a6CZz22K5iyeJqoYDJRNtLAmHF4kqyBdm2FY5s8JuIsxuMljBHG" +
    "N0nGSMbMs4vwmPCIMEHP9LsEj+vTdsLpZdyPlOAM5fEgVMAowcggyWDhYua0" +
    "eGjlaLQE2u0TDs4sYcA1h4HxOVz7BwM9XCaD00v9wk73FN1uF7quQ1VVyLIM" +
    "URQhCAIajQZ4nke9Xkez2USr1YIkSVAUBZqmQdV0XNn/8DctJzPiO8LmFwAA" +
    "AABJRU5ErkJggg==";

JSDirectoryTree.SPACER =
    "data:image/png;base64," +
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAASCAYAAABb0P4QAAAAHklEQVR42mM4" +
    "8/8/AzUxw6iBowaOGjhq4KiBtDEQAJU17D60KeKFAAAAAElFTkSuQmCC";

JSDirectoryTree.TRIANGLE_DOWN =
    "data:image/png;base64," +
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAASCAYAAABb0P4QAAAAdUlEQVR42u3T" +
    "MQrAIAwFUE/v5iC4ODm6egbnrA6C4JRzfMGtpS1YpVBw+OuD5CeCALEyYoM/" +
    "ArXWkFLexlqLITCl9AjWWjE8svf+Egsh4PUOz5hSClOlxBgPIBFhumVjTMec" +
    "c1hyNqWUDjIzlt1hzhn79b4BG3ST6z8NEG2zAAAAAElFTkSuQmCC";

JSDirectoryTree.TRIANGLE_RIGHT =
    "data:image/png;base64," +
    "iVBORw0KGgoAAAANSUhEUgAAABQAAAASCAYAAABb0P4QAAAAYklEQVR42mM4" +
    "8/8/AzUxw6iBtDVw0uTJ/y9eufafaga2tbX9B+EVK1f9p6qBINze0fH/wKEj" +
    "/6lmIAxPmzb9/83b9/5TzUAQbmlp+X/p6vX/g8uFVA1DqsUy1dPhaOFAFAYA" +
    "j3HqjtXcj+8AAAAASUVORK5CYII=";

var JSDirectoryTreeListener = function(dialog) {
    this.dialog = dialog;
};

JSDirectoryTreeListener.prototype.mouseClicked = function(e) {
    if (e.getClickCount() === 2) this.dialog.execute("Load");
};

JSDirectoryTreeListener.prototype.mouseEntered = function(e) {
    /* Empty */
};

JSDirectoryTreeListener.prototype.mouseExited = function(e) {
    /* Empty */
};

JSDirectoryTreeListener.prototype.mousePressed = function(e) {
    /* Empty */
};

JSDirectoryTreeListener.prototype.mouseReleased = function(e) {
    /* Empty */
};

/* JSLoadCookieDialog */

var JSLoadCookieDialog = function(chooser) {
    if (!(this instanceof JSLoadCookieDialog)) {
        return new JSLoadCookieDialog(chooser);
    }
    JSDialog.call(this, chooser.target);
    this.chooser = chooser;
    this.setLayout(new BorderLayout());
    this.titleBar = new JSTitleBar("Load File");
    this.element.style.backgroundColor = "white";
    this.element.style.border = "solid 1px #666666";
    this.add(this.titleBar, BorderLayout.NORTH);
    this.add(this.createDefaultPanel(), BorderLayout.CENTER);
    this.setSize(JSLoadCookieDialog.DIALOG_WIDTH,
                 JSLoadCookieDialog.DIALOG_HEIGHT);
};

JSLoadCookieDialog.prototype =
    jslib.inheritPrototype(JSDialog, "JSLoadCookieDialog extends JSDialog");
JSLoadCookieDialog.prototype.constructor = JSLoadCookieDialog;
JSLoadCookieDialog.prototype.$class =
    new Class("JSLoadCookieDialog", JSLoadCookieDialog);

JSLoadCookieDialog.DIALOG_WIDTH = 400;
JSLoadCookieDialog.DIALOG_HEIGHT = 300;

JSLoadCookieDialog.prototype.setVisible = function(flag) {
    if (flag !== this.visible) {
        this.visible = flag;
        if (flag) {
            this.body.appendChild(this.glassPane);
            var origin = this.getWindowCoordinates(new Point(0, 0));
            this.element.style.left = (this.location.x + origin.x) + "px";
            this.element.style.top = (this.location.y + origin.y) + "px";
            this.element.style.position = "absolute";
            this.element.style.zIndex = 11;
            this.titleBar.repaint();
        } else {
            this.body.removeChild(this.glassPane);
        }
    }
};

JSLoadCookieDialog.prototype.createDefaultPanel = function() {
    var dialog = this;
    var chooser = dialog.chooser;
    var panel = new JSPanel();
    var buttons = new JSPanel();
    var filePanel = new JSPanel();
    var fileElement = filePanel.element;
    fileElement.style.marginLeft = 1;
    fileElement.style.marginTop = 1;
    fileElement.style.position = "absolute";
    fileElement.style.whiteSpace = "pre";
    fileElement.style.overflow = "auto";
    fileElement.style.font = "12px 'Lucida Grande','Sans-Serif'";
    dialog.fileElement = fileElement;
    panel.setLayout(new BorderLayout());
    buttons.setLayout(new FlowLayout(FlowLayout.CENTER));
    var cancelButton = new JButton("Cancel");
    var loadButton = new JButton("Load");
    var uploadButton = new JButton("Upload");
    cancelButton.addActionListener(dialogAction);
    cancelButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    cancelButton.setPreferredSize(75, 24);
    dialog.dialogAction = dialogAction;
    loadButton.addActionListener(dialogAction);
    loadButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    loadButton.setPreferredSize(72, 24);
    uploadButton.addActionListener(dialogAction);
    uploadButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    uploadButton.setPreferredSize(80, 24);
    buttons.add(cancelButton);
    buttons.add(loadButton);
    buttons.add(uploadButton);
    panel.add(filePanel, BorderLayout.CENTER);
    panel.add(buttons, BorderLayout.SOUTH);
    return panel;

    function dialogAction(e) {
        if (e === "Load") {
            if (dialog.selectedFile === null) {
                chooser.setPath(null);
            } else {
                chooser.setPath(dialog.selectedFile);
                var text = JSCookie.get(chooser.prefix + dialog.selectedFile);
                if (text !== null) {
                    e = JSEvent.createActionEvent(chooser, text);
                    chooser.fireLoadListeners(e);
                }
            }
            dialog.setVisible(false);
        } else if (e === "Upload") {
            dialog.setVisible(false);
            dialog.uploadFile(chooser);
        } else {
            dialog.setVisible(false);
        }
    }
};

JSLoadCookieDialog.prototype.uploadFile = function(chooser) {

    var dialog = document.getElementById("HTMLDialog");
    if (!dialog) {
        dialog = document.createElement("input");
        dialog.setAttribute("style",
                            "visibility:hidden;" +
                            "position:fixed;" +
                            "bottom:-10;");
        dialog.setAttribute("type", "file");
        dialog.setAttribute("id", "HTMLDialog");
        dialog.addEventListener("change", fileSelected, false);
        document.body.appendChild(dialog);
    }
    dialog.click();

    function fileSelected(e) {
        var file = e.target.files[0];
        chooser.path = file.name;
        var rd = new FileReader();
        rd.onload = fireLoadListeners;
        rd.readAsText(file);
        function fireLoadListeners() {
            var e = JSEvent.createActionEvent(chooser, rd.result);
            chooser.fireLoadListeners(e);
        }
    }

};

JSLoadCookieDialog.prototype.setFileList = function(list) {
    this.files = list;
    this.updateFileList();
};

JSLoadCookieDialog.prototype.updateFileList = function() {
    var dialog = this;
    dialog.fileElement.innerHTML = "";
    for (var i = 0; i < dialog.files.length; i++) {
        dialog.fileElement.appendChild(createDiv(dialog.files[i]));
    }

    function createDiv(filename) {
        var div = document.createElement("div");
        if (filename === dialog.selectedFile) {
            div.style.backgroundColor = JSLoadCookieDialog.SELECTED_COLOR;
        }
        div.innerHTML = filename;
        div.addEventListener("click", selectFile);
        div.addEventListener("dblclick", loadFile);
        return div;

        function selectFile(e) {
            dialog.setSelectedFile(filename);
        }

        function loadFile(e) {
            dialog.dialogAction("Load");
        }
    }
        
};

JSLoadCookieDialog.prototype.setSelectedFile = function(name) {
    this.selectedFile = name;
    this.updateFileList();
};

JSLoadCookieDialog.SELECTED_COLOR = "Highlight";

/* JSSaveCookieDialog */

var JSSaveCookieDialog = function(target, title) {
    if (!(this instanceof JSSaveCookieDialog)) {
        return new JSSaveCookieDialog(target, title);
    }
    JSDialog.call(this, target);
    if (title === undefined) title = "Save File";
    this.setLayout(new BorderLayout());
    this.titleBar = new JSTitleBar(title);
    this.downloadCallback = function(dialog) { };
    this.element.style.backgroundColor = "white";
    this.element.style.border = "solid 1px #666666";
    this.add(this.titleBar, BorderLayout.NORTH);
    this.add(this.createDefaultPanel(), BorderLayout.CENTER);
    this.setSize(JSSaveCookieDialog.DIALOG_WIDTH,
                 JSSaveCookieDialog.DIALOG_HEIGHT);
};

JSSaveCookieDialog.prototype =
    jslib.inheritPrototype(JSDialog, "JSSaveCookieDialog extends JSDialog");
JSSaveCookieDialog.prototype.constructor = JSSaveCookieDialog;
JSSaveCookieDialog.prototype.$class =
    new Class("JSSaveCookieDialog", JSSaveCookieDialog);

JSSaveCookieDialog.DIALOG_WIDTH = 300;
JSSaveCookieDialog.DIALOG_HEIGHT = 120;

JSSaveCookieDialog.prototype.setVisible = function(flag) {
    if (flag !== this.visible) {
        this.visible = flag;
        if (flag) {
            this.body.appendChild(this.glassPane);
            var origin = this.getWindowCoordinates(new Point(0, 0));
            this.element.style.left = (this.location.x + origin.x) + "px";
            this.element.style.top = (this.location.y + origin.y) + "px";
            this.element.style.position = "absolute";
            this.element.style.zIndex = 11;
            this.titleBar.repaint();
        } else {
            this.body.removeChild(this.glassPane);
        }
    }
};

JSSaveCookieDialog.prototype.createDefaultPanel = function() {
    var dialog = this;
    var panel = new JSPanel();
    var filePanel = new JSPanel();
    filePanel.setLayout(new FlowLayout(FlowLayout.CENTER));
    this.fileField = new JTextField();
    this.fileField.setActionCommand("Save");
    this.fileField.addActionListener(dialogAction);
    this.fileField.setPreferredSize(150, 24);
    this.fileField.element.style.font = "16px 'Helvetica Neue','Sans-Serif'";
    filePanel.add(this.fileField);
    var buttons = new JSPanel();
    panel.setLayout(new BorderLayout());
    buttons.setLayout(new FlowLayout(FlowLayout.CENTER));
    var cancelButton = new JButton("Cancel");
    var saveButton = new JButton("Save");
    var downloadButton = new JButton("Download");
    cancelButton.addActionListener(dialogAction);
    cancelButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    cancelButton.setPreferredSize(75, 24);
    saveButton.addActionListener(dialogAction);
    saveButton.element.style.font = "14px 'Lucida Grande','Helvetica Neue'";
    saveButton.setPreferredSize(72, 24);
    downloadButton.addActionListener(dialogAction);
    downloadButton.element.style.font =
        "14px 'Lucida Grande','Helvetica Neue'";
    downloadButton.setPreferredSize(90, 24);
    buttons.add(cancelButton);
    buttons.add(saveButton);
    buttons.add(downloadButton);
    panel.add(filePanel, BorderLayout.CENTER);
    panel.add(buttons, BorderLayout.SOUTH);
    return panel;

    function dialogAction(e) {
        if (e == "Save") {
            dialog.saveCallback(dialog);
        } else if (e == "Download") {
            dialog.downloadCallback(dialog);
        } else {
            dialog.cancelCallback(dialog);
        }
        dialog.setVisible(false);
    }

};

JSSaveCookieDialog.prototype.setCancelCallback = function(fn) {
    this.cancelCallback = fn;
};

JSSaveCookieDialog.prototype.setSaveCallback = function(fn) {
    this.saveCallback = fn;
};

JSSaveCookieDialog.prototype.setDownloadCallback = function(fn) {
    this.downloadCallback = fn;
};

JSSaveCookieDialog.prototype.getFilename = function() {
    return this.fileField.getText();
}

JSSaveCookieDialog.prototype.setFilename = function(filename) {
    this.fileField.setText(filename);
}

return {
    JSCanvas : JSCanvas,
    JSColor : JSColor,
    JSCookie : JSCookie,
    JSDialog : JSDialog,
    JSDownloadDialog : JSDownloadDialog,
    JSElementList : JSElementList,
    JSErrorDialog : JSErrorDialog,
    JSErrorEvent : JSErrorEvent,
    JSEvent : JSEvent,
    JSFile : JSFile,
    JSFileChooser : JSFileChooser,
    JSFileInteractor : JSFileInteractor,
    JSFont : JSFont,
    JSFrame : JSFrame,
    JSImage : JSImage,
    JSLineReader : JSLineReader,
    JSPackage : JSPackage,
    JSPanel : JSPanel,
    JSPlatform : JSPlatform,
    JSProgram : JSProgram,
    JSScrollPane : JSScrollPane,
    JSTitleBar : JSTitleBar
};

});
