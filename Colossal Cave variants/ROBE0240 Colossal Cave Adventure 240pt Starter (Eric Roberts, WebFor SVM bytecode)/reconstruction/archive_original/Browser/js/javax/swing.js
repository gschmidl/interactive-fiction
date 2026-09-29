/*
 * File: swing.js
 * --------------
 * This package simulates parts of the javax.swing package.
 */

/* Header for requirejs */

define([ "jslib",
         "java/awt",
         "java/lang" ],

function(jslib,
         java_awt,
  java_lang) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var addListener = jslib.addListener;
var ActionEvent = java_awt.ActionEvent;
var AdjustmentEvent = java_awt.AdjustmentEvent;
var Color = java_awt.Color;
var ComponentEvent = java_awt.ComponentEvent;
var Dimension = java_awt.Dimension;
var FlowLayout = java_awt.FlowLayout;
var FocusEvent = java_awt.FocusEvent;
var Font = java_awt.Font;
var FontMetrics = java_awt.FontMetrics;
var Graphics = java_awt.Graphics;
var KeyEvent = java_awt.KeyEvent;
var MouseEvent = java_awt.MouseEvent;
var Point = java_awt.Point;
var Rectangle = java_awt.Rectangle;
var Class = java_lang.Class;

/* EmptyBorder.js */

var EmptyBorder = function() {
    if (!(this instanceof EmptyBorder)) return new EmptyBorder();
    /* Empty */
};

EmptyBorder.prototype.$class = new Class("EmptyBorder", EmptyBorder);

EmptyBorder.prototype.addBorder = function(element) {
    /* Empty */
};

EmptyBorder.prototype.getBorders = function() {
    return { left: 0, right: 0, top: 0, bottom: 0 };
};

EmptyBorder.prototype.toString = function() {
    return "EmptyBorder()";
};

/* LineBorder.js */

var LineBorder = function(color, thickness) {
    if (!(this instanceof LineBorder)) {
        return new LineBorder(color, thickness);
    }
    if (thickness === undefined) thickness = 1;
    this.color = color;
    this.thickness = thickness;
};

LineBorder.prototype.$class = new Class("LineBorder", LineBorder);

LineBorder.prototype.addBorder = function(element) {
    element.style.border = this.color.getColorTag() + " solid " +
                           this.thickness + "px";
};

LineBorder.prototype.getBorders = function() {
    return {
        left: this.thickness,
        right: this.thickness,
        top: this.thickness,
        bottom: this.thickness
    };
};

LineBorder.prototype.toString = function() {
    return "LineBorder(" + this.color + ", " + this.thickness + ")";
};

/* MatteBorder.js */

var MatteBorder = function(top, left, bottom, right, color) {
    if (!(this instanceof MatteBorder)) {
        return new MatteBorder(top, left, bottom, right, color);
    }
    this.top = top;
    this.left = left;
    this.bottom = bottom;
    this.right = right;
    this.color = color || Color.BLACK;
};

MatteBorder.prototype.$class = new Class("MatteBorder", MatteBorder);

MatteBorder.prototype.addBorder = function(element) {
    var tag = this.color.getColorTag();
    element.style.borderTop = tag + " solid " + this.top + "px";
    element.style.borderLeft = tag + " solid " + this.left + "px";
    element.style.borderBottom = tag + " solid " + this.bottom + "px";
    element.style.borderRight = tag + " solid " + this.Right + "px";
};

MatteBorder.prototype.getBorders = function() {
    return {
        left: this.left,
        right: this.right,
        top: this.top,
        bottom: this.bottom
    };
}

MatteBorder.prototype.toString = function() {
    return "MatteBorder(" + this.color + ", " + this.thickness + ")";
};

/* BorderFactory.js */

var BorderFactory = function() {
    throw new Error("BorderFactoryy should not be instantiated");
};

BorderFactory.prototype.$class = new Class("BorderFactory", BorderFactory);

BorderFactory.createEmptyBorder = function() {
    return new EmptyBorder();
};

BorderFactory.createLineBorder = function(color, thickness) {
    return new LineBorder(color, thickness);
};

BorderFactory.createMatteBorder = function(top, left, bottom, right, color) {
    return new MatteBorder(top, left, bottom, right, color);
};

/* ChangeEvent.js */

var ChangeEvent = function(source) {
    if (!(this instanceof ChangeEvent)) return new ChangeEvent(source);
    this.source = source;
};

ChangeEvent.prototype.$class = new Class("ChangeEvent", ChangeEvent);

ChangeEvent.prototype.dispatch = function() {
    this.source.fireChangeListeners(this);
};

ChangeEvent.prototype.getSource = function() {
    return this.source;
};

/* JComponent.js */

var JComponent = function(type) {
    if (!(this instanceof JComponent)) return new JComponent(type);
    if (!type) type = "div";
    if (typeof(type) === "string") {
        this.element = document.createElement(type);
    } else {
        this.element = type;
    }
    this.uid = JComponent.uid++;
    this.element.source = this;
    this.element.style.overflowX = "hidden";
    this.element.style.overflowY = "hidden";
    this.element.tabindex = -1;
    this.children = [];
    this.mouseListeners = [];
    this.mouseMotionListeners = [];
    this.keyListeners = [];
    this.componentListeners = [];
    this.focusListeners = [];
    this.bg = Color.WHITE;
    this.fg = Color.BLACK;
    this.focusKeysEnabled = true;
    this.layout = null;
    this.font = Font.decode("SansSerif-14");
    this.opaque = true;
};

JComponent.prototype.$class = new Class("JComponent", JComponent);

JComponent.currentFocus = null;

JComponent.prototype.add = function(child, arg) {
    if (typeof(arg) === "number") {
        if (arg === -1 || arg >= this.children.length) {
            this.children.push(child);
            this.element.appendChild(child.element);
        } else {
            this.children.splice(arg, 0, child);
            this.element.insertBefore(child.element,
                                      this.element.children[arg]);
        }
    } else {
        if (this.layout) this.layout.addLayoutComponent(arg, child);
        this.children.push(child);
        this.element.appendChild(child.element);
    }
    this.element.style.position = "absolute";
    child.element.style.position = "absolute";
    child.parent = this;
};

JComponent.prototype.remove = function(child) {
    var index = this.children.indexOf(child);
    if (index >= 0) this.children.splice(index, 1);
    this.element.removeChild(child.element);
};

JComponent.prototype.addMouseListener = function(listener) {
    this.mouseListeners.push(listener);
    var source = this;
    var mouseDown = function(e) {
        source.requestFocus();
        source.lastX = e.x;
        source.lastY = e.y;
        new MouseEvent(e, source).dispatch();
    };
    var mouseClick = function(e) {
        if (source.lastX == e.x && source.lastY == e.y) {
            new MouseEvent(e, source).dispatch();
        }
    };
    var mouseEvent = function(e) {
        new MouseEvent(e, source).dispatch();
    };
    var focusEvent = function(e) {
        JComponent.currentFocus = source;
        new FocusEvent(source, FocusEvent.FOCUS_GAINED).dispatch();
    }
    var blurEvent = function(e) {
        JComponent.currentFocus = null;
        new FocusEvent(source, FocusEvent.FOCUS_LOST).dispatch();
    }
    addListener(this.element, "mousedown", mouseDown);
    addListener(this.element, "mouseup", mouseEvent);
    addListener(this.element, "click", mouseClick);
    addListener(this.element, "dblclick", mouseClick);
    addListener(this.element, "mouseout", mouseEvent);
    addListener(this.element, "mouseover", mouseEvent);
    addListener(this.element, "focus", focusEvent);
    addListener(this.element, "blur", blurEvent);
};

JComponent.prototype.addMouseMotionListener = function(listener) {
    this.mouseMotionListeners.push(listener);
    var source = this;
    var callback = function(e) { new MouseEvent(e, source).dispatch(); };
    addListener(this.element, "mousemove", callback);
};

JComponent.prototype.addKeyListener = function(listener) {
    this.keyListeners.push(listener);
    var callback = function(e) {
        var target = e.target || e.srcElement;
        var source = target.source;
        if (source !== JComponent.currentFocus) return true;
        var code = e.keyCode;
        if (code === KeyEvent.VK_TAB && !source.focusKeysEnabled) {
            if (e.preventDefault) e.preventDefault();
            new KeyEvent(e, source).dispatch();
            return false;
        } else {
            if (code === KeyEvent.VK_BACK_SPACE ||
                code === KeyEvent.VK_DELETE) {
                if (e.preventDefault) e.preventDefault();
            }
            new KeyEvent(e, source).dispatch();
        }
        return true;
    };
    if (!document.keyListenersInitialized) {
        addListener(document, "keydown", callback);
        addListener(document, "keypress", callback);
        addListener(document, "keyup", callback);
        document.keyListenersInitialized = true;
    }
};

JComponent.prototype.addComponentListener = function(listener) {
    this.componentListeners.push(listener);
};

JComponent.prototype.addFocusListener = function(listener) {
    /* Ignored for now */
};

JComponent.prototype.setPreferredSize = function(width, height) {
    if (typeof(width) === "object") {
        height = width.height;
        width = width.width;
    }
    this.preferredSize = new Dimension(width, height);
};

JComponent.prototype.getPreferredSize = function() {
    if (this.preferredSize) return this.preferredSize;
    if (this.layout) return this.layout.preferredLayoutSize(this);
    return new Dimension(0, 0);
};

JComponent.prototype.setMinimumSize = function(width, height) {
    if (typeof(width) === "object") {
        height = width.height;
        width = width.width;
    }
    this.minimumSize = new Dimension(width, height);
};

JComponent.prototype.getMinimumSize = function() {
    if (this.preferredSize) return this.preferredSize;
    if (this.layout) return this.layout.preferredLayoutSize(this);
    return new Dimension(0, 0);
};

JComponent.prototype.getWidth = function() {
    return this.element.width;
};

JComponent.prototype.getHeight = function() {
    return this.element.height;
};

JComponent.prototype.getInsets = function() {
    return { left: 0, right: 0, top: 0, bottom: 0 };
};

JComponent.prototype.isOpaque = function() {
    return this.opaque;
};

JComponent.prototype.setOpaque = function(flag) {
    this.opaque = flag;
};

JComponent.prototype.getForeground = function() {
    return this.fg;
};

JComponent.prototype.getBackground = function() {
    return this.bg;
};

JComponent.prototype.setForeground = function(c) {
    this.fg = c;
};

JComponent.prototype.setBackground = function(c) {
    this.bg = c;
};

JComponent.prototype.getFont = function() {
    return this.font;
};

JComponent.prototype.setFont = function(font) {
    this.font = font;
    if (this.element) this.element.style.font = font.getFontTag();
};

JComponent.prototype.getFontMetrics = function(font) {
    return new FontMetrics(font);
};

JComponent.prototype.getSize = function() {
    return new Dimension(this.element.width, this.element.height);
};

JComponent.prototype.setSize = function(width, height) {
    if (typeof(width) === "object") {
        height = width.height;
        width = width.width;
    }
    this.width = width;
    this.height = height;
    if (this.element && this.element.style) {
        this.element.style.width = width + "px";
        this.element.style.height = height + "px";
    }
    if (this.element) {
        this.element.width = width;
        this.element.height = height;
    }
    if (this.canvas) {
        this.canvas.width = width;
        this.canvas.height = height;
    }
    if (this.layout) this.layout.layoutContainer(this);
    var e = new ComponentEvent(this);
    var n = this.componentListeners.length;
    for (var i = 0; i < n; i++) {
        this.componentListeners[i].componentResized(e);
    }
};

JComponent.prototype.setLocation = function(x, y) {
    if (this.parent && this.parent.x) {
        x += this.parent.x;
        y += this.parent.y;
    }
    this.x = x;
    this.y = y;
    if (this.element) {
        this.element.style.left = x + "px";
        this.element.style.top = y + "px";
    }
};

JComponent.prototype.setBounds = function(x, y, width, height) {
    this.setSize(width, height);
    this.setLocation(x, y);
};

JComponent.prototype.getBounds = function() {
    var bb = this.element.getBoundingClientRect();
    return new Rectangle(bb.x, bb.y, bb.width, bb.height);
};
    
JComponent.prototype.setFocusTraversalKeysEnabled = function(flag) {
    this.focusKeysEnabled = flag;
};

JComponent.prototype.setLayout = function(layout) {
    this.layout = layout;
};

JComponent.prototype.getLayout = function() {
    return this.layout;
};

JComponent.prototype.pack = function() {
    /* Ignored */
};

JComponent.prototype.setBorder = function(border) {
    this.border = border;
    border.addBorder(this.element);
};

JComponent.prototype.getComponent = function(k) {
    return this.children[k];
};

JComponent.prototype.getComponentCount = function() {
    return this.children.length;
};

JComponent.prototype.toString = function() {
    return "JComponent#" + this.uid;
};

JComponent.prototype.getGraphics = function() {
    var g = new Graphics(this);
    var ctx = g.ctx;
    return g;
};

JComponent.prototype.repaint = function() {
    var g = this.getGraphics();
    if (g.ctx && this.bg) {
        g.ctx.fillStyle = this.bg.getColorTag();
        g.ctx.fillRect(0, 0, this.element.width, this.element.height);
    }
    for (var i in this.children) {
// Figure out this patch
if (this.children[i].img) alert(jslib.toString(this.children[i]));
      this.children[i].repaint();
    }
};

JComponent.prototype.requestFocus = function() {
    this.element.focus();
};

JComponent.prototype.paint = function(g) {
    /* Empty */
};

JComponent.prototype.fireMouseListeners = function(e) {
    var listeners = this.mouseListeners;
    switch (e.action) {
      case "mousePressed":
        this.mouseDown = true;
        break;
      case "mouseReleased":
        this.mouseDown = false;
        break;
      case "mouseMoved":
        if (this.mouseDown) e.action = "mouseDragged";
        listeners = this.mouseMotionListeners;
        break;
    }
    for (var i = 0; i < listeners.length; i++) {
        var listener = listeners[i];
        if (listener[e.action]) {
            listener[e.action](e);
        }
    }
};

JComponent.prototype.fireKeyListeners = function(e) {
    var listeners = this.keyListeners;
    for (var i = 0; i < listeners.length; i++) {
        var listener = listeners[i];
        if (listener[e.action]) {
            listener[e.action](e);
        }
    }
};

JComponent.prototype.fireFocusListeners = function(e) {
    var listeners = this.focusListeners;
    for (var i = 0; i < listeners.length; i++) {
        var listener = listeners[i];
        if (listener[e.id]) listener[e.id](e);
    }
};

JComponent.prototype.addWindowListener = function(listener) {
    /* Empty */
};

JComponent.prototype.setVisible = function(flag) {
    /* Empty */
};

JComponent.prototype.getTreeLock = function() {
    return true;
};

JComponent.uid = 0;

/* JButton.js */

var JButton = function(label) {
    if (!(this instanceof JButton)) return new JButton(label);
    JComponent.call(this);
    var button = document.createElement("button");
    button.type = "button";
    button.innerHTML = label;
    this.text = label;
    this.actionCommand = label;
    this.listeners = [];
    this.element = button;
    var body = document.getElementsByTagName("body")[0];
    button.style.opacity = 0;
    button.style.position = "absolute";
    button.style.left = "0px";
    button.style.top = "0px";
    body.appendChild(button);
    var width = parseInt(button.clientWidth) + JButton.EXTRA_WIDTH;
    var height = parseInt(button.clientHeight);
    body.removeChild(button);
    button.style.opacity = 1;
    this.setPreferredSize(new Dimension(width, height));
};

JButton.prototype =
    jslib.inheritPrototype(JComponent, "JButton extends JComponent");
JButton.prototype.constructor = JButton;
JButton.prototype.$class = new Class("JButton", JButton);

JButton.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
    var button = this;
    var callback = function(e) {
        button.fireActionListeners();
        e.stopPropagation();
    };
    var ignore = function(e) {
        e.stopPropagation();
    };
    addListener(this.element, "mousedown", ignore);
    addListener(this.element, "mouseup", ignore);
    addListener(this.element, "click", callback);
};

