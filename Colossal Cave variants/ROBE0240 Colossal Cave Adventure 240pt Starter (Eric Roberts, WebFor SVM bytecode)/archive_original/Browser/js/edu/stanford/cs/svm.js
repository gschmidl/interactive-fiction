/*
 * File: svm.js
 * Created on Thu May 27 16:19:49 PDT 2021 by java2js
 * --------------------------------------------------
 * This file was generated mechanically by the java2js utility.
 * Permanent edits must be made in the Java file.
 */

define([ "jslib",
         "edu/stanford/cs/controller",
         "edu/stanford/cs/csslib",
         "edu/stanford/cs/exp",
         "edu/stanford/cs/java2js",
         "edu/stanford/cs/jsconsole",
         "edu/stanford/cs/parser",
         "edu/stanford/cs/svmops",
         "edu/stanford/cs/tokenscanner",
         "edu/stanford/cs/utf8",
         "java/awt",
         "java/lang",
         "java/util" ],

function(jslib,
         edu_stanford_cs_controller,
         edu_stanford_cs_csslib,
         edu_stanford_cs_exp,
         edu_stanford_cs_java2js,
         edu_stanford_cs_jsconsole,
         edu_stanford_cs_parser,
         edu_stanford_cs_svmops,
         edu_stanford_cs_tokenscanner,
         edu_stanford_cs_utf8,
         java_awt,
         java_lang,
         java_util) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var Controller = edu_stanford_cs_controller.Controller;
var ErrorHandler = edu_stanford_cs_controller.ErrorHandler;
var Steppable = edu_stanford_cs_controller.Steppable;
var CSSFont = edu_stanford_cs_csslib.CSSFont;
var Value = edu_stanford_cs_exp.Value;
var JSElementList = edu_stanford_cs_java2js.JSElementList;
var JSPackage = edu_stanford_cs_java2js.JSPackage;
var JSProgram = edu_stanford_cs_java2js.JSProgram;
var JSConsole = edu_stanford_cs_jsconsole.JSConsole;
var NBConsole = edu_stanford_cs_jsconsole.NBConsole;
var CodeVector = edu_stanford_cs_parser.CodeVector;
var SyntaxError = edu_stanford_cs_parser.SyntaxError;
var SVMOp = edu_stanford_cs_svmops.SVMOp;
var TokenScanner = edu_stanford_cs_tokenscanner.TokenScanner;
var UTF8 = edu_stanford_cs_utf8.UTF8;
var Dimension = java_awt.Dimension;
var ActionEvent = java_awt.ActionEvent;
var ActionListener = java_awt.ActionListener;
var Class = java_lang.Class;
var Double = java_lang.Double;
var Integer = java_lang.Integer;
var RuntimeException = java_lang.RuntimeException;
var System = java_lang.System;
var ArrayDeque = java_util.ArrayDeque;
var ArrayList = java_util.ArrayList;
var HashMap = java_util.HashMap;
var HashSet = java_util.HashSet;
var Stack = java_util.Stack;
var TreeMap = java_util.TreeMap;

/* SVMInstruction.js */

var SVMInstruction = function(name, code) {
    if (!(this instanceof SVMInstruction)) return new SVMInstruction(name, code);
    this.name = name;
    this.code = code;
};

SVMInstruction.prototype.getName = function() {
    return this.name;
};

SVMInstruction.prototype.getCode = function() {
    return this.code;
};

SVMInstruction.prototype.execute = function(svm, addr) {
    throw new RuntimeException("Not yet implemented");
};

SVMInstruction.prototype.unparse = function(svm, addr) {
    return this.name;
};

SVMInstruction.prototype.assemble = function(cv, scanner) {
    cv.addWord(this.code << 24);
};

SVMInstruction.lookup = function(name) {
    if (SVMInstruction.instructionTable === null) SVMInstruction.initializeInstructionTable();
    return SVMInstruction.instructionTable.get(name);
};

SVMInstruction.get = function(code) {
    if (SVMInstruction.instructionTable === null) SVMInstruction.initializeInstructionTable();
    return SVMInstruction.codeTable.get(code);
};

SVMInstruction.initializeInstructionTable = function() {
    SVMInstruction.instructionTable = new TreeMap();
    SVMInstruction.codeTable = new TreeMap();
    SVMInstruction.define(new END_Ins());
    SVMInstruction.define(new VERSION_Ins());
    SVMInstruction.define(new PSTACK_Ins());
    SVMInstruction.define(new STMT_Ins());
    SVMInstruction.define(new HALT_Ins());
    SVMInstruction.define(new NOP_Ins());
    SVMInstruction.define(new TRACE_Ins());
    SVMInstruction.define(new PUSHINT_Ins());
    SVMInstruction.define(new PUSHNUM_Ins());
    SVMInstruction.define(new PUSHSTR_Ins());
    SVMInstruction.define(new PUSHFN_Ins());
    SVMInstruction.define(new POP_Ins());
    SVMInstruction.define(new DUP_Ins());
    SVMInstruction.define(new EXCH_Ins());
    SVMInstruction.define(new ROLL_Ins());
    SVMInstruction.define(new COPY_Ins());
    SVMInstruction.define(new ADD_Ins());
    SVMInstruction.define(new SUB_Ins());
    SVMInstruction.define(new MUL_Ins());
    SVMInstruction.define(new DIV_Ins());
    SVMInstruction.define(new IDIV_Ins());
    SVMInstruction.define(new REM_Ins());
    SVMInstruction.define(new NEG_Ins());
    SVMInstruction.define(new EQ_Ins());
    SVMInstruction.define(new NE_Ins());
    SVMInstruction.define(new LT_Ins());
    SVMInstruction.define(new LE_Ins());
    SVMInstruction.define(new GT_Ins());
    SVMInstruction.define(new GE_Ins());
    SVMInstruction.define(new JUMP_Ins());
    SVMInstruction.define(new JUMPT_Ins());
    SVMInstruction.define(new JUMPF_Ins());
    SVMInstruction.define(new DISPATCH_Ins());
    SVMInstruction.define(new TRY_Ins());
    SVMInstruction.define(new ENDTRY_Ins());
    SVMInstruction.define(new THROW_Ins());
    SVMInstruction.define(new NOT_Ins());
    SVMInstruction.define(new AND_Ins());
    SVMInstruction.define(new OR_Ins());
    SVMInstruction.define(new XOR_Ins());
    SVMInstruction.define(new LSH_Ins());
    SVMInstruction.define(new ASH_Ins());
    SVMInstruction.define(new CALL_Ins());
    SVMInstruction.define(new CALLM_Ins());
    SVMInstruction.define(new CALLFN_Ins());
    SVMInstruction.define(new RETURN_Ins());
    SVMInstruction.define(new LOCALS_Ins());
    SVMInstruction.define(new PUSHLOC_Ins());
    SVMInstruction.define(new POPLOC_Ins());
    SVMInstruction.define(new ARG_Ins());
    SVMInstruction.define(new VAR_Ins());
    SVMInstruction.define(new PARAMS_Ins());
    SVMInstruction.define(new NARGS_Ins());
    SVMInstruction.define(new VARGS_Ins());
    SVMInstruction.define(new PUSHVAR_Ins());
    SVMInstruction.define(new POPVAR_Ins());
    SVMInstruction.define(new PUSHFRM_Ins());
    SVMInstruction.define(new POPFRM_Ins());
};

SVMInstruction.define = function(ins) {
    SVMInstruction.instructionTable.put(ins.getName(), ins);
    SVMInstruction.codeTable.put(ins.getCode(), ins);
};

SVMInstruction.instructionTable = null;
SVMInstruction.codeTable = null;
var SVMStringInstruction = function(name, code) {
    if (!(this instanceof SVMStringInstruction)) return new SVMStringInstruction(name, code);
    SVMInstruction.call(this, name, code);
};

SVMStringInstruction.prototype = 
    jslib.inheritPrototype(SVMInstruction, "SVMStringInstruction extends SVMInstruction");
SVMStringInstruction.prototype.constructor = SVMStringInstruction;
SVMStringInstruction.prototype.$class = 
    new Class("SVMStringInstruction", SVMStringInstruction);

SVMStringInstruction.prototype.assemble = function(cv, scanner) {
    var str = scanner.getStringValue(scanner.nextToken());
    cv.addWord((this.getCode() << 24) | cv.stringRef(str));
};

SVMStringInstruction.prototype.unparse = function(svm, addr) {
    return this.getName() + " \"" + svm.getString(addr) + "\"";
};

var SVMVarInstruction = function(name, code) {
    if (!(this instanceof SVMVarInstruction)) return new SVMVarInstruction(name, code);
    SVMInstruction.call(this, name, code);
};

SVMVarInstruction.prototype = 
    jslib.inheritPrototype(SVMInstruction, "SVMVarInstruction extends SVMInstruction");
SVMVarInstruction.prototype.constructor = SVMVarInstruction;
SVMVarInstruction.prototype.$class = 
    new Class("SVMVarInstruction", SVMVarInstruction);

SVMVarInstruction.prototype.assemble = function(cv, scanner) {
    var str = scanner.nextToken();
    cv.addWord((this.getCode() << 24) | cv.stringRef(str));
};

SVMVarInstruction.prototype.unparse = function(svm, addr) {
    return this.getName() + " " + svm.getString(addr);
};

var SVMAddressInstruction = function(name, code) {
    if (!(this instanceof SVMAddressInstruction)) return new SVMAddressInstruction(name, code);
    SVMInstruction.call(this, name, code);
};

SVMAddressInstruction.prototype = 
    jslib.inheritPrototype(SVMInstruction, "SVMAddressInstruction extends SVMInstruction");
SVMAddressInstruction.prototype.constructor = SVMAddressInstruction;
SVMAddressInstruction.prototype.$class = 
    new Class("SVMAddressInstruction", SVMAddressInstruction);

SVMAddressInstruction.prototype.assemble = function(cv, scanner) {
    var token = scanner.nextToken();
    var type = scanner.getTokenType(token);
    switch (type) {
      case TokenScanner.NUMBER:
        cv.addWord((this.getCode() << 24) | Integer.parseInt(token));
        break;
      case TokenScanner.WORD:
        cv.addWord((this.getCode() << 24) | cv.labelRef(token));
        break;
      default:
        throw new SyntaxError("Illegal argument " + token);
    }
};

SVMAddressInstruction.prototype.unparse = function(svm, addr) {
    return this.getName() + " " + addr;
};

var SVMOffsetInstruction = function(name, code) {
    if (!(this instanceof SVMOffsetInstruction)) return new SVMOffsetInstruction(name, code);
    SVMInstruction.call(this, name, code);
};

SVMOffsetInstruction.prototype = 
    jslib.inheritPrototype(SVMInstruction, "SVMOffsetInstruction extends SVMInstruction");
SVMOffsetInstruction.prototype.constructor = SVMOffsetInstruction;
SVMOffsetInstruction.prototype.$class = 
    new Class("SVMOffsetInstruction", SVMOffsetInstruction);

SVMOffsetInstruction.prototype.assemble = function(cv, scanner) {
    var token = scanner.nextToken();
    var type = scanner.getTokenType(token);
    switch (type) {
      case TokenScanner.NUMBER:
        cv.addWord((this.getCode() << 24) | Integer.parseInt(token));
        break;
      case TokenScanner.WORD:
        cv.addWord((this.getCode() << 24) | cv.getLabel(token));
        break;
      default:
        throw new SyntaxError("Illegal argument " + token);
    }
};

SVMOffsetInstruction.prototype.unparse = function(svm, addr) {
    return this.getName() + " " + addr;
};

var SVMNameInstruction = function(name, code) {
    if (!(this instanceof SVMNameInstruction)) return new SVMNameInstruction(name, code);
    SVMInstruction.call(this, name, code);
};

SVMNameInstruction.prototype = 
    jslib.inheritPrototype(SVMInstruction, "SVMNameInstruction extends SVMInstruction");
SVMNameInstruction.prototype.constructor = SVMNameInstruction;
SVMNameInstruction.prototype.$class = 
    new Class("SVMNameInstruction", SVMNameInstruction);

SVMNameInstruction.prototype.assemble = function(cv, scanner) {
    var name = "";
    while (true) {
        var token = scanner.nextToken();
        if (jslib.equals(token, "\n")) break;
        name += token;
    }
    scanner.saveToken("\n");
    cv.addWord((this.getCode() << 24) | cv.stringRef(name));
};

var ArithmeticOp = function(name, code) {
    if (!(this instanceof ArithmeticOp)) return new ArithmeticOp(name, code);
    SVMInstruction.call(this, name, code);
};

ArithmeticOp.prototype = 
    jslib.inheritPrototype(SVMInstruction, "ArithmeticOp extends SVMInstruction");
ArithmeticOp.prototype.constructor = ArithmeticOp;
ArithmeticOp.prototype.$class = 
    new Class("ArithmeticOp", ArithmeticOp);

ArithmeticOp.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    if (!lhs.isNumeric() || !rhs.isNumeric()) {
        throw new RuntimeException("Illegal to apply " + this.getName() +
        " to " + lhs + " and " + rhs);
    }
    if (lhs.getType() === Value.INTEGER && rhs.getType() === Value.INTEGER) {
        var x = lhs.getIntegerValue();
        var y = rhs.getIntegerValue();
        svm.pushInteger(this.applyInteger(x, y));
    } else {
        var x = lhs.getDoubleValue();
        var y = rhs.getDoubleValue();
        svm.pushDouble(this.applyDouble(x, y));
    }
};

var RelationalOp = function(name, code) {
    if (!(this instanceof RelationalOp)) return new RelationalOp(name, code);
    SVMInstruction.call(this, name, code);
};

RelationalOp.prototype = 
    jslib.inheritPrototype(SVMInstruction, "RelationalOp extends SVMInstruction");
