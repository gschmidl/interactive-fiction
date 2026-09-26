/*
 * File: awt.js
 * ------------
 * This package simulates parts of the java.awt package.
 */

/* Header for requirejs */

define([ "jslib",
         "java/lang" ],

function(jslib,
  java_lang) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var addListener = jslib.addListener;
var round = jslib.round;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var Class = java_lang.Class;

/*
 * Class: ActionEvent
 * ------------------
 * Simulates the <code>ActionEvent</code> class in JavaScript.
 */

var ActionEvent = function(source, op, cmd, modifiers) {
    if (!(this instanceof ActionEvent)) {
        return new ActionEvent(source, op, cmd, modifiers);
    }
    this.source = source;
    this.op = op;
    this.cmd = cmd;
    this.modifiers = modifiers || 0;
    this.id = ActionEvent.ACTION_PERFORMED;
};

ActionEvent.prototype.$class = new Class("ActionEvent", ActionEvent);

ActionEvent.prototype.dispatch = function() {
    this.source.fireActionListeners(this);
};

ActionEvent.prototype.getSource = function() {
    return this.source;
};

ActionEvent.prototype.getActionCommand = function() {
    return this.cmd;
};

ActionEvent.prototype.getID = function() {
    return this.id;
};

ActionEvent.prototype.getModifiers = function() {
    return this.modifiers;
};

ActionEvent.ACTION_PERFORMED = "actionPerformed";
ActionEvent.ALT_MASK = 512;
ActionEvent.CTRL_MASK = 128;
ActionEvent.META_MASK = 256;
ActionEvent.SHIFT_MASK = 64;

/*
 * Class: Adjustable
 * -----------------
 * Simulates the <code>Adjustable</code> interface in JavaScript.
 */

var Adjustable = function() {
    /* Empty */
};

Adjustable.HORIZONTAL = 0;
Adjustable.VERTICAL = 1;

/*
 * Class: AdjustmentEvent
 * ----------------------
 * Simulates the <code>AdjustmentEvent</code> class in JavaScript.
 */

var AdjustmentEvent = function(source, id, type, value) {
    if (!(this instanceof AdjustmentEvent)) {
        return new AdjustmentEvent(source, id, type, value);
    }
    this.source = source;
    this.id = id;
};

AdjustmentEvent.prototype.$class = new Class("AdjustmentEvent",
                                             AdjustmentEvent);

AdjustmentEvent.prototype.getSource = function() {
    return this.source;
};

AdjustmentEvent.prototype.getID = function() {
    return this.id;
};

AdjustmentEvent.ADJUSTMENT_VALUE_CHANGED = "adjustmentValueChanged";
AdjustmentEvent.TRACK = "track";

/*
 * Class: AffineTransform
 * ----------------------
 * Simulates parts of the AffineTransform class in JavaScript.
 */

var AffineTransform = function(a1, a2, a3, a4, a5, a6) {
    if (!(this instanceof AffineTransform)) {
        return new AffineTransform(a1, a2, a3, a4, a5, a6);
    }
    if (a1 === undefined) {
        this.setTransform(1, 0, 0, 1, 0, 0);
    } else if (a2 === undefined) {
        this.setTransform(a1.a, a1.b, a1.c, a1.d, a1.tx, a1.ty);
    } else if (a5 === undefined) {
        this.setTransform(a1, a2, a3, a4, 0, 0);
    } else {
        this.setTransform(a1, a2, a3, a4, a5, a6);
    }
};

AffineTransform.prototype.$class = new Class("AffineTransform",
                                             AffineTransform);

AffineTransform.prototype.concatenate = function(t) {
    this.setTransform(this.a * t.a + this.c * t.b,
                      this.b * t.a + this.d * t.b,
                      this.a * t.c + this.c * t.d,
                      this.b * t.c + this.d * t.d,
                      this.a * t.tx + this.c * t.ty + this.tx,
                      this.b * t.tx + this.d * t.ty + this.ty);
};

AffineTransform.prototype.createInverse = function() {
    var det = this.getDeterminant();
    if (det == 0) throw new RuntimeException("Noninvertible transform");
    return new AffineTransform(this.d / det, -this.b / det,
                              -this.c / det, this.a / det,
                              (this.c * this.ty - this.tx * this.d) / det,
                              (this.b * this.tx - this.a * this.ty) / det);
};

AffineTransform.prototype.getDeterminant = function() {
    return this.a * this.d - this.c * this.d;
};

AffineTransform.prototype.getScaleX = function() {
    return this.a;
};

AffineTransform.prototype.getScaleY = function() {
    return this.d;
};

AffineTransform.prototype.getShearX = function() {
    return this.c;
};

AffineTransform.prototype.getShearY = function() {
    return this.b;
};

AffineTransform.prototype.getTranslateX = function() {
    return this.tx;
};

AffineTransform.prototype.getTranslateY = function() {
    return this.ty;
};

AffineTransform.prototype.inverseTransform = function(src, dst) {
    return this.createInverse().transform(src, dst);
};

AffineTransform.prototype.rotate = function(theta) {
    var cosTheta = Math.cos(theta);
    var sinTheta = Math.sin(theta);
    this.setTransform(this.a * cosTheta + this.c * sinTheta,
                      this.b * cosTheta + this.d * sinTheta,
                      this.c * cosTheta - this.a * sinTheta,
                      this.d * cosTheta - this.b * sinTheta,
                      this.tx, this.ty);
};

AffineTransform.prototype.scale = function(sx, sy) {
    this.a *= sx;
    this.b *= sx;
    this.c *= sy;
    this.d *= sy;
};

AffineTransform.prototype.setTransform = function(a1, a2, a3, a4, a5, a6) {
    if (a2 === undefined) {
        this.setTransform(a1.a, a1.b, a1.c, a1.d, a1.tx, a1.ty);
    } else {
        this.a = a1;
        this.b = a2;
        this.c = a3;
        this.d = a4;
        this.tx = a5;
        this.ty = a6;
    }
};

AffineTransform.prototype.shear = function(sx, sy) {
    this.setTransform(this.a + (sy * this.c), this.c + (sx * this.a),
                      this.b + (sy * this.d), this.d + (sx * this.b),
                      this.tx, this.ty);
};

AffineTransform.prototype.transform = function(src, dst) {
    if (dst === undefined || dst === null) dst = new Point(0, 0);
    var nx = this.a * src.x + this.c * src.y + this.tx;
    var ny = this.b * src.x + this.d * src.y + this.ty;
    dst.x = nx;
    dst.y = ny;
    return dst;
};

AffineTransform.prototype.translate = function(tx, ty) {
    this.tx += tx * this.a + ty * this.c;
    this.ty += tx * this.b + ty * this.d;
};

AffineTransform.prototype.toString = function() {
    return "[" + this.a + ", " + this.b + ", " + this.c + ", " + this.d +
                          ", " + this.tx + ", " + this.ty + "]";
};

/*
 * Class: BasicStroke
 * ------------------
 * Simulates the BasicStroke class in JavaScript.
 */

var BasicStroke = function(width, cap, join, limit, dashes, offset) {
    if (!(this instanceof BasicStroke)) {
        return new BasicStroke(width, cap, join, limit, dashes, offset);
    }
    this.lineWidth = (width === undefined) ? 1 : width;
    this.lineCap = (cap === undefined) ? BasicStroke.CAP_SQUARE : cap;
    this.lineJoin = (join === undefined) ? BasicStroke.JOIN_MITER : cap;
    this.limit = (limit === undefined) ? 10.0 : limit;
    this.dashes = (dashes === undefined) ? null : dashes;
    this.offset = (offset === null) ? 0 : offset;
};

BasicStroke.prototype.$class = new Class("BasicStroke", BasicStroke);

BasicStroke.CAP_BUTT = "butt";
BasicStroke.CAP_ROUND = "round";
BasicStroke.CAP_SQUARE = "square";
BasicStroke.JOIN_BEVEL = "bevel";
BasicStroke.JOIN_MITER = "miter";
BasicStroke.JOIN_ROUND = "round";

/*
 * Class: BorderLayout
 * -------------------
 * This class simulates the BorderLayout class.
 */

var BorderLayout = function() {
    if (!(this instanceof BorderLayout)) return new BorderLayout();
    this.center = null;
    this.north = null;
    this.south = null;
    this.east = null;
    this.west = null;
    this.parent = null;
};

BorderLayout.prototype.$class = new Class("BorderLayout", BorderLayout);

BorderLayout.prototype.addLayoutComponent = function(name, comp) {
    if (!name) name = "center";
    this[name.toLowerCase()] = comp;
};

BorderLayout.prototype.removeLayoutComponent = function(name, comp) {
    if (!name) name = "center";
    this[name.toLowerCase()] = null;
};

BorderLayout.prototype.preferredLayoutSize = function(target) {
    return this.computeLayoutSize(target, "getPreferredSize");
};

BorderLayout.prototype.minimumLayoutSize = function(target) {
    return this.computeLayoutSize(target, "getMinimumSize");
};

BorderLayout.prototype.computeLayoutSize = function(target, mode) {
    var width = 0;
    var height = 0;
    for (var i in target.children) {
        var child = target.children[i];
        var size = child[mode]();
        if (child === this.center) {
            width += size.width;
            height += size.height;
        } else if (child === this.north || child === this.south) {
            height += size.height;
        } else if (child === this.west || child === this.east) {
            width += size.width;
        }
    }
    return new Dimension(width, height);
};

BorderLayout.prototype.layoutContainer = function(target) {
    var pSize = target.getSize();
    var x = 0;
    var y = 0;
    var width = pSize.width;
    var height = pSize.height;
    if (this.south) {
        var cSize = this.south.getPreferredSize();
        this.south.setBounds(x, pSize.height - cSize.height,
                             width, cSize.height);
        height -= cSize.height;
    }
    if (this.north) {
        var cSize = this.north.getPreferredSize();
        this.north.setBounds(x, y, width, cSize.height);
        y += cSize.height;
        height -= cSize.height;
    }
    if (this.east) {
        var cSize = this.east.getPreferredSize();
        this.east.setBounds(pSize.width - cSize.width, y,
                            cSize.width, height);
        width -= cSize.width;
    }
    if (this.west) {
        var cSize = this.west.getPreferredSize();
        this.west.setBounds(x, y, cSize.width, height);
        x += cSize.width;
        width -= cSize.width;
    }
    if (this.center) {
        this.center.setBounds(x, y, width, height);
    }
};

BorderLayout.NORTH = "NORTH";
BorderLayout.EAST = "EAST";
BorderLayout.SOUTH = "SOUTH";
BorderLayout.WEST = "WEST";
BorderLayout.CENTER = "CENTER";

/*
 * Class: CardLayout
 * -----------------
 * This class simulates the CardLayout class.
 */

var CardLayout = function() {
    if (!(this instanceof CardLayout)) return new CardLayout();
    this.cards = { };
    this.active = null;
};

CardLayout.prototype.$class = new Class("CardLayout", CardLayout);

CardLayout.prototype.addLayoutComponent = function(name, comp) {
    this.cards[name] = comp;
    if (this.active === null) active = comp;
};

CardLayout.prototype.removeLayoutComponent = function(name, comp) {
    // Ignored for now
};

CardLayout.prototype.preferredLayoutSize = function(target) {
    return this.computeLayoutSize(target, "getPreferredSize");
};

CardLayout.prototype.minimumLayoutSize = function(target) {
    return this.computeLayoutSize(target, "getMinimumSize");
};

CardLayout.prototype.computeLayoutSize = function(target, mode) {
    var width = 0;
    var height = 0;
    var n = target.children.length;
    for (var i = 0; i < n; i++) {
        var child = target.children[i];
        var size = child[mode]();
        width = Math.max(width, size.width);
        height = Math.max(height, size.height);
    }
    return new Dimension(width, height);
};

CardLayout.prototype.layoutContainer = function(target) {
    while (target.element.lastChild) {
        target.element.removeChild(target.element.lastChild);
    }
    if (this.active !== null) {
        var size = target.getSize();
        var width = size.width;
        var height = size.height;
        if (this.active.border) {
            var border = this.active.border.getBorders();
            width -= border.left + border.right;
            height -= border.top + border.bottom;
        }   
        target.element.appendChild(this.active.element);
        if (width && height) {
            this.active.setSize(width, height);
            this.active.repaint();
        }
    }
};

CardLayout.prototype.show = function(target, name) {
    if (this.cards[name]) this.active = this.cards[name];
    this.layoutContainer(target);
};

/*
 * Class: Color
 * ------------
 * Simulates the <code>Color</code> class in JavaScript.
 */

var Color = function(r, g, b) {
    if (!(this instanceof Color)) return new Color(r, g, b);
    if (g === undefined) {
        this.rgb = r;
    } else {
        this.rgb = r << 16 | g << 8 | b;
    }
};

Color.prototype.$class = new Class("Color", Color);

Color.prototype.getColorTag = function() {
    return "#" +
           (this.rgb | 0x1000000).toString(16).substring(1).toUpperCase();
};

Color.prototype.getRGB = function() {
    return rgb;
};

Color.prototype.getRed = function() {
    return (rgb >> 16) & 0xFF;
};

Color.prototype.getGreen = function() {
    return (rgb >> 8) & 0xFF;
};

Color.prototype.getBlue = function() {
    return rgb & 0xFF;
};

Color.BLACK = new Color(0x000000);
Color.BLUE = new Color(0x0000FF);
Color.CYAN = new Color(0x00FFFF);
Color.DARK_GRAY = new Color(0x404040);
Color.GRAY = new Color(0x808080);
Color.GREEN = new Color(0x00FF00);
Color.LIGHT_GRAY = new Color(0xC0C0C0);
Color.MAGENTA = new Color(0xFF00FF);
Color.ORANGE = new Color(0xFFC800);
Color.PINK = new Color(0xFFAFAF);
Color.RED = new Color(0xFF0000);
Color.WHITE = new Color(0xFFFFFF);
Color.YELLOW = new Color(0xFFFF00);

/*
 * Class: ComponentEvent
 * ---------------------
 * Simulates the <code>componentResized</code> feature of the
 * <code>ComponentEvent</code> class in JavaScript.
 */

var ComponentEvent = function(source) {
    if (!(this instanceof ComponentEvent)) return new ComponentEvent(source);
    this.source = source;
    this.id = ComponentEvent.COMPONENT_RESIZED;
};

ComponentEvent.prototype.$class = new Class("ComponentEvent", ComponentEvent);

ComponentEvent.prototype.getSource = function() {
    return this.source;
};

ComponentEvent.getID = function() {
    return this.id;
};

ComponentEvent.COMPONENT_RESIZED = "componentResized";

/*
 * Class: Dimension
 * ----------------
 * Simulates the <code>Dimension</code> and <code>Dimension2D</code> classes
 * in JavaScript.
 */

var Dimension = function(width, height) {
    if (!(this instanceof Dimension)) return new Dimension(width, height);
    this.width = width;
    this.height = height;
};

Dimension.prototype.$class = new Class("Dimension", Dimension);

Dimension.prototype.getWidth = function() {
    return this.width;
};

Dimension.prototype.getHeight = function() {
    return this.height;
};

Dimension.prototype.toString = function() {
    return "(" + this.width + "x" + this.height + ")";
};

Dimension.getElementSize = function(e) {
    return new Dimension(e.offsetWidth, e.offsetHeight);
};

/*
 * Class: FlowLayout
 * -----------------
 * Simulates the FlowLayout class in JavaScript.
 */

var FlowLayout = function(align, hgap, vgap) {
    if (!(this instanceof FlowLayout)) {
        return new FlowLayout(align, hgap, vgap);
    }
    if (align === undefined) align = FlowLayout.CENTER;
    if (hgap === undefined) hgap = 5;
    if (vgap === undefined) vgap = 5;
    this.align = align;
    this.hgap = hgap;
    this.vgap = vgap;
    this.parent = null;
};

FlowLayout.prototype.$class = new Class("FlowLayout", FlowLayout);

FlowLayout.prototype.addLayoutComponent = function(name, comp) {
    /* Empty */
};

FlowLayout.prototype.removeLayoutComponent = function(name, comp) {
    /* Empty */
};

FlowLayout.prototype.getAlignment = function() {
    return this.align;
};

FlowLayout.prototype.setAlignment = function(align) {
    this.align = align;
};

FlowLayout.prototype.getHGap = function() {
    return this.hgap;
};

FlowLayout.prototype.setHGap = function(hgap) {
    this.hgap = hgap;
};

FlowLayout.prototype.getVGap = function() {
    return this.vgap;
};

FlowLayout.prototype.setVGap = function(vgap) {
    this.vgap = vgap;
};

FlowLayout.prototype.preferredLayoutSize = function(target) {
    return this.computeLayoutSize(target, "getPreferredSize");
};

FlowLayout.prototype.minimumLayoutSize = function(target) {
    return this.computeLayoutSize(target, "getMinimumSize");
};

FlowLayout.prototype.computeLayoutSize = function(target, mode) {
    var width = 0;
    var height = 0;
    for (var i = 0; i < target.children.length; i++) {
        var child = target.children[i];
        var size = child[mode]();
        if (width !== 0) width += this.hgap;
        width += size.width;
        if (size.height > height) height = size.height;
    }
    width += 2 * this.hgap;
    height += 2 * this.vgap;
    return new Dimension(width, height);
};

FlowLayout.prototype.layoutContainer = function(target) {
    var pSize = target.getSize();
    var x = this.hgap;
    var y = this.vgap;
    var width = 0;
    var height = 0;
    var row = [];
    var sizes = [];
    var n = target.children.length;
    for (var i = 0; i < n; i++) {
        var child = target.children[i];
        var cSize = child.getPreferredSize();
        if (x > this.hgap && x + cSize.width >= pSize.width - this.hgap) {
            switch (this.align) {
              case FlowLayout.LEFT:
                x = this.hgap;
                break;
              case FlowLayout.CENTER:
                x = (pSize.width - width) / 2;
                break;
              case FlowLayout.RIGHT:
                x = pSize.width - this.hgap - width;
                break;
            }
            this.layoutRow(row, x, y, sizes);
            x = this.hgap;
            y += height + this.vgap;
            width = 0;
            height = 0;
            row = [];
            sizes = [];
        }
        row.push(child);
        sizes.push(cSize);
        width += cSize.width + this.hgap;
        if (cSize.height > height) height = cSize.height;
    }
    if (row.length > 0) {
        switch (this.align) {
          case FlowLayout.LEFT:
            x = this.hgap;
            break;
          case FlowLayout.CENTER:
            x = (pSize.width - width) / 2;
            break;
          case FlowLayout.RIGHT:
            x = pSize.width - this.hgap - width;
            break;
        }
        this.layoutRow(row, x, y, sizes);
    }
};

FlowLayout.prototype.layoutRow = function(row, x, y, sizes) {
    for (var i = 0; i < row.length; i++) {
        var child = row[i];
        var size = sizes[i];
        child.setBounds(x, y, size.width, size.height);
        x += size.width + this.hgap;
    }
};

FlowLayout.CENTER = 0;
FlowLayout.LEFT = 1;
FlowLayout.RIGHT = 2;

/*
 * Class: FontMetrics
 * ------------------
 * Simulates the <code>FontMetrics</code> class in JavaScript.
 */

var FontMetrics = function(f) {
    if (!(this instanceof FontMetrics)) return new FontMetrics(f);
    this.font = f;
};

FontMetrics.prototype.$class = new Class("FontMetrics", FontMetrics);

FontMetrics.prototype.getHeight = function() {
    return round(1.15 * this.font.size);
};

FontMetrics.prototype.getAscent = function() {
    return round(0.90 * this.font.size);
};

FontMetrics.prototype.getDescent = function() {
    return round(0.15 * this.font.size);
};

FontMetrics.prototype.stringWidth = function(str) {
    var ctx = FontMetrics.ctx;
    if (!ctx) {
        ctx = document.createElement("canvas").getContext("2d");
        FontMetrics.ctx = ctx;
    }
    ctx.save();
    ctx.font = this.font.getFontTag();
    var metrics = ctx.measureText(str);
    ctx.restore();
    return metrics.width;
};

/*
 * Class: Font
 * -----------
 * Simulates the <code>Font</code> class in JavaScript.
 */

var Font = function(family, style, size) {
    if (!(this instanceof Font)) return new Font(family, style, size);
    if (style === undefined) {
        var font = Font.decode(family);
        this.family = font.family;
        this.size = font.size;
        this.isItalic = font.isItalic;
        this.isBold = font.isBold;
        this.jsFont = font.jsFont;
    } else {
        this.family = family;
        this.size = size;
        this.isItalic = style & Font.ITALIC;
        this.isBold = style & Font.BOLD;
    }
};

Font.prototype.$class = new Class("Font", Font);

Font.prototype.getFamily = function() {
    return this.family;
};

Font.prototype.getSize = function() {
    return this.size;
};

Font.prototype.getFontMetrics = function() {
    if (!this.metrics) this.metrics = new FontMetrics(this);
    return this.metrics;
};

Font.prototype.getFontTag = function() {
    if (this.jsFont) return this.jsFont;
    var str = "";
    if (this.isItalic) str += "italic ";
    if (this.isBold) str += "bold ";
    return str + this.size + "px " + this.getFamilyTag();
};

Font.prototype.getFamilyTag = function() {
    switch (this.family.toLowerCase()) {
      case "system": return "'lucida grande',arial,helvetica,sans-serif";
      case "serif": return "'times new roman',times,serif";
      case "sans-serif":
      case "sansserif": return "'arial',helvetica,sans-serif";
      case "monospaced": return "'courier new',courier,monospaced";
      default:
         return (this.family.indexOf(' ') == -1) ? this.family
                                                 : "'" + this.family + "'";
    }
};

Font.prototype.setStyleProperties = function(element) {
    if (this.jsFont) {
        element.style.font = this.jsFont;
        return;
    }
    var family = this.family;
    switch (family.toLowerCase()) {
      case "system":
        family = "'lucida grande',arial,helvetica,sans-serif";
        break;
      case "serif":
        family = "'times new roman',times,serif";
        break;
      case "sans-serif":
      case "sansserif":
        family = "arial,helvetica,sans-serif";
        break;
      case "monospaced":
        family = "'courier new',courier,monospaced";
        break;
      default:
        if (family.indexOf(" ") !== -1) {
            if (family.indexOf("'") === -1 && family.indexOf("\"") === -1) {
                family = "'" + this.family + "'";
            }
        }
        break;
    }
    var fontStyle = this.isItalic ? "italic" : "normal";
    var fontWeight = this.isBold ? "bold" : "normal";
    var fontSize = this.size + "px";
    element.style.font = fontStyle + " " + fontWeight + " " +
                         fontSize + " " + family;
};

Font.prototype.toString = function() {
    var str = this.family;
    var style = "";
    if (this.isBold) style += "Bold";
    if (this.isItalic) style += "Italic";
    if (style) str += "-" + style;
    return str + "-" + this.size;
};

Font.decode = function(str) {
    var isJavaFont = false;
    var jsFont = undefined;
    var family = "system";
    var style = Font.PLAIN;
    var size = 0;
    var start = 0;
    while (true) {
        var hyphen = str.indexOf("-", start);
        if (hyphen === -1 || hyphen === str.length - 1) break;
        var ch = str.charAt(hyphen + 1);
        if (ch >= "0" && ch <= "9") {
            isJavaFont = true;
            break;
        }
        start = hyphen + 1;
    }
    if (isJavaFont) {
        var h1 = str.indexOf("-");
        var h2 = str.indexOf("-", h1 + 1);
        family = str.substring(0, h1);
        if (h2 === -1) {
            size = parseInt(str.substring(h1 + 1));
        } else {
            var ss = str.substring(h1 + 1, h2).toLowerCase();
            if (ss === "plain") style = Font.PLAIN;
            if (ss === "bold") style = Font.BOLD;
            if (ss === "italic") style = Font.ITALIC;
            if (ss === "bolditalic") style = Font.BOLD + Font.ITALIC;
            size = parseInt(str.substring(h2 + 1));
        }
    } else {
        jsFont = str;
        var fields = str.split(" ");
        for (var i = 0; i < fields.length; i++) {
            var field = fields[i];
            if (field.toLowerCase() === "italic") style += Font.ITALIC;
            if (field.toLowerCase() === "bold") style += Font.BOLD;
            var units = Font.parseJSUnits(field.toLowerCase());
            if (units !== -1) {
                size = units;
                var tail = str.substring(str.indexOf(field) + field.length);
                var family = tail.trim().split(",")[0];
                break;
            }
        }
    }
    var font = new Font(family, style, size);
    font.jsFont = jsFont;
    return font;
};

Font.parseJSUnits = function(str) {
    var ux = str.length;
    while (ux > 0 && str.charCodeAt(ux - 1) > 64) {
        ux--;
    }
    if (ux === 0 || ux === str.length) return -1;
    var value = Number(str.substring(0, ux));
    var units = str.substring(ux);
    switch (units) {
      case "em": return 12 * value;
      case "px": return 0.75 * value;
      default: return value;
    }
};
    
Font.PLAIN = 0;
Font.BOLD = 1;
Font.ITALIC = 2;

/* FocusEvent.js */

/*
 * Class: FocusEvent
 * -----------------
 * Simulates the <code>FocusEvent</code> class in JavaScript.
 */

var FocusEvent = function(source, id) {
    if (!(this instanceof FocusEvent)) return new FocusEvent(source, id);
    this.source = source;
    this.id = id;
}
FocusEvent.prototype.$class = new Class("FocusEvent", FocusEvent);

FocusEvent.prototype.getComponent = function() {
    return this.source;
};

FocusEvent.prototype.getID = function() {
    return this.id;
};

FocusEvent.prototype.dispatch = function() {
    this.source.fireFocusListeners(this);
};

FocusEvent.prototype.toString = function() {
    return "FocusEvent:" + this.id;
};

FocusEvent.FOCUS_GAINED = "focusGained";
FocusEvent.FOCUS_LOST = "focusLost";

/*
 * Class: Graphics
 * ---------------
 * Simulates the <code>Graphics2D</code> class in JavaScript.
 */

var Graphics = function(source) {
    if (!(this instanceof Graphics)) return new Graphics(source);
    this.element = source.element || source;
    if (this.element.getContext) {
        this.ctx = this.element.getContext("2d");
    }
    if (source.font) this.font = source.font;
};

Graphics.prototype.$class = new Class("Graphics", Graphics);

Graphics.prototype.clear = function() {
    this.ctx.clearRect(0, 0, this.element.width, this.element.height);
};

/*
 * Implementation notes: create
 * ----------------------------
 * This method is a hack that supports the common use of create to
 * introduce a temporary local context with new parameters.  The HTML
 * Canvas context doesn't support copying; this design typically
 * achieves the same effect by embedding a save in the create method
 * and a restore in the dispose method.  Note that the original context
 * is therefore unusable while the copy is in effect.
 */

Graphics.prototype.create = function(x, y, width, height) {
    this.ctx.save();
    if (x !== undefined) {
        this.ctx.translate(x, y);
        this.ctx.beginPath();
        this.ctx.moveTo(0, 0);
        this.ctx.lineTo(width, 0);
        this.ctx.lineTo(width, height);
        this.ctx.lineTo(0, height);
        this.ctx.lineTo(0, 0);
        this.ctx.closePath();
        this.ctx.clip();
    }
    return this;
};

Graphics.prototype.dispose = function() {
    this.ctx.restore();
};

Graphics.prototype.draw = function(path) {
    this.ctx.save();
    this.ctx.beginPath();
    path.createPath(this.ctx);
    this.ctx.stroke();
    this.ctx.restore();
};

Graphics.prototype.fill = function(path) {
    this.ctx.save();
    this.ctx.beginPath();
    path.createPath(this.ctx);
    this.ctx.fill();
    this.ctx.restore();
};

Graphics.prototype.drawLine = function(x0, y0, x1, y1) {
    this.ctx.save();
    this.ctx.beginPath();
    this.ctx.moveTo(x0, y0);
    this.ctx.lineTo(x1, y1);
    this.ctx.stroke();
    this.ctx.restore();
};

Graphics.prototype.drawRect = function(x, y, width, height) {
    this.ctx.save();
    this.ctx.strokeRect(x, y, width, height);
    this.ctx.restore();
};

Graphics.prototype.fillRect = function(x, y, width, height) {
    this.ctx.save();
    this.ctx.fillRect(x, y, width, height);
    this.ctx.restore();
};

Graphics.prototype.drawRoundRect = function(x, y, width, height, arcw, arch) {
    // Fix when you get home
    this.drawRect(x, y, width, height);
};

Graphics.prototype.fillRoundRect = function(x, y, width, height, arcw, arch) {
    // Fix when you get home
    this.drawRect(x, y, width, height);
};

Graphics.prototype.drawOval = function(x, y, width, height) {
    this.ctx.save();
    this.ctx.translate(x + width / 2, y + height / 2);
    this.ctx.scale(1, height / width);
    this.ctx.beginPath();
    this.ctx.arc(0, 0, width / 2, 0, Math.PI * 2, true);
    this.ctx.closePath();
    this.ctx.stroke();
    this.ctx.restore();
};

Graphics.prototype.fillOval = function(x, y, width, height) {
    this.ctx.save();
    this.ctx.translate(x + width / 2, y + height / 2);
    this.ctx.scale(1, height / width);
    this.ctx.beginPath();
    this.ctx.arc(0, 0, width / 2, 0, Math.PI * 2, true);
    this.ctx.closePath();
    this.ctx.fill();
    this.ctx.restore();
};

Graphics.prototype.drawArc = function(x, y, width, height, start, sweep) {
    this.ctx.save();
    this.ctx.translate(x + width / 2, y + height / 2);
    this.ctx.scale(1, -height / width);
    this.ctx.beginPath();
    this.ctx.arc(0, 0, width / 2, Math.PI / 180 * start,
                                  Math.PI / 180 * (start + sweep), false);
    this.ctx.stroke();
    this.ctx.restore();
};

Graphics.prototype.fillArc = function(x, y, width, height, start, sweep) {
    this.ctx.save();
    this.ctx.translate(x + width / 2, y + height / 2);
    this.ctx.scale(1, -height / width);
    this.ctx.beginPath();
    this.ctx.moveTo(0, 0);
    this.ctx.arc(0, 0, width / 2, Math.PI / 180 * start,
                                  Math.PI / 180 * (start + sweep), false);
    this.ctx.closePath();
    this.ctx.fill();
    this.ctx.restore();
};

Graphics.prototype.drawString = function(str, x, y) {
    this.ctx.save();
    this.ctx.font = this.font.getFontTag();
    this.ctx.fillText(str, x, y);
    this.ctx.restore();
};

Graphics.prototype.setStroke = function(s) {
    this.stroke = s;
    this.ctx.lineWidth = s.lineWidth;
    this.ctx.lineCap = s.lineCap;
    this.ctx.lineJoin = s.lineJoin;
    this.ctx.miterLimit = s.miterLimit;
    if (this.ctx.setLineDash) {
        this.ctx.setLineDash(s.dashes || []);
        this.ctx.lineDashOffset = s.offset;
    }
};

Graphics.prototype.getStroke = function() {
    return this.stroke;
};

Graphics.prototype.setColor = function(c) {
    this.color = c;
    this.ctx.fillStyle = this.ctx.strokeStyle = c.getColorTag();
};

Graphics.prototype.getColor = function() {
    return this.color;
};

Graphics.prototype.setFont = function(f) {
    this.font = f;
};

Graphics.prototype.getFont = function() {
    return this.font;
};

Graphics.prototype.getFontMetrics = function() {
    return this.font.getFontMetrics();
};

Graphics.prototype.drawImage = function(image, x, y, observer) {
    this.ctx.save();
    if (image.getImage) image = image.getImage();
    if (image.complete) {
        this.ctx.drawImage(image, x, y);
    } else {
        var callback = function() {
            if (observer && observer.repaint) observer.repaint();
            image.onload = null;
        }
        image.onload = callback;
    }
    this.ctx.restore();
};

Graphics.prototype.drawPolygon = function(poly) {
    this.ctx.save();
    this.ctx.beginPath();
    this.ctx.moveTo(poly.xPoints[0], poly.yPoints[0]);
    var n = poly.nPoints;
    for (var i = 1; i <= n; i++) {
        this.ctx.lineTo(poly.xPoints[i % n], poly.yPoints[i % n]);
    }
    this.ctx.closePath();
    this.ctx.stroke();
    this.ctx.restore();
};

Graphics.prototype.fillPolygon = function(poly) {
    this.ctx.save();
    this.ctx.beginPath();
    this.ctx.moveTo(poly.xPoints[0], poly.yPoints[0]);
    var n = poly.nPoints;
    for (var i = 1; i <= n; i++) {
        this.ctx.lineTo(poly.xPoints[i % n], poly.yPoints[i % n]);
    }
    this.ctx.closePath();
    this.ctx.fill();
    this.ctx.restore();
};

Graphics.prototype.translate = function(x, y) {
    this.ctx.translate(x, y);
};

Graphics.prototype.rotate = function(theta) {
    this.ctx.rotate(theta);
};

Graphics.prototype.scale = function(sx, sy) {
    this.ctx.scale(sx, (sy === undefined) ? sx : sy);
};

Graphics.prototype.setRenderingHints = function() {
    /* Ignored */
};

Graphics.prototype.setClip = function(x, y, width, height) {
    this.ctx.beginPath();
    this.ctx.moveTo(x, y);
    this.ctx.lineTo(x + width, y);
    this.ctx.lineTo(x + width, y + height);
    this.ctx.lineTo(x, y + height);
    this.ctx.lineTo(x, y);
    this.ctx.closePath();
    this.ctx.clip();
};

/*
 * Class: InputEvent
 * -----------------
 * Implements the common superclass for <code>MouseEvent</code> and
 * <code>KeyEvent</code> and of a <code>Insets</code> class.
 */

var InputEvent = function(e, source) {
    if (!(this instanceof InputEvent)) return new InputEvent(e, source);
    if (!e) e = window.event;
    this.source = source;
    this.event = e;
    this.modifiers = 0;
    if (e.shiftKey) this.modifiers |= InputEvent.SHIFT_DOWN_MASK;
    if (e.ctrlKey) this.modifiers |= InputEvent.CTRL_DOWN_MASK;
    if (e.altKey) this.modifiers |= InputEvent.ALT_DOWN_MASK;
    if (e.metaKey) this.modifiers |= InputEvent.META_DOWN_MASK;
};

InputEvent.prototype.$class = new Class("InputEvent", InputEvent);

InputEvent.prototype.getID = function() {
    return this.action;
};

InputEvent.prototype.getModifiers = function() {
    return this.modifiers;
};

InputEvent.prototype.isShiftDown = function() {
    return (this.modifiers & InputEvent.SHIFT_DOWN_MASK) != 0;
};

InputEvent.prototype.isControlDown = function() {
    return (this.modifiers & InputEvent.CTRL_DOWN_MASK) != 0;
};

InputEvent.prototype.isAltDown = function() {
    return (this.modifiers & InputEvent.ALT_DOWN_MASK) != 0;
};

InputEvent.prototype.isMetaDown = function() {
    return (this.modifiers & InputEvent.META_DOWN_MASK) != 0;
};

InputEvent.ALT_DOWN_MASK = 512;
InputEvent.ALT_GRAPH_DOWN_MASK = 8192;
InputEvent.ALT_GRAPH_MASK = 32;
InputEvent.ALT_MASK = 8;
InputEvent.BUTTON1_DOWN_MASK = 1024;
InputEvent.BUTTON1_MASK = 16;
InputEvent.BUTTON2_DOWN_MASK = 2048;
InputEvent.BUTTON2_MASK = 8;
InputEvent.BUTTON3_DOWN_MASK = 4096;
InputEvent.BUTTON3_MASK = 4;
InputEvent.CTRL_DOWN_MASK = 128;
InputEvent.CTRL_MASK = 2;
InputEvent.META_DOWN_MASK = 256;
InputEvent.META_MASK = 4;
InputEvent.SHIFT_DOWN_MASK = 64;
InputEvent.SHIFT_MASK = 1;

/*
 * Class: Insets
 * -------------
 * Simulates the Java <code>Insets</code> class.
 */

var Insets = function(top, left, bottom, right) {
    if (!(this instanceof Insets)) {
        return new Insets(top, left, bottom, right);
    }
    this.top = top;
    this.left = left;
    this.bottom = bottom;
    this.right = right;
};

Insets.prototype.$class = new Class("Insets", Insets);

Insets.prototype.toString = function() {
    return "{top:" + top + ", left:" + left + ", bottom:" + bottom +
                           ", right:" + right + "}";
};

/*
 * Class: KeyEvent
 * ---------------
 * Simulates the <code>KeyEvent</code> class in JavaScript.
 */

var KeyEvent = function(e, source) {
    if (!(this instanceof KeyEvent)) return new KeyEvent(e, source);
    InputEvent.call(this, e, source);
    if (!e) e = window.event;
    switch (e.type) {
      case "keydown":
        this.action = KeyEvent.KEY_PRESSED;
        this.keyCode = e.keyCode;
        this.keyChar = 0;
        break;
      case "keyup":
        this.action = KeyEvent.KEY_RELEASED;
        this.keyCode = e.keyCode;
        this.keyChar = 0;
        break;
      case "keypress":
        this.action = KeyEvent.KEY_TYPED;
        this.keyCode = e.keyCode;
        this.keyChar = e.charCode || e.keyCode;
        break;
    }
};

KeyEvent.prototype =
    jslib.inheritPrototype(InputEvent, "KeyEvent extends InputEvent");
KeyEvent.prototype.constructor = KeyEvent;
KeyEvent.prototype.$class = new Class("KeyEvent", KeyEvent);

KeyEvent.KEY_TYPED = "keyTyped";
KeyEvent.KEY_PRESSED = "keyPressed";
KeyEvent.KEY_RELEASED = "keyReleased";

KeyEvent.ALT_DOWN_MASK = InputEvent.ALT_DOWN_MASK;
KeyEvent.ALT_GRAPH_DOWN_MASK = InputEvent.ALT_GRAPH_DOWN_MASK;
KeyEvent.ALT_GRAPH_MASK = InputEvent.ALT_GRAPH_MASK;
KeyEvent.ALT_MASK = InputEvent.ALT_MASK;
KeyEvent.BUTTON1_DOWN_MASK = InputEvent.BUTTON1_DOWN_MASK;
KeyEvent.BUTTON1_MASK = InputEvent.BUTTON1_MASK;
KeyEvent.BUTTON2_DOWN_MASK = InputEvent.BUTTON2_DOWN_MASK;
KeyEvent.BUTTON2_MASK = InputEvent.BUTTON2_MASK;
KeyEvent.BUTTON3_DOWN_MASK = InputEvent.BUTTON3_DOWN_MASK;
KeyEvent.BUTTON3_MASK = InputEvent.BUTTON3_MASK;
KeyEvent.CTRL_DOWN_MASK = InputEvent.CTRL_DOWN_MASK;
KeyEvent.CTRL_MASK = InputEvent.CTRL_MASK;
KeyEvent.META_DOWN_MASK = InputEvent.META_DOWN_MASK;
KeyEvent.META_MASK = InputEvent.META_MASK;
KeyEvent.SHIFT_DOWN_MASK = InputEvent.SHIFT_DOWN_MASK;
KeyEvent.SHIFT_MASK = InputEvent.SHIFT_MASK;

KeyEvent.VK_ALT = 18;
KeyEvent.VK_ALT_GRAPH = 65406;
KeyEvent.VK_BACK_SPACE = 8;
KeyEvent.VK_CLEAR = 12;
KeyEvent.VK_COPY = 65485;
KeyEvent.VK_CUT = 65489;
KeyEvent.VK_DELETE = 127;
KeyEvent.VK_DOWN = 40;
KeyEvent.VK_END = 35;
KeyEvent.VK_ESCAPE = 27;
KeyEvent.VK_F1 = 112;
KeyEvent.VK_F2 = 113;
KeyEvent.VK_F3 = 114;
KeyEvent.VK_F4 = 115;
KeyEvent.VK_F5 = 116;
KeyEvent.VK_F6 = 117;
KeyEvent.VK_F7 = 118;
KeyEvent.VK_F8 = 119;
KeyEvent.VK_F9 = 120;
KeyEvent.VK_F10 = 121;
KeyEvent.VK_F11 = 122;
KeyEvent.VK_F12 = 123;
KeyEvent.VK_FIND = 65488;
KeyEvent.VK_INSERT = 155;
KeyEvent.VK_LEFT = 37;
KeyEvent.VK_META = 157;
KeyEvent.VK_NUM_LOCK = 144;
KeyEvent.VK_PAGE_DOWN = 34;
KeyEvent.VK_PAGE_UP = 33;
KeyEvent.VK_PASTE = 65487;
KeyEvent.VK_RIGHT = 39;
KeyEvent.VK_TAB = 9;
KeyEvent.VK_UP = 38;

KeyEvent.prototype.dispatch = function() {
    this.source.fireKeyListeners(this);
};

KeyEvent.prototype.getSource = function() {
    return this.source;
};

KeyEvent.prototype.getKeyCode = function() {
    return this.keyCode;
};

KeyEvent.prototype.getKeyChar = function() {
    return this.keyChar;
};

KeyEvent.prototype.consume = function() {
    if (this.event.stopPropagation) this.event.stopPropagation();
    if (this.event.preventDefault) this.event.preventDefault();
};

/*
 * Class: MouseEvent
 * -----------------
 * Simulates the <code>MouseEvent</code> class in JavaScript.
 */

var MouseEvent = function(e, source) {
    if (!(this instanceof MouseEvent)) return new MouseEvent(e, source);
    InputEvent.call(this, e, source);
    if (!e) e = window.event;
    switch (e.type) {
      case "mousedown": this.action = MouseEvent.MOUSE_PRESSED; break;
      case "mouseup": this.action = MouseEvent.MOUSE_RELEASED; break;
      case "mouseout": this.action = MouseEvent.MOUSE_EXITED; break;
      case "mouseover": this.action = MouseEvent.MOUSE_ENTERED; break;
      case "mousemove": this.action = MouseEvent.MOUSE_MOVED; break;
      case "click":
        this.action = MouseEvent.MOUSE_CLICKED;
        this.clickCount = 1;
        break;
      case "dblclick":
        this.action = MouseEvent.MOUSE_CLICKED;
        this.clickCount = 2;
        break;
    }
    this.setPoint(e);
};

MouseEvent.prototype =
    jslib.inheritPrototype(InputEvent, "MouseEvent extends InputEvent");
MouseEvent.prototype.constructor = MouseEvent;
MouseEvent.prototype.$class = new Class("MouseEvent", MouseEvent);

MouseEvent.MOUSE_CLICKED = "mouseClicked";
MouseEvent.MOUSE_PRESSED = "mousePressed";
MouseEvent.MOUSE_RELEASED = "mouseReleased";
MouseEvent.MOUSE_DRAGGED = "mouseDragged";
MouseEvent.MOUSE_MOVED = "mouseMoved";
MouseEvent.MOUSE_ENTERED = "mouseEntered";
MouseEvent.MOUSE_EXITED = "mouseExited";

MouseEvent.ALT_DOWN_MASK = InputEvent.ALT_DOWN_MASK;
MouseEvent.ALT_GRAPH_DOWN_MASK = InputEvent.ALT_GRAPH_DOWN_MASK;
MouseEvent.ALT_GRAPH_MASK = InputEvent.ALT_GRAPH_MASK;
MouseEvent.ALT_MASK = InputEvent.ALT_MASK;
MouseEvent.BUTTON1_DOWN_MASK = InputEvent.BUTTON1_DOWN_MASK;
MouseEvent.BUTTON1_MASK = InputEvent.BUTTON1_MASK;
MouseEvent.BUTTON2_DOWN_MASK = InputEvent.BUTTON2_DOWN_MASK;
MouseEvent.BUTTON2_MASK = InputEvent.BUTTON2_MASK;
MouseEvent.BUTTON3_DOWN_MASK = InputEvent.BUTTON3_DOWN_MASK;
MouseEvent.BUTTON3_MASK = InputEvent.BUTTON3_MASK;
MouseEvent.CTRL_DOWN_MASK = InputEvent.CTRL_DOWN_MASK;
MouseEvent.CTRL_MASK = InputEvent.CTRL_MASK;
MouseEvent.META_DOWN_MASK = InputEvent.META_DOWN_MASK;
MouseEvent.META_MASK = InputEvent.META_MASK;
MouseEvent.SHIFT_DOWN_MASK = InputEvent.SHIFT_DOWN_MASK;
MouseEvent.SHIFT_MASK = InputEvent.SHIFT_MASK;

MouseEvent.prototype.getComponent = function() {
    return this.source;
};

MouseEvent.prototype.getPoint = function() {
    return new Point(this.x, this.y);
};

MouseEvent.prototype.getX = function() {
    return this.x;
};

MouseEvent.prototype.getY = function() {
    return this.y;
};

MouseEvent.prototype.getClickCount = function() {
    return this.clickCount;
};

MouseEvent.prototype.dispatch = function() {
    this.source.fireMouseListeners(this);
};

MouseEvent.prototype.consume = function() {
    this.event.stopPropagation();
}

MouseEvent.prototype.setPoint = function(e) {
    var px = 0;
    var py = 0;
    if (e.pageX) {
        px = e.pageX;
        py = e.pageY;
    } else if (e.clientX) {
        px = e.clientX + document.body.scrollLeft +
                         document.documentElement.scrollLeft;
        py = e.clientY + document.body.scrollTop +
                         document.documentElement.scrollTop;
    }
    var obj = this.source.element;
    var cx = 0;
    var cy = 0;
    while (obj) {
        cx += obj.offsetLeft;
        cy += obj.offsetTop;
        obj = obj.offsetParent;
    }
    this.x = px - cx;
    this.y = py - cy;
};

MouseEvent.prototype.toString = function() {
    return "MouseEvent:" + this.action + "@(" + this.x + "," + this.y + ")";
};

/*
 * Class: Path2D
 * -------------
 * Simulates the <code>Path2D</code> class in JavaScript.
 */

var Path2D = function() {
    if (!(this instanceof Path2D)) return new Path2D();
    this.segments = [];
};

Path2D.prototype.$class = new Class("Path2D", Path2D);

Path2D.Double = function() {
    Path2D.call(this);
};

Path2D.Double.prototype =
    jslib.inheritPrototype(Path2D, "Path2D extends Path2D");
Path2D.Double.prototype = Path2D.Double;

Path2D.prototype.moveTo = function(x, y) {
    this.segments.push(new MoveToElement(x, y));
};

Path2D.prototype.lineTo = function(x, y) {
    this.segments.push(new LineToElement(x, y));
};

Path2D.prototype.curveTo = function(x1, y1, x2, y2, x3, y3) {
    this.segments.push(new CurveToElement(x1, y1, x2, y2, x3, y3));
};

Path2D.prototype.createPath = function(ctx) {
    var n = this.segments.length;
    for (var i = 0; i < n; i++) {
        this.segments[i].append(ctx);
    }
};

var MoveToElement = function(x, y) {
    if (!(this instanceof MoveToElement)) return new MoveToElement(x, y);
    this.x = x;
    this.y = y;
};

MoveToElement.prototype.append = function(ctx) {
    ctx.moveTo(this.x, this.y);
};

var LineToElement = function(x, y) {
    if (!(this instanceof LineToElement)) return new LineToElement(x, y);
    this.x = x;
    this.y = y;
};

LineToElement.prototype.append = function(ctx) {
    ctx.lineTo(this.x, this.y);
};

var CurveToElement = function(x1, y1, x2, y2, x3, y3) {
    if (!(this instanceof CurveToElement)) {
        return new CurveToElement(x1, y1, x2, y2, x3, y3);
    }
    this.x1 = x1;
    this.y1 = y1;
    this.x2 = x2;
    this.y2 = y2;
    this.x3 = x3;
    this.y3 = y3;
};

CurveToElement.prototype.append = function(ctx) {
    ctx.bezierCurveTo(this.x1, this.y1, this.x2, this.y2, this.x3, this.y3);
};

/*
 * Class: Point
 * ------------
 * Simulates the <code>Point</code> and <code>Point2D</code> classes
 * in JavaScript.
 */

var Point = function(x, y) {
    if (!(this instanceof Point)) return new Point(x, y);
    this.x = x;
    this.y = y;
};

Point.prototype.$class = new Class("Point", Point);

Point.prototype.getX = function() {
    return this.x;
};

Point.prototype.getY = function() {
    return this.y;
};

Point.prototype.toString = function() {
    return "(" + this.x + "," + this.y + ")";
};

Point.getElementLocation = function(e) {
    var obj = e;
    var cx = 0;
    var cy = 0;
    while (obj) {
        cx += obj.offsetLeft;
        cy += obj.offsetTop;
        // Parent here is null
        obj = obj.offsetParent;
    }
    return new Point(cx, cy);
};

/*
 * Class: Polygon
 * --------------
 * Simulates the <code>Polygon</code> and <code>Polygon2D</code> classes
 * in JavaScript.
 */

var Polygon = function(xPoints, yPoints, nPoints) {
    if (!(this instanceof Polygon)) {
        return new Polygon(xPoints, yPoints, nPoints);
    }
    if (xPoints === undefined) {
        this.xPoints = [ ];
        this.yPoints = [ ];
        this.nPoints = 0;
    } else {
        this.xPoints = xPoints;
        this.yPoints = yPoints;
        this.nPoints = xPoints.length;
    }
};

Polygon.prototype.$class = new Class("Polygon", Polygon);

Polygon.prototype.addPoint = function(x, y) {
    this.xPoints.push(x);
    this.yPoints.push(y);
    this.nPoints++;
};

/*
 * Class: Rectangle
 * ----------------
 * Simulates the <code>Rectangle</code> class in JavaScript.
 */

var Rectangle = function(x, y, width, height) {
    if (!(this instanceof Rectangle)) {
        return new Rectangle(x, y, width, height);
    }
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
};

Rectangle.prototype.$class = new Class("Rectangle", Rectangle);

Rectangle.prototype.getX = function() {
    return this.x;
};

Rectangle.prototype.getY = function() {
    return this.y;
};

Rectangle.prototype.getWidth = function() {
    return this.width;
};

Rectangle.prototype.getHeight = function() {
    return this.height;
};

Rectangle.prototype.contains = function(x, y) {
    if (typeof(x) === "object") {
        y = x.y;
        x = x.x;
    }
    if (x < this.x || x > this.x + this.width) return false;
    if (y < this.y || y > this.y + this.height) return false;
    return true;
};

Rectangle.prototype.isEmpty = function() {
    return (width <= 0 || height <= 0);
};

Rectangle.prototype.toString = function() {
    return "(" + this.width + "x" + this.height + "@" +
                 this.x + "," + this.y + ")";
};

/*
 * Class: Toolkit
 * --------------
 * Most of this class is unimplemented.
 */

var Toolkit = function() {
    /* Empty */
};

Toolkit.prototype.$class = new Class("Toolkit", Toolkit);

Toolkit.defaultToolkit = new Toolkit();

Toolkit.prototype.beep = function() {
    alert("beep");
};

Toolkit.getDefaultToolkit = function() {
    return Toolkit.defaultToolkit;
};

/* Define the package */

return {
    ActionEvent : ActionEvent,
    Adjustable : Adjustable,
    AdjustmentEvent : AdjustmentEvent,
    AffineTransform : AffineTransform,
    BasicStroke : BasicStroke,
    BorderLayout : BorderLayout,
    CardLayout : CardLayout,
    Color : Color,
    ComponentEvent : ComponentEvent,
    Dimension : Dimension,
    FlowLayout : FlowLayout,
    FocusEvent : FocusEvent,
    Font : Font,
    FontMetrics : FontMetrics,
    Graphics : Graphics,
    InputEvent : InputEvent,
    Insets : Insets,
    KeyEvent : KeyEvent,
    MouseEvent : MouseEvent,
    Path2D : Path2D,
    Point : Point,
    Polygon : Polygon,
    Rectangle : Rectangle,
    Toolkit : Toolkit
};

/* Trailer for require.js */

});
