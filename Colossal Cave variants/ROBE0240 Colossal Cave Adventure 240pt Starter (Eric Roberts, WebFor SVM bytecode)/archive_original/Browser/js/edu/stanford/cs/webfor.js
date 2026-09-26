/*
 * File: webfor.js
 * Created on Sat May 29 09:52:47 PDT 2021 by java2js
 * --------------------------------------------------
 * This file was generated mechanically by the java2js utility.
 * Permanent edits must be made in the Java file.
 */

define([ "jslib",
         "edu/stanford/cs/exp",
         "edu/stanford/cs/java2js",
         "edu/stanford/cs/jsconsole",
         "edu/stanford/cs/svm",
         "java/awt",
         "java/lang",
         "java/util" ],

function(jslib,
         edu_stanford_cs_exp,
         edu_stanford_cs_java2js,
         edu_stanford_cs_jsconsole,
         edu_stanford_cs_svm,
         java_awt,
         java_lang,
         java_util) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var Value = edu_stanford_cs_exp.Value;
var JSFile = edu_stanford_cs_java2js.JSFile;
var JSPlatform = edu_stanford_cs_java2js.JSPlatform;
var JSConsole = edu_stanford_cs_jsconsole.JSConsole;
var NBConsole = edu_stanford_cs_jsconsole.NBConsole;
var SVM = edu_stanford_cs_svm.SVM;
var SVMArray = edu_stanford_cs_svm.SVMArray;
var SVMClass = edu_stanford_cs_svm.SVMClass;
var SVMConsoleProgram = edu_stanford_cs_svm.SVMConsoleProgram;
var SVMMethod = edu_stanford_cs_svm.SVMMethod;
var ActionEvent = java_awt.ActionEvent;
var ActionListener = java_awt.ActionListener;
var Class = java_lang.Class;
var Integer = java_lang.Integer;
var RuntimeException = java_lang.RuntimeException;
var System = java_lang.System;
var HashMap = java_util.HashMap;

/* WFProgram.js */

var WFProgram = function() {
    if (!(this instanceof WFProgram)) return new WFProgram();
    SVMConsoleProgram.call(this);
    this.getSVM().defineClass("WFLib", new WFLibClass());
};

WFProgram.prototype = 
    jslib.inheritPrototype(SVMConsoleProgram, "WFProgram extends SVMConsoleProgram");
WFProgram.prototype.constructor = WFProgram;
WFProgram.prototype.$class = 
    new Class("WFProgram", WFProgram);


/* WFLibClass.js */

var WFLibClass = function() {
    if (!(this instanceof WFLibClass)) return new WFLibClass();
    SVMClass.call(this);
    this.defineMethod("abs", new WFLib_abs());
    this.defineMethod("mod", new WFLib_mod());
    this.defineMethod("int", new WFLib_int());
    this.defineMethod("min", new WFLib_min());
    this.defineMethod("max", new WFLib_max());
    this.defineMethod("rand", new WFLib_rand());
    this.defineMethod("lower", new WFLib_lower());
    this.defineMethod("upper", new WFLib_upper());
    this.defineMethod("length", new WFLib_length());
    this.defineMethod("substr", new WFLib_substr());
    this.defineMethod("find", new WFLib_find());
    this.defineMethod("parseint", new WFLib_parseint());
    this.defineMethod("ord", new WFLib_ord());
    this.defineMethod("chr", new WFLib_chr());
    this.defineMethod("write", new WFLib_write());
    this.defineMethod("dim", new WFLib_dim());
    this.defineMethod("exit", new WFLib_exit());
    this.defineMethod("execjs", new WFLib_execjs());
    this.defineMethod("rdfile", new WFLib_rdfile());
    this.defineMethod("wrfile", new WFLib_wrfile());
    new WFReadListener(null);
    new WFWriteListener(null);
};

WFLibClass.prototype = 
    jslib.inheritPrototype(SVMClass, "WFLibClass extends SVMClass");
WFLibClass.prototype.constructor = WFLibClass;
WFLibClass.prototype.$class = 
    new Class("WFLibClass", WFLibClass);

WFLibClass.defineBuiltinFunctions = function() {
    WFLibClass.builtins = new HashMap();
    WFLibClass.builtins.put("IABS", "WFLib.abs");
    WFLibClass.builtins.put("ABS", "WFLib.abs");
    WFLibClass.builtins.put("MOD", "WFLib.mod");
    WFLibClass.builtins.put("INT", "WFLib.int");
    WFLibClass.builtins.put("MIN", "WFLib.min");
    WFLibClass.builtins.put("MIN0", "WFLib.min");
    WFLibClass.builtins.put("MAX", "WFLib.max");
    WFLibClass.builtins.put("MAX0", "WFLib.max");
    WFLibClass.builtins.put("RAND", "WFLib.rand");
    WFLibClass.builtins.put("LOWER", "WFLib.lower");
    WFLibClass.builtins.put("UPPER", "WFLib.upper");
    WFLibClass.builtins.put("LENGTH", "WFLib.length");
    WFLibClass.builtins.put("SUBSTR", "WFLib.substr");
    WFLibClass.builtins.put("FIND", "WFLib.find");
    WFLibClass.builtins.put("PARSEINT", "WFLib.parseint");
    WFLibClass.builtins.put("ORD", "WFLib.ord");
    WFLibClass.builtins.put("CHR", "WFLib.chr");
    WFLibClass.builtins.put("DIM", "WFLib.dim");
    WFLibClass.builtins.put("EXIT", "WFLib.exit");
    WFLibClass.builtins.put("EXECJS", "WFLib.execjs");
    WFLibClass.builtins.put("RDFILE", "WFLib.rdfile");
    WFLibClass.builtins.put("WRFILE", "WFLib.wrfile");
    WFLibClass.builtins.put("READLINE", "Console.readline");
    WFLibClass.builtins.put("READINT", "Console.readint");
    WFLibClass.builtins.put("PRINTLN", "Console.println");
};

WFLibClass.getBuiltinFunction = function(name) {
    if (WFLibClass.builtins === null) WFLibClass.defineBuiltinFunctions();
    return WFLibClass.builtins.get(name);
};

WFLibClass.builtins = null;
var WFLib_abs = function() {
    SVMMethod.call(this);
};

WFLib_abs.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_abs extends SVMMethod");
WFLib_abs.prototype.constructor = WFLib_abs;
WFLib_abs.prototype.$class = 
    new Class("WFLib_abs", WFLib_abs);

WFLib_abs.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.abs", "I");
    svm.pushInteger(Math.abs(svm.popInteger()));
};