RelationalOp.prototype.constructor = RelationalOp;
RelationalOp.prototype.$class = 
    new Class("RelationalOp", RelationalOp);

RelationalOp.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    var lhsType = lhs.getType();
    var rhsType = rhs.getType();
    if (lhsType === Value.STRING && rhsType === Value.STRING) {
        var s1 = lhs.getStringValue();
        var s2 = rhs.getStringValue();
        svm.pushBoolean(this.applyInteger(s1.localeCompare(s2), 0));
    } else if (lhsType === Value.INTEGER && rhsType === Value.INTEGER) {
        var x = lhs.getIntegerValue();
        var y = rhs.getIntegerValue();
        svm.pushBoolean(this.applyInteger(x, y));
    } else if (lhs.isNumeric() && rhs.isNumeric()) {
        var x = lhs.getDoubleValue();
        var y = rhs.getDoubleValue();
        svm.pushBoolean(this.applyDouble(x, y));
    } else {
        var v1 = lhs.getValue();
        var v2 = rhs.getValue();
        svm.pushBoolean(this.applyObject(v1, v2));
    }
};

RelationalOp.prototype.applyObject = function(v1, v2) {
    throw new RuntimeException("Illegal object comparison");
};

var LogicalOp = function(name, code) {
    if (!(this instanceof LogicalOp)) return new LogicalOp(name, code);
    SVMInstruction.call(this, name, code);
};

LogicalOp.prototype = 
    jslib.inheritPrototype(SVMInstruction, "LogicalOp extends SVMInstruction");
LogicalOp.prototype.constructor = LogicalOp;
LogicalOp.prototype.$class = 
    new Class("LogicalOp", LogicalOp);

LogicalOp.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    var lhsType = lhs.getType();
    var rhsType = rhs.getType();
    if (lhsType === Value.BOOLEAN && rhsType === Value.BOOLEAN) {
        var v1 = lhs.getBooleanValue() ? -1 : 0;
        var v2 = rhs.getBooleanValue() ? -1 : 0;
        svm.pushBoolean(this.applyInteger(v1, v2) !== 0);
    } else {
        var x = lhs.getIntegerValue();
        var y = rhs.getIntegerValue();
        svm.pushInteger(this.applyInteger(x, y));
    }
};

var END_Ins = function() {
    if (!(this instanceof END_Ins)) return new END_Ins();
    SVMInstruction.call(this, "END", SVMOp.END);
};

END_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "END_Ins extends SVMInstruction");
END_Ins.prototype.constructor = END_Ins;
END_Ins.prototype.$class = 
    new Class("END_Ins", END_Ins);

END_Ins.prototype.execute = function(svm, addr) {
    svm.setPC(-1);
};

var VERSION_Ins = function() {
    if (!(this instanceof VERSION_Ins)) return new VERSION_Ins();
    SVMOffsetInstruction.call(this, "VERSION", SVMOp.VERSION);
};

VERSION_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "VERSION_Ins extends SVMOffsetInstruction");
VERSION_Ins.prototype.constructor = VERSION_Ins;
VERSION_Ins.prototype.$class = 
    new Class("VERSION_Ins", VERSION_Ins);

VERSION_Ins.prototype.execute = function(svm, addr) {
    if (addr !== SVMOp.SVM_VERSION) {
        throw new RuntimeException("Incompatible SVM version");
    }
};

var PSTACK_Ins = function() {
    if (!(this instanceof PSTACK_Ins)) return new PSTACK_Ins();
    SVMInstruction.call(this, "PSTACK", SVMOp.PSTACK);
};

PSTACK_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "PSTACK_Ins extends SVMInstruction");
PSTACK_Ins.prototype.constructor = PSTACK_Ins;
PSTACK_Ins.prototype.$class = 
    new Class("PSTACK_Ins", PSTACK_Ins);

PSTACK_Ins.prototype.execute = function(svm, addr) {
    svm.pstack();
};

var STMT_Ins = function() {
    if (!(this instanceof STMT_Ins)) return new STMT_Ins();
    SVMOffsetInstruction.call(this, "STMT", SVMOp.STMT);
};

STMT_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "STMT_Ins extends SVMOffsetInstruction");
STMT_Ins.prototype.constructor = STMT_Ins;
STMT_Ins.prototype.$class = 
    new Class("STMT_Ins", STMT_Ins);

STMT_Ins.prototype.execute = function(svm, addr) {
    svm.setStatementOffset(addr);
    svm.restoreStackBase();
};

var HALT_Ins = function() {
    if (!(this instanceof HALT_Ins)) return new HALT_Ins();
    SVMInstruction.call(this, "HALT", SVMOp.HALT);
};

HALT_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "HALT_Ins extends SVMInstruction");
HALT_Ins.prototype.constructor = HALT_Ins;
HALT_Ins.prototype.$class = 
    new Class("HALT_Ins", HALT_Ins);

HALT_Ins.prototype.execute = function(svm, addr) {
    svm.setPC(-1);
};

var NOP_Ins = function() {
    if (!(this instanceof NOP_Ins)) return new NOP_Ins();
    SVMInstruction.call(this, "NOP", SVMOp.NOP);
};

NOP_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "NOP_Ins extends SVMInstruction");
NOP_Ins.prototype.constructor = NOP_Ins;
NOP_Ins.prototype.$class = 
    new Class("NOP_Ins", NOP_Ins);

NOP_Ins.prototype.execute = function(svm, addr) {
    /* Empty */
};

var TRACE_Ins = function() {
    if (!(this instanceof TRACE_Ins)) return new TRACE_Ins();
    SVMAddressInstruction.call(this, "TRACE", SVMOp.TRACE);
};

TRACE_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "TRACE_Ins extends SVMAddressInstruction");
TRACE_Ins.prototype.constructor = TRACE_Ins;
TRACE_Ins.prototype.$class = 
    new Class("TRACE_Ins", TRACE_Ins);

TRACE_Ins.prototype.execute = function(svm, addr) {
    svm.setTraceFlag(addr !== 0);
};

var PUSHINT_Ins = function() {
    if (!(this instanceof PUSHINT_Ins)) return new PUSHINT_Ins();
    SVMAddressInstruction.call(this, "PUSHINT", SVMOp.PUSHINT);
};

PUSHINT_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "PUSHINT_Ins extends SVMAddressInstruction");
PUSHINT_Ins.prototype.constructor = PUSHINT_Ins;
PUSHINT_Ins.prototype.$class = 
    new Class("PUSHINT_Ins", PUSHINT_Ins);

PUSHINT_Ins.prototype.execute = function(svm, addr) {
    svm.pushInteger(addr);
};

var PUSHNUM_Ins = function() {
    if (!(this instanceof PUSHNUM_Ins)) return new PUSHNUM_Ins();
    SVMInstruction.call(this, "PUSHNUM", SVMOp.PUSHNUM);
};

PUSHNUM_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "PUSHNUM_Ins extends SVMInstruction");
PUSHNUM_Ins.prototype.constructor = PUSHNUM_Ins;
PUSHNUM_Ins.prototype.$class = 
    new Class("PUSHNUM_Ins", PUSHNUM_Ins);

PUSHNUM_Ins.prototype.assemble = function(cv, scanner) {
    var token = scanner.nextToken();
    if (jslib.equals(token, "-")) token += scanner.nextToken();
    cv.addWord((this.getCode() << 24) | cv.stringRef(token));
};

PUSHNUM_Ins.prototype.execute = function(svm, addr) {
    var str = svm.getString(addr);
    if (str.indexOf(".") === -1 && str.toUpperCase().indexOf("E") === -1) {
        svm.pushInteger(Integer.parseInt(str));
    } else {
        svm.pushDouble(Double.parseDouble(str));
    }
};

PUSHNUM_Ins.prototype.unparse = function(svm, addr) {
    return "PUSHNUM " + svm.getString(addr);
};

var PUSHCH_Ins = function() {
    if (!(this instanceof PUSHCH_Ins)) return new PUSHCH_Ins();
    SVMAddressInstruction.call(this, "PUSHCH", SVMOp.PUSHCH);
};

PUSHCH_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "PUSHCH_Ins extends SVMAddressInstruction");
PUSHCH_Ins.prototype.constructor = PUSHCH_Ins;
PUSHCH_Ins.prototype.$class = 
    new Class("PUSHCH_Ins", PUSHCH_Ins);

PUSHCH_Ins.prototype.execute = function(svm, addr) {
    svm.push(Value.createCharacter(toStr(addr)));
};

var PUSHSTR_Ins = function() {
    if (!(this instanceof PUSHSTR_Ins)) return new PUSHSTR_Ins();
    SVMStringInstruction.call(this, "PUSHSTR", SVMOp.PUSHSTR);
};

PUSHSTR_Ins.prototype = 
    jslib.inheritPrototype(SVMStringInstruction, "PUSHSTR_Ins extends SVMStringInstruction");
PUSHSTR_Ins.prototype.constructor = PUSHSTR_Ins;
PUSHSTR_Ins.prototype.$class = 
    new Class("PUSHSTR_Ins", PUSHSTR_Ins);

PUSHSTR_Ins.prototype.execute = function(svm, addr) {
    svm.pushString(svm.getString(addr));
};

var PUSHFN_Ins = function() {
    if (!(this instanceof PUSHFN_Ins)) return new PUSHFN_Ins();
    SVMAddressInstruction.call(this, "PUSHFN", SVMOp.PUSHFN);
};

PUSHFN_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "PUSHFN_Ins extends SVMAddressInstruction");
PUSHFN_Ins.prototype.constructor = PUSHFN_Ins;
PUSHFN_Ins.prototype.$class = 
    new Class("PUSHFN_Ins", PUSHFN_Ins);

PUSHFN_Ins.prototype.execute = function(svm, addr) {
    var cf = svm.getCurrentFrame();
    var code = svm.getCode();
    var closure = new SVMFunctionClosure(code, addr, cf);
    svm.push(Value.createObject(closure, "FunctionClosure"));
};

var POP_Ins = function() {
    if (!(this instanceof POP_Ins)) return new POP_Ins();
    SVMInstruction.call(this, "POP", SVMOp.POP);
};

POP_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "POP_Ins extends SVMInstruction");
POP_Ins.prototype.constructor = POP_Ins;
POP_Ins.prototype.$class = 
    new Class("POP_Ins", POP_Ins);

POP_Ins.prototype.assemble = function(cv, scanner) {
    cv.addWord(SVMOp.POP << 24);
};

POP_Ins.prototype.execute = function(svm, addr) {
    svm.pop();
};

var DUP_Ins = function() {
    if (!(this instanceof DUP_Ins)) return new DUP_Ins();
    SVMInstruction.call(this, "DUP", SVMOp.DUP);
};

DUP_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "DUP_Ins extends SVMInstruction");
DUP_Ins.prototype.constructor = DUP_Ins;
DUP_Ins.prototype.$class = 
    new Class("DUP_Ins", DUP_Ins);

DUP_Ins.prototype.execute = function(svm, addr) {
    svm.push(svm.peekBack(0));
};

var EXCH_Ins = function() {
    if (!(this instanceof EXCH_Ins)) return new EXCH_Ins();
    SVMInstruction.call(this, "EXCH", SVMOp.EXCH);
};

EXCH_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "EXCH_Ins extends SVMInstruction");
EXCH_Ins.prototype.constructor = EXCH_Ins;
EXCH_Ins.prototype.$class = 
    new Class("EXCH_Ins", EXCH_Ins);

EXCH_Ins.prototype.execute = function(svm, addr) {
    svm.exch();
};

var ROLL_Ins = function() {
    if (!(this instanceof ROLL_Ins)) return new ROLL_Ins();
    SVMOffsetInstruction.call(this, "ROLL", SVMOp.ROLL);
};

ROLL_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "ROLL_Ins extends SVMOffsetInstruction");
ROLL_Ins.prototype.constructor = ROLL_Ins;
ROLL_Ins.prototype.$class = 
    new Class("ROLL_Ins", ROLL_Ins);

ROLL_Ins.prototype.execute = function(svm, addr) {
    svm.roll(addr);
};

var COPY_Ins = function() {
    if (!(this instanceof COPY_Ins)) return new COPY_Ins();
    SVMOffsetInstruction.call(this, "COPY", SVMOp.COPY);
};

COPY_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "COPY_Ins extends SVMOffsetInstruction");
COPY_Ins.prototype.constructor = COPY_Ins;
COPY_Ins.prototype.$class = 
    new Class("COPY_Ins", COPY_Ins);

COPY_Ins.prototype.execute = function(svm, addr) {
    svm.copy(addr);
};

var ADD_Ins = function() {
    if (!(this instanceof ADD_Ins)) return new ADD_Ins();
    SVMInstruction.call(this, "ADD", SVMOp.ADD);
};

ADD_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "ADD_Ins extends SVMInstruction");
ADD_Ins.prototype.constructor = ADD_Ins;
ADD_Ins.prototype.$class = 
    new Class("ADD_Ins", ADD_Ins);

ADD_Ins.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    var lhsType = lhs.getType();
    var rhsType = rhs.getType();
    if (lhsType === Value.STRING || rhsType === Value.STRING) {
        svm.pushString(svm.stringify(lhs) + svm.stringify(rhs));
    } else if (!lhs.isNumeric() || !rhs.isNumeric()) {
        throw new RuntimeException("Illegal to apply " + this.getName() +
        " to " + lhs + " and " + rhs);
    } else if (lhsType === Value.INTEGER && rhsType === Value.INTEGER) {
        var x = lhs.getIntegerValue();
        var y = rhs.getIntegerValue();
        svm.pushInteger(x + y);
    } else {
        var x = lhs.getDoubleValue();
        var y = rhs.getDoubleValue();
        svm.pushDouble(x + y);
    }
};

