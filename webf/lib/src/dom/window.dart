/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:math' as math;
import 'dart:async';
import 'dart:ui';

import 'package:sensors_plus/sensors_plus.dart';
import 'package:webf/bridge.dart';
import 'package:webf/dom.dart';
import 'package:webf/foundation.dart';
import 'package:webf/rendering.dart';
import 'package:webf/module.dart';
import 'package:webf/src/css/computed_style_declaration.dart';

const String WINDOW = 'WINDOW';

class Window extends EventTarget {
  final Document document;
  final Screen screen;

  Window(BindingContext? context, this.document)
      : screen = Screen(context!.contextId, document.controller.ownerFlutterView, document.controller.view),
        super(context) {
    BindingBridge.listenEvent(this, 'load');
    BindingBridge.listenEvent(this, 'gcopen');
  }

  @override
  EventTarget? get parentEventTarget => null;

  @override
  void initializeMethods(Map<String, BindingObjectMethod> methods) {
    methods['scroll'] = methods['scrollTo'] =
        BindingObjectMethodSync(call: (args) => scrollTo(castToType<double>(args[0]), castToType<double>(args[1])));
    methods['scrollBy'] = BindingObjectMethodSync(call: (args) => scrollBy(castToType<double>(args[0]), castToType<double>(args[1])));
    methods['open'] = BindingObjectMethodSync(call: (args) => open(castToType<String>(args[0])));
    methods['getComputedStyle'] = BindingObjectMethodSync(call: (args) => getComputedStyle(args[0] as Element));
  }

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {
    // https://www.w3.org/TR/cssom-view-1/#extensions-to-the-window-interface
    properties['innerWidth'] = BindingObjectProperty(getter: () => innerWidth);
    properties['innerHeight'] = BindingObjectProperty(getter: () => innerHeight);
    properties['scrollX'] = BindingObjectProperty(getter: () => scrollX);
    properties['scrollY'] = BindingObjectProperty(getter: () => scrollY);
    properties['pageXOffset'] = BindingObjectProperty(getter: () => scrollX);
    properties['pageYOffset'] = BindingObjectProperty(getter: () => scrollY);
    properties['screen'] = BindingObjectProperty(getter: () => screen);
    properties['colorScheme'] = BindingObjectProperty(getter: () => colorScheme);
    properties['devicePixelRatio'] = BindingObjectProperty(getter: () => devicePixelRatio);
  }

  void open(String url) {
    String? sourceUrl = document.controller.view.rootController.url;
    document.controller.view.handleNavigationAction(sourceUrl, url, WebFNavigationType.navigate);
  }

  ComputedCSSStyleDeclaration getComputedStyle(Element element) {
    return ComputedCSSStyleDeclaration(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), element, element.tagName);
  }

  double get scrollX => document.documentElement!.scrollLeft;

  double get scrollY => document.documentElement!.scrollTop;

  void scrollTo(double x, double y, [bool withAnimation = false]) {
    document.flushStyle();
    document.documentElement!
      ..flushLayout()
      ..scrollTo(x, y, withAnimation);
  }

  void scrollBy(double x, double y, [bool withAnimation = false]) {
    document.flushStyle();
    document.documentElement!
      ..flushLayout()
      ..scrollBy(x, y, withAnimation);
  }

  String get colorScheme => document.controller.ownerFlutterView.platformDispatcher.platformBrightness == Brightness.light ? 'light' : 'dark';

  double get devicePixelRatio => document.controller.ownerFlutterView.devicePixelRatio;

  // The innerWidth/innerHeight attribute must return the viewport width/height
  // including the size of a rendered scroll bar (if any), or zero if there is no viewport.
  // https://drafts.csswg.org/cssom-view/#dom-window-innerwidth
  // This is a read only idl attribute.
  double get innerWidth => _viewportSize.width;

  double get innerHeight => _viewportSize.height;

  Size get _viewportSize {
    RenderViewportBox? viewport = document.viewport;
    if (viewport != null && viewport.hasSize) {
      return viewport.size;
    } else {
      return Size.zero;
    }
  }

  @override
  Future<void> dispatchEvent(Event event) async {
    // Events such as EVENT_DOM_CONTENT_LOADED need to ensure that listeners are flushed and registered.
    if (contextId != null && event.type == EVENT_DOM_CONTENT_LOADED ||
        event.type == EVENT_LOAD ||
        event.type == EVENT_ERROR) {
      flushUICommandWithContextId(contextId!, pointer!);
    }
    return super.dispatchEvent(event);
  }

  @override
  void addEventListener(String eventType, EventHandler handler, {EventListenerOptions? addEventListenerOptions}) {
    super.addEventListener(eventType, handler, addEventListenerOptions: addEventListenerOptions);
    switch (eventType) {
      case EVENT_SCROLL:
        // Fired at the Document or element when the viewport or element is scrolled, respectively.
        document.documentElement?.addEventListener(eventType, handler, addEventListenerOptions: addEventListenerOptions);
        break;
      case EVENT_DEVICE_ORIENTATION:
        _registerGyroscope();
        break;
    }
  }

  @override
  void removeEventListener(String eventType, EventHandler handler, {bool isCapture = false}) {
    super.removeEventListener(eventType, handler, isCapture: isCapture);
    switch (eventType) {
      case EVENT_SCROLL:
        document.documentElement?.removeEventListener(eventType, handler);
        break;
      case EVENT_DEVICE_ORIENTATION:
        _unRegisterGyroscope(false);
        break;
    }
  }

  /// Moves the focus to the window's browsing context, if any.
  /// https://html.spec.whatwg.org/multipage/interaction.html#dom-window-focus
  void focus() {
    // TODO
  }

  StreamSubscription<GyroscopeEvent>? _gyroscopeStream;
  void _registerGyroscope() {
    _gyroscopeStream ??= gyroscopeEventStream().listen((GyroscopeEvent event) {
      dispatchDeviceOrientationEvent(event.x, event.y, event.z);
    },
      cancelOnError: true,
    );
  }

  void _unRegisterGyroscope(bool dispose) {
    if (dispose || !hasEventListener(EVENT_DEVICE_ORIENTATION)) {
      _gyroscopeStream?.cancel();
      _gyroscopeStream = null;
    }
  }

  /// Convert gyroscope data obtained in Flutter into rotation angle in the Web
  /// In the w3c standard:
  /// alpha: A number representing the motion of the device around the z axis, express in degrees with values ranging from 0 (inclusive) to 360 (exclusive).
  /// beta: A number representing the motion of the device around the x axis, expressed in degrees with values ranging from -180 (inclusive) to 180 (exclusive).
  /// gamma: A number representing the motion of the device around the y axis, expressed in degrees with values ranging from -90 (inclusive) to 90 (exclusive).
  void dispatchDeviceOrientationEvent(x, y, z) {
    var xAxisAngle= math.atan2(y, z) * (180 / math.pi); // Angle of rotation around the X-axis
    var yAxisAngle = math.atan2(-x, math.sqrt(y * y + z * z)) * (180 / math.pi); // Rotation angle around Y axis
    var zAxisAngle = math.atan2(y, z) * (180 / math.pi); // Rotation angle around Z axis
    var alpha = zAxisAngle + 180;
    var beta = xAxisAngle;
    var gamma = yAxisAngle;
    if (hasEventListener(EVENT_DEVICE_ORIENTATION)) {
      dispatchEvent(DeviceOrientationEvent(alpha, beta, gamma));
    }
  }

  @override
  void dispose() async {
    _unRegisterGyroscope(true);
    super.dispose();
  }
}