JButton.prototype.fireActionListeners = function() {
    var e = null;
    for (var i in this.listeners) {
        var listener = this.listeners[i];
        if (typeof(listener) == "function") {
            listener(this.actionCommand);
        } else {
            if (!e) e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                                        this.actionCommand);
            listener.actionPerformed(e);
        }
    }
};

JButton.prototype.getActionCommand = function() {
    return this.actionCommand;
};

JButton.prototype.setLocation = function(x, y) {
    if (!y) {
        y = x.y;
        x = x.x;
    }
    JComponent.prototype.setLocation.call(this, x, y + JButton.DELTA_Y);
};

JButton.prototype.toString = function() {
    return "JButton(" + this.text + ")";
};

JButton.EXTRA_WIDTH = 5;
JButton.DELTA_Y = 2;

/* JCheckBox.js */

var JCheckBox = function(label) {
    if (!(this instanceof JCheckBox)) return new JCheckBox(label);
    JComponent.call(this);
    var checkBox = document.createElement("input");
    checkBox.type = "checkbox";
    var interactor = document.createElement("div");
    interactor.style.fontFamily = "'Lucida Grande',ariel,helvetica";
    interactor.style.fontSize = "11px";
    var span = document.createElement("span");
    interactor.appendChild(checkBox);
    interactor.appendChild(span);
    span.innerHTML = label;
    this.actionCommand = label;
    this.listeners = [];
    this.checkBox = checkBox;
    this.element = interactor;
    var body = document.getElementsByTagName("body")[0];
    interactor.style.opacity = 0;
    interactor.style.position = "absolute";
    interactor.style.left = "0px";
    interactor.style.top = "0px";
    body.appendChild(interactor);
    var width = parseInt(interactor.offsetWidth) + JCheckBox.EXTRA_WIDTH;
    var height = parseInt(interactor.offsetHeight);
    body.removeChild(interactor);
    interactor.style.opacity = 1;
    this.setPreferredSize(new Dimension(width, height));
};