var SUB_Ins = function() {
    if (!(this instanceof SUB_Ins)) return new SUB_Ins();
    ArithmeticOp.call(this, "SUB", SVMOp.SUB);
};

SUB_Ins.prototype = 
    jslib.inheritPrototype(ArithmeticOp, "SUB_Ins extends ArithmeticOp");
SUB_Ins.prototype.constructor = SUB_Ins;
SUB_Ins.prototype.$class = 
    new Class("SUB_Ins", SUB_Ins);

SUB_Ins.prototype.applyInteger = function(x, y) {
    return x - y;
};

SUB_Ins.prototype.applyDouble = function(x, y) {
    return x - y;
};

var MUL_Ins = function() {
    if (!(this instanceof MUL_Ins)) return new MUL_Ins();
    ArithmeticOp.call(this, "MUL", SVMOp.MUL);
};

MUL_Ins.prototype = 
    jslib.inheritPrototype(ArithmeticOp, "MUL_Ins extends ArithmeticOp");
MUL_Ins.prototype.constructor = MUL_Ins;
MUL_Ins.prototype.$class = 
    new Class("MUL_Ins", MUL_Ins);

MUL_Ins.prototype.applyInteger = function(x, y) {
    return x * y;
};

MUL_Ins.prototype.applyDouble = function(x, y) {
    return x * y;
};

var DIV_Ins = function() {
    if (!(this instanceof DIV_Ins)) return new DIV_Ins();
    SVMInstruction.call(this, "DIV", SVMOp.DIV);
};

DIV_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "DIV_Ins extends SVMInstruction");
DIV_Ins.prototype.constructor = DIV_Ins;
DIV_Ins.prototype.$class = 
    new Class("DIV_Ins", DIV_Ins);

DIV_Ins.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    if (!lhs.isNumeric() || !rhs.isNumeric()) {
        throw new RuntimeException("Illegal to apply " + this.getName() +
        " to " + lhs + " and " + rhs);
    }
    if (lhs.getType() === Value.INTEGER && rhs.getType() === Value.INTEGER) {
        var num = lhs.getIntegerValue();
        var den = rhs.getIntegerValue();
        if (den !== 0 && toInt((num / den))* den === num) {
            svm.pushInteger(toInt((num / den)));
        } else {
            svm.pushDouble(num / den);
        }
    } else {
        svm.pushDouble(lhs.getDoubleValue() / rhs.getDoubleValue());
    }
};

var IDIV_Ins = function() {
    if (!(this instanceof IDIV_Ins)) return new IDIV_Ins();
    SVMInstruction.call(this, "IDIV", SVMOp.IDIV);
};

IDIV_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "IDIV_Ins extends SVMInstruction");
IDIV_Ins.prototype.constructor = IDIV_Ins;
IDIV_Ins.prototype.$class = 
    new Class("IDIV_Ins", IDIV_Ins);

IDIV_Ins.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    if (!lhs.isNumeric() || !rhs.isNumeric()) {
        throw new RuntimeException("Illegal to apply " + this.getName() +
        " to " + lhs + " and " + rhs);
    }
    var result = toInt((lhs.getDoubleValue() / rhs.getDoubleValue()));
    svm.pushInteger(result);
};

var REM_Ins = function() {
    if (!(this instanceof REM_Ins)) return new REM_Ins();
    SVMInstruction.call(this, "REM", SVMOp.REM);
};

REM_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "REM_Ins extends SVMInstruction");
REM_Ins.prototype.constructor = REM_Ins;
REM_Ins.prototype.$class = 
    new Class("REM_Ins", REM_Ins);

REM_Ins.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    var lhs = svm.pop();
    if (!lhs.isNumeric() || !rhs.isNumeric()) {
        throw new RuntimeException("Illegal to apply " + this.getName() +
        " to " + lhs + " and " + rhs);
    }
    if (lhs.getType() === Value.INTEGER && rhs.getType() === Value.INTEGER) {
        var num = lhs.getIntegerValue();
        var den = rhs.getIntegerValue();
        if (den === 0) {
            svm.pushDouble(num % 0.0);
        } else {
            svm.pushInteger(num % den);
        }
    } else {
        svm.pushDouble(lhs.getDoubleValue() % rhs.getDoubleValue());
    }
};

var NEG_Ins = function() {
    if (!(this instanceof NEG_Ins)) return new NEG_Ins();
    SVMInstruction.call(this, "NEG", SVMOp.NEG);
};

NEG_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "NEG_Ins extends SVMInstruction");
NEG_Ins.prototype.constructor = NEG_Ins;
NEG_Ins.prototype.$class = 
    new Class("NEG_Ins", NEG_Ins);

NEG_Ins.prototype.execute = function(svm, addr) {
    var rhs = svm.pop();
    if (!rhs.isNumeric()) {
        throw new RuntimeException("Illegal to apply " + this.getName() +
        " to " + rhs);
    }
    if (rhs.getType() === Value.INTEGER) {
        var x = rhs.getIntegerValue();
        svm.pushInteger(-x);
    } else {
        var x = rhs.getDoubleValue();
        svm.pushDouble(-x);
    }
};

var EQ_Ins = function() {
    if (!(this instanceof EQ_Ins)) return new EQ_Ins();
    RelationalOp.call(this, "EQ", SVMOp.EQ);
};

EQ_Ins.prototype = 
    jslib.inheritPrototype(RelationalOp, "EQ_Ins extends RelationalOp");
EQ_Ins.prototype.constructor = EQ_Ins;
EQ_Ins.prototype.$class = 
    new Class("EQ_Ins", EQ_Ins);

EQ_Ins.prototype.applyObject = function(x, y) {
    return x === y;
};

EQ_Ins.prototype.applyInteger = function(x, y) {
    return x === y;
};

EQ_Ins.prototype.applyDouble = function(x, y) {
    return x === y;
};

var NE_Ins = function() {
    if (!(this instanceof NE_Ins)) return new NE_Ins();
    RelationalOp.call(this, "NE", SVMOp.NE);
};

NE_Ins.prototype = 
    jslib.inheritPrototype(RelationalOp, "NE_Ins extends RelationalOp");
NE_Ins.prototype.constructor = NE_Ins;
NE_Ins.prototype.$class = 
    new Class("NE_Ins", NE_Ins);

NE_Ins.prototype.applyObject = function(x, y) {
    return x !== y;
};

NE_Ins.prototype.applyInteger = function(x, y) {
    return x !== y;
};

NE_Ins.prototype.applyDouble = function(x, y) {
    return x !== y;
};

var LT_Ins = function() {
    if (!(this instanceof LT_Ins)) return new LT_Ins();
    RelationalOp.call(this, "LT", SVMOp.LT);
};

LT_Ins.prototype = 
    jslib.inheritPrototype(RelationalOp, "LT_Ins extends RelationalOp");
LT_Ins.prototype.constructor = LT_Ins;
LT_Ins.prototype.$class = 
    new Class("LT_Ins", LT_Ins);

LT_Ins.prototype.applyInteger = function(x, y) {
    return x < y;
};

LT_Ins.prototype.applyDouble = function(x, y) {
    return x < y;
};

var LE_Ins = function() {
    if (!(this instanceof LE_Ins)) return new LE_Ins();
    RelationalOp.call(this, "LE", SVMOp.LE);
};

LE_Ins.prototype = 
    jslib.inheritPrototype(RelationalOp, "LE_Ins extends RelationalOp");
LE_Ins.prototype.constructor = LE_Ins;
LE_Ins.prototype.$class = 
    new Class("LE_Ins", LE_Ins);

LE_Ins.prototype.applyInteger = function(x, y) {
    return x <= y;
};

LE_Ins.prototype.applyDouble = function(x, y) {
    return x <= y;
};

var GT_Ins = function() {
    if (!(this instanceof GT_Ins)) return new GT_Ins();
    RelationalOp.call(this, "GT", SVMOp.GT);
};

GT_Ins.prototype = 
    jslib.inheritPrototype(RelationalOp, "GT_Ins extends RelationalOp");
GT_Ins.prototype.constructor = GT_Ins;
GT_Ins.prototype.$class = 
    new Class("GT_Ins", GT_Ins);

GT_Ins.prototype.applyInteger = function(x, y) {
    return x > y;
};

GT_Ins.prototype.applyDouble = function(x, y) {
    return x > y;
};

var GE_Ins = function() {
    if (!(this instanceof GE_Ins)) return new GE_Ins();
    RelationalOp.call(this, "GE", SVMOp.GE);
};

GE_Ins.prototype = 
    jslib.inheritPrototype(RelationalOp, "GE_Ins extends RelationalOp");
GE_Ins.prototype.constructor = GE_Ins;
GE_Ins.prototype.$class = 
    new Class("GE_Ins", GE_Ins);

GE_Ins.prototype.applyInteger = function(x, y) {
    return x >= y;
};

GE_Ins.prototype.applyDouble = function(x, y) {
    return x >= y;
};

var JUMP_Ins = function() {
    if (!(this instanceof JUMP_Ins)) return new JUMP_Ins();
    SVMAddressInstruction.call(this, "JUMP", SVMOp.JUMP);
};

JUMP_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "JUMP_Ins extends SVMAddressInstruction");
JUMP_Ins.prototype.constructor = JUMP_Ins;
JUMP_Ins.prototype.$class = 
    new Class("JUMP_Ins", JUMP_Ins);

JUMP_Ins.prototype.execute = function(svm, addr) {
    svm.setPC(addr);
};

var JUMPT_Ins = function() {
    if (!(this instanceof JUMPT_Ins)) return new JUMPT_Ins();
    SVMAddressInstruction.call(this, "JUMPT", SVMOp.JUMPT);
};

JUMPT_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "JUMPT_Ins extends SVMAddressInstruction");
JUMPT_Ins.prototype.constructor = JUMPT_Ins;
JUMPT_Ins.prototype.$class = 
    new Class("JUMPT_Ins", JUMPT_Ins);

JUMPT_Ins.prototype.execute = function(svm, addr) {
    if (svm.popBoolean()) svm.setPC(addr);
};

var JUMPF_Ins = function() {
    if (!(this instanceof JUMPF_Ins)) return new JUMPF_Ins();
    SVMAddressInstruction.call(this, "JUMPF", SVMOp.JUMPF);
};

JUMPF_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "JUMPF_Ins extends SVMAddressInstruction");
JUMPF_Ins.prototype.constructor = JUMPF_Ins;
JUMPF_Ins.prototype.$class = 
    new Class("JUMPF_Ins", JUMPF_Ins);

JUMPF_Ins.prototype.execute = function(svm, addr) {
    if (!svm.popBoolean()) svm.setPC(addr);
};

var DISPATCH_Ins = function() {
    if (!(this instanceof DISPATCH_Ins)) return new DISPATCH_Ins();
    SVMInstruction.call(this, "DISPATCH", SVMOp.DISPATCH);
};

DISPATCH_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "DISPATCH_Ins extends SVMInstruction");
DISPATCH_Ins.prototype.constructor = DISPATCH_Ins;
DISPATCH_Ins.prototype.$class = 
    new Class("DISPATCH_Ins", DISPATCH_Ins);

DISPATCH_Ins.prototype.execute = function(svm, addr) {
    svm.pushFrame();
    svm.getCurrentFrame().setReturnAddress(svm.getPC());
    svm.setPC(svm.popInteger());
};

var TRY_Ins = function() {
    if (!(this instanceof TRY_Ins)) return new TRY_Ins();
    SVMAddressInstruction.call(this, "TRY", SVMOp.TRY);
};

TRY_Ins.prototype = 
    jslib.inheritPrototype(SVMAddressInstruction, "TRY_Ins extends SVMAddressInstruction");
TRY_Ins.prototype.constructor = TRY_Ins;
TRY_Ins.prototype.$class = 
    new Class("TRY_Ins", TRY_Ins);

TRY_Ins.prototype.execute = function(svm, addr) {
    svm.pushExceptionFrame(addr);
};

var ENDTRY_Ins = function() {
    if (!(this instanceof ENDTRY_Ins)) return new ENDTRY_Ins();
    SVMInstruction.call(this, "ENDTRY", SVMOp.ENDTRY);
};

ENDTRY_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "ENDTRY_Ins extends SVMInstruction");
ENDTRY_Ins.prototype.constructor = ENDTRY_Ins;
ENDTRY_Ins.prototype.$class = 
    new Class("ENDTRY_Ins", ENDTRY_Ins);

ENDTRY_Ins.prototype.execute = function(svm, addr) {
    svm.popExceptionFrame();
};

var THROW_Ins = function() {
    if (!(this instanceof THROW_Ins)) return new THROW_Ins();
    SVMInstruction.call(this, "THROW", SVMOp.THROW);
};

THROW_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "THROW_Ins extends SVMInstruction");
THROW_Ins.prototype.constructor = THROW_Ins;
THROW_Ins.prototype.$class = 
    new Class("THROW_Ins", THROW_Ins);

THROW_Ins.prototype.execute = function(svm, addr) {
    var v = svm.pop();
    svm.throwException(new RuntimeException(v.toString()), v);
};

var NOT_Ins = function() {
    if (!(this instanceof NOT_Ins)) return new NOT_Ins();
    SVMInstruction.call(this, "NOT", SVMOp.NOT);
};

NOT_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "NOT_Ins extends SVMInstruction");
NOT_Ins.prototype.constructor = NOT_Ins;
NOT_Ins.prototype.$class = 
    new Class("NOT_Ins", NOT_Ins);

NOT_Ins.prototype.execute = function(svm, addr) {
    var v = svm.pop();
    var type = v.getType();
    if (type === Value.BOOLEAN) {
        svm.pushBoolean(!v.getBooleanValue());
    } else {
        svm.pushInteger(~v.getIntegerValue());
    }
};

var AND_Ins = function() {
    if (!(this instanceof AND_Ins)) return new AND_Ins();
    LogicalOp.call(this, "AND", SVMOp.AND);
};