var WFLib_mod = function() {
    SVMMethod.call(this);
};

WFLib_mod.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_mod extends SVMMethod");
WFLib_mod.prototype.constructor = WFLib_mod;
WFLib_mod.prototype.$class = 
    new Class("WFLib_mod", WFLib_mod);

WFLib_mod.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.mod", "II");
    var i2 = svm.popInteger();
    var i1 = svm.popInteger();
    svm.pushInteger(i1 % i2);
};

var WFLib_int = function() {
    SVMMethod.call(this);
};

WFLib_int.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_int extends SVMMethod");
WFLib_int.prototype.constructor = WFLib_int;
WFLib_int.prototype.$class = 
    new Class("WFLib_int", WFLib_int);

WFLib_int.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.int", "*");
    var v = svm.pop();
    switch (v.getType()) {
      case Value.INTEGER:
        svm.pushInteger(v.getIntegerValue());
        break;
      case Value.DOUBLE:
        svm.pushInteger(toInt(v.getDoubleValue()));
        break;
      case Value.STRING:
        svm.pushInteger(Integer.parseInt(v.getStringValue()));
        break;
      default:
        throw new RuntimeException("Illegal argument to INT()");
    }
};

var WFLib_min = function() {
    SVMMethod.call(this);
};

WFLib_min.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_min extends SVMMethod");
WFLib_min.prototype.constructor = WFLib_min;
WFLib_min.prototype.$class = 
    new Class("WFLib_min", WFLib_min);

WFLib_min.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.min", "II");
    var i1 = svm.popInteger();
    var i2 = svm.popInteger();
    svm.pushInteger(Math.min(i1, i2));
};

var WFLib_max = function() {
    SVMMethod.call(this);
};

WFLib_max.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_max extends SVMMethod");
WFLib_max.prototype.constructor = WFLib_max;
WFLib_max.prototype.$class = 
    new Class("WFLib_max", WFLib_max);

WFLib_max.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.max", "II");
    var i1 = svm.popInteger();
    var i2 = svm.popInteger();
    svm.pushInteger(Math.max(i1, i2));
};

var WFLib_rand = function() {
    SVMMethod.call(this);
};

WFLib_rand.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_rand extends SVMMethod");
WFLib_rand.prototype.constructor = WFLib_rand;
WFLib_rand.prototype.$class = 
    new Class("WFLib_rand", WFLib_rand);

