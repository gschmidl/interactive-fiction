/*
 * File: svmops.js
 * Created on Thu May 27 16:19:48 PDT 2021 by java2js
 * --------------------------------------------------
 * This file was generated mechanically by the java2js utility.
 * Permanent edits must be made in the Java file.
 */

define([ "jslib" ],

function(jslib) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;

/* SVMOp.js */

var SVMOp = function() {
    /* Empty */
};

SVMOp.SVM_VERSION = 5;
SVMOp.END =      0x00;     /* END                 */
SVMOp.VERSION =  0x01;     /* VERSION n           */
SVMOp.PSTACK =   0x02;     /* PSTACK              */
SVMOp.STMT =     0x03;     /* STMT index          */
SVMOp.HALT =     0x04;     /* HALT                */
SVMOp.NOP =      0x05;     /* NOP                 */
SVMOp.TRACE =    0x06;     /* TRACE {0,1}         */
SVMOp.PUSHINT =  0x10;     /* PUSHINT int         */
SVMOp.PUSHNUM =  0x11;     /* PUSHNUM num         */
SVMOp.PUSHCH =   0x12;     /* PUSHCH ch           */
SVMOp.PUSHSTR =  0x13;     /* PUSHSTR str         */
SVMOp.PUSHFN =   0x14;     /* PUSHFN addr         */
SVMOp.POP =      0x15;     /* POP                 */
SVMOp.DUP =      0x16;     /* DUP                 */
SVMOp.EXCH =     0x17;     /* EXCH                */
SVMOp.ROLL =     0x18;     /* ROLL n              */
SVMOp.COPY =     0x19;     /* COPY n              */
SVMOp.ADD =      0x20;     /* ADD                 */
SVMOp.SUB =      0x21;     /* SUB                 */
SVMOp.MUL =      0x22;     /* MUL                 */
SVMOp.DIV =      0x23;     /* DIV                 */
SVMOp.IDIV =     0x24;     /* DIV                 */
SVMOp.REM =      0x25;     /* REM                 */
SVMOp.NEG =      0x26;     /* NEG                 */
SVMOp.EQ =       0x30;     /* EQ                  */
SVMOp.NE =       0x31;     /* NE                  */
SVMOp.LT =       0x32;     /* LT                  */
SVMOp.LE =       0x33;     /* LE                  */
SVMOp.GT =       0x34;     /* GT                  */
SVMOp.GE =       0x35;     /* GE                  */
SVMOp.JUMP =     0x40;     /* JUMP addr           */
SVMOp.JUMPT =    0x41;     /* JUMPT addr          */
SVMOp.JUMPF =    0x42;     /* JUMPF addr          */
SVMOp.DISPATCH = 0x43;     /* DISPATCH            */
SVMOp.TRY =      0x44;     /* TRY addr            */
SVMOp.ENDTRY =   0x45;     /* ENDTRY              */
SVMOp.THROW =    0x46;     /* THROW               */
SVMOp.NOT =      0x50;     /* NOT                 */
SVMOp.AND =      0x51;     /* AND                 */
SVMOp.OR =       0x52;     /* OR                  */
SVMOp.XOR =      0x53;     /* XOR                 */
SVMOp.LSH =      0x54;     /* LSH                 */
SVMOp.ASH =      0x55;     /* ASH                 */
SVMOp.CALL =     0x60;     /* CALL addr           */
SVMOp.CALLM =    0x61;     /* CALLM name          */
SVMOp.CALLFN =   0x62;     /* CALLFN              */
SVMOp.RETURN =   0x63;     /* RETURN              */
SVMOp.LOCALS =   0x64;     /* LOCALS vars         */
SVMOp.PUSHLOC =  0x65;     /* PUSHLOC n           */
SVMOp.POPLOC =   0x66;     /* POPLOC n            */
SVMOp.ARG =      0x67;     /* ARG name            */
SVMOp.VAR =      0x68;     /* VAR name            */
SVMOp.PARAMS =   0x69;     /* PARAMS n            */
SVMOp.NARGS =    0x6A;     /* NARGS n             */
SVMOp.VARGS =    0x6B;     /* VARGS               */
SVMOp.PUSHVAR =  0x6C;     /* PUSHVAR name        */
SVMOp.POPVAR =   0x6D;     /* POPVAR name         */
SVMOp.PUSHFRM =  0x6E;     /* PUSHFRM             */
SVMOp.POPFRM =   0x6F;     /* POPFRM              */

/* Exports */

return {
    SVMOp : SVMOp
};

});
