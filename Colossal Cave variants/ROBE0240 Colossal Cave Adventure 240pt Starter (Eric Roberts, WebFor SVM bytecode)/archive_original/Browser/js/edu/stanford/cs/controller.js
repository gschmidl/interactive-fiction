/*
 * File: controller.js
 * Created on Thu May 27 16:19:46 PDT 2021 by java2js
 * --------------------------------------------------
 * This file was generated mechanically by the java2js utility.
 * Permanent edits must be made in the Java file.
 */

define([ "jslib",
         "edu/stanford/cs/java2js",
         "java/awt",
         "java/lang",
         "java/util",
         "javax/swing" ],

function(jslib,
         edu_stanford_cs_java2js,
         java_awt,
         java_lang,
         java_util,
         javax_swing) {

/* Imports */

var inheritPrototype = jslib.inheritPrototype;
var newArray = jslib.newArray;
var toInt = jslib.toInt;
var toStr = jslib.toStr;
var JSElementList = edu_stanford_cs_java2js.JSElementList;
var Adjustable = java_awt.Adjustable;
var ActionEvent = java_awt.ActionEvent;
var ActionListener = java_awt.ActionListener;
var AdjustmentEvent = java_awt.AdjustmentEvent;
var AdjustmentListener = java_awt.AdjustmentListener;
var Class = java_lang.Class;
var RuntimeException = java_lang.RuntimeException;
var ArrayList = java_util.ArrayList;
var HashMap = java_util.HashMap;
var SwingUtilities = javax_swing.SwingUtilities;
var Timer = javax_swing.Timer;
var ChangeEvent = javax_swing.ChangeEvent;
var ChangeListener = javax_swing.ChangeListener;

/* Controller.js */

var Controller = function() {
    if (!(this instanceof Controller)) return new Controller();
    this.listeners = new ArrayList();
    this.controlTable = new HashMap();
    this.errorHandler = null;
    this.speedControl = null;
    this.target = null;
    this.timer = null;
    this.controllerState = Controller.INITIAL;
    this.deferExecution = true;
    this.failOnErrors = false;
};

Controller.prototype.setTarget = function(obj) {
    this.target = obj;
    this.timerListener = this.createTimerListener();
    this.timer = new Timer(0, this.timerListener);
    this.timer.setRepeats(false);
    this.speedListener = new SpeedListener(this);
    this.setSpeed(50);
};

Controller.prototype.getTarget = function() {
    return this.target;
};

Controller.prototype.addControl = function(control) {
    this.controlTable.put(control.getName(), control);
    control.setController(this);
    this.addChangeListener(control);
    if (jslib.equals(control.getName(), "Speed")) {
        this.speedControl = control;
        this.speedControl.addAdjustmentListener(this.speedListener);
        this.setSpeed(this.speedControl.getValue());
    }
};

Controller.prototype.getControl = function(name) {
    return this.controlTable.get(name);
};

Controller.prototype.setControllerState = function(state) {
    this.controllerState = state;
    this.fireChangeListeners();
};

Controller.prototype.getControllerState = function() {
    return this.controllerState;
};

Controller.prototype.setDeferredExecution = function(flag) {
    this.deferExecution = flag;
};

Controller.prototype.isDeferredExecution = function() {
    return this.deferExecution;
};

Controller.prototype.setFailOnErrors = function(flag) {
    this.failOnErrors = flag;
};

Controller.prototype.getFailOnErrors = function() {
    return this.failOnErrors;
};

Controller.getStateName = function(state) {
    switch (state) {
        case Controller.INITIAL: return "INITIAL";
        case Controller.RUNNING: return "RUNNING";
        case Controller.STEPPING: return "STEPPING";
        case Controller.CALLING: return "CALLING";
        case Controller.STOPPED: return "STOPPED";
        case Controller.FINISHED: return "FINISHED";
        case Controller.WAITING: return "WAITING";
        case Controller.ERROR: return "ERROR";
        default: return "???";
    }
};

Controller.prototype.isRunning = function() {
    switch (this.controllerState) {
        case Controller.RUNNING: case Controller.CALLING: case Controller.STEPPING: return true;
        default: return false;
    }
};

Controller.prototype.startAction = function() {
    this.start(Controller.RUNNING);
};

Controller.prototype.stopAction = function() {
    if (this.isRunning()) this.stop(Controller.STOPPED);
};

Controller.prototype.stepAction = function() {
    this.start(Controller.STEPPING);
};

Controller.prototype.callAction = function() {
    if (this.target.isCallable()) {
        this.callDepth = this.target.getStackDepth();
        this.start(Controller.CALLING);
    } else {
        this.start(Controller.STEPPING);
    }
};

Controller.speedToTimerDelay = function(speed) {
    if (speed > 90) return 0;
    return toInt((Controller.SLOW_DELAY + (Controller.FAST_DELAY - Controller.SLOW_DELAY) * speed / 90.0));
};

Controller.speedToCycleCount = function(speed) {
    if (speed < 90) return 1;
    return Math.min(Controller.MAX_CYCLE_COUNT, 1 << (speed - 90));
};

Controller.prototype.setSpeed = function(speed) {
    if (this.speedControl === null) {
        this.setSpeedCallback(speed);
    } else {
        this.speedControl.setValue(speed);
    }
};

Controller.prototype.getTimerDelay = function() {
    return Controller.speedToTimerDelay(this.speed);
};

Controller.prototype.getCycleCount = function() {
    return Controller.speedToCycleCount(this.speed);
};

Controller.prototype.createTimerListener = function() {
    return new TimerListener(this);
};

Controller.prototype.setSpeedCallback = function(speed) {
    this.speed = speed;
    if (this.timer !== null) {
        this.timer.setInitialDelay(Controller.speedToTimerDelay(speed));
    }
};

Controller.prototype.getSpeed = function() {
    return this.speed;
};

Controller.prototype.addChangeListener = function(listener) {
    this.listeners.add(listener);
};

Controller.prototype.removeChangeListener = function(listener) {
    var index = this.listeners.indexOf(listener);
    if (index >= 0) this.listeners.remove(index);
};

Controller.prototype.update = function() {
    this.fireChangeListeners();
};

Controller.prototype.signalError = function(msg) {
    if (this.failOnErrors) throw new RuntimeException(msg);
    this.setErrorMessage(msg);
    this.setControllerState(Controller.ERROR);
    if (this.errorHandler !== null) this.errorHandler.error(msg);
};

Controller.prototype.setErrorHandler = function(handler) {
    this.errorHandler = handler;
};

Controller.prototype.getErrorHandler = function() {
    return this.errorHandler;
};

Controller.prototype.getErrorMessage = function() {
    return this.errorMessage;
};

Controller.prototype.setErrorMessage = function(msg) {
    this.errorMessage = msg;
};

Controller.prototype.executeTimeStep = function() {
    try {
        this.stepTarget();
     } catch (ex) {
        var msg = (ex instanceof RuntimeException) ? RuntimeException.patchMessage(ex) : ex.toString();
        if (msg === null) msg = ex.toString();
        this.signalError(msg);
    }
};

Controller.prototype.stop = function(state) {
    this.setControllerState(state);
};

Controller.prototype.start = function(state) {
    this.setControllerState(state);
    this.timer.restart();
};

Controller.prototype.getCallDepth = function() {
    return this.callDepth;
};

Controller.prototype.fireChangeListeners = function() {
    var e = new ChangeEvent(this);
    var el0 = new JSElementList(this.listeners);
    for (var ei0 = 0; ei0 < el0.size(); ei0++) {
        var listener = el0.get(ei0);
        listener.stateChanged(e);
    }
};

Controller.prototype.stepTarget = function() {
    if (this.deferExecution) {
        SwingUtilities.invokeLater(new ControllerStepper(this, this.target));
    } else {
        var oldState = this.controllerState;
        this.target.step();
        this.stepComplete(oldState);
    }
};

Controller.prototype.shouldKeepRunning = function() {
    switch (this.controllerState) {
        case Controller.RUNNING: return true;
        case Controller.CALLING: return this.target.getStackDepth() > this.callDepth;
        default: return false;
    }
};

Controller.prototype.stepComplete = function(oldState) {
    switch (oldState) {
      case Controller.RUNNING:
        if (this.controllerState === Controller.RUNNING) this.timer.restart();
        break;
      case Controller.STEPPING:
        this.stopAction();
        break;
      case Controller.CALLING:
        if (this.target.getStackDepth() <= this.callDepth) {
            this.stopAction();
        } else {
            if (this.controllerState === Controller.CALLING) this.timer.restart();
        }
        break;
    }
};

Controller.INITIAL = 0;
Controller.RUNNING = 1;
Controller.STEPPING = 2;
Controller.CALLING = 3;
Controller.STOPPED = 4;
Controller.FINISHED = 5;
Controller.WAITING = 6;
Controller.ERROR = 7;
Controller.MAX_CYCLE_COUNT = 10000;
Controller.SLOW_DELAY = 500;
Controller.FAST_DELAY = 5;
var SpeedListener = function(controller) {
    if (!(this instanceof SpeedListener)) return new SpeedListener(controller);
    this.controller = controller;
};

SpeedListener.prototype.adjustmentValueChanged = function(e) {
    this.controller.setSpeedCallback((e.getSource()).getValue());
};

var TimerListener = function(controller) {
    if (!(this instanceof TimerListener)) return new TimerListener(controller);
    this.controller = controller;
};

TimerListener.prototype.actionPerformed = function(e) {
    this.controller.executeTimeStep();
};

var ControllerStepper = function(controller, target) {
    if (!(this instanceof ControllerStepper)) return new ControllerStepper(controller, target);
    this.controller = controller;
    this.target = target;
};

ControllerStepper.prototype.run = function() {
    try {
        var oldState = this.controller.getControllerState();
        var nCycles = this.controller.getCycleCount();
        this.target.step();
        while (this.controller.shouldKeepRunning() && --nCycles > 0) {
            this.target.step();
        }
        this.controller.stepComplete(oldState);
     } catch (ex) {
        var msg = (ex instanceof RuntimeException) ? RuntimeException.patchMessage(ex) : ex.toString();
        this.controller.signalError(msg);
    }
};


/* Exports */

return {
    Controller : Controller
};

});
