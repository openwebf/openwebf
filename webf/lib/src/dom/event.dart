/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:webf/bridge.dart';
import 'package:webf/dom.dart';
import 'package:webf/rendering.dart';

enum AppearEventType { none, appear, disappear }

const String EVENT_CLICK = 'click';
const String EVENT_INPUT = 'input';
const String EVENT_APPEAR = 'appear';
const String EVENT_DISAPPEAR = 'disappear';
const String EVENT_COLOR_SCHEME_CHANGE = 'colorschemechange';
const String EVENT_ERROR = 'error';
const String EVENT_MEDIA_ERROR = 'mediaerror';
const String EVENT_TOUCH_START = 'touchstart';
const String EVENT_TOUCH_MOVE = 'touchmove';
const String EVENT_TOUCH_END = 'touchend';
const String EVENT_TOUCH_CANCEL = 'touchcancel';
const String EVENT_MESSAGE = 'message';
const String EVENT_CLOSE = 'close';
const String EVENT_OPEN = 'open';
const String EVENT_INTERSECTION_CHANGE = 'intersectionchange';
const String EVENT_CANCEL = 'cancel';
const String EVENT_FINISH = 'finish';
const String EVENT_TRANSITION_RUN = 'transitionrun';
const String EVENT_TRANSITION_CANCEL = 'transitioncancel';
const String EVENT_TRANSITION_START = 'transitionstart';
const String EVENT_TRANSITION_END = 'transitionend';
const String EVENT_FOCUS = 'focus';
const String EVENT_BLUR = 'blur';
const String EVENT_LOAD = 'load';
const String EVENT_PRELOADED = 'preloaded';
const String EVENT_PRERENDERED = 'prerendered';
const String EVENT_DOM_CONTENT_LOADED = 'DOMContentLoaded';
const String EVENT_READY_STATE_CHANGE = 'readystatechange';
const String EVENT_UNLOAD = 'unload';
const String EVENT_CHANGE = 'change';
const String EVENT_CAN_PLAY = 'canplay';
const String EVENT_CAN_PLAY_THROUGH = 'canplaythrough';
const String EVENT_ENDED = 'ended';
const String EVENT_PAUSE = 'pause';
const String EVENT_POP_STATE = 'popstate';
const String EVENT_HASH_CHANGE = 'hashchange';
const String EVENT_PLAY = 'play';
const String EVENT_SEEKED = 'seeked';
const String EVENT_SEEKING = 'seeking';
const String EVENT_VOLUME_CHANGE = 'volumechange';
const String EVENT_SCROLL = 'scroll';
const String EVENT_SWIPE = 'swipe';
const String EVENT_PAN = 'pan';
const String EVENT_SCALE = 'scale';
const String EVENT_LONG_PRESS = 'longpress';
const String EVENT_DOUBLE_CLICK = 'dblclick';
const String EVENT_DRAG = 'drag';
const String EVENT_RESIZE = 'resize';
const String EVENT_ANIMATION_CANCEL = 'animationcancel';
const String EVENT_ANIMATION_START = 'animationstart';
const String EVENT_ANIMATION_END = 'animationend';
const String EVENT_ANIMATION_ITERATION = 'animationiteration';
const String EVENT_STATE_START = 'start';
const String EVENT_STATE_UPDATE = 'update';
const String EVENT_STATE_END = 'end';
const String EVENT_STATE_CANCEL = 'cancel';
const String EVENT_DEVICE_ORIENTATION = 'deviceorientation';