JCheckBox.prototype =
    jslib.inheritPrototype(JComponent, "JCheckBox extends JComponent");
JCheckBox.prototype.constructor = JCheckBox;
JCheckBox.prototype.$class = new Class("JCheckBox", JCheckBox);

JCheckBox.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
    var checkBox = this;
    var callback = function(e) {
        checkBox.fireActionListeners();
        e.stopPropagation();
    };
    var ignore = function(e) {
        e.stopPropagation();
    };
    addListener(this.element, "mousedown", ignore);
    addListener(this.element, "mouseup", ignore);
    addListener(this.element, "click", callback);
};

JCheckBox.prototype.fireActionListeners = function() {
    var e = null;
    for (var i in this.listeners) {
        var listener = this.listeners[i];
        if (typeof(listener) == "function") {
            listener(this.actionCommand);
        } else {
            if (!e) e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                                        this.actionCommand);
            listener.actionPerformed(e);
        }
    }
};

JCheckBox.prototype.getText = function() {
    return this.text;
};

JCheckBox.prototype.isSelected = function() {
    return this.checkBox.checked;
};

JCheckBox.prototype.getActionCommand = function() {
    return this.actionCommand;
};

JCheckBox.prototype.setLocation = function(x, y) {
    if (!y) {
        y = x.y;
        x = x.x;
    }
    JComponent.prototype.setLocation.call(this, x, y + JCheckBox.DELTA_Y);
};