WFLib_rand.prototype.execute = function(svm, receiver) {
    if (svm.getArgumentCount() === 0) {
        svm.checkSignature("WFLib.rand", "");
    } else {
        svm.checkSignature("WFLib.rand", "I");
        svm.pop();
    }
    svm.pushDouble(Math.random());
};

var WFLib_lower = function() {
    SVMMethod.call(this);
};

WFLib_lower.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_lower extends SVMMethod");
WFLib_lower.prototype.constructor = WFLib_lower;
WFLib_lower.prototype.$class = 
    new Class("WFLib_lower", WFLib_lower);

WFLib_lower.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.lower", "S");
    svm.pushString(svm.popString().toLowerCase());
};

var WFLib_upper = function() {
    SVMMethod.call(this);
};

WFLib_upper.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_upper extends SVMMethod");
WFLib_upper.prototype.constructor = WFLib_upper;
WFLib_upper.prototype.$class = 
    new Class("WFLib_upper", WFLib_upper);

WFLib_upper.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.upper", "S");
    svm.pushString(svm.popString().toUpperCase());
};

var WFLib_length = function() {
    SVMMethod.call(this);
};

WFLib_length.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_length extends SVMMethod");
WFLib_length.prototype.constructor = WFLib_length;
WFLib_length.prototype.$class = 
    new Class("WFLib_length", WFLib_length);

WFLib_length.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.length", "S");
    svm.pushInteger(svm.popString().length);
};

var WFLib_substr = function() {
    SVMMethod.call(this);
};

WFLib_substr.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_substr extends SVMMethod");
WFLib_substr.prototype.constructor = WFLib_substr;
WFLib_substr.prototype.$class = 
    new Class("WFLib_substr", WFLib_substr);

WFLib_substr.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.substr", "SII");
    var p2 = svm.popInteger();
    var p1 = svm.popInteger();
    svm.pushString(svm.popString().substring(p1, p2));
};

var WFLib_find = function() {
    SVMMethod.call(this);
};

WFLib_find.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_find extends SVMMethod");
WFLib_find.prototype.constructor = WFLib_find;
WFLib_find.prototype.$class = 
    new Class("WFLib_find", WFLib_find);

WFLib_find.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.find", "SSI");
    var start = svm.popInteger();
    var needle = svm.popString();
    var haystack = svm.popString();
    svm.pushInteger(haystack.indexOf(needle, start));
};

var WFLib_parseint = function() {
    SVMMethod.call(this);
};

WFLib_parseint.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_parseint extends SVMMethod");
WFLib_parseint.prototype.constructor = WFLib_parseint;
WFLib_parseint.prototype.$class = 
    new Class("WFLib_parseint", WFLib_parseint);

WFLib_parseint.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.parseint", "S");
    svm.pushInteger(Integer.parseInt(svm.popString()));
};

var WFLib_ord = function() {
    SVMMethod.call(this);
};

WFLib_ord.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_ord extends SVMMethod");
WFLib_ord.prototype.constructor = WFLib_ord;
WFLib_ord.prototype.$class = 
    new Class("WFLib_ord", WFLib_ord);

WFLib_ord.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.ord", "S");
    svm.pushInteger(toInt(svm.popString()).charCodeAt(0));
};

var WFLib_chr = function() {
    SVMMethod.call(this);
};

WFLib_chr.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_chr extends SVMMethod");
WFLib_chr.prototype.constructor = WFLib_chr;
WFLib_chr.prototype.$class = 
    new Class("WFLib_chr", WFLib_chr);

WFLib_chr.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.chr", "I");
    svm.pushString("" + toStr(svm.popInteger()));
};

var WFLib_read = function() {
    SVMMethod.call(this);
};

WFLib_read.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_read extends SVMMethod");
WFLib_read.prototype.constructor = WFLib_read;
WFLib_read.prototype.$class = 
    new Class("WFLib_read", WFLib_read);

WFLib_read.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.read", "");
    var console = svm.getConsole();
    svm.setGlobal("CONSOLE_WAIT", Value.createString("getLine"));
    svm.setState(SVM.WAITING);
    console.requestInput("");
};

var WFLib_write = function() {
    SVMMethod.call(this);
};

WFLib_write.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_write extends SVMMethod");
WFLib_write.prototype.constructor = WFLib_write;
WFLib_write.prototype.$class = 
    new Class("WFLib_write", WFLib_write);

WFLib_write.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.write", "*");
    var value = svm.pop();
    if (value !== Value.UNDEFINED) {
        svm.getConsole().println(value);
    }
};

var WFLib_dim = function() {
    SVMMethod.call(this);
};