mixin ElementEventMixin on ElementBase {
  AppearEventType _prevAppearState = AppearEventType.none;

  void clearEventResponder(RenderEventListenerMixin renderBox) {
    renderBox.getEventTarget = null;
  }

  void ensureEventResponderBound() {
    // Must bind event responder on render box model whatever there is no event listener.
    RenderBoxModel? renderBox = renderBoxModel;
    if (renderBox != null) {
      // Make sure pointer responder bind.
      renderBox.getEventTarget = getEventTarget;

      if (_hasIntersectionObserverEvent()) {
        renderBox.addIntersectionChangeListener(handleIntersectionChange);
        // Mark the compositing state for this render object as dirty
        // cause it will create new layer.
        renderBox.markNeedsCompositingBitsUpdate();
      } else {
        // Remove listener when no intersection related event
        renderBox.removeIntersectionChangeListener(handleIntersectionChange);
      }
      if (_hasResizeObserverEvent()) {
        renderBox.addResizeListener(handleResizeChange);
      } else {
        renderBox.removeResizeListener(handleResizeChange);
      }
    }
  }

  bool _hasIntersectionObserverEvent() {
    return hasEventListener(EVENT_APPEAR) ||
        hasEventListener(EVENT_DISAPPEAR) ||
        hasEventListener(EVENT_INTERSECTION_CHANGE);
  }

  bool _hasResizeObserverEvent() {
    return hasEventListener(EVENT_RESIZE);
  }

  @override
  void addEventListener(String eventType, EventHandler handler, {EventListenerOptions? addEventListenerOptions}) {
    super.addEventListener(eventType, handler, addEventListenerOptions: addEventListenerOptions);
    RenderBoxModel? renderBox = renderBoxModel;
    if (renderBox != null) {
      ensureEventResponderBound();
    }
  }

  @override
  void removeEventListener(String eventType, EventHandler handler, {bool isCapture = false}) {
    super.removeEventListener(eventType, handler, isCapture: isCapture);
    RenderBoxModel? renderBox = renderBoxModel;
    if (renderBox != null) {
      ensureEventResponderBound();
    }
  }

  EventTarget getEventTarget() {
    return this;
  }

  void handleAppear() {
    if (_prevAppearState == AppearEventType.appear) return;
    _prevAppearState = AppearEventType.appear;

    dispatchEvent(AppearEvent());
  }

  void handleDisappear() {
    if (_prevAppearState == AppearEventType.disappear) return;
    _prevAppearState = AppearEventType.disappear;

    dispatchEvent(DisappearEvent());
  }

  void handleIntersectionChange(IntersectionObserverEntry entry) {
    dispatchEvent(IntersectionChangeEvent(entry.intersectionRatio));
    if (entry.intersectionRatio > 0) {
      handleAppear();
    } else {
      handleDisappear();
    }
  }

  void handleResizeChange(ResizeObserverEntry entry) {
    dispatchEvent(ResizeEvent(entry).toCustomEvent());
  }
}

// @TODO: inherit BindingObject to receive value from Cpp side.
/// reference: https://developer.mozilla.org/zh-CN/docs/Web/API/Event
class Event {
  String type;

  // A boolean value indicating whether the event bubbles. The default is false.
  bool bubbles;

  // A boolean value indicating whether the event can be cancelled. The default is false.
  bool cancelable;

  // A boolean value indicating whether the event will trigger listeners outside of a shadow root (see Event.composed for more details).
  bool composed;
  EventTarget? currentTarget;
  EventTarget? target;
  int timeStamp = DateTime.now().millisecondsSinceEpoch;
  bool defaultPrevented = false;
  bool _immediateBubble = true;
  bool propagationStopped = false;

  Pointer<Void> sharedJSProps = nullptr;
  int propLen = 0;
  int allocateLen = 0;

  Event(
    this.type, {
    this.bubbles = false,
    this.cancelable = false,
    this.composed = false,
  });

  void preventDefault() {
    if (cancelable) {
      defaultPrevented = true;
    }
  }

  bool canBubble() => _immediateBubble;

  void stopImmediatePropagation() {
    _immediateBubble = false;
  }

  void stopPropagation() {
    bubbles = false;
  }

  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    Pointer<RawEvent> event = malloc.allocate<RawEvent>(sizeOf<RawEvent>());

    EventTarget? _target = target;
    EventTarget? _currentTarget = currentTarget;

    List<int> methods = [
      stringToNativeString(type).address,
      bubbles ? 1 : 0,
      cancelable ? 1 : 0,
      composed ? 1 : 0,
      timeStamp,
      defaultPrevented ? 1 : 0,
      (_target != null && _target.pointer != null) ? _target.pointer!.address : nullptr.address,
      (_currentTarget != null && _currentTarget.pointer != null) ? _currentTarget.pointer!.address : nullptr.address,
      sharedJSProps.address, // EventProps* props
      propLen,  // int64_t props_len
      allocateLen   // int64_t alloc_size;
    ];

    // Allocate extra bytes to store subclass's members.
    int nativeStructSize = methods.length + extraLength;

    final Pointer<Uint64> bytes = malloc.allocate<Uint64>(nativeStructSize * sizeOf<Uint64>());
    bytes.asTypedList(methods.length).setAll(0, methods);
    event.ref.bytes = bytes;
    event.ref.length = methods.length;
    event.ref.is_custom_event = isCustomEvent ? 1 : 0;

    return event.cast<Pointer>();
  }

  @override
  String toString() {
    return 'Event($type)';
  }
}