AND_Ins.prototype = 
    jslib.inheritPrototype(LogicalOp, "AND_Ins extends LogicalOp");
AND_Ins.prototype.constructor = AND_Ins;
AND_Ins.prototype.$class = 
    new Class("AND_Ins", AND_Ins);

AND_Ins.prototype.applyInteger = function(x, y) {
    return x & y;
};

var OR_Ins = function() {
    if (!(this instanceof OR_Ins)) return new OR_Ins();
    LogicalOp.call(this, "OR", SVMOp.OR);
};

OR_Ins.prototype = 
    jslib.inheritPrototype(LogicalOp, "OR_Ins extends LogicalOp");
OR_Ins.prototype.constructor = OR_Ins;
OR_Ins.prototype.$class = 
    new Class("OR_Ins", OR_Ins);

OR_Ins.prototype.applyInteger = function(x, y) {
    return x | y;
};

var XOR_Ins = function() {
    if (!(this instanceof XOR_Ins)) return new XOR_Ins();
    LogicalOp.call(this, "XOR", SVMOp.XOR);
};

XOR_Ins.prototype = 
    jslib.inheritPrototype(LogicalOp, "XOR_Ins extends LogicalOp");
XOR_Ins.prototype.constructor = XOR_Ins;
XOR_Ins.prototype.$class = 
    new Class("XOR_Ins", XOR_Ins);

XOR_Ins.prototype.applyInteger = function(x, y) {
    return x ^ y;
};

var LSH_Ins = function() {
    if (!(this instanceof LSH_Ins)) return new LSH_Ins();
    LogicalOp.call(this, "LSH", SVMOp.LSH);
};

LSH_Ins.prototype = 
    jslib.inheritPrototype(LogicalOp, "LSH_Ins extends LogicalOp");
LSH_Ins.prototype.constructor = LSH_Ins;
LSH_Ins.prototype.$class = 
    new Class("LSH_Ins", LSH_Ins);

LSH_Ins.prototype.applyInteger = function(x, y) {
    return (y < 0) ? (x >>> -y) : (x << y);
};

var ASH_Ins = function() {
    if (!(this instanceof ASH_Ins)) return new ASH_Ins();
    LogicalOp.call(this, "ASH", SVMOp.ASH);
};

ASH_Ins.prototype = 
    jslib.inheritPrototype(LogicalOp, "ASH_Ins extends LogicalOp");
ASH_Ins.prototype.constructor = ASH_Ins;
ASH_Ins.prototype.$class = 
    new Class("ASH_Ins", ASH_Ins);

ASH_Ins.prototype.applyInteger = function(x, y) {
    return (y < 0) ? (x >> -y) : (x << y);
};

var CALL_Ins = function() {
    if (!(this instanceof CALL_Ins)) return new CALL_Ins();
    SVMInstruction.call(this, "CALL", SVMOp.CALL);
};

CALL_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "CALL_Ins extends SVMInstruction");
CALL_Ins.prototype.constructor = CALL_Ins;
CALL_Ins.prototype.$class = 
    new Class("CALL_Ins", CALL_Ins);

CALL_Ins.prototype.assemble = function(cv, scanner) {
    var token = scanner.nextToken();
    var type = scanner.getTokenType(token);
    if (type === TokenScanner.NUMBER) {
        cv.addWord((this.getCode() << 24) | Integer.parseInt(token));
    } else if (type === TokenScanner.WORD) {
        var next = scanner.nextToken();
        if (jslib.equals(next, ".")) {
            token += "." + scanner.nextToken();
            cv.addWord((SVMOp.CALLM << 24) | cv.stringRef(token));
        } else {
            scanner.saveToken(next);
            cv.addWord((SVMOp.CALL << 24) | cv.labelRef(token));
        }
    } else {
        throw new SyntaxError("Illegal argument " + token);
    }
};

CALL_Ins.prototype.execute = function(svm, addr) {
    svm.executeCallHooks();
    svm.pushFrame();
    var cf = svm.getCurrentFrame();
    cf.setReturnAddress(svm.getPC());
    cf.setArgumentCount(svm.getNARGSCount());
    svm.setPC(addr);
};

CALL_Ins.prototype.unparse = function(svm, addr) {
    return "CALL " + addr;
};

var CALLM_Ins = function() {
    if (!(this instanceof CALLM_Ins)) return new CALLM_Ins();
    SVMNameInstruction.call(this, "CALLM", SVMOp.CALLM);
};

CALLM_Ins.prototype = 
    jslib.inheritPrototype(SVMNameInstruction, "CALLM_Ins extends SVMNameInstruction");
CALLM_Ins.prototype.constructor = CALLM_Ins;
CALLM_Ins.prototype.$class = 
    new Class("CALLM_Ins", CALLM_Ins);

CALLM_Ins.prototype.assemble = function(cv, scanner) {
    var token = scanner.nextToken();
    if (scanner.getTokenType(token) !== TokenScanner.WORD) {
        throw new SyntaxError("CALLM requires a class and method name");
    }
    scanner.verifyToken(".");
    token += "." + scanner.nextToken();
    cv.addWord((SVMOp.CALLM << 24) | cv.stringRef(token));
};

CALLM_Ins.prototype.execute = function(svm, addr) {
    var cf = svm.getCurrentFrame();
    cf.setArgumentCount(svm.getNARGSCount());
    var name = svm.getString(addr);
    var dot = name.lastIndexOf(".");
    var cname = (dot === -1) ? this.receiverClass(svm) : name.substring(0, dot);
    var mname = name.substring(dot + 1);
    var c = SVMClass.forName(cname);
    var m = c.getMethod(mname);
    m.execute(svm, null);
};

CALLM_Ins.prototype.unparse = function(svm, addr) {
    return "CALLM " + svm.getString(addr);
};

CALLM_Ins.prototype.receiverClass = function(svm) {
    var nArgs = svm.getArgumentCount();
    return svm.peekBack(nArgs).getClassName();
};

var CALLFN_Ins = function() {
    if (!(this instanceof CALLFN_Ins)) return new CALLFN_Ins();
    SVMInstruction.call(this, "CALLFN", SVMOp.CALLFN);
};

CALLFN_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "CALLFN_Ins extends SVMInstruction");
CALLFN_Ins.prototype.constructor = CALLFN_Ins;
CALLFN_Ins.prototype.$class = 
    new Class("CALLFN_Ins", CALLFN_Ins);

CALLFN_Ins.prototype.execute = function(svm, addr) {
    svm.call(svm.getNARGSCount());
};

var RETURN_Ins = function() {
    if (!(this instanceof RETURN_Ins)) return new RETURN_Ins();
    SVMInstruction.call(this, "RETURN", SVMOp.RETURN);
};

RETURN_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "RETURN_Ins extends SVMInstruction");
RETURN_Ins.prototype.constructor = RETURN_Ins;
RETURN_Ins.prototype.$class = 
    new Class("RETURN_Ins", RETURN_Ins);

RETURN_Ins.prototype.execute = function(svm, addr) {
    svm.setPC(svm.getCurrentFrame().getReturnAddress());
    svm.popFrame();
    if (svm.getCurrentFrame() === null) svm.setPC(-1);
    svm.executeReturnHooks();
};

var PUSHLOC_Ins = function() {
    if (!(this instanceof PUSHLOC_Ins)) return new PUSHLOC_Ins();
    SVMOffsetInstruction.call(this, "PUSHLOC", SVMOp.PUSHLOC);
};

PUSHLOC_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "PUSHLOC_Ins extends SVMOffsetInstruction");
PUSHLOC_Ins.prototype.constructor = PUSHLOC_Ins;
PUSHLOC_Ins.prototype.$class = 
    new Class("PUSHLOC_Ins", PUSHLOC_Ins);

PUSHLOC_Ins.prototype.execute = function(svm, addr) {
    svm.push(svm.getCurrentFrame().getLocal(addr));
};

var POPLOC_Ins = function() {
    if (!(this instanceof POPLOC_Ins)) return new POPLOC_Ins();
    SVMOffsetInstruction.call(this, "POPLOC", SVMOp.POPLOC);
};

POPLOC_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "POPLOC_Ins extends SVMOffsetInstruction");
POPLOC_Ins.prototype.constructor = POPLOC_Ins;
POPLOC_Ins.prototype.$class = 
    new Class("POPLOC_Ins", POPLOC_Ins);

POPLOC_Ins.prototype.execute = function(svm, addr) {
    svm.getCurrentFrame().setLocal(addr, svm.pop());
};

var LOCALS_Ins = function() {
    if (!(this instanceof LOCALS_Ins)) return new LOCALS_Ins();
    SVMOffsetInstruction.call(this, "LOCALS", SVMOp.LOCALS);
};

LOCALS_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "LOCALS_Ins extends SVMOffsetInstruction");
LOCALS_Ins.prototype.constructor = LOCALS_Ins;
LOCALS_Ins.prototype.$class = 
    new Class("LOCALS_Ins", LOCALS_Ins);

LOCALS_Ins.prototype.execute = function(svm, addr) {
    svm.getCurrentFrame().setFrameSize(addr);
};

LOCALS_Ins.prototype.assemble = function(cv, scanner) {
    var nLocals = 0;
    while (true) {
        var token = scanner.nextToken();
        if (jslib.equals(token, "\n")) break;
        cv.defineSymbol(token, nLocals++);
    }
    scanner.saveToken("\n");
    cv.addWord((this.getCode() << 24) | nLocals);
};

var ARG_Ins = function() {
    if (!(this instanceof ARG_Ins)) return new ARG_Ins();
    SVMVarInstruction.call(this, "ARG", SVMOp.ARG);
};

ARG_Ins.prototype = 
    jslib.inheritPrototype(SVMVarInstruction, "ARG_Ins extends SVMVarInstruction");
ARG_Ins.prototype.constructor = ARG_Ins;
ARG_Ins.prototype.$class = 
    new Class("ARG_Ins", ARG_Ins);

ARG_Ins.prototype.execute = function(svm, addr) {
    var cf = svm.getCurrentFrame();
    var name = svm.getString(addr);
    var v = svm.pop();
    cf.declareVar(name);
    cf.setVar(name, v);
};

var VAR_Ins = function() {
    if (!(this instanceof VAR_Ins)) return new VAR_Ins();
    SVMVarInstruction.call(this, "VAR", SVMOp.VAR);
};

VAR_Ins.prototype = 
    jslib.inheritPrototype(SVMVarInstruction, "VAR_Ins extends SVMVarInstruction");
VAR_Ins.prototype.constructor = VAR_Ins;
VAR_Ins.prototype.$class = 
    new Class("VAR_Ins", VAR_Ins);

VAR_Ins.prototype.execute = function(svm, addr) {
    var cf = svm.getCurrentFrame();
    var name = svm.getString(addr);
    cf.declareVar(name);
};

var PARAMS_Ins = function() {
    if (!(this instanceof PARAMS_Ins)) return new PARAMS_Ins();
    SVMOffsetInstruction.call(this, "PARAMS", SVMOp.PARAMS);
};

PARAMS_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "PARAMS_Ins extends SVMOffsetInstruction");
PARAMS_Ins.prototype.constructor = PARAMS_Ins;
PARAMS_Ins.prototype.$class = 
    new Class("PARAMS_Ins", PARAMS_Ins);

PARAMS_Ins.prototype.execute = function(svm, addr) {
    var nParams = addr;
    var nArgs = svm.getArgumentCount();
    if (nArgs !== -1) {
        svm.checkArgumentCount(nArgs, nParams);
        svm.setStackBase(nArgs);
    }
};

var NARGS_Ins = function() {
    if (!(this instanceof NARGS_Ins)) return new NARGS_Ins();
    SVMOffsetInstruction.call(this, "NARGS", SVMOp.NARGS);
};

NARGS_Ins.prototype = 
    jslib.inheritPrototype(SVMOffsetInstruction, "NARGS_Ins extends SVMOffsetInstruction");
NARGS_Ins.prototype.constructor = NARGS_Ins;
NARGS_Ins.prototype.$class = 
    new Class("NARGS_Ins", NARGS_Ins);

NARGS_Ins.prototype.execute = function(svm, addr) {
    /* Empty */
};

var VARGS_Ins = function() {
    if (!(this instanceof VARGS_Ins)) return new VARGS_Ins();
    SVMInstruction.call(this, "VARGS", SVMOp.VARGS);
};

VARGS_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "VARGS_Ins extends SVMInstruction");
VARGS_Ins.prototype.constructor = VARGS_Ins;
VARGS_Ins.prototype.$class = 
    new Class("VARGS_Ins", VARGS_Ins);

VARGS_Ins.prototype.execute = function(svm, addr) {
    var args = new SVMArray();
    var nArgs = svm.getArgumentCount();
    if (nArgs !== -1) {
        for (var i = 0; i < nArgs; i++) {
            args.add(0, svm.pop());
        }
    }
    svm.push(Value.createObject(args, "Array"));
};

var PUSHVAR_Ins = function() {
    if (!(this instanceof PUSHVAR_Ins)) return new PUSHVAR_Ins();
    SVMVarInstruction.call(this, "PUSHVAR", SVMOp.PUSHVAR);
};

PUSHVAR_Ins.prototype = 
    jslib.inheritPrototype(SVMVarInstruction, "PUSHVAR_Ins extends SVMVarInstruction");
PUSHVAR_Ins.prototype.constructor = PUSHVAR_Ins;
PUSHVAR_Ins.prototype.$class = 
    new Class("PUSHVAR_Ins", PUSHVAR_Ins);

PUSHVAR_Ins.prototype.execute = function(svm, addr) {
    var cf = svm.getCurrentFrame();
    var name = svm.getString(addr);
    while (cf !== null) {
        if (cf.isDeclared(name)) {
            svm.push(cf.getVar(name));
            return;
        }
        cf = cf.getFrameLink();
    }
    if (svm.isGlobal(name)) {
        svm.push(svm.getGlobal(name));
    } else {
        throw new RuntimeException(name + " has not been declared");
    }
};

