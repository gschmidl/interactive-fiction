/*
 * File: exp.js
 * Created on Thu May 27 16:19:47 PDT 2021 by java2js
 * --------------------------------------------------
 * This file was generated mechanically by the java2js utility.
 * Permanent edits must be made in the Java file.
 */

define([ "jslib",
         "java/lang",
         "java/util" ],

function(jslib,
         java_lang,
         java_util) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var Class = java_lang.Class;
var RuntimeException = java_lang.RuntimeException;
var Comparator = java_util.Comparator;
var TreeMap = java_util.TreeMap;

/* Expression.js */

var Expression = function() {
    if (!(this instanceof Expression)) return new Expression();
    this.data = null;
    this.startIndex = -1;
    this.endIndex = -1;
};

Expression.prototype.getValue = function() {
    throw new RuntimeException("getValue must be called on a Constant");
};

Expression.prototype.getName = function() {
    throw new RuntimeException("getName must be called on an Identifier");
};

Expression.prototype.getFunction = function() {
    throw new RuntimeException("getFunction must be called on a Compound");
};

Expression.prototype.getArgs = function() {
    throw new RuntimeException("getArgs must be called on a Compound");
};

Expression.evalArgs = function(ec, args) {
    var actuals = jslib.newArray(args.length);
    for (var i = 0; i < args.length; i++) {
        actuals[i] = args[i].eval(ec);
    }
    return actuals;
};

Expression.prototype.matches = function(str) {
    return false;
};

Expression.prototype.setIndices = function(start, end) {
    this.startIndex = start;
    this.endIndex = end;
};

Expression.prototype.setStartIndex = function(index) {
    this.startIndex = index;
};

Expression.prototype.getStartIndex = function() {
    return this.startIndex;
};

Expression.prototype.setEndIndex = function(index) {
    this.endIndex = index;
};

Expression.prototype.getEndIndex = function() {
    return this.endIndex;
};

Expression.prototype.getClientData = function() {
    return this.data;
};

Expression.prototype.setClientData = function(data) {
    this.data = data;
};

Expression.prototype.isOperator = function() {
    return false;
};

Expression.prototype.clone = function() {
    try {
        return jslib.clone(this);
     } catch (ex) {
        throw new RuntimeException(ex);
    }
};

Expression.CONSTANT = 1;
Expression.IDENTIFIER = 2;
Expression.COMPOUND = 3;
Expression.OPERATOR = 4;
Expression.FUNCTION = 5;

/* Identifier.js */

var Identifier = function(name) {
    if (!(this instanceof Identifier)) return new Identifier(name);
    Expression.call(this);
    this.name = name;
};

Identifier.prototype = 
    jslib.inheritPrototype(Expression, "Identifier extends Expression");
Identifier.prototype.constructor = Identifier;
Identifier.prototype.$class = 
    new Class("Identifier", Identifier);

Identifier.prototype.eval = function(ec) {
    return ec.evalIdentifier(this);
};

Identifier.prototype.getType = function() {
    return Expression.IDENTIFIER;
};

Identifier.prototype.getName = function() {
    return this.name;
};

Identifier.prototype.toString = function() {
    return this.name;
};

Identifier.prototype.matches = function(name) {
    return jslib.equals(this.name, name);
};

Identifier.prototype.get = function(ec) {
    return ec.getValue(this.name);
};

Identifier.prototype.set = function(ec, value) {
    ec.setValue(this.name, value);
};


/* ValueComparator.js */

var ValueComparator = function() {
    /* Empty */
};

ValueComparator.prototype.compare = function(v1, v2) {
    var t1 = v1.getType();
    var t2 = v2.getType();
    if ((t1 === Value.INTEGER || t1 === Value.DOUBLE) && (t2 === Value.INTEGER || t2 === Value.DOUBLE)) {
        var d1 = v1.getDoubleValue();
        var d2 = v2.getDoubleValue();
        return (d1 === d2) ? 0 : (d1 < d2) ? -1 : 1;
    } else {
        return v1.toString().localeCompare(v2.toString());
    }
};


/* Compound.js */

var Compound = function(fn, args) {
    if (!(this instanceof Compound)) return new Compound(fn, args);
    Expression.call(this);
    this.function = fn;
    this.arguments = args;
};

Compound.prototype = 
    jslib.inheritPrototype(Expression, "Compound extends Expression");
Compound.prototype.constructor = Compound;
Compound.prototype.$class = 
    new Class("Compound", Compound);

Compound.prototype.getFunction = function() {
    return this.function;
};

Compound.prototype.getArgs = function() {
    return this.arguments;
};

Compound.prototype.eval = function(ec) {
    return ec.evalCompound(this);
};

Compound.prototype.toString = function() {
    var result = this.function.toString();
    result += "(";
    for (var i = 0; i < this.arguments.length; i++) {
        if (i > 0) result += ",";
        result += this.arguments[i].toString();
    }
    return result + ")";
};

Compound.prototype.getType = function() {
    return Expression.COMPOUND;
};


/* Constant.js */

var Constant = function(v) {
    if (!(this instanceof Constant)) return new Constant(v);
    Expression.call(this);
    this.value = v;
};

Constant.prototype = 
    jslib.inheritPrototype(Expression, "Constant extends Expression");
Constant.prototype.constructor = Constant;
Constant.prototype.$class = 
    new Class("Constant", Constant);

Constant.prototype.getValue = function() {
    return this.value;
};

Constant.prototype.eval = function(ec) {
    return ec.evalConstant(this);
};

Constant.prototype.toString = function() {
    return this.value.toString();
};

Constant.prototype.getType = function() {
    return Expression.CONSTANT;
};


/* Value.js */

var Value = function(type, value) {
    if (!(this instanceof Value)) return new Value(type, value);
    this.type = type;
    this.value = value;
    this.properties = null;
    switch (type) {
        case Value.BOOLEAN: this.className = "Boolean"; break;
        case Value.CHARACTER: this.className = "Character"; break;
        case Value.DOUBLE: this.className = "Double"; break;
        case Value.INTEGER: this.className = "Integer"; break;
        case Value.LONG: this.className = "Long"; break;
        case Value.OBJECT: this.className = "Object"; break;
        case Value.STRING: this.className = "String"; break;
    }
};

Value.prototype.getType = function() {
    return this.type;
};

Value.prototype.getClassName = function() {
    return this.className;
};

Value.prototype.setClassName = function(name) {
    this.className = name;
};

Value.prototype.getValue = function() {
    return this.value;
};

Value.prototype.setProperty = function(name, value) {
    if (this.properties === null) this.properties = new TreeMap();
    this.properties.put(name, value);
};

Value.prototype.getProperty = function(name) {
    if (this.properties === null) return null;
    return this.properties.get(name);
};

Value.prototype.toString = function() {
    if (this.value === null) return "null";
    if (this.isIntegral()) return "" + this.getIntegerValue();
    return this.value.toString();
};

Value.prototype.isIntegral = function() {
    switch (this.type) {
      case Value.INTEGER:
        return true;
      case Value.DOUBLE:
        var d = this.value;
        return (toInt(d)== d);
      default:
        return false;
    }
};

Value.prototype.isNumeric = function() {
    switch (this.type) {
      case Value.INTEGER: case Value.DOUBLE:
        return true;
      default:
        return false;
    }
};

Value.prototype.getIntegerValue = function() {
    switch (this.type) {
      case Value.INTEGER:
        return this.value;
      case Value.DOUBLE:
        var d = this.value;
        if (toInt(d)== d) return toInt(d);
        throw new RuntimeException("Illegal integer");
      default:
        throw new RuntimeException("Illegal integer");
    }
};

Value.prototype.getDoubleValue = function() {
    switch (this.type) {
      case Value.INTEGER:
        return this.value;
      case Value.DOUBLE:
        return this.value;
      default:
        throw new RuntimeException("Illegal double");
    }
};

Value.prototype.getStringValue = function() {
    return this.toString();
};

Value.prototype.getBooleanValue = function() {
    if (this.type !== Value.BOOLEAN) {
        throw new RuntimeException("Illegal boolean");
    }
    return this.value;
};

Value.createInteger = function(n) {
    return new Value(Value.INTEGER, n);
};

Value.createDouble = function(d) {
    return new Value(Value.DOUBLE, d);
};

Value.createBoolean = function(b) {
    return new Value(Value.BOOLEAN, b);
};

Value.createCharacter = function(ch) {
    return new Value(Value.CHARACTER, ch);
};

Value.createString = function(s) {
    return new Value(Value.STRING, s);
};

Value.createObject = function(obj, className) {
    this.value = new Value(Value.OBJECT, obj);
    this.value.setClassName(className);
    return this.value;
};

Value.ASSIGNABLE = 'A';
Value.BOOLEAN = 'B';
Value.CHARACTER = 'C';
Value.DOUBLE = 'D';
Value.FUNCTION = 'F';
Value.INTEGER = 'I';
Value.LONG = 'L';
Value.OBJECT = 'O';
Value.REF = 'R';
Value.STRING = 'S';
Value.VOID = 'V';
Value.TRUE = new Value(Value.BOOLEAN, true);
Value.FALSE = new Value(Value.BOOLEAN, false);
Value.NULL = new Value(Value.REF, "null");
Value.UNDEFINED = new Value(Value.VOID, "undefined");

/* SimpleEvalContext.js */

var SimpleEvalContext = function() {
    if (!(this instanceof SimpleEvalContext)) return new SimpleEvalContext();
    this.variables = new TreeMap();
};

SimpleEvalContext.prototype.getValue = function(name) {
    return this.variables.get(name);
};

SimpleEvalContext.prototype.setValue = function(name, value) {
    this.variables.put(name, value);
};

SimpleEvalContext.prototype.isDefined = function(name) {
    return this.variables.containsKey(name);
};

SimpleEvalContext.prototype.createLValue = function(exp) {
    if (exp.getType() !== Expression.IDENTIFIER) {
        throw new RuntimeException("Illegal assignment");
    }
    return exp;
};

SimpleEvalContext.prototype.evalConstant = function(exp) {
    return exp.getValue();
};

SimpleEvalContext.prototype.evalIdentifier = function(exp) {
    return this.getValue(exp.getName());
};

SimpleEvalContext.prototype.evalCompound = function(exp) {
    var fn = exp.getFunction();
    if (!fn.isOperator()) {
        throw new RuntimeException("Functions are not implemented");
    }
    return (fn).apply(this, exp.getArgs());
};

SimpleEvalContext.prototype.isTrue = function(v) {
    if (v.getType() !== Value.BOOLEAN) {
        throw new RuntimeException("Illegal boolean value");
    }
    return v.getValue();
};

SimpleEvalContext.prototype.getInfixType = function(v1, v2) {
    switch (v1.getType()) {
      case Value.INTEGER: case Value.CHARACTER:
        switch (v2.getType()) {
          case Value.INTEGER: case Value.CHARACTER:
            return Value.INTEGER;
          case Value.DOUBLE:
            return Value.DOUBLE;
        }
        break;
      case Value.DOUBLE:
        switch (v2.getType()) {
          case Value.INTEGER: case Value.CHARACTER: case Value.DOUBLE:
            return Value.DOUBLE;
        }
        break;
    }
    return -1;
};


/* Exports */

return {
    Compound : Compound,
    Constant : Constant,
    Expression : Expression,
    Identifier : Identifier,
    SimpleEvalContext : SimpleEvalContext,
    Value : Value,
    ValueComparator : ValueComparator
};

});