class PopStateEvent extends Event {
  final dynamic state;

  PopStateEvent({this.state}) : super(EVENT_POP_STATE);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [jsonEncode(state).toNativeUtf8().address];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

class HashChangeEvent extends Event {
  final String newUrl;
  final String oldUrl;

  HashChangeEvent({required this.newUrl, required this.oldUrl}) : super(EVENT_HASH_CHANGE);

  @override
  Pointer<NativeType> toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [
      stringToNativeString(newUrl).address,
      stringToNativeString(oldUrl).address
    ];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

class UIEvent extends Event {
  // Returns a long with details about the event, depending on the event type.
  // For click or dblclick events, UIEvent.detail is the current click count.
  //
  // For mousedown or mouseup events, UIEvent.detail is 1 plus the current click count.
  //
  // For all other UIEvent objects, UIEvent.detail is always zero.
  double detail;

  // The UIEvent.view read-only property returns the WindowProxy object from which the event was generated. In browsers, this is the Window object the event happened in.
  EventTarget? view;

  // @Deprecated
  // The UIEvent.which read-only property of the UIEvent interface returns a number that indicates which button was pressed on the mouse, or the numeric keyCode or the character code (charCode) of the key pressed on the keyboard.
  double which;

  UIEvent(
    String type, {
    this.detail = 0.0,
    this.view,
    this.which = 0.0,
    super.bubbles,
    super.cancelable,
    super.composed,
  }) : super(type);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [doubleToUint64(detail), view?.pointer?.address ?? nullptr.address, doubleToUint64(which)];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length + extraLength).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

class FocusEvent extends UIEvent {
  EventTarget? relatedTarget;

  FocusEvent(
    String type, {
    this.relatedTarget,
    super.detail,
    super.view,
    super.which,
    super.bubbles,
    super.cancelable,
    super.composed,
  }) : super(type);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [relatedTarget?.pointer?.address ?? nullptr.address];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length + extraLength).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

/// reference: https://developer.mozilla.org/zh-CN/docs/Web/API/MouseEvent
class MouseEvent extends UIEvent {
  double clientX;
  double clientY;
  double offsetX;
  double offsetY;

  MouseEvent(
    String type, {
    this.clientX = 0.0,
    this.clientY = 0.0,
    this.offsetX = 0.0,
    this.offsetY = 0.0,
    double detail = 0.0,
    EventTarget? view,
    double which = 0.0,
  }) : super(type, detail: detail, view: view, which: which, bubbles: true, cancelable: true, composed: false);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [
      doubleToUint64(clientX),
      doubleToUint64(clientY),
      doubleToUint64(offsetX),
      doubleToUint64(offsetY)
    ];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length + extraLength).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

class DeviceOrientationEvent extends Event {
  DeviceOrientationEvent(this.alpha, this.beta, this.gamma) : super(EVENT_DEVICE_ORIENTATION);

  final bool absolute = true;
  final double alpha;
  final double beta;
  final double gamma;

  @override
  Pointer<NativeType> toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [
      absolute ? 1 : 0,
      doubleToUint64(alpha),
      doubleToUint64(beta),
      doubleToUint64(gamma)
    ];
    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

/// reference: https://developer.mozilla.org/en-US/docs/Web/API/GestureEvent
class GestureEvent extends Event {
  final String state;
  final String direction;
  final double rotation;
  final double deltaX;
  final double deltaY;
  final double velocityX;
  final double velocityY;
  final double scale;

  GestureEvent(
    String type, {
    this.state = '',
    this.direction = '',
    this.rotation = 0.0,
    this.deltaX = 0.0,
    this.deltaY = 0.0,
    this.velocityX = 0.0,
    this.velocityY = 0.0,
    this.scale = 0.0,
    super.bubbles,
    super.cancelable,
    super.composed,
  }) : super(type);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [
      stringToNativeString(state).address,
      stringToNativeString(direction).address,
      doubleToUint64(deltaX),
      doubleToUint64(deltaY),
      doubleToUint64(velocityX),
      doubleToUint64(velocityY),
      doubleToUint64(scale),
      doubleToUint64(rotation)
    ];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

/// reference: http://dev.w3.org/2006/webapi/DOM-Level-3-Events/html/DOM3-Events.html#interface-CustomEvent
/// Attention: Detail now only can be a string.
class CustomEvent extends Event {
  dynamic detail;