JCheckBox.prototype.toString = function() {
    return "JCheckBox(" + this.text + ")";
};

JCheckBox.EXTRA_WIDTH = 1;
JCheckBox.DELTA_Y = 2;

/* JPanel.js */

var JPanel = function() {
    if (!(this instanceof JPanel)) return new JPanel();
    JComponent.call(this);
    this.setLayout(new FlowLayout());
};

JPanel.prototype =
    jslib.inheritPrototype(JComponent, "JPanel extends JComponent");
JPanel.prototype.constructor = JPanel;
JPanel.prototype.$class = new Class("JPanel", JPanel);

/* JLabel.js */

var JLabel = function(text) {
    if (!(this instanceof JLabel)) return new JLabel(text);
    JComponent.call(this)
    var div = document.createElement("div");
    div.innerHTML = text;
    div.source = this;
    div.style.fontFamily = "arial,sans-serif";
    div.style.fontSize = "11px";
    this.text = text;
    this.element = div;
    var body = document.getElementsByTagName("body")[0];
    div.style.opacity = 0;
    div.style.position = "absolute";
    div.style.left = "0px";
    div.style.top = "0px";
    body.appendChild(div);
    var width = parseInt(div.clientWidth);
    var height = parseInt(div.clientHeight);
    body.removeChild(div);
    div.style.opacity = 1;
    this.setPreferredSize(new Dimension(width, height));
};