var POPVAR_Ins = function() {
    if (!(this instanceof POPVAR_Ins)) return new POPVAR_Ins();
    SVMVarInstruction.call(this, "POPVAR", SVMOp.POPVAR);
};

POPVAR_Ins.prototype = 
    jslib.inheritPrototype(SVMVarInstruction, "POPVAR_Ins extends SVMVarInstruction");
POPVAR_Ins.prototype.constructor = POPVAR_Ins;
POPVAR_Ins.prototype.$class = 
    new Class("POPVAR_Ins", POPVAR_Ins);

POPVAR_Ins.prototype.execute = function(svm, addr) {
    var cf = svm.getCurrentFrame();
    var name = svm.getString(addr);
    while (cf !== null) {
        if (cf.isDeclared(name)) {
            cf.setVar(name, svm.pop());
            return;
        }
        cf = cf.getFrameLink();
    }
    if (svm.isGlobal(name)) {
        svm.setGlobal(name, svm.pop());
    } else {
        throw new RuntimeException(name + " has not been declared");
    }
};

var PUSHFRM_Ins = function() {
    if (!(this instanceof PUSHFRM_Ins)) return new PUSHFRM_Ins();
    SVMInstruction.call(this, "PUSHFRM", SVMOp.PUSHFRM);
};

PUSHFRM_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "PUSHFRM_Ins extends SVMInstruction");
PUSHFRM_Ins.prototype.constructor = PUSHFRM_Ins;
PUSHFRM_Ins.prototype.$class = 
    new Class("PUSHFRM_Ins", PUSHFRM_Ins);

PUSHFRM_Ins.prototype.execute = function(svm, addr) {
    throw new RuntimeException("Not yet implemented");
};

var POPFRM_Ins = function() {
    if (!(this instanceof POPFRM_Ins)) return new POPFRM_Ins();
    SVMInstruction.call(this, "POPFRM", SVMOp.POPFRM);
};

POPFRM_Ins.prototype = 
    jslib.inheritPrototype(SVMInstruction, "POPFRM_Ins extends SVMInstruction");
POPFRM_Ins.prototype.constructor = POPFRM_Ins;
POPFRM_Ins.prototype.$class = 
    new Class("POPFRM_Ins", POPFRM_Ins);

POPFRM_Ins.prototype.execute = function(svm, addr) {
    throw new RuntimeException("Not yet implemented");
};


/* SVMEventClosure.js */

var SVMEventClosure = function(fn) {
    if (!(this instanceof SVMEventClosure)) return new SVMEventClosure(fn);
    this.fn = fn;
    this.args = new ArrayList();
};

SVMEventClosure.prototype.add = function(arg) {
    this.args.add(arg);
};

SVMEventClosure.prototype.getArgumentCount = function() {
    return this.args.size();
};

SVMEventClosure.prototype.pushEventData = function(svm) {
    for (var i = this.args.size() - 1; i >= 0; i--) {
        svm.push(this.args.get(i));
    }
    svm.push(this.fn);
};


/* SVMClass.js */

var SVMClass = function() {
    if (!(this instanceof SVMClass)) return new SVMClass();
    this.methodTable = new HashMap();
};

SVMClass.prototype.getMethod = function(name) {
    var m = this.methodTable.get(name);
    if (m === null) throw new RuntimeException(name + " is not defined");
    return m;
};

SVMClass.prototype.defineMethod = function(name, m) {
    this.methodTable.put(name, m);
};

SVMClass.prototype.init = function(svm) {
    var fn = this.methodTable.get("_init");
    if (fn !== null) fn.execute(svm, null);
};

SVMClass.isDefined = function(name) {
    return SVMClass.classTable.containsKey(name);
};

SVMClass.forName = function(name) {
    var c = SVMClass.classTable.get(name);
    if (c === null) throw new RuntimeException(name + " is not defined");
    return c;
};

SVMClass.defineClass = function(svm, name, c) {
    c.init(svm);
    if (SVMClass.classTable === null) SVMClass.classTable = new HashMap();
    SVMClass.classTable.put(name, c);
    svm.setGlobal(name, Value.createObject(name, "Class"));
};

SVMClass.classTable = null;

/* SVMMethod.js */

var SVMMethod = function() {
    /* Empty */
};

SVMMethod.prototype.isConstant = function() {
    return false;
};


/* SVMGlobalClass.js */

var SVMGlobalClass = function() {
    if (!(this instanceof SVMGlobalClass)) return new SVMGlobalClass();
    SVMClass.call(this);
    this.defineMethod("get", new Global_get());
    this.defineMethod("set", new Global_set());
    this.defineMethod("isDefined", new Global_isDefined());
};

SVMGlobalClass.prototype = 
    jslib.inheritPrototype(SVMClass, "SVMGlobalClass extends SVMClass");
SVMGlobalClass.prototype.constructor = SVMGlobalClass;
SVMGlobalClass.prototype.$class = 
    new Class("SVMGlobalClass", SVMGlobalClass);

var Global_get = function() {
    SVMMethod.call(this);
};

Global_get.prototype =
    jslib.inheritPrototype(SVMMethod, "Global_get extends SVMMethod");
Global_get.prototype.constructor = Global_get;
Global_get.prototype.$class = 
    new Class("Global_get", Global_get);

Global_get.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Global.get", "S");
    svm.push(svm.getGlobal(svm.popString()));
};

var Global_set = function() {
    SVMMethod.call(this);
};

Global_set.prototype =
    jslib.inheritPrototype(SVMMethod, "Global_set extends SVMMethod");
Global_set.prototype.constructor = Global_set;
Global_set.prototype.$class = 
    new Class("Global_set", Global_set);

Global_set.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Global.set", "S*");
    var value = svm.pop();
    var key = svm.popString();
    svm.setGlobal(key, value);
};

var Global_isDefined = function() {
    SVMMethod.call(this);
};

Global_isDefined.prototype =
    jslib.inheritPrototype(SVMMethod, "Global_isDefined extends SVMMethod");
Global_isDefined.prototype.constructor = Global_isDefined;
Global_isDefined.prototype.$class = 
    new Class("Global_isDefined", Global_isDefined);

Global_isDefined.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Global.isDefined", "S");
    svm.pushBoolean(svm.isGlobal(svm.popString()));
};


/* SVMArray.js */

var SVMArray = function() {
    ArrayList.call(this);
};

SVMArray.prototype =
    jslib.inheritPrototype(ArrayList, "SVMArray extends ArrayList");
SVMArray.prototype.constructor = SVMArray;
SVMArray.prototype.$class = 
    new Class("SVMArray", SVMArray);

SVMArray.prototype.toString = function() {
    var n = this.size();
    var str = "";
    for (var i = 0; i < n; i++) {
        if (i > 0) str += ",";
        str += this.get(i);
    }
    return str;
};


/* SVMPackage.js */

var SVMPackage = function() {
    JSPackage.call(this);
};

SVMPackage.prototype =
    jslib.inheritPrototype(JSPackage, "SVMPackage extends JSPackage");
SVMPackage.prototype.constructor = SVMPackage;
SVMPackage.prototype.$class = 
    new Class("SVMPackage", SVMPackage);

SVMPackage.prototype.init = function(obj) {
    if (obj !== null) {
        this.defineClasses(obj);
        this.defineGlobals(obj);
    }
};

SVMPackage.prototype.defineClasses = function(svm) {
    /* Empty */
};

SVMPackage.prototype.defineGlobals = function(svm) {
    /* Empty */
};

SVMPackage.prototype.getDependencies = function() {
    return jslib.newArray(0);
};

SVMPackage.prototype.getWrapper = function() {
    return jslib.newArray(0);
};

SVMPackage.prototype.defineClass = function(svm, name, c) {
    SVMClass.defineClass(svm, name, c);
};

SVMPackage.prototype.defineMethod = function(svm, name, methodName) {
    var dot = methodName.indexOf(".");
    if (dot === -1) {
        throw new RuntimeException("Unqualified method name: " +
        methodName);
    }
    var className = methodName.substring(0, dot);
    methodName = methodName.substring(dot + 1);
    var obj = new SVMMethodClosure(null, className, methodName);
    svm.setGlobal(name, Value.createObject(obj, "MethodClosure"));
};


/* SVM.js */

var SVM = function() {
    if (!(this instanceof SVM)) return new SVM();
    Controller.call(this);
    this.globals = new HashMap();
    this.properties = new HashMap();
    this.valueStack = new Stack();
    this.frameStack = new Stack();
    this.exceptionStack = new Stack();
    this.eventQueue = new ArrayDeque();
    this.cf = new SVMStackFrame();
    this.pc = 0;
    this.code = null;
    this.console = null;
    this.source = null;
    this.program = null;
    this.traceFlag = false;
    this.traceErrors = false;
    this.stepMode = SVM.BY_INSTRUCTION;
    this.callHooks = new ArrayList();
    this.returnHooks = new ArrayList();
    this.stepHooks = new ArrayList();
    this.stopHooks = new ArrayList();
    this.setState(SVM.INITIAL);
    this.setErrorHandler(new SVMErrorHandler(this));
    this.defineClass("Core", new SVMCoreClass());
    this.defineClass("Console", new SVMConsoleClass());
    this.defineClass("Global", new SVMGlobalClass());
};

SVM.prototype = 
    jslib.inheritPrototype(Controller, "SVM extends Controller");
SVM.prototype.constructor = SVM;
SVM.prototype.$class = 
    new Class("SVM", SVM);

SVM.prototype.reset = function() {
    this.valueStack.clear();
    this.frameStack.clear();
    this.exceptionStack.clear();
    this.setState(SVM.INITIAL);
    this.setPC(0);
};

SVM.prototype.run = function() {
    if (this.code === null) {
        this.setState(SVM.INITIAL);
    } else {
        this.setState(SVM.RUNNING);
        while (this.getState() === SVM.RUNNING) {
            try {
                this.executeInstruction();
             } catch (ex) {
                var msg = (ex instanceof RuntimeException) ? RuntimeException.patchMessage(ex) : ex.toString();
                if (this.traceErrors) ex.printStackTrace();
                this.signalError(msg);
            }
        }
    }
};

SVM.prototype.setProgram = function(program) {
    this.program = program;
};

SVM.prototype.getProgram = function() {
    return this.program;
};

SVM.prototype.stopAction = function() {
    if (this.isRunning()) this.stop(SVM.STOPPED);
    this.executeStopHooks();
};

SVM.prototype.step = function() {
    if (this.code === null) {
        this.setState(SVM.INITIAL);
    } else {
        if (this.stepMode === SVM.BY_INSTRUCTION) {
            this.executeInstruction();
        } else {
            this.executeInstruction();
            while (this.pc >= 0 && this.pc < this.code.length) {
                var state = this.getState();
                if (state === SVM.ERROR || state === SVM.FINISHED) break;
                this.executeInstruction();
                if (((this.lastInstruction >> 24) & 0xFF) === SVMOp.STMT) break;
            }
        }
        this.executeStepHooks();
    }
};

SVM.prototype.isFinished = function() {
    return this.getState() === SVM.ERROR || this.getState() !== SVM.FINISHED;
};

SVM.prototype.isCallable = function() {
    return true;
};

SVM.prototype.getStackDepth = function() {
    return this.frameStack.size();
};

SVM.prototype.setStepMode = function(mode) {
    this.stepMode = mode;
};

SVM.prototype.getStepMode = function() {
    return this.stepMode;
};

SVM.prototype.executeInstruction = function() {
    if (this.code === null) {
        this.signalError("Null program");
    } else if (this.pc === SVM.ERROR_PC) {
        this.signalError("Illegal return without a function call");
    } else if (this.pc >= 0 && this.pc < this.code.length) {
        this.lastInstruction = this.code[this.pc];
        var op = (this.lastInstruction >> 24) & 0xFF;
        var addr = this.lastInstruction & 0xFFFFFF;
        var ins = SVMInstruction.get(op);
        if (this.traceFlag) {
            console.log("(" + this.pc + ") " + ins.unparse(this, addr));
        }
        this.pc++;
        ins.execute(this, addr);
    } else {
        this.setState(SVM.FINISHED);
    }
};

SVM.prototype.getLastInstruction = function() {
    return this.lastInstruction;
};

SVM.prototype.setState = function(state) {
    this.setControllerState(state);
    if (state === SVM.WAITING || state === SVM.FINISHED) {
        this.processEvents();
    }
};

SVM.prototype.getState = function() {
    return this.getControllerState();
};

SVM.prototype.setTraceFlag = function(flag) {
    this.traceFlag = flag;
};

SVM.prototype.getTraceFlag = function() {
    return this.traceFlag;
};

SVM.prototype.setTraceErrors = function(flag) {
    this.traceErrors = flag;
};

SVM.prototype.getTraceErrors = function() {
    return this.traceFlag;
};

SVM.prototype.setConsole = function(console) {
    this.console = console;
};

SVM.prototype.getConsole = function() {
    return this.console;
};

SVM.prototype.setSource = function(source) {
    this.source = source;
};

SVM.prototype.getSource = function() {
    return this.source;
};

SVM.prototype.setStatementOffset = function(offset) {
    this.statementOffset = offset;
};

SVM.prototype.getStatementOffset = function() {
    return this.statementOffset;
};

SVM.prototype.getSourceMarker = function(index) {
    if (this.source === null || index < 0) return null;
    var start = this.source.lastIndexOf("\n", index) + 1;
    var finish = this.source.indexOf("\n", start);
    if (finish === -1) finish = this.source.length;
    return new SVMSourceMarker(this.source.substring(start, finish), start);
};