  CustomEvent(
    String type, {
    this.detail = null,
    super.bubbles,
    super.cancelable,
    super.composed,
  }) : super(type);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    Pointer<NativeValue> detailValue = malloc.allocate(sizeOf<NativeValue>());
    toNativeValue(detailValue, detail);
    List<int> methods = [detailValue.address];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length, true).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

// https://w3c.github.io/input-events/
class InputEvent extends UIEvent {
  // A String containing the type of input that was made.
  // There are many possible values, such as insertText,
  // deleteContentBackward, insertFromPaste, and formatBold.
  final String inputType;
  final String data;

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [stringToNativeString(inputType).address, stringToNativeString(data).address];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }

  InputEvent({
    this.inputType = '',
    this.data = '',
    super.bubbles,
    super.cancelable,
    super.composed,
  }) : super(EVENT_INPUT);
}

class AppearEvent extends Event {
  AppearEvent() : super(EVENT_APPEAR);
}

class DisappearEvent extends Event {
  DisappearEvent() : super(EVENT_DISAPPEAR);
}

class ResizeEvent extends Event {
  ResizeObserverEntry entry;

  ResizeEvent(this.entry) : super(EVENT_RESIZE);

  toCustomEvent() {
    return CustomEvent(EVENT_RESIZE, detail: entry.toJson());
  }
}

class ColorSchemeChangeEvent extends Event {
  ColorSchemeChangeEvent(this.platformBrightness) : super(EVENT_COLOR_SCHEME_CHANGE);
  final String platformBrightness;
}

class MediaErrorCode {
  // The fetching of the associated resource was aborted by the user's request.
  static const double MEDIA_ERR_ABORTED = 1;

  // Some kind of network error occurred which prevented the media from being successfully fetched, despite having previously been available.
  static const double MEDIA_ERR_NETWORK = 2;

  // Despite having previously been determined to be usable, an error occurred while trying to decode the media resource, resulting in an error.
  static const double MEDIA_ERR_DECODE = 3;

  // The associated resource or media provider object (such as a MediaStream) has been found to be unsuitable.
  static const double MEDIA_ERR_SRC_NOT_SUPPORTED = 4;
}

class MediaError extends Event {
  /// A number which represents the general type of error that occurred, as follow
  final int code;

  /// a human-readable string which provides specific diagnostic information to help the reader understand the error condition which occurred;
  /// specifically, it isn't simply a summary of what the error code means, but actual diagnostic information to help in understanding what exactly went wrong.
  /// This text and its format is not defined by the specification and will vary from one user agent to another.
  /// If no diagnostics are available, or no explanation can be provided, this value is an empty string ("").
  final String message;

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [code, stringToNativeString(message).address];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }

  MediaError(this.code, this.message) : super(EVENT_MEDIA_ERROR);
}

/// reference: https://developer.mozilla.org/en-US/docs/Web/API/MessageEvent
class MessageEvent extends Event {
  /// The data sent by the message emitter.
  final dynamic data;

  /// A USVString representing the origin of the message emitter.
  final String origin;
  final String lastEventId;
  final String source;

  MessageEvent(this.data, {this.origin = '', this.lastEventId = '', this.source = ''}) : super(EVENT_MESSAGE);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    Pointer<NativeValue> nativeData = malloc.allocate(sizeOf<NativeValue>());
    toNativeValue(nativeData, data);
    List<int> methods = [
      nativeData.address,
      stringToNativeString(origin).address,
      stringToNativeString(lastEventId).address,
      stringToNativeString(source).address
    ];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

/// reference: https://developer.mozilla.org/en-US/docs/Web/API/CloseEvent/CloseEvent
class CloseEvent extends Event {
  /// An unsigned short containing the close code sent by the server
  final int code;

  /// Indicating the reason the server closed the connection.
  final String reason;

  /// Indicates whether or not the connection was cleanly closed
  final bool wasClean;

  CloseEvent(this.code, this.reason, this.wasClean) : super(EVENT_CLOSE);

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [code, stringToNativeString(reason).address, wasClean ? 1 : 0];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

class IntersectionChangeEvent extends Event {
  IntersectionChangeEvent(this.intersectionRatio) : super(EVENT_INTERSECTION_CHANGE);
  final double intersectionRatio;

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [doubleToUint64(intersectionRatio)];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

/// reference: https://w3c.github.io/touch-events/#touchevent-interface
class TouchEvent extends UIEvent {
  TouchEvent(
    String type, {
    TouchList? touches,
    TouchList? targetTouches,
    TouchList? changedTouches,
    this.altKey = false,
    this.metaKey = false,
    this.ctrlKey = false,
    this.shiftKey = false,
  })  : touches = touches ?? TouchList(),
        targetTouches = targetTouches ?? TouchList(),
        changedTouches = changedTouches ?? TouchList(),
        super(type, bubbles: true, cancelable: true, composed: true);