JLabel.prototype =
    jslib.inheritPrototype(JComponent, "JLabel extends JComponent");
JLabel.prototype.constructor = JLabel;
JLabel.prototype.$class = new Class("JLabel", JLabel);

JLabel.prototype.getText = function() {
    return this.text;
};

JLabel.prototype.toString = function() {
    return "JLabel(" + this.text + ")";
};

/* JMenu.js */

// Stub implementation

var JMenu = function(name) {
    if (!(this instanceof JMenu)) return new JMenu(name);
    this.name = name;
    this.items = [];
};

JMenu.prototype.$class = new Class("JMenu", JMenu);

JMenu.prototype.add = function(item) {
    this.items.push(item);
};

JMenu.prototype.fireMouseListeners = function(e) {
    this.table = document.createElement("table");
    this.table.cellPadding = 2;
    this.table.cellSpacing = 0;
    for (var i in this.items) {
        var item = this.items[i];
        var tr = document.createElement("tr");
        var td = document.createElement("td");
        td.style.backgroundColor = "white";
        td.style.fontFamily = "arial,sans-serif";
        td.style.fontSize = "11px";
        td.innerHTML = item.name + "&nbsp;&nbsp;&nbsp;";
        tr.appendChild(td);
        this.addItemCallback(td, item);
        this.table.appendChild(tr);
        this.element = td;
    }
    var element = e.source.element;
    this.table.style.position = "absolute";
    var loc = Point.getElementLocation(element);
    loc.y += element.offsetHeight;
    // Just to test what it looks like
    this.table.style.left = loc.x * 0 + 220;
    this.table.style.top = loc.y * 0 + 25;
    var div = document.getElementById("canvas");
    div.appendChild(this.table);
};

JMenu.prototype.addItemCallback = function(td, source) {
    var callback = function(e) { new MouseEvent(e, source).dispatch(); };
    if (td.addListener) {
        td.addListener("mouseup", callback, false);
        td.addListener("mouseover", callback, false);
        td.addListener("mouseout", callback, false);
    } else if (div.attachEvent) {
        td.attachEvent("onmouseup", callback);
        td.attachEvent("onmouseover", callback);
        td.attachEvent("onmouseout", callback);
    } else {
        td.onmouseup = callback;
        td.onmouseover = callback;
        td.onmouseout = callback;
    }
};

/* JMenuBar.js */

// Stub implementation

var JMenuBar = function() {
    if (!(this instanceof JMenuBar)) return new JMenuBar();
    this.menus = [];
};

JMenuBar.prototype.$class = new Class("JMenuBar", JMenuBar);

JMenuBar.prototype.add = function(menu) {
    this.menus.push(menu);
};

