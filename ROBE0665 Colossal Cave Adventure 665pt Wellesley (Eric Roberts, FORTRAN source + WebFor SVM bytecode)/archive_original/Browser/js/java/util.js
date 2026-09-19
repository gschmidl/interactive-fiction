/*
 * File: util.js
 * -------------
 * This package simulates parts of the java.util package.
 */

/* Header for requirejs */

define([ "jslib",
         "java/lang" ],

function(jslib,
  java_lang) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var Class = java_lang.Class;
var RuntimeException = java_lang.RuntimeException;

/*
 * Class: ArrayDeque
 * -----------------
 * Simulates the <code>ArrayDeque</code> class.
 */

var ArrayDeque = function() {
    if (!(this instanceof ArrayDeque)) return new ArrayDeque();
    this.elements = [];
};

ArrayDeque.prototype.$class = new Class("ArrayDeque", ArrayDeque);

ArrayDeque.prototype.add = function(e) {
    this.elements.push(e);
};

ArrayDeque.prototype.addFirst = function(e) {
    this.elements.unshift(e);
};

ArrayDeque.prototype.addLast = function(e) {
    this.elements.push(e);
};

ArrayDeque.prototype.clear = function() {
    this.elements.splice(0, this.elements.length);
};

ArrayDeque.prototype.contains = function(elem) {
    return this.elements.indexOf(elem) === -1;
};

ArrayDeque.prototype.getFirst = function() {
    return this.elements[0];
};

ArrayDeque.prototype.getLast = function() {
    return this.elements[this.elements.length - 1];
};

ArrayDeque.prototype.isEmpty = function() {
    return this.elements.length === 0;
};

ArrayDeque.prototype.peekFirst = function() {
    if (this.elements.length === 0) return null;
    return this.elements[0];
};

ArrayDeque.prototype.peekLast = function() {
    if (this.elements.length === 0) return null;
    return this.elements[this.elements.length - 1];
};

ArrayDeque.prototype.poll = function() {
    if (this.elements.length === 0) return null;
    return this.elements.shift();
};

ArrayDeque.prototype.pollFirst = function() {
    if (this.elements.length === 0) return null;
    return this.elements.shift();
};

ArrayDeque.prototype.pollLast = function() {
    if (this.elements.length === 0) return null;
    return this.elements.pop();
};

ArrayDeque.prototype.pop = function() {
    return this.elements.pop();
};

ArrayDeque.prototype.push = function(e) {
    this.elements.push(e);
};

ArrayDeque.prototype.remove = function(index) {
    if (index === undefined) {
        return this.elements.shift();
    } else {
        this.elements.splice(index, 1);
    }
};

ArrayDeque.prototype.removeAt = function(index) {
    if (index === undefined) {
        return this.elements.shift();
    } else {
        this.elements.splice(index, 1);
    }
};

ArrayDeque.prototype.size = function() {
    return this.elements.length;
};

ArrayDeque.prototype.toArray = function() {
    return this.elements;
};

ArrayDeque.prototype.toString = function() {
    return this.elements.toString();
};

/*
 * Class: ArrayList
 * ----------------
 * Simulates the <code>ArrayList</code> class.
 */

var ArrayList = function() {
    if (!(this instanceof ArrayList)) return new ArrayList();
    this.elements = [];
};

ArrayList.prototype.$class = new Class("ArrayList", ArrayList);

ArrayList.prototype.add = function(a1, a2) {
    if (a2 === undefined) {
        this.elements.push(a1);
    } else if (a1 === this.elements.length) {
        this.elements.push(a2);
    } else {
        this.elements.splice(a1, 0, a2);
    }
};

ArrayList.prototype.remove = function(index) {
    this.elements.splice(index, 1);
};

ArrayList.prototype.removeAt = function(index) {
    this.elements.splice(index, 1);
};

ArrayList.prototype.get = function(index) {
    return this.elements[index];
};

ArrayList.prototype.set = function(index, element) {
    this.elements[index] = element;
};

ArrayList.prototype.clear = function() {
    this.elements.splice(0, this.elements.length);
};

ArrayList.prototype.size = function() {
    return this.elements.length;
};

ArrayList.prototype.isEmpty = function() {
    return this.elements.length === 0;
};

ArrayList.prototype.contains = function(elem) {
    return this.indexOf(elem) !== -1;
};

ArrayList.prototype.indexOf = function(elem) {
    for (var i = 0; i < this.elements.length; i++) {
        if (elem === this.elements[i]) return i;
    }
    return -1;
};

ArrayList.prototype.lastIndexOf = function(elem, x) {
    for (var i = this.elements.length - 1; i >= 0 ; i--) {
        if (elem === this.elements[i]) return i;
    }
    return -1;
};

ArrayList.prototype.toArray = function() {
    return this.elements;
};

ArrayList.prototype.toString = function() {
    return this.elements.toString();
};

/*
 * Class: Arrays
 * -------------
 * Simulates the <code>Arrays</code> class.
 */

var Arrays = function() {
    if (!(this instanceof Arrays)) return new Arrays();
    throw new RuntimeException("Arrays should not be instantiated");
};

Arrays.prototype.$class = new Class("Arrays", Arrays);

Arrays.sort = function(array, cmp) {
    var fn = null;
    if (cmp) {
        if ((typeof cmp) == "function") {
            fn = cmp;
        } else if (cmp.compare) {
            fn = cmp.compare;
        } else {
            fn = function(x, y) {
                return cmp.compare(x, y);
            }
        }
    } else {
        if ((typeof array[0]) == "number") {
            fn = function(x, y) { return x - y; };
        }
    }
    if (fn) {
        array.sort(fn);
    } else {
        array.sort();
    }
};

/*
 * Class: HashMap
 * --------------
 * Simulates the <code>HashMap</code> class.
 */

var HashMap = function() {
    if (!(this instanceof HashMap)) return new HashMap();
    this.map = new Map();
};

HashMap.prototype.$class = new Class("HashMap", HashMap);

HashMap.prototype.put = function(key, value) {
    this.map.set(key, value);
};

HashMap.prototype.get = function(key) {
    var value = this.map.get(key);
    return (value === undefined) ? null : value;
};

HashMap.prototype.remove = function(key) {
    this.map.delete(key);
};

HashMap.prototype.clear = function() {
    this.map.clear();
};

HashMap.prototype.containsKey = function(key) {
    return this.map.has(key);
};

HashMap.prototype.containsValue = function(value) {
    for (var key of this.map.keys()) {
        if (this.map.get(key) === value) return true;
    }
    return false;
};

HashMap.prototype.isEmpty = function() {
    return this.map.size === 0;
};

HashMap.prototype.size = function() {
    return this.map.size;
};

HashMap.prototype.keySet = function() {
    var set = new HashSet();
    for (var key of this.map.keys()) {
        set.add(key);
    }
    return set;
};

HashMap.prototype.toString = function() {
    str = "{";
    for (var key of this.map.keys()) {
        if (str.length > 1) str += ",";
        str += key + ":" + this.map.get(key);
    }
    return str + "}";
};

/*
 * Class: HashSet
 * --------------
 * Simulates the <code>HashSet</code> class.
 */

var HashSet = function() {
    if (!(this instanceof HashSet)) return new HashSet();
    this.clear()
};

HashSet.prototype.$class = new Class("HashSet", HashSet);

HashSet.prototype.add = function(x) {
    if (!this.elements["" + x]) {
        this.elements["" + x] = [x];
        this.count++;
    }
};

HashSet.prototype.remove = function(x) {
    if (delete this.elements["" + x]) this.count--;
};

HashSet.prototype.clear = function() {
    this.elements = {};
    this.count = 0;
};

HashSet.prototype.contains = function(x) {
    return !!this.elements["" + x];
};

HashSet.prototype.isEmpty = function() {
    return this.count == 0;
};

HashSet.prototype.size = function() {
    return this.count;
};

HashSet.prototype.toArray = function() {
    var array = [];
    for (var key in this.elements) {
        array.push(this.elements[key][0]);
    }
    return array;
};

HashSet.prototype.toString = function() {
    str = "{";
    for (var key in this.elements) {
        if (str.length > 1) str += ",";
        str += key;
    }
    return str + "}";
};

/*
 * Class: LinkedList
 * -----------------
 * Simulates enough of the <code>LinkedList</code> class to implement a queue.
 */

var LinkedList = function() {
    if (!(this instanceof LinkedList)) return new LinkedList();
    this.elements = [];
};

LinkedList.prototype.$class = new Class("LinkedList", LinkedList);

LinkedList.prototype.add = function(a1, a2) {
    if (a2 === undefined) {
        this.elements.push(a1);
    } else if (a1 === this.elements.length) {
        this.elements.push(a2);
    } else {
        this.elements.splice(a1, 0, a2);
    }
};

LinkedList.prototype.element = function() {
    return this.elements[0];
}

LinkedList.prototype.remove = function() {
    return this.elements.shift();
};

LinkedList.prototype.clear = function() {
    this.elements = [];
};

LinkedList.prototype.size = function() {
    return this.elements.length;
};

LinkedList.prototype.isEmpty = function() {
    return this.elements.length === 0;
};

LinkedList.prototype.toString = function() {
    return this.elements.toString();
};

/*
 * Class: TreeMap
 * --------------
 * Simulates the <code>TreeMap</code> class.
 */

var TreeMap = function() {
    if (!(this instanceof TreeMap)) return new TreeMap();
    this.map = new Map();
};

TreeMap.prototype.$class = new Class("TreeMap", TreeMap);

TreeMap.prototype.put = function(key, value) {
    this.map.set(key, value);
};

TreeMap.prototype.get = function(key) {
    var value = this.map.get(key);
    return (value === undefined) ? null : value;
};

TreeMap.prototype.remove = function(key) {
    this.map.delete(key);
};

TreeMap.prototype.clear = function() {
    this.map.clear();
};

TreeMap.prototype.containsKey = function(key) {
    return this.map.has(key);
};

TreeMap.prototype.containsValue = function(value) {
    for (var key of this.map.keys()) {
        if (this.map.get(key) === value) return true;
    }
    return false;
};

TreeMap.prototype.isEmpty = function() {
    return this.map.size === 0;
};

TreeMap.prototype.size = function() {
    return this.map.size;
};

TreeMap.prototype.keySet = function() {
    var set = new TreeSet();
    for (var key of this.map.keys()) {
        set.add(key);
    }
    return set;
};

TreeMap.prototype.toString = function() {
    str = "{";
    for (var key of this.map.keys()) {
        if (str.length > 1) str += ",";
        str += key + ":" + this.map.get(key);
    }
    return str + "}";
};

/*
 * Class: TreeSet
 * --------------
 * Simulates the <code>TreeSet</code> class.
 */

var TreeSet = function() {
    if (!(this instanceof TreeSet)) return new TreeSet();
    this.clear()
};

TreeSet.prototype.$class = new Class("TreeSet", TreeSet);

TreeSet.prototype.add = function(x) {
    if (!this.elements["" + x]) {
        this.elements["" + x] = [x];
        this.count++;
    }
};

TreeSet.prototype.remove = function(x) {
    if (delete this.elements["" + x]) this.count--;
};

TreeSet.prototype.clear = function() {
    this.elements = {};
    this.count = 0;
};

TreeSet.prototype.contains = function(x) {
    return !!this.elements["" + x];
};

TreeSet.prototype.isEmpty = function() {
    return this.count == 0;
};

TreeSet.prototype.size = function() {
    return this.count;
};

TreeSet.prototype.toArray = function() {
    var array = [];
    for (var key in this.elements) {
        array.push(this.elements[key][0]);
    }
    array.sort();
    return array;
};

TreeSet.prototype.toString = function() {
    str = "{";
    for (var key in this.elements) {
        if (str.length > 1) str += ",";
        str += key;
    }
    return str + "}";
};

/*
 * Class: Stack
 * ------------
 * Simulates the <code>Stack</code> class.
 */

var Stack = function() {
    if (!(this instanceof Stack)) return new Stack();
    ArrayList.call(this);
};

Stack.prototype = inheritPrototype(ArrayList, "Stack extends ArrayList");
Stack.prototype.constructor = Stack;
Stack.prototype.$class = new Class("Stack", Stack);

Stack.prototype.push = function(element) {
    this.add(element);
};

Stack.prototype.pop = function() {
    if (this.isEmpty()) throw new RuntimeException("pop: empty stack");
    var n = this.size();
    var result = this.get(n - 1);
    this.remove(n - 1);
    return result;
};

Stack.prototype.peek = function() {
    if (this.isEmpty()) throw new RuntimeException("pop: empty stack");
    return this.get(this.size() - 1);
};

/* Define the package */

return {
    ArrayDeque : ArrayDeque,
    ArrayList : ArrayList,
    Arrays : Arrays,
    HashMap : HashMap,
    HashSet : HashSet,
    LinkedList : LinkedList,
    Stack : Stack,
    TreeMap : TreeMap,
    TreeSet : TreeSet

};

/* Trailer for require.js */

});