SVM.prototype.setCode = function(code) {
    this.code = code;
    if (this.cf !== null) this.cf.setCode(code);
};

SVM.prototype.getCode = function() {
    return this.code;
};

SVM.prototype.get = function(addr) {
    return this.code[addr];
};

SVM.prototype.getString = function(addr) {
    return UTF8.decode(this.code, addr);
};

SVM.prototype.setPC = function(addr) {
    this.pc = addr;
};

SVM.prototype.getPC = function() {
    return this.pc;
};

SVM.prototype.push = function(v) {
    this.valueStack.push(v);
};

SVM.prototype.pushInteger = function(n) {
    this.valueStack.push(Value.createInteger(n));
};

SVM.prototype.pushDouble = function(d) {
    this.valueStack.push(Value.createDouble(d));
};

SVM.prototype.pushBoolean = function(b) {
    this.valueStack.push(Value.createBoolean(b));
};

SVM.prototype.pushString = function(str) {
    this.valueStack.push(Value.createString(str));
};

SVM.prototype.pop = function() {
    return this.valueStack.pop();
};

SVM.prototype.popInteger = function() {
    return this.valueStack.pop().getIntegerValue();
};

SVM.prototype.popDouble = function() {
    return this.valueStack.pop().getDoubleValue();
};

SVM.prototype.popBoolean = function() {
    return this.valueStack.pop().getBooleanValue();
};

SVM.prototype.popString = function() {
    return this.valueStack.pop().getStringValue();
};

SVM.prototype.exch = function() {
    var x = this.pop();
    var y = this.pop();
    this.push(x);
    this.push(y);
};

SVM.prototype.roll = function(n) {
    var values = jslib.newArray(n);
    for (var i = 0; i < n; i++) {
        values[i] = this.pop();
    }
    this.push(values[0]);
    for (var i = n - 1; i > 0; i--) {
        this.push(values[i]);
    }
};

SVM.prototype.copy = function(n) {
    for (var i = 0; i < n; i++) {
        this.push(this.peekBack(n - 1));
    }
};

SVM.prototype.getValueStackDepth = function() {
    return this.valueStack.size();
};

SVM.prototype.peekBack = function(k) {
    return this.valueStack.get(this.valueStack.size() - k - 1);
};

SVM.prototype.getCurrentFrame = function() {
    return this.cf;
};

SVM.prototype.pushFrame = function() {
    this.frameStack.push(this.cf);
    this.cf = new SVMStackFrame();
    this.cf.setCode(this.code);
};

SVM.prototype.popFrame = function() {
    this.cf = (this.frameStack.isEmpty()) ? null : this.frameStack.pop();
    if (this.cf !== null) this.code = this.cf.getCode();
};

SVM.prototype.getArgumentCount = function() {
    return this.getCurrentFrame().getArgumentCount();
};

SVM.prototype.getNARGSCount = function() {
    if (this.pc < 0 || this.pc >= this.code.length) return -1;
    var ins = this.get(this.pc);
    if (((ins >> 24) & 0xFF) !== SVMOp.NARGS) return -1;
    return ins & 0xFFFFFF;
};

SVM.prototype.checkSignature = function(name, sig) {
    var nArgs = this.getArgumentCount();
    if (nArgs === -1) return;
    var len = sig.length;
    if (len !== nArgs) {
        throw new RuntimeException("Wrong number of arguments to " + name);
    }
    for (var i = 0; i < len; i++) {
        if (!this.checkArgType(this.peekBack(len - i - 1), sig.charCodeAt(i))) {
            throw new RuntimeException("Type mismatch in call to " + name);
        }
    }
};

SVM.prototype.checkArgumentCount = function(nArgs, nParams) {
    for (var i = nParams; i < nArgs; i++) {
        this.pop();
    }
    for (var i = nArgs; i < nParams; i++) {
        this.push(Value.UNDEFINED);
    }
};

SVM.prototype.setGlobal = function(name, value) {
    this.globals.put(name, value);
};

SVM.prototype.getGlobal = function(name) {
    var v = this.globals.get(name);
    return (v === null) ? Value.UNDEFINED : v;
};

SVM.prototype.getGlobalInteger = function(name, defValue) {
    if (this.isGlobal(name)) {
        return this.globals.get(name).getIntegerValue();
    }
    return defValue;
};

SVM.prototype.getGlobalDouble = function(name, defValue) {
    if (this.isGlobal(name)) {
        return this.globals.get(name).getDoubleValue();
    }
    return defValue;
};

SVM.prototype.getGlobalBoolean = function(name, defValue) {
    if (this.isGlobal(name)) {
        return this.globals.get(name).getBooleanValue();
    }
    return defValue;
};

SVM.prototype.getGlobalString = function(name, defValue) {
    if (this.isGlobal(name)) {
        return this.globals.get(name).getStringValue();
    }
    return defValue;
};

SVM.prototype.isGlobal = function(name) {
    return this.globals.containsKey(name);
};

SVM.prototype.call = function(n) {
    var fn = this.peekBack(0);
    var type = fn.getClassName();
    if (!jslib.equals(type, "SVMMethod")) {
        this.executeCallHooks();
    }
    this.pop();
    if (jslib.equals(type, "FunctionClosure")) {
        var fc = fn.getValue();
        var receiver = this.getCurrentFrame().getReceiver();
        this.pushFrame();
        this.cf = this.getCurrentFrame();
        this.cf.setThis(receiver);
        this.cf.setReturnAddress(this.getPC());
        this.cf.setFrameLink(fc.getFrame());
        this.cf.setArgumentCount(n);
        this.code = fc.getCode();
        if (this.code !== null) this.setCode(this.code);
        this.setPC(fc.getAddress());
    } else if (jslib.equals(type, "MethodClosure")) {
        var mc = fn.getValue();
        var receiver = mc.getReceiver();
        var c = SVMClass.forName(mc.getClassName());
        var m = c.getMethod(mc.getMethodName());
        this.getCurrentFrame().setArgumentCount(n);
        m.execute(this, receiver);
    } else if (jslib.equals(type, "SVMMethod")) {
        var m = fn.getValue();
        this.getCurrentFrame().setArgumentCount(n);
        m.execute(this, null);
    } else if (jslib.equals(type, "Class")) {
        var c = SVMClass.forName(fn.getValue());
        var m = c.getMethod("new");
        this.getCurrentFrame().setArgumentCount(n);
        m.execute(this, null);
    } else {
        throw new RuntimeException("Illegal function call");
    }
};

SVM.prototype.postEvent = function(closure) {
    this.eventQueue.add(closure);
    this.processEvents();
};

SVM.prototype.setProperty = function(name, value) {
    this.properties.put(name, value);
};

SVM.prototype.getProperty = function(name) {
    return this.properties.get(name);
};

SVM.prototype.defineClass = function(name, c) {
    SVMClass.defineClass(this, name, c);
};

SVM.prototype.checkArgType = function(v, type) {
    switch (type) {
        case toInt('B'): return v.getType() === Value.BOOLEAN;
        case toInt('D'): return v.isNumeric();
        case toInt('I'): return v.isIntegral();
        case toInt('O'): return v.getType() === Value.OBJECT;
        case toInt('S'): return v.getType() === Value.STRING;
        case toInt('*'): return true;
      default:
        throw new RuntimeException("Illegal type code: " + toStr(type));
    }
};

SVM.prototype.processEvents = function() {
    var state = this.getState();
    if (state === SVM.WAITING || state === SVM.FINISHED) {
        while (!this.eventQueue.isEmpty()) {
            var closure = this.eventQueue.poll();
            closure.pushEventData(this);
            this.call(closure.getArgumentCount());
            this.run();
            // Look at error codes
        }
    }
};

SVM.prototype.setStackBase = function(offset) {
    this.cf.setStackBase(this.valueStack.size() - offset);
};

SVM.prototype.restoreStackBase = function() {
    if (this.cf === null) return;
    var base = this.cf.getStackBase();
    while (this.valueStack.size() > base) {
        this.valueStack.pop();
    }
};

SVM.prototype.pushExceptionFrame = function(addr) {
    this.exceptionStack.push(new ExceptionFrame(addr, this.frameStack.size()));
};

SVM.prototype.popExceptionFrame = function() {
    this.exceptionStack.pop();
};

SVM.prototype.throwException = function(ex, v) {
    if (this.exceptionStack.isEmpty()) {
        var ex;
        //         JSProgram.alert(this.getStatementOffset() + ": " + ex);
    } else {
        var ef = this.exceptionStack.pop();
        var depth = ef.getStackDepth();
        while (this.frameStack.size() > depth) {
            this.popFrame();
        }
        this.restoreStackBase();
        this.valueStack.push(v);
        this.setPC(ef.getDispatchAddress());
    }
};

SVM.prototype.pstack = function() {
    if (this.valueStack.isEmpty()) {
        this.println("");
    } else {
        var v = this.valueStack.pop();
        this.print(this.stringify(v) + " ");
        this.pstack();
        this.valueStack.push(v);
    }
};

SVM.prototype.list = function() {
    this.listCode(this.code);
};

SVM.prototype.listCode = function(code) {
    var n = code.length;
    for (var i = 0; i < n; i++) {
        this.print("(" + i + ") ");
        var ir = code[i];
        var op = (ir >> 24) & 0xFF;
        var addr = ir & 0xFFFFFF;
        var ins = SVMInstruction.get(op);
        if (ins === null) {
            this.println("" + ir);
        } else {
            this.println(ins.unparse(this, addr));
        }
        if (ir === SVMOp.END) break;
    }
};

SVM.prototype.print = function(s) {
    if (this.console === null) {
        System.out.print(s);
    } else {
        this.console.print(s);
    }
};

SVM.prototype.println = function(s) {
    if (this.console === null) {
        console.log(s);
    } else {
        this.console.println(s);
    }
};

SVM.prototype.stringify = function(obj) {
    return obj.toString();
};

SVM.prototype.addCallHook = function(hook) {
    this.callHooks.add(hook);
};

SVM.prototype.addReturnHook = function(hook) {
    this.returnHooks.add(hook);
};

SVM.prototype.addStepHook = function(hook) {
    this.stepHooks.add(hook);
};

SVM.prototype.addStopHook = function(hook) {
    this.stopHooks.add(hook);
};

SVM.prototype.executeCallHooks = function() {
    var el0 = new JSElementList(this.callHooks);
    for (var ei0 = 0; ei0 < el0.size(); ei0++) {
        var callHook = el0.get(ei0);
        callHook.execute();
    }
};

SVM.prototype.executeReturnHooks = function() {
    var el0 = new JSElementList(this.returnHooks);
    for (var ei0 = 0; ei0 < el0.size(); ei0++) {
        var returnHook = el0.get(ei0);
        returnHook.execute();
    }
};

SVM.prototype.executeStepHooks = function() {
    var el0 = new JSElementList(this.stepHooks);
    for (var ei0 = 0; ei0 < el0.size(); ei0++) {
        var stepHook = el0.get(ei0);
        stepHook.execute();
    }
};

SVM.prototype.executeStopHooks = function() {
    var el0 = new JSElementList(this.stopHooks);
    for (var ei0 = 0; ei0 < el0.size(); ei0++) {
        var stopHook = el0.get(ei0);
        stopHook.execute();
    }
};

SVM.ERROR_PC = -999;
SVM.BY_INSTRUCTION = 0;
SVM.BY_STATEMENT = 1;
SVM.INITIAL = Controller.INITIAL;
SVM.RUNNING = Controller.RUNNING;
SVM.STEPPING = Controller.STEPPING;
SVM.CALLING = Controller.CALLING;
SVM.STOPPED = Controller.STOPPED;
SVM.FINISHED = Controller.FINISHED;
SVM.WAITING = Controller.WAITING;
SVM.ERROR = Controller.ERROR;
var ExceptionFrame = function(addr, depth) {
    if (!(this instanceof ExceptionFrame)) return new ExceptionFrame(addr, depth);
    this.addr = addr;
    this.depth = depth;
};

ExceptionFrame.prototype.getDispatchAddress = function() {
    return this.addr;
};

ExceptionFrame.prototype.getStackDepth = function() {
    return this.depth;
};

var SVMErrorHandler = function(svm) {
    if (!(this instanceof SVMErrorHandler)) return new SVMErrorHandler(svm);
    this.svm = svm;
};

SVMErrorHandler.prototype.error = function(msg) {
    this.svm.getConsole().showErrorMessage(msg);
};


/* SVMC.js */

var SVMC = function() {
    /* Empty */
};

SVMC.DEFAULT_WINDOW_WIDTH = 500;
SVMC.DEFAULT_WINDOW_HEIGHT = 300;
SVMC.DEFAULT_CONSOLE_WIDTH = 500;
SVMC.DEFAULT_CONSOLE_HEIGHT = 200;

/* SVMConsoleClass.js */

var SVMConsoleClass = function() {
    if (!(this instanceof SVMConsoleClass)) return new SVMConsoleClass();
    SVMClass.call(this);
    this.defineMethod("print", new Console_print());
    this.defineMethod("println", new Console_println());
    this.defineMethod("readline", new Console_readline());
    this.defineMethod("readint", new Console_readint());
};

SVMConsoleClass.prototype = 
    jslib.inheritPrototype(SVMClass, "SVMConsoleClass extends SVMClass");
SVMConsoleClass.prototype.constructor = SVMConsoleClass;
SVMConsoleClass.prototype.$class = 
    new Class("SVMConsoleClass", SVMConsoleClass);

var Console_readline = function() {
    SVMMethod.call(this);
};

Console_readline.prototype =
    jslib.inheritPrototype(SVMMethod, "Console_readline extends SVMMethod");
Console_readline.prototype.constructor = Console_readline;
Console_readline.prototype.$class = 
    new Class("Console_readline", Console_readline);