/* JMenuItem.js */

// Stub implementation

var JMenuItem = function(name) {
    if (!(this instanceof JMenuItem)) return new JMenuItem(name);
    this.name = name;
    this.listeners = [];
    this.accelerator = null;
};

JMenuItem.prototype.$class = new Class("JMenuItem", JMenuItem);

JMenuItem.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
};

JMenuItem.prototype.setAccelerator = function(accelerator) {
    this.accelerator = accelerator;
};

JMenuItem.prototype.fireMouseListeners = function(e) {
    alert("e = " + e);
};

/* JScrollBar.js */

var JScrollBar = function(orientation) {
    if (!(this instanceof JScrollBar)) return new JScrollBar(orientation);
    if (!orientation) orientation = JScrollBar.VERTICAL;
    this.orientation = orientation;
    this.listeners = [];
};

JScrollBar.prototype.$class = new Class("JScrollBar", JScrollBar);

JScrollBar.prototype.getOrientation = function() {
    return this.orientation;
};

JScrollBar.prototype.addAdjustmentListener = function(listener) {
    this.listeners.push(listener);
};

JScrollBar.prototype.fireAdjustmentListeners = function() {
    var e = new AdjustmentEvent(this,
                                AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED,
                                AdjustmentEvent.TRACK, 0);
    for (var i in this.listeners) {
        this.listeners[i].adjustmentValueChanged(e);
    }
};

/*
 * Implementation notes: getScrollBarWidth
 * ---------------------------------------
 * This code is taken from Joseph Portelli's posting on JSFiddle at
 * http://jsfiddle.net/UU9kg/17/.
 */

JScrollBar.getScrollBarWidth = function() {
    var outer = document.createElement("div");
    outer.style.visibility = "hidden";
    outer.style.width = "100px";
    outer.style.msOverflowStyle = "scrollbar"; /* needed for WinJS apps */
    document.body.appendChild(outer);
    var widthNoScroll = outer.offsetWidth;
    outer.style.overflow = "scroll";
    var inner = document.createElement("div");
    inner.style.width = "100%";
    outer.appendChild(inner);
    var widthWithScroll = inner.offsetWidth;
    outer.parentNode.removeChild(outer);
    return widthNoScroll - widthWithScroll;
};

JScrollBar.VERTICAL = "overflowY";
JScrollBar.HORIZONTAL = "overflowX";

/* JScrollPane.js */

var JScrollPane = function(view, vPolicy, hPolicy) {
    if (!(this instanceof JScrollPane)) {
        return new JScrollPane(view, vPolicy, hPolicy);
    }
    JComponent.call(this);
    this.element.style.position = "relative";
    this.view = view;
    this.add(view);
    view.setLocation(0, 0);
    this.element.style.overflowY = vPolicy || "auto";
    this.element.style.overflowX = hPolicy || "auto";
    this.vScrollBar = new JScrollBar(JScrollBar.VERTICAL);
    this.hScrollBar = new JScrollBar(JScrollBar.HORIZONTAL);
    var scrollPane = this;
    view.revalidate = function() {
        var pSize = view.getPreferredSize();
        view.setSize(pSize.width, pSize.height);
        view.repaint();
    };
    var callback = function(e) {
        scrollPane.vScrollBar.fireAdjustmentListeners();
        scrollPane.hScrollBar.fireAdjustmentListeners();
    };
    addListener(view.element, "onscroll", callback);
};

JScrollPane.prototype =
    jslib.inheritPrototype(JComponent, "JScrollPane extends JComponent");
JScrollPane.prototype.constructor = JScrollPane;
JScrollPane.prototype.$class = new Class("JScrollPane", JScrollPane);

JScrollPane.prototype.getVerticalScrollBar = function() {
    return this.vScrollBar;
};

JScrollPane.prototype.getHorizontalScrollBar = function() {
    return this.hScrollBar;
};

JScrollPane.HORIZONTAL_SCROLLBAR_NEVER = "hidden";
JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS = "scroll";
JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED = "auto";
JScrollPane.VERTICAL_SCROLLBAR_NEVER = "hidden";
JScrollPane.VERTICAL_SCROLLBAR_ALWAYS = "scroll";
JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED = "auto";

/* JSlider.js */

