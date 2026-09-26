/*
 * File: csslib.js
 * ---------------
 * Implements the csslib package on the JavaScript side.
 */

/* Standard header for requirejs */

define([ "jslib",
         "java/awt" ],

function(jslib,
         java_awt) {

/* Imports */

var Color = java_awt.Color;
var Font = java_awt.Font;

var CSSColor = function() {
    /* Empty */
};

CSSColor.decode = function(name) {
    if (jslib.startsWith(name, "#")) {
        return new Color(parseInt(name.substring(1), 16));
    } else if (jslib.startsWith(name, "0x")) {
        return new Color(parseInt(name.substring(2), 16));
    } else {
        if (CSSColor.jsColors === null) CSSColor.initCSSColors();
        var color = CSSColor.jsColors[name.toLowerCase()];
        if (color === undefined) {
            throw new RuntimeException("No color named " + name);
        }
        return color;
    }
};

CSSColor.initCSSColors = function() {
    CSSColor.jsColors = { };
    CSSColor.jsColors["aliceblue"] = new Color(0xF0F8FF);
    CSSColor.jsColors["antiquewhite"] = new Color(0xFAEBD7);
    CSSColor.jsColors["aqua"] = new Color(0x00FFFF);
    CSSColor.jsColors["aquamarine"] = new Color(0x7FFFD4);
    CSSColor.jsColors["azure"] = new Color(0xF0FFFF);
    CSSColor.jsColors["beige"] = new Color(0xF5F5DC);
    CSSColor.jsColors["bisque"] = new Color(0xFFE4C4);
    CSSColor.jsColors["black"] = new Color(0x000000);
    CSSColor.jsColors["blanchedalmond"] = new Color(0xFFEBCD);
    CSSColor.jsColors["blue"] = new Color(0x0000FF);
    CSSColor.jsColors["blueviolet"] = new Color(0x8A2BE2);
    CSSColor.jsColors["brown"] = new Color(0xA52A2A);
    CSSColor.jsColors["burlywood"] = new Color(0xDEB887);
    CSSColor.jsColors["cadetblue"] = new Color(0x5F9EA0);
    CSSColor.jsColors["chartreuse"] = new Color(0x7FFF00);
    CSSColor.jsColors["chocolate"] = new Color(0xD2691E);
    CSSColor.jsColors["coral"] = new Color(0xFF7F50);
    CSSColor.jsColors["cornflowerblue"] = new Color(0x6495ED);
    CSSColor.jsColors["cornsilk"] = new Color(0xFFF8DC);
    CSSColor.jsColors["crimson"] = new Color(0xDC143C);
    CSSColor.jsColors["cyan"] = new Color(0x00FFFF);
    CSSColor.jsColors["darkblue"] = new Color(0x00008B);
    CSSColor.jsColors["darkcyan"] = new Color(0x008B8B);
    CSSColor.jsColors["darkgoldenrod"] = new Color(0xB8860B);
    CSSColor.jsColors["darkgray"] = new Color(0xA9A9A9);
    CSSColor.jsColors["darkgrey"] = new Color(0xA9A9A9);
    CSSColor.jsColors["darkgreen"] = new Color(0x006400);
    CSSColor.jsColors["darkkhaki"] = new Color(0xBDB76B);
    CSSColor.jsColors["darkmagenta"] = new Color(0x8B008B);
    CSSColor.jsColors["darkolivegreen"] = new Color(0x556B2F);
    CSSColor.jsColors["darkorange"] = new Color(0xFF8C00);
    CSSColor.jsColors["darkorchid"] = new Color(0x9932CC);
    CSSColor.jsColors["darkred"] = new Color(0x8B0000);
    CSSColor.jsColors["darksalmon"] = new Color(0xE9967A);
    CSSColor.jsColors["darkseagreen"] = new Color(0x8FBC8F);
    CSSColor.jsColors["darkslateblue"] = new Color(0x483D8B);
    CSSColor.jsColors["darkslategray"] = new Color(0x2F4F4F);
    CSSColor.jsColors["darkslategrey"] = new Color(0x2F4F4F);
    CSSColor.jsColors["darkturquoise"] = new Color(0x00CED1);
    CSSColor.jsColors["darkviolet"] = new Color(0x9400D3);
    CSSColor.jsColors["deeppink"] = new Color(0xFF1493);
    CSSColor.jsColors["deepskyblue"] = new Color(0x00BFFF);
    CSSColor.jsColors["dimgray"] = new Color(0x696969);
    CSSColor.jsColors["dimgrey"] = new Color(0x696969);
    CSSColor.jsColors["dodgerblue"] = new Color(0x1E90FF);
    CSSColor.jsColors["firebrick"] = new Color(0xB22222);
    CSSColor.jsColors["floralwhite"] = new Color(0xFFFAF0);
    CSSColor.jsColors["forestgreen"] = new Color(0x228B22);
    CSSColor.jsColors["fuchsia"] = new Color(0xFF00FF);
    CSSColor.jsColors["gainsboro"] = new Color(0xDCDCDC);
    CSSColor.jsColors["ghostwhite"] = new Color(0xF8F8FF);
    CSSColor.jsColors["gold"] = new Color(0xFFD700);
    CSSColor.jsColors["goldenrod"] = new Color(0xDAA520);
    CSSColor.jsColors["gray"] = new Color(0x808080);
    CSSColor.jsColors["grey"] = new Color(0x808080);
    CSSColor.jsColors["green"] = new Color(0x008000);
    CSSColor.jsColors["greenyellow"] = new Color(0xADFF2F);
    CSSColor.jsColors["honeydew"] = new Color(0xF0FFF0);
    CSSColor.jsColors["hotpink"] = new Color(0xFF69B4);
    CSSColor.jsColors["indianred"] = new Color(0xCD5C5C);
    CSSColor.jsColors["indigo"] = new Color(0x4B0082);
    CSSColor.jsColors["ivory"] = new Color(0xFFFFF0);
    CSSColor.jsColors["khaki"] = new Color(0xF0E68C);
    CSSColor.jsColors["lavender"] = new Color(0xE6E6FA);
    CSSColor.jsColors["lavenderblush"] = new Color(0xFFF0F5);
    CSSColor.jsColors["lawngreen"] = new Color(0x7CFC00);
    CSSColor.jsColors["lemonchiffon"] = new Color(0xFFFACD);
    CSSColor.jsColors["lightblue"] = new Color(0xADD8E6);
    CSSColor.jsColors["lightcoral"] = new Color(0xF08080);
    CSSColor.jsColors["lightcyan"] = new Color(0xE0FFFF);
    CSSColor.jsColors["lightgoldenrodyellow"] = new Color(0xFAFAD2);
    CSSColor.jsColors["lightgray"] = new Color(0xD3D3D3);
    CSSColor.jsColors["lightgrey"] = new Color(0xD3D3D3);
    CSSColor.jsColors["lightgreen"] = new Color(0x90EE90);
    CSSColor.jsColors["lightpink"] = new Color(0xFFB6C1);
    CSSColor.jsColors["lightsalmon"] = new Color(0xFFA07A);
    CSSColor.jsColors["lightseagreen"] = new Color(0x20B2AA);
    CSSColor.jsColors["lightskyblue"] = new Color(0x87CEFA);
    CSSColor.jsColors["lightslategray"] = new Color(0x778899);
    CSSColor.jsColors["lightslategrey"] = new Color(0x778899);
    CSSColor.jsColors["lightsteelblue"] = new Color(0xB0C4DE);
    CSSColor.jsColors["lightyellow"] = new Color(0xFFFFE0);
    CSSColor.jsColors["lime"] = new Color(0x00FF00);
    CSSColor.jsColors["limegreen"] = new Color(0x32CD32);
    CSSColor.jsColors["linen"] = new Color(0xFAF0E6);
    CSSColor.jsColors["magenta"] = new Color(0xFF00FF);
    CSSColor.jsColors["maroon"] = new Color(0x800000);
    CSSColor.jsColors["mediumaquamarine"] = new Color(0x66CDAA);
    CSSColor.jsColors["mediumblue"] = new Color(0x0000CD);
    CSSColor.jsColors["mediumorchid"] = new Color(0xBA55D3);
    CSSColor.jsColors["mediumpurple"] = new Color(0x9370DB);
    CSSColor.jsColors["mediumseagreen"] = new Color(0x3CB371);
    CSSColor.jsColors["mediumslateblue"] = new Color(0x7B68EE);
    CSSColor.jsColors["mediumspringgreen"] = new Color(0x00FA9A);
    CSSColor.jsColors["mediumturquoise"] = new Color(0x48D1CC);
    CSSColor.jsColors["mediumvioletred"] = new Color(0xC71585);
    CSSColor.jsColors["midnightblue"] = new Color(0x191970);
    CSSColor.jsColors["mintcream"] = new Color(0xF5FFFA);
    CSSColor.jsColors["mistyrose"] = new Color(0xFFE4E1);
    CSSColor.jsColors["moccasin"] = new Color(0xFFE4B5);
    CSSColor.jsColors["navajowhite"] = new Color(0xFFDEAD);
    CSSColor.jsColors["navy"] = new Color(0x000080);
    CSSColor.jsColors["oldlace"] = new Color(0xFDF5E6);
    CSSColor.jsColors["olive"] = new Color(0x808000);
    CSSColor.jsColors["olivedrab"] = new Color(0x6B8E23);
    CSSColor.jsColors["orange"] = new Color(0xFFA500);
    CSSColor.jsColors["orangered"] = new Color(0xFF4500);
    CSSColor.jsColors["orchid"] = new Color(0xDA70D6);
    CSSColor.jsColors["palegoldenrod"] = new Color(0xEEE8AA);
    CSSColor.jsColors["palegreen"] = new Color(0x98FB98);
    CSSColor.jsColors["paleturquoise"] = new Color(0xAFEEEE);
    CSSColor.jsColors["palevioletred"] = new Color(0xDB7093);
    CSSColor.jsColors["papayawhip"] = new Color(0xFFEFD5);
    CSSColor.jsColors["peachpuff"] = new Color(0xFFDAB9);
    CSSColor.jsColors["peru"] = new Color(0xCD853F);
    CSSColor.jsColors["pink"] = new Color(0xFFC0CB);
    CSSColor.jsColors["plum"] = new Color(0xDDA0DD);
    CSSColor.jsColors["powderblue"] = new Color(0xB0E0E6);
    CSSColor.jsColors["purple"] = new Color(0x800080);
    CSSColor.jsColors["rebeccapurple"] = new Color(0x663399);
    CSSColor.jsColors["red"] = new Color(0xFF0000);
    CSSColor.jsColors["rosybrown"] = new Color(0xBC8F8F);
    CSSColor.jsColors["royalblue"] = new Color(0x4169E1);
    CSSColor.jsColors["saddlebrown"] = new Color(0x8B4513);
    CSSColor.jsColors["salmon"] = new Color(0xFA8072);
    CSSColor.jsColors["sandybrown"] = new Color(0xF4A460);
    CSSColor.jsColors["seagreen"] = new Color(0x2E8B57);
    CSSColor.jsColors["seashell"] = new Color(0xFFF5EE);
    CSSColor.jsColors["sienna"] = new Color(0xA0522D);
    CSSColor.jsColors["silver"] = new Color(0xC0C0C0);
    CSSColor.jsColors["skyblue"] = new Color(0x87CEEB);
    CSSColor.jsColors["slateblue"] = new Color(0x6A5ACD);
    CSSColor.jsColors["slategray"] = new Color(0x708090);
    CSSColor.jsColors["slategrey"] = new Color(0x708090);
    CSSColor.jsColors["snow"] = new Color(0xFFFAFA);
    CSSColor.jsColors["springgreen"] = new Color(0x00FF7F);
    CSSColor.jsColors["steelblue"] = new Color(0x4682B4);
    CSSColor.jsColors["tan"] = new Color(0xD2B48C);
    CSSColor.jsColors["teal"] = new Color(0x008080);
    CSSColor.jsColors["thistle"] = new Color(0xD8BFD8);
    CSSColor.jsColors["tomato"] = new Color(0xFF6347);
    CSSColor.jsColors["turquoise"] = new Color(0x40E0D0);
    CSSColor.jsColors["violet"] = new Color(0xEE82EE);
    CSSColor.jsColors["wheat"] = new Color(0xF5DEB3);
    CSSColor.jsColors["white"] = new Color(0xFFFFFF);
    CSSColor.jsColors["whitesmoke"] = new Color(0xF5F5F5);
    CSSColor.jsColors["yellow"] = new Color(0xFFFF00);
    CSSColor.jsColors["yellowgreen"] = new Color(0x9ACD32);
};

CSSColor.jsColors = null;

/* CSSFont */

var CSSFont = function() {
    /* Empty */
};

CSSFont.decode = function(str) {
    return Font(str);
};

/* Exports */

return {
    CSSColor : CSSColor,
    CSSFont : CSSFont
};

});