  TouchList touches;
  TouchList targetTouches;
  TouchList changedTouches;

  bool altKey = false;
  bool metaKey = false;
  bool ctrlKey = false;
  bool shiftKey = false;

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [
      touches.toNative().address,
      targetTouches.toNative().address,
      changedTouches.toNative().address,
      altKey ? 1 : 0,
      metaKey ? 1 : 0,
      ctrlKey ? 1 : 0,
      shiftKey ? 1 : 0
    ];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}

enum TouchType {
  direct,
  stylus,
}

/// reference: https://w3c.github.io/touch-events/#dom-touch
class Touch {
  final int identifier;
  final EventTarget target;
  final double clientX;
  final double clientY;
  final double screenX;
  final double screenY;
  final double pageX;
  final double pageY;
  final double radiusX;
  final double radiusY;
  final double rotationAngle;
  final double force;
  final double altitudeAngle;
  final double azimuthAngle;

  const Touch({
    required this.identifier,
    required this.target,
    this.clientX = 0,
    this.clientY = 0,
    this.screenX = 0,
    this.screenY = 0,
    this.pageX = 0,
    this.pageY = 0,
    this.radiusX = 0,
    this.radiusY = 0,
    this.rotationAngle = 0,
    this.force = 0,
    this.altitudeAngle = 0,
    this.azimuthAngle = 0,
  });

  void toNative(Pointer<NativeTouch> nativeTouch) {
    nativeTouch.ref.identifier = identifier;
    nativeTouch.ref.target = target.pointer!;
    nativeTouch.ref.clientX = clientX;
    nativeTouch.ref.clientY = clientY;
    nativeTouch.ref.screenX = screenX;
    nativeTouch.ref.screenY = screenY;
    nativeTouch.ref.pageX = pageX;
    nativeTouch.ref.pageY = pageY;
    nativeTouch.ref.radiusX = radiusX;
    nativeTouch.ref.radiusY = radiusY;
    nativeTouch.ref.rotationAngle = rotationAngle;
    nativeTouch.ref.force = force;
    nativeTouch.ref.altitudeAngle = altitudeAngle;
    nativeTouch.ref.azimuthAngle = azimuthAngle;
  }
}

/// reference: https://w3c.github.io/touch-events/#touchlist-interface
class TouchList {
  final List<Touch> _items = [];

  int get length => _items.length;

  Touch item(int index) {
    return _items[index];
  }

  Touch operator [](int index) {
    return _items[index];
  }

  // https://github.com/WebKit/WebKit/blob/main/Source/WebCore/dom/TouchList.h#L54
  void append(Touch touch) {
    _items.add(touch);
  }

  Pointer<NativeTouchList> toNative() {
    Pointer<NativeTouchList> touchList = malloc.allocate(sizeOf<NativeTouchList>());
    Pointer<NativeTouch> touches = malloc.allocate<NativeTouch>(sizeOf<NativeTouch>() * _items.length);
    for (int i = 0; i < _items.length; i++) {
      _items[i].toNative(touches.elementAt(i));
    }
    touchList.ref.length = _items.length;
    touchList.ref.touches = touches;
    return touchList;
  }
}

class AnimationEvent extends Event {
  AnimationEvent(String type, {String? animationName, double? elapsedTime, String? pseudoElement})
      : animationName = animationName ?? '',
        elapsedTime = elapsedTime ?? 0.0,
        pseudoElement = pseudoElement ?? '',
        super(type) {}

  String animationName;
  double elapsedTime;
  String pseudoElement;

  @override
  Pointer toRaw([int extraLength = 0, bool isCustomEvent = false]) {
    List<int> methods = [
      stringToNativeString(animationName).address,
      doubleToUint64(elapsedTime),
      stringToNativeString(pseudoElement).address
    ];

    Pointer<RawEvent> rawEvent = super.toRaw(methods.length).cast<RawEvent>();
    int currentStructSize = rawEvent.ref.length + methods.length;
    Uint64List bytes = rawEvent.ref.bytes.asTypedList(currentStructSize);
    bytes.setAll(rawEvent.ref.length, methods);
    rawEvent.ref.length = currentStructSize;

    return rawEvent;
  }
}