var JSlider = function(a1, a2, a3, a4) {
    if (!(this instanceof JSlider)) return new JSlider(a1, a2, a3, a4);
    this.orientation = JSlider.HORIZONTAL;
    this.min = 0;
    this.max = 100;
    this.value = 50;
    if (a4) {
        this.orientation = a1;
        this.min = a2;
        this.max = a3;
        this.value = a4;
    } else if (a3) {
        this.min = a1;
        this.max = a2;
        this.value = a3;
    } else if (a2) {
        this.min = a1;
        this.max = a2;
        this.value = Math.floor((a1 + a2) / 2);
    }
    var div = document.createElement("div");
    var table = document.createElement("table");
    div.appendChild(table);
    var tr = document.createElement("tr");
    var td = document.createElement("td");
    var track = document.createElement("div");
    var knob = document.createElement("div");
    table.cellSpacing = 0;
    table.cellPadding = 0;
    table.appendChild(tr);
    td.align = "left";
    td.vAlign = "top";
    tr.appendChild(td);
    track.align = "left";
    track.style.position = "relative";
    track.style.border = "solid black 1px";
    track.style.top = "17px";
    track.style.left = "0px";
    track.style.width = "100px";
    track.style.height = "4px";
    td.appendChild(track);
    knob.align = "left";
    knob.textAlign = "left";
    knob.margin = "0px";
    knob.style.position = "relative";
    knob.style.border = "solid black 1px";
    knob.style.backgroundColor = "darkGray";
    knob.style.top = "5px";
    knob.style.width = "7px";
    knob.style.height = "16px";
    div.style.cursor = "default";
    var source = this;
    var callback = function(e) {
        new MouseEvent(e, source).dispatch();
        e.stopPropagation();
    };
    if (div.addListener) {
        div.addListener("mousedown", callback, false);
        div.addListener("mouseup", callback, false);
        div.addListener("mouseout", callback, false);
        div.addListener("mousemove", callback, false);
    } else if (div.attachEvent) {
        div.attachEvent("onmousedown", callback);
        div.attachEvent("onmouseup", callback);
        div.attachEvent("mouseout", callback);
        div.attachEvent("onmousemove", callback);
    } else {
        div.onmousedown = callback;
        div.onmouseup = callback;
        div.onmouseout = callback;
        div.onmousemove = callback;
    }
    this.element = div;
    this.knob = knob;
    this.setKnobPosition();
    td.appendChild(knob);
};;
JSlider.prototype.$class = new Class("JSlider", JSlider);

JSlider.HORIZONTAL = 0;
JSlider.VERTICAL = 1;

JSlider.prototype.getValue = function() {
    return this.value;
};

JSlider.prototype.toString = function() {
    return "JSlider(" + this.value + " in " + this.min + ":" + this.max + ")";
};

JSlider.prototype.fireMouseListeners = function(e) {
    switch (e.action) {
      case "mousePressed":
        var mx = e.getX();
        var kx = this.value * 93 / (this.max - this.min);
        if (mx > kx && mx < kx + 7) {
            this.draggingKnob = true;
            this.x0 = mx;
            this.v0 = this.value;
        } else if (mx < kx) {
            this.value -= 7;
            this.setKnobPosition();
        } else {
            this.value += 7;
            this.setKnobPosition();
        }
        break;
      case "mouseReleased":
        this.draggingKnob = false;
        break;
      case "mouseMoved":
        if (this.draggingKnob) {
            this.value = this.v0 + (e.getX() - this.x0) / 93 *
                                   (this.max - this.min);
            this.setKnobPosition();
        }
        break;
    }
};

JSlider.prototype.setKnobPosition = function() {
    this.value = Math.min(Math.max(this.value, this.min), this.max);
    var fraction = this.value / (this.max - this.min);
    var x = fraction * 93;
    this.knob.style.left = x + "px";
};

/* JTextField.js */

var JTextField = function(nchars) {
    if (!(this instanceof JTextField)) return new JTextField(nchars);
    JComponent.call(this);
    if (nchars === undefined) nchars = 10;
    this.nchars = nchars;
    var field = document.createElement("input");
    field.type = "text";
    this.actionCommand = "";
    this.listeners = [];
    this.element = field;
    this.setPreferredSize(new Dimension(8 * nchars, 10));
};

JTextField.prototype =
    jslib.inheritPrototype(JComponent, "JTextField extends JComponent");
JTextField.prototype.constructor = JTextField;
JTextField.prototype.$class = new Class("JTextField", JTextField);

JTextField.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
    var field = this;
    var callback = function(e) {
        if (e.which === 10 || e.which === 13) {
            field.fireActionListeners();
            e.stopPropagation();
        }
    };
    addListener(this.element, "keypress", callback);
};

JTextField.prototype.fireActionListeners = function() {
    var e = null;
    for (var i in this.listeners) {
        var listener = this.listeners[i];
        if (typeof(listener) == "function") {
            listener(this.actionCommand);
        } else {
            if (!e) e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                                        this.actionCommand);
            listener.actionPerformed(e);
        }
    }
};

JTextField.prototype.getText = function() {
    return this.element.value;
};

JTextField.prototype.setText = function(str) {
    this.element.value = str;
};

JTextField.prototype.setActionCommand = function(cmd) {
    this.actionCommand = cmd;
};

