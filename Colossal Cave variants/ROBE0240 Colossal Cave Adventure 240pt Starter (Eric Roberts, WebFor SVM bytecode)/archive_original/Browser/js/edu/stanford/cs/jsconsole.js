/*
 * File: jsconsole.js
 * ------------------
 * This package implements the JSConsole class.
 */

/* Standard header for requirejs */

define([ "jslib",
         "java/awt",
         "java/lang",
         "javax/swing" ],

function(jslib,
         java_awt,
         java_lang,
         javax_swing) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var addListener = jslib.addListener;
var quoteHTML = jslib.quoteHTML;
var ActionEvent = java_awt.ActionEvent;
var Color = java_awt.Color;
var Font = java_awt.Font;
var Class = java_lang.Class;
var JComponent = javax_swing.JComponent;
var LineBorder = javax_swing.LineBorder;

/* JSConsole.js */

var JSConsole = function() {
    if (!(this instanceof JSConsole)) return new JSConsole();
    JComponent.call(this);
    var console = this;
    console.buffer = null;
    var div = console.element;
    div.align = "left";
    div.style.overflowY = "auto";
    div.style.overflowX = "auto";
    div.style.background = "white";
    div.contentEditable = true;
    console.setBorder(new LineBorder(Color.BLACK, 1));
    console.margin = JSConsole.DEFAULT_SCROLL_MARGIN;
    console.nLines = 0;
    console.actionListeners = [];
    console.focusListeners = [];
    console.setFont(Font.decode("Monospaced-Bold-14"));
    addListener(div, "click", function(e) { console.click(e); });
    addListener(div, "keydown", function(e) { console.keydown(e); });
    addListener(div, "keypress", function(e) { console.keypress(e); });
    addListener(div, "focus", function(e) { console.focus(e); });
    addListener(div, "blur", function(e) { console.blur(e); });
    console.font.setStyleProperties(div);
};

JSConsole.prototype =
    jslib.inheritPrototype(JComponent, "JSConsole extends JComponent");
JSConsole.prototype.constructor = JSConsole;
JSConsole.prototype.$class = new Class("JSConsole", JSConsole);

JSConsole.prototype.isSwingComponent = function() {
    return true;
};

JSConsole.prototype.repaint = function() {
    /* Empty */
};

JSConsole.prototype.setFont = function(font) {
    if (typeof(font) === "string") font = new Font(font);
    this.font = font;
    if (this.element) font.setStyleProperties(this.element);
};

JSConsole.prototype.toString = function() {
    var str = "JConsole:";
    var div = this.element;
    if (div === null) return str + "(?)";
    return str + "(" + div.clientWidth + "x" + div.clientHeight + ")";
};

JSConsole.prototype.print = function(str) {
    if (this.buffer === null) {
        this.element.innerHTML += quoteHTML("" + str);
        this.scrollToEnd();
    } else {
        this.buffer += str;
    }
};

JSConsole.prototype.println = function(str) {
    if (this.buffer === null) {
        if (str === undefined) str = "";
        this.element.innerHTML += quoteHTML("" + str) + "<br>";
        this.scrollToEnd();
    } else {
        this.buffer += str + "\n";
    }
};

JSConsole.prototype.log = function(str) {
    this.println(str);
};

JSConsole.prototype.showErrorMessage = function(msg) {
    this.element.innerHTML += "<font color=red>" + quoteHTML("" + msg) +
                              "</font><br>";
    this.scrollToEnd();
};

JSConsole.prototype.clear = function() {
    this.element.innerHTML = "";
};

JSConsole.prototype.requestFocus = function() {
    this.element.focus();
};

JSConsole.prototype.requestInput = function(prompt) {
    if (prompt === undefined) prompt = "";
    this.print(prompt);
    this.start = this.element.innerHTML.length;
    this.moveCursorToEnd();
    this.input = "";
    this.requestFocus();
};

JSConsole.prototype.keydown = function(e) {
    var ch = String.fromCharCode(e.which || e.keyCode);
    if (ch === '\177' || ch === '\b') this.keypress(e);
};

JSConsole.prototype.keypress = function(e) {
    if (!e.metaKey && e.preventDefault) e.preventDefault();
    this.scrollToEnd();
    var ch = String.fromCharCode(e.which || e.keyCode);
    if (ch === '\n' || ch === '\r') {
        this.element.innerHTML += "<br>";
        try {
            this.fireActionListeners();
        } catch (msg) {
            this.showErrorMessage("Error: " + msg);
        }
    } else {
        if (ch === '\177' || ch === '\b') {
            if (this.input.length > 0) {
                this.input = this.input.substring(0, this.input.length - 1);
            }
        } else {
            this.input += ch;
        }
        this.element.innerHTML =
            this.element.innerHTML.substring(0, this.start) +
            "<span style='color:blue;'>" + quoteHTML(this.input) + "</span>";
    }
    this.moveCursorToEnd();
    this.requestFocus();
};

JSConsole.prototype.click = function(e) {
    this.moveCursorToEnd();
};

JSConsole.prototype.focus = function(e) {
    var e = new FocusEvent(this, FocusEvent.FOCUS_GAINED);
    for (var i in this.focusListeners) {
        this.focusListeners[i].focusGained(e);
    }
};

JSConsole.prototype.blur = function(e) {
    var e = new FocusEvent(this, FocusEvent.FOCUS_LOST);
    for (var i in this.focusListeners) {
        this.focusListeners[i].focusLost(e);
    }
};

/*
 * Method: addFocusListener
 * Usage: console.addFocusListener(listener);
 * ------------------------------------------
 * Adds a focus listener to the console, which is triggered when the
 * console gains or loses focus.
 */

JSConsole.prototype.addFocusListener = function(listener) {
    this.focusListeners.push(listener);
};

/*
 * Method: addActionListener
 * Usage: console.addActionListener(listener);
 * -------------------------------------------
 * Adds an action listener to the console, which is triggered when the
 * user types a newline character.  For convenience, the listener can
 * be either a Java-like object with an <code>actionPerformed</code>
 * method or a function that expects the text of the line.
 */

JSConsole.prototype.addActionListener = function(listener) {
    this.actionListeners.push(listener);
};

JSConsole.prototype.fireActionListeners = function() {
    var e = null;
    for (var i in this.actionListeners) {
        var listener = this.actionListeners[i];
        if (typeof(listener) === "function") {
  listener(this.input);
      } else {
         if (!e) e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                                     this.input);
  listener.actionPerformed(e);
      }
    }
};

JSConsole.prototype.startConsoleLog = function() {
    this.buffer = "";
};

JSConsole.prototype.endConsoleLog = function() {
    var log = this.buffer;
    this.buffer = "";
    return log;
};

JSConsole.prototype.scrollToEnd = function() {
    var div = this.element;
    if (div.scrollHeight > div.scrollTop + div.clientHeight - this.margin) {
        div.scrollTop = div.scrollHeight - div.clientHeight + this.margin;
    }
};

JSConsole.prototype.moveCursorToEnd = function() {
    var node = this.element.lastChild;
    while (node !== null && !(node instanceof Text)) {
        node = node.lastChild;
    }
    if (node === null) return;
    var range = document.createRange();
    var selection = window.getSelection();
    range.selectNode(node);
    range.collapse(false);
    selection.removeAllRanges();
    selection.addRange(range);
};

JSConsole.DEFAULT_WIDTH = 800;
JSConsole.DEFAULT_HEIGHT = 360;
JSConsole.DEFAULT_SCROLL_MARGIN = 25;
 
return {
    JSConsole : JSConsole
};

});