WFLib_dim.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_dim extends SVMMethod");
WFLib_dim.prototype.constructor = WFLib_dim;
WFLib_dim.prototype.$class = 
    new Class("WFLib_dim", WFLib_dim);

WFLib_dim.prototype.execute = function(svm, receiver) {
    var n = svm.getArgumentCount();
    var dims = jslib.newArray(n);
    for (var i = 0; i < n; i++) {
        dims[i] = svm.popInteger();
    }
    svm.push(this.createMultidimensionalArray(dims, 0));
};

WFLib_dim.prototype.createMultidimensionalArray = function(dims, k) {
    var list = new SVMArray();
    var n = dims[k];
    list.add(Value.UNDEFINED);
    for (var i = 0; i < n; i++) {
        if (k === dims.length - 1) {
            list.add(Value.createInteger(0));
        } else {
            list.add(this.createMultidimensionalArray(dims, k + 1));
        }
    }
    return Value.createObject(list, "Array");
};

var WFLib_exit = function() {
    SVMMethod.call(this);
};

WFLib_exit.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_exit extends SVMMethod");
WFLib_exit.prototype.constructor = WFLib_exit;
WFLib_exit.prototype.$class = 
    new Class("WFLib_exit", WFLib_exit);

WFLib_exit.prototype.execute = function(svm, receiver) {
    System.exit(0);
};

var WFLib_execjs = function() {
    SVMMethod.call(this);
};

WFLib_execjs.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_execjs extends SVMMethod");
WFLib_execjs.prototype.constructor = WFLib_execjs;
WFLib_execjs.prototype.$class = 
    new Class("WFLib_execjs", WFLib_execjs);

WFLib_execjs.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.execjs", "S");
    eval(svm.popString());
};

var WFLib_rdfile = function() {
    SVMMethod.call(this);
};

WFLib_rdfile.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_rdfile extends SVMMethod");
WFLib_rdfile.prototype.constructor = WFLib_rdfile;
WFLib_rdfile.prototype.$class = 
    new Class("WFLib_rdfile", WFLib_rdfile);

WFLib_rdfile.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.rdfile", "S");
    var filename = svm.popString();
    var listener = new WFReadListener(svm);
    JSFile.readClientFile(filename, listener, svm.getConsole());
    svm.setGlobal("CONSOLE_WAIT", Value.createString("rdfile"));
    svm.setState(SVM.WAITING);
};

var WFReadListener = function(svm) {
    if (!(this instanceof WFReadListener)) return new WFReadListener(svm);
    this.svm = svm;
};

WFReadListener.prototype.actionPerformed = function(e) {
    this.svm.pushString(e.getActionCommand());
    this.svm.run();
};

var WFLib_wrfile = function() {
    SVMMethod.call(this);
};

WFLib_wrfile.prototype =
    jslib.inheritPrototype(SVMMethod, "WFLib_wrfile extends SVMMethod");
WFLib_wrfile.prototype.constructor = WFLib_wrfile;
WFLib_wrfile.prototype.$class = 
    new Class("WFLib_wrfile", WFLib_wrfile);

WFLib_wrfile.prototype.execute = function(svm, receiver) {
    svm.checkSignature("WFLib.wrfile", "SS");
    var text = svm.popString();
    var filename = svm.popString();
    var listener = new WFWriteListener(svm);
    JSFile.writeClientFile(filename, text, listener, svm.getConsole());
    svm.setState(SVM.WAITING);
};

var WFWriteListener = function(svm) {
    if (!(this instanceof WFWriteListener)) return new WFWriteListener(svm);
    this.svm = svm;
};

WFWriteListener.prototype.actionPerformed = function(e) {
    this.svm.pushString(e.getActionCommand());
    this.svm.run();
};


/* WFJSConsole.js */

var WFJSConsole = function(svm) {
    if (!(this instanceof WFJSConsole)) return new WFJSConsole(svm);
    JSConsole.call(this);
    this.svm = svm;
};

WFJSConsole.prototype = 
    jslib.inheritPrototype(JSConsole, "WFJSConsole extends JSConsole");
WFJSConsole.prototype.constructor = WFJSConsole;
WFJSConsole.prototype.$class = 
    new Class("WFJSConsole", WFJSConsole);

WFJSConsole.prototype.requestInput = function(prompt) {
    if (prompt !== null) System.out.print(prompt);
    this.svm.setState(SVM.WAITING);
};


/* Exports */

return {
    WFLibClass : WFLibClass,
    WFProgram : WFProgram
};

});