JTextField.prototype.getActionCommand = function() {
    return this.actionCommand;
};

JTextField.prototype.toString = function() {
    return "JTextField(" + this.text + ")";
};

/* KeyStroke.js */

// Stub implementation

var KeyStroke = function() {
    if (!(this instanceof KeyStroke)) return new KeyStroke();
    this.menus = [];
};

KeyStroke.prototype.$class = new Class("KeyStroke", KeyStroke);

KeyStroke.getKeyStroke = function(ch, modifiers) {
    return toStr(ch);
};

/* SwingUtilities.js */

var SwingUtilities = function() {
    /* Empty */
};

SwingUtilities.prototype.$class = new Class("SwingUtilities", SwingUtilities);

SwingUtilities.invokeLater = function(target) {
    setTimeout(function() { target.run(); }, 1);
};

/* Timer.js */

var Timer = function(delay, listener) {
    if (!(this instanceof Timer)) return new Timer(delay, listener);
    this.delay = delay;
    this.initialDelay = delay;
    this.listeners = [];
    if (listener) this.addActionListener(listener);
    this.repeats = true;
    this.actionCommand = null;
    this.runFlag = false;
};

Timer.prototype.$class = new Class("Timer", Timer);

Timer.prototype.addActionListener = function(listener) {
    this.listeners.push(listener);
};

Timer.prototype.fireActionListeners = function() {
    var e = null;
    for (var i in this.listeners) {
        var listener = this.listeners[i];
        if (typeof(listener) == "function") {
            listener(this.actionCommand);
        } else {
            if (!e) e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED,
                                        this.actionCommand);
            listener.actionPerformed(e);
        }
    }
};

Timer.prototype.getActionCommand = function() {
    return this.actionCommand;
};

Timer.prototype.getActionListeners = function() {
    throw new Error("getActionListeners is not yet implemented");
};

Timer.prototype.getDelay = function() {
    return this.delay;
};

Timer.prototype.getInitialDelay = function() {
    return this.initialDelay;
};

Timer.prototype.getListeners = function(listenerType) {
    throw new Error("getListeners is not yet implemented");
};

Timer.getLogTimers = function() {
    throw new Error("getLogTimers is not yet implemented");
};

Timer.prototype.isCoalesce = function() {
    throw new Error("isCoalesce is not yet implemented");
};

Timer.prototype.isRepeats = function() {
    return this.repeats;
};

Timer.prototype.isRunning = function() {
    return runFlag;
};

Timer.prototype.removeActionListener = function(listener) {
    for (var i in this.listeners) {
        if (this.listeners[i] === listener) {
            this.listeners.splice(i, 1);
            return;
        }
    }
};

Timer.prototype.restart = function() {
    this.stop();
    this.start();
};

Timer.prototype.setActionCommand = function(command) {
    this.actionCommand = command;
};

Timer.prototype.setCoalesce = function(flag) {
    throw new Error("setCoalesce is not yet implemented");
};

Timer.prototype.setDelay = function(delay) {
    this.delay = delay;
};

Timer.prototype.setInitialDelay = function(initialDelay) {
    this.initialDelay = initialDelay;
};

Timer.setLogTimers = function(flag) {
    throw new Error("setLogTimers is not yet implemented");
};

Timer.prototype.setRepeats = function(flag) {
    this.repeats = flag;
};

Timer.prototype.start = function() {
    if (this.runFlag) return;
    var timer = this;
    var callback = function() {
        if (timer.repeats) {
            if (timer.timeout) clearTimeout(timer.timeout);
            timer.timeout = setTimeout(callback, timer.delay);
        }
        timer.fireActionListeners();
    }
    timer.runFlag = true;
    timer.timeout = setTimeout(callback, timer.initialDelay);
};

Timer.prototype.stop = function() {
    if (this.runFlag) {
        clearTimeout(this.timeout);
        this.runFlag = false;
    }
};

Timer.prototype.fireActionPerformed = function(e) {
    for (var i in this.listeners) {
        this.listeners[i].actionPerformed(e);
    }
};

/* Define the package */

return {
    BorderFactory : BorderFactory,
    ChangeEvent : ChangeEvent,
    EmptyBorder : EmptyBorder,
    JButton : JButton,
    JCheckBox : JCheckBox,
    JComponent : JComponent,
    JLabel : JLabel,
    JMenu : JMenu,
    JMenuBar : JMenuBar,
    JMenuItem : JMenuItem,
    JPanel : JPanel,
    JScrollBar : JScrollBar,
    JScrollPane : JScrollPane,
    JSlider : JSlider,
    JTextField : JTextField,
    KeyStroke : KeyStroke,
    LineBorder : LineBorder,
    MatteBorder : MatteBorder,
    SwingUtilities : SwingUtilities,
    Timer : Timer
};

/* Trailer for require.js */

});