Console_readline.prototype.execute = function(svm, receiver) {
    var prompt = "";
    if (svm.getArgumentCount() === 0) {
        svm.checkSignature("Console.readline", "");
    } else {
        svm.checkSignature("Console.readline", "S");
        prompt = svm.popString();
    }
    var console = svm.getConsole();
    svm.setGlobal("CONSOLE_WAIT", Value.createString("getLine"));
    svm.setState(SVM.WAITING);
    console.requestInput(prompt);
};

var Console_readint = function() {
    SVMMethod.call(this);
};

Console_readint.prototype =
    jslib.inheritPrototype(SVMMethod, "Console_readint extends SVMMethod");
Console_readint.prototype.constructor = Console_readint;
Console_readint.prototype.$class = 
    new Class("Console_readint", Console_readint);

Console_readint.prototype.execute = function(svm, receiver) {
    var prompt = "";
    if (svm.getArgumentCount() === 0) {
        svm.checkSignature("Console.readint", "");
    } else {
        svm.checkSignature("Console.readint", "S");
        prompt = svm.popString();
    }
    var console = svm.getConsole();
    svm.setGlobal("CONSOLE_WAIT", Value.createString("getInteger"));
    svm.setState(SVM.WAITING);
    console.requestInput(prompt);
};

var Console_print = function() {
    SVMMethod.call(this);
};

Console_print.prototype =
    jslib.inheritPrototype(SVMMethod, "Console_print extends SVMMethod");
Console_print.prototype.constructor = Console_print;
Console_print.prototype.$class = 
    new Class("Console_print", Console_print);

Console_print.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Console.print", "*");
    svm.getConsole().print(svm.pop());
};

var Console_println = function() {
    SVMMethod.call(this);
};

Console_println.prototype =
    jslib.inheritPrototype(SVMMethod, "Console_println extends SVMMethod");
Console_println.prototype.constructor = Console_println;
Console_println.prototype.$class = 
    new Class("Console_println", Console_println);

Console_println.prototype.execute = function(svm, receiver) {
    if (svm.getArgumentCount() === 0) {
        svm.checkSignature("Console.println", "");
        svm.getConsole().println();
    } else {
        svm.checkSignature("Console.println", "*");
        svm.getConsole().println(svm.pop());
    }
};


/* SVMStackFrame.js */

var SVMStackFrame = function() {
    if (!(this instanceof SVMStackFrame)) return new SVMStackFrame();
    this.symbolTable = null;
    this.frameSize = 0;
    this.argCount = 0;
    this.returnAddress = -1;
    this.link = null;
    this.code = null;
    this.thisRef = Value.NULL;
    this.receiver = Value.NULL;
};

SVMStackFrame.prototype.setFrameSize = function(n) {
    this.frameSize = n;
    this.locals = jslib.newArray(n);
};

SVMStackFrame.prototype.getFrameSize = function() {
    return this.frameSize;
};

SVMStackFrame.prototype.setArgumentCount = function(n) {
    this.argCount = n;
};

SVMStackFrame.prototype.getArgumentCount = function() {
    return this.argCount;
};

SVMStackFrame.prototype.setStackBase = function(base) {
    this.stackBase = base;
};

SVMStackFrame.prototype.getStackBase = function() {
    return this.stackBase;
};

SVMStackFrame.prototype.setCode = function(code) {
    this.code = code;
};

SVMStackFrame.prototype.getCode = function() {
    return this.code;
};

SVMStackFrame.prototype.setReturnAddress = function(addr) {
    this.returnAddress = addr;
};

SVMStackFrame.prototype.getReturnAddress = function() {
    return this.returnAddress;
};

SVMStackFrame.prototype.setSourceFile = function(filename) {
    this.sourceFile = filename;
};

SVMStackFrame.prototype.getSourceFile = function() {
    return this.sourceFile;
};

SVMStackFrame.prototype.setLocal = function(k, value) {
    this.locals[k] = value;
};

SVMStackFrame.prototype.getLocal = function(k) {
    return this.locals[k];
};

SVMStackFrame.prototype.declareVar = function(name) {
    if (this.symbolTable === null) this.symbolTable = new HashMap();
    this.symbolTable.put(name, Value.UNDEFINED);
};

SVMStackFrame.prototype.isDeclared = function(name) {
    if (this.symbolTable === null) return false;
    return this.symbolTable.containsKey(name);
};

SVMStackFrame.prototype.setVar = function(name, value) {
    if (this.symbolTable === null) this.symbolTable = new HashMap();
    this.symbolTable.put(name, value);
};

SVMStackFrame.prototype.getVar = function(name) {
    if (this.symbolTable === null) return Value.UNDEFINED;
    if (!this.symbolTable.containsKey(name)) return Value.UNDEFINED;
    return this.symbolTable.get(name);
};

SVMStackFrame.prototype.setFrameLink = function(frame) {
    this.link = frame;
};

SVMStackFrame.prototype.getFrameLink = function() {
    return this.link;
};

SVMStackFrame.prototype.setReceiver = function(value) {
    this.receiver = value;
};

SVMStackFrame.prototype.getReceiver = function() {
    return this.receiver;
};

SVMStackFrame.prototype.setThis = function(value) {
    this.thisRef = value;
};

SVMStackFrame.prototype.getThis = function() {
    return this.thisRef;
};


/* SVMProgram.js */

var SVMProgram = function() {
    if (!(this instanceof SVMProgram)) return new SVMProgram();
    JSProgram.call(this);
    this.svm = this.createSVM();
    this.svm.setProgram(this);
    var className = this.$class.getName();
    this.setTitle(className.substring(className.lastIndexOf(".") + 1));
    this.setCode(SVMProgram.EMPTY_PROGRAM);
};

SVMProgram.prototype = 
    jslib.inheritPrototype(JSProgram, "SVMProgram extends JSProgram");
SVMProgram.prototype.constructor = SVMProgram;
SVMProgram.prototype.$class = 
    new Class("SVMProgram", SVMProgram);

SVMProgram.prototype.init = function() {
    /* Empty */
};

SVMProgram.prototype.getSVM = function() {
    return this.svm;
};

SVMProgram.prototype.isCompiler = function() {
    return false;
};

SVMProgram.prototype.setCode = function(code) {
    this.svm.setCode(code);
};

SVMProgram.prototype.getCode = function() {
    return this.svm.getCode();
};

SVMProgram.prototype.setConsole = function(console) {
    this.svm.setConsole(console);
};

SVMProgram.prototype.getConsole = function() {
    return this.svm.getConsole();
};

SVMProgram.prototype.run = function() {
    this.init();
    this.pack();
    this.setVisible(true);
    this.svm.run();
};

SVMProgram.prototype.createSVM = function() {
    return new SVM();
};

SVMProgram.EMPTY_PROGRAM = [
    SVMOp.END << 24
];


/* SVMConsoleProgram.js */

var SVMConsoleProgram = function() {
    if (!(this instanceof SVMConsoleProgram)) return new SVMConsoleProgram();
    SVMProgram.call(this);
    this.this("console");
};

SVMConsoleProgram.prototype = 
    jslib.inheritPrototype(SVMProgram, "SVMConsoleProgram extends SVMProgram");
SVMConsoleProgram.prototype.constructor = SVMConsoleProgram;
SVMConsoleProgram.prototype.$class = 
    new Class("SVMConsoleProgram", SVMConsoleProgram);

var SVMConsoleProgram = function(id) {
    if (!(this instanceof SVMConsoleProgram)) return new SVMConsoleProgram(id);
    SVMProgram.call(this);
    var svm = this.getSVM();
    this.console = this.createConsole(svm);
    var listener = this.createConsoleListener(svm);
    if (listener !== null) {
        this.console.addActionListener(listener);
    }
    this.add(this.console, "console");
    this.setConsole(this.console);
};

SVMConsoleProgram.prototype = 
    jslib.inheritPrototype(SVMProgram, "SVMConsoleProgram extends SVMProgram");
SVMConsoleProgram.prototype.constructor = SVMConsoleProgram;
SVMConsoleProgram.prototype.$class = 
    new Class("SVMConsoleProgram", SVMConsoleProgram);

SVMConsoleProgram.prototype.setPreferredSize = function(width, height) {
    this.console = this.getConsole();
    if (this.console !== null) {
        this.console.setPreferredSize(new Dimension(width, height));
    }
};

SVMConsoleProgram.prototype.createConsole = function(svm) {
    this.console = new JSConsole();
    this.console.setFont(CSSFont.decode("Courier New-Bold-14"));
    return this.console;
};

SVMConsoleProgram.prototype.createConsoleListener = function(svm) {
    return new SVMConsoleListener(svm);
};


/* SVMSourceMarker.js */

var SVMSourceMarker = function(line, index) {
    if (!(this instanceof SVMSourceMarker)) return new SVMSourceMarker(line, index);
    this.line = line;
    this.index = index;
};

SVMSourceMarker.prototype.getSourceLine = function() {
    return this.line;
};

SVMSourceMarker.prototype.getStartingIndex = function() {
    return this.index;
};


/* SVMAssembler.js */

var SVMAssembler = function() {
    if (!(this instanceof SVMAssembler)) return new SVMAssembler();
    this.imports = new HashSet();
};

SVMAssembler.prototype.execute = function(source) {
    var scanner = this.createTokenScanner();
    scanner.setInput(source);
    var cv = new CodeVector();
    this.assemble(cv, scanner);
    var svm = new SVM();
    svm.setCode(cv.getCode());
    svm.run();
};

SVMAssembler.prototype.assemble = function(cv, scanner) {
    cv.setModuleName("MAIN");
    while (scanner.hasMoreTokens()) {
        var token = scanner.nextToken();
        while (scanner.hasMoreTokens() && jslib.equals(token, "\n")) {
            token = scanner.nextToken();
        }
        if (!scanner.hasMoreTokens()) break;
        var str = scanner.nextToken();
        if (jslib.equals(str, ":")) {
            cv.defineLabel(token);
        } else {
            scanner.saveToken(str);
            token = token.toUpperCase();
            if (jslib.equals(token, "MODULE")) {
                cv.setModuleName(scanner.nextToken());
            } else if (jslib.equals(token, "IMPORT")) {
                var name = scanner.nextToken();
                token = scanner.nextToken();
                while (jslib.equals(token, ".")) {
                    name += "." + scanner.nextToken();
                    token = scanner.nextToken();
                }
                scanner.saveToken(token);
                this.imports.add(name);
            } else {
                var ins = SVMInstruction.lookup(token);
                if (ins === null) {
                    console.log("Undefined instruction " + token);
                } else {
                    ins.assemble(cv, scanner);
                }
            }
            token = scanner.nextToken();
            if (!jslib.equals(token, "\n")) {
                console.log("Extra token " + token + " on line");
            }
        }
    }
};

SVMAssembler.prototype.getImports = function() {
    return this.imports;
};

SVMAssembler.prototype.createTokenScanner = function() {
    var scanner = new TokenScanner();
    scanner.addWordCharacters("_$#");
    scanner.ignoreWhitespace();
    scanner.ignoreComments();
    scanner.scanStrings();
    scanner.scanNumbers();
    scanner.addOperator("\n");
    return scanner;
};


/* SVMConstant.js */

var SVMConstant = function() {
    SVMMethod.call(this);
};

SVMConstant.prototype =
    jslib.inheritPrototype(SVMMethod, "SVMConstant extends SVMMethod");
SVMConstant.prototype.constructor = SVMConstant;
SVMConstant.prototype.$class = 
    new Class("SVMConstant", SVMConstant);

SVMConstant.prototype.isConstant = function() {
    return true;
};


/* SVMCoreClass.js */

var SVMCoreClass = function() {
    if (!(this instanceof SVMCoreClass)) return new SVMCoreClass();
    SVMClass.call(this);
    this.defineMethod("FALSE", new Core_FALSE());
    this.defineMethod("TRUE", new Core_TRUE());
    this.defineMethod("NULL", new Core_NULL());
    this.defineMethod("UNDEFINED", new Core_UNDEFINED());
    this.defineMethod("INFINITY", new Core_INFINITY());
    this.defineMethod("NaN", new Core_NaN());
    this.defineMethod("this", new Core_this());
    this.defineMethod("setReceiver", new Core_setReceiver());
    this.defineMethod("display", new Core_display());
    this.defineMethod("select", new Core_select());
    this.defineMethod("assign", new Core_assign());
    this.defineMethod("list", new Core_list());
    this.defineMethod("array", new Core_array());
    this.defineMethod("object", new Core_object());
    this.defineMethod("length", new Core_length());
};

SVMCoreClass.prototype = 
    jslib.inheritPrototype(SVMClass, "SVMCoreClass extends SVMClass");
SVMCoreClass.prototype.constructor = SVMCoreClass;
SVMCoreClass.prototype.$class = 
    new Class("SVMCoreClass", SVMCoreClass);

var Core_FALSE = function() {
    SVMConstant.call(this);
};

Core_FALSE.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_FALSE extends SVMConstant");
Core_FALSE.prototype.constructor = Core_FALSE;
Core_FALSE.prototype.$class = 
    new Class("Core_FALSE", Core_FALSE);

Core_FALSE.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.FALSE", "");
    svm.pushBoolean(false);
};

var Core_TRUE = function() {
    SVMConstant.call(this);
};

Core_TRUE.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_TRUE extends SVMConstant");
Core_TRUE.prototype.constructor = Core_TRUE;
Core_TRUE.prototype.$class = 
    new Class("Core_TRUE", Core_TRUE);

Core_TRUE.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.TRUE", "");
    svm.pushBoolean(true);
};

var Core_NULL = function() {
    SVMConstant.call(this);
};

Core_NULL.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_NULL extends SVMConstant");
Core_NULL.prototype.constructor = Core_NULL;
Core_NULL.prototype.$class = 
    new Class("Core_NULL", Core_NULL);

Core_NULL.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.NULL", "");
    svm.push(Value.NULL);
};

var Core_UNDEFINED = function() {
    SVMConstant.call(this);
};

Core_UNDEFINED.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_UNDEFINED extends SVMConstant");
Core_UNDEFINED.prototype.constructor = Core_UNDEFINED;
Core_UNDEFINED.prototype.$class = 
    new Class("Core_UNDEFINED", Core_UNDEFINED);

Core_UNDEFINED.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.UNDEFINED", "");
    svm.push(Value.UNDEFINED);
};

var Core_NaN = function() {
    SVMConstant.call(this);
};

Core_NaN.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_NaN extends SVMConstant");
Core_NaN.prototype.constructor = Core_NaN;
Core_NaN.prototype.$class = 
    new Class("Core_NaN", Core_NaN);

Core_NaN.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.NaN", "");
    svm.push(Value.createDouble(Double.NaN));
};

var Core_INFINITY = function() {
    SVMConstant.call(this);
};

Core_INFINITY.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_INFINITY extends SVMConstant");
Core_INFINITY.prototype.constructor = Core_INFINITY;
Core_INFINITY.prototype.$class = 
    new Class("Core_INFINITY", Core_INFINITY);

Core_INFINITY.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.INFINITY", "");
    svm.push(Value.createDouble(Double.POSITIVE_INFINITY));
};

var Core_this = function() {
    SVMConstant.call(this);
};

Core_this.prototype =
    jslib.inheritPrototype(SVMConstant, "Core_this extends SVMConstant");
Core_this.prototype.constructor = Core_this;
Core_this.prototype.$class = 
    new Class("Core_this", Core_this);

Core_this.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.this", "");
    svm.push(svm.getCurrentFrame().getThis());
};

var Core_setReceiver = function() {
    SVMMethod.call(this);
};

Core_setReceiver.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_setReceiver extends SVMMethod");
Core_setReceiver.prototype.constructor = Core_setReceiver;
Core_setReceiver.prototype.$class = 
    new Class("Core_setReceiver", Core_setReceiver);

Core_setReceiver.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.setReceiver", "*");
    receiver = svm.pop();
    svm.getCurrentFrame().setReceiver(receiver);
};

var Core_display = function() {
    SVMMethod.call(this);
};

Core_display.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_display extends SVMMethod");
Core_display.prototype.constructor = Core_display;
Core_display.prototype.$class = 
    new Class("Core_display", Core_display);

Core_display.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.display", "*");
    var value = svm.pop();
    if (value !== Value.UNDEFINED) {
        svm.getConsole().println(value);
    }
};

var Core_select = function() {
    SVMMethod.call(this);
};

Core_select.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_select extends SVMMethod");
Core_select.prototype.constructor = Core_select;
Core_select.prototype.$class = 
    new Class("Core_select", Core_select);

Core_select.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.select", "**");
    var ix = svm.pop();
    var obj = (receiver === null) ? svm.pop() : receiver;
    var type = obj.getType();
    if (type === Value.STRING) {
        var s = obj.getStringValue();
        if (ix.getType() === Value.STRING) {
            var name = ix.getStringValue();
            if (jslib.equals(name, "length")) {
                svm.pushInteger(s.length);
                return;
            }
            var mc =
            new SVMMethodClosure(obj, "String", name);
            svm.push(Value.createObject(mc, "MethodClosure"));
            return;
        }
        var index = ix.getIntegerValue();
        var n = s.length;
        if (index < 0) index += n;
        if (index >= 0 && index < n) {
            svm.pushString(s.substring(index, index + 1));
        } else {
            throw new RuntimeException("String index out of bounds");
        }
        return;
    } else if (type === Value.OBJECT) {
        var cname = obj.getClassName();
        if (jslib.equals(cname, "Array")) {
            var array = obj.getValue();
            if (ix.getType() === Value.STRING) {
                var name = ix.getStringValue();
                if (jslib.equals(name, "length")) {
                    svm.pushInteger(array.size());
                    return;
                }
                var mc =
                new SVMMethodClosure(obj, "Array", name);
                svm.push(Value.createObject(mc, "MethodClosure"));
            } else {
                var index = ix.getIntegerValue();
                if (index >= 0 && index < array.size()) {
                    svm.push(array.get(index));
                } else {
                    svm.push(Value.UNDEFINED);
                }
            }
        } else if (jslib.equals(cname, "Object") || jslib.equals(cname, "Map")) {
            var map = obj.getValue();
            var v = map.get(ix.getStringValue());
            svm.push((v === null) ? Value.UNDEFINED : v);
        } else {
            var name = ix.getStringValue();
            var prop = obj.getProperty(name);
            if (prop !== null) {
                svm.push(prop);
            } else {
                if (jslib.equals(cname, "Class")) {
                    cname = obj.getValue();
                    obj = null;
                }
                var c = SVMClass.forName(cname);
                if (c === null) {
                    throw new RuntimeException("Undefined class " + cname);
                }
                var m = null;
                try {
                    m = c.getMethod(name);
                 } catch (ex) {
                    svm.push(Value.UNDEFINED);
                    return;
                }
                if (m === null) {
                    throw new RuntimeException("Undefined method " + name);
                }
                if (m.isConstant()) {
                    m.execute(svm, null);
                    return;
                }
                var mc =
                new SVMMethodClosure(obj, cname, name);
                svm.push(Value.createObject(mc, "MethodClosure"));
            }
        }
    } else if (type === Value.INTEGER || type === Value.DOUBLE) {
        if (ix.getType() === Value.STRING) {
            var name = ix.getStringValue();
            var mc =
            new SVMMethodClosure(obj, "Number", name);
            svm.push(Value.createObject(mc, "MethodClosure"));
            return;
        }
    } else {
        throw new RuntimeException("Illegal selection");
    }
};

var Core_assign = function() {
    SVMMethod.call(this);
};

Core_assign.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_assign extends SVMMethod");
Core_assign.prototype.constructor = Core_assign;
Core_assign.prototype.$class = 
    new Class("Core_assign", Core_assign);

Core_assign.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.assign", "***");
    var value = svm.pop();
    var ix = svm.pop();
    var obj = (receiver === null) ? svm.pop() : receiver;
    if (obj.getType() === Value.OBJECT) {
        var cname = obj.getClassName();
        if (jslib.equals(cname, "Array")) {
            var array = obj.getValue();
            var index = ix.getIntegerValue();
            while (array.size() <= index) {
                array.add(Value.UNDEFINED);
            }
            array.set(index, value);
            return;
        } else if (jslib.equals(cname, "Object") || jslib.equals(cname, "Map")) {
            var map = obj.getValue();
            map.put(ix.getStringValue(), value);
            return;
        } else {
            var name = ix.getStringValue();
            obj.setProperty(name, value);
            return;
        }
    }
    throw new RuntimeException("Illegal selection");
};

var Core_list = function() {
    SVMMethod.call(this);
};

Core_list.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_list extends SVMMethod");
Core_list.prototype.constructor = Core_list;
Core_list.prototype.$class = 
    new Class("Core_list", Core_list);

Core_list.prototype.execute = function(svm, receiver) {
    var list = new SVMArray();
    var nArgs = svm.getArgumentCount();
    for (var i = 0; i < nArgs; i++) {
        list.add(0, svm.pop());
    }
    svm.push(Value.createObject(list, "Array"));
};

var Core_array = function() {
    SVMMethod.call(this);
};

Core_array.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_array extends SVMMethod");
Core_array.prototype.constructor = Core_array;
Core_array.prototype.$class = 
    new Class("Core_array", Core_array);

Core_array.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.array", "I");
    var array = new SVMArray();
    var init = Value.UNDEFINED;
    var n = svm.popInteger();
    for (var i = 0; i < n; i++) {
        array.add(init);
    }
    svm.push(Value.createObject(array, "Array"));
};

var Core_object = function() {
    SVMMethod.call(this);
};

Core_object.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_object extends SVMMethod");
Core_object.prototype.constructor = Core_object;
Core_object.prototype.$class = 
    new Class("Core_object", Core_object);

Core_object.prototype.execute = function(svm, receiver) {
    var nargs = svm.getArgumentCount();
    var obj = new SVMObject(svm);
    for (var i = 0; i < nargs; i += 2) {
        var value = svm.pop();
        var key = svm.popString();
        obj.put(key, value);
    }
    svm.push(Value.createObject(obj, "Object"));
};

var Core_length = function() {
    SVMMethod.call(this);
};

Core_length.prototype =
    jslib.inheritPrototype(SVMMethod, "Core_length extends SVMMethod");
Core_length.prototype.constructor = Core_length;
Core_length.prototype.$class = 
    new Class("Core_length", Core_length);

Core_length.prototype.execute = function(svm, receiver) {
    svm.checkSignature("Core.length", "*");
    var array = svm.pop().getValue();
    svm.pushInteger(array.size());
};


/* SVMFunctionClosure.js */

var SVMFunctionClosure = function(code, addr, frame) {
    if (!(this instanceof SVMFunctionClosure)) return new SVMFunctionClosure(code, addr, frame);
    this.code = code;
    this.addr = addr;
    this.frame = frame;
};

SVMFunctionClosure.prototype.getAddress = function() {
    return this.addr;
};

SVMFunctionClosure.prototype.getCode = function() {
    return this.code;
};

SVMFunctionClosure.prototype.getFrame = function() {
    return this.frame;
};

SVMFunctionClosure.prototype.toString = function() {
    return "FunctionClosure@" + this.addr;
};


/* SVMConsoleListener.js */

var SVMConsoleListener = function(svm) {
    if (!(this instanceof SVMConsoleListener)) return new SVMConsoleListener(svm);
    this.svm = svm;
};

SVMConsoleListener.prototype.actionPerformed = function(e) {
    var ok = false;
    var key = "";
    var action = this.svm.getGlobal("CONSOLE_WAIT");
    if (action !== Value.UNDEFINED) {
        this.svm.setGlobal("CONSOLE_WAIT", Value.UNDEFINED);
        if (action.getType() === Value.STRING) {
            key = action.getStringValue();
            if (jslib.startsWith(key, "getInt")) {
                try {
                    this.svm.pushInteger( Integer.parseInt(e.getActionCommand()));
                    ok = true;
                 } catch (ex) {
                    this.svm.getConsole().showErrorMessage(RuntimeException.patchMessage(ex));
                }
            } else if (jslib.startsWith(key, "getNumber")) {
                try {
                    this.svm.pushDouble( Double.parseDouble(e.getActionCommand()));
                    ok = true;
                 } catch (ex) {
                    this.svm.getConsole().showErrorMessage(RuntimeException.patchMessage(ex));
                }
            } else {
                this.svm.pushString(e.getActionCommand());
                ok = true;
            }
        } else if (jslib.equals(action.getClassName(), "FunctionClosure")) {
            this.svm.pushString(e.getActionCommand());
            this.svm.push(action);
            this.svm.call(1);
            this.svm.run();
            return;
        }
        if (ok) {
            this.svm.run();
        } else {
            var prompt = "";
            var colon = key.indexOf(":");
            if (colon > 0) prompt = key.substring(colon + 1);
            this.svm.getConsole().requestInput(prompt);
        }
    } else {
        this.processConsoleLine(e.getActionCommand());
    }
};

SVMConsoleListener.prototype.processConsoleLine = function(line) {
    /* Empty */
};


/* SVMMethodClosure.js */

var SVMMethodClosure = function(receiver, className, methodName) {
    if (!(this instanceof SVMMethodClosure)) return new SVMMethodClosure(receiver, className, methodName);
    this.receiver = receiver;
    this.className = className;
    this.methodName = methodName;
};

SVMMethodClosure.prototype.getReceiver = function() {
    return this.receiver;
};

SVMMethodClosure.prototype.getClassName = function() {
    return this.className;
};

SVMMethodClosure.prototype.getMethodName = function() {
    return this.methodName;
};

SVMMethodClosure.prototype.toString = function() {
    return this.className + "." + this.methodName;
};


/* SVMObject.js */

var SVMObject = function(svm) {
    if (!(this instanceof SVMObject)) return new SVMObject(svm);
    TreeMap.call(this);
    this.svm = svm;
};

SVMObject.prototype = 
    jslib.inheritPrototype(TreeMap, "SVMObject extends TreeMap");
SVMObject.prototype.constructor = SVMObject;
SVMObject.prototype.$class = 
    new Class("SVMObject", SVMObject);

SVMObject.prototype.toString = function() {
    if (this.containsKey("toString")) {
        this.svm.push(this.get("toString"));
        this.svm.call(0);
        return this.svm.popString();
    }
    var str = "";
    var el0 = new JSElementList(this.keySet());
    for (var ei0 = 0; ei0 < el0.size(); ei0++) {
        var key = el0.get(ei0);
        if (str.length > 0) str += ", ";
        str += key + ":" + this.get(key).toString();
    }
    return "{" + str + "}";
};


/* Exports */

return {
    SVM : SVM,
    SVMArray : SVMArray,
    SVMAssembler : SVMAssembler,
    SVMC : SVMC,
    SVMClass : SVMClass,
    SVMConsoleClass : SVMConsoleClass,
    SVMConsoleListener : SVMConsoleListener,
    SVMConsoleProgram : SVMConsoleProgram,
    SVMConstant : SVMConstant,
    SVMCoreClass : SVMCoreClass,
    SVMEventClosure : SVMEventClosure,
    SVMFunctionClosure : SVMFunctionClosure,
    SVMGlobalClass : SVMGlobalClass,
    SVMInstruction : SVMInstruction,
    SVMMethod : SVMMethod,
    SVMMethodClosure : SVMMethodClosure,
    SVMObject : SVMObject,
    SVMPackage : SVMPackage,
    SVMProgram : SVMProgram,
    SVMSourceMarker : SVMSourceMarker,
    SVMStackFrame : SVMStackFrame
};

});
