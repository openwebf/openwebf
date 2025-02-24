/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

// Bind the JavaScript side object,
// provide interface such as property setter/getter, call a property as function.
import 'dart:async';
import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:webf/bridge.dart';
import 'package:webf/dom.dart';
import 'package:webf/geometry.dart';
import 'package:webf/foundation.dart';
import 'package:webf/launcher.dart';
import 'package:webf/src/geometry/dom_point.dart';
import 'package:webf/src/html/canvas/canvas_path_2d.dart';
import 'package:webf/src/dom/intersection_observer.dart';

// We have some integrated built-in behavior starting with string prefix reuse the callNativeMethod implements.
enum BindingMethodCallOperations {
  GetProperty,
  SetProperty,
  GetAllPropertyNames,
  AnonymousFunctionCall,
  AsyncAnonymousFunction,
}

typedef NativeAsyncAnonymousFunctionCallback = Void Function(
    Pointer<Void> callbackContext, Pointer<NativeValue> nativeValue, Double contextId, Pointer<Utf8> errmsg);
typedef DartAsyncAnonymousFunctionCallback = void Function(
    Pointer<Void> callbackContext, Pointer<NativeValue> nativeValue, double contextId, Pointer<Utf8> errmsg);

typedef BindingCallFunc = dynamic Function(BindingObject bindingObject, List<dynamic> args, { BindingOpItem? profileOp });

List<BindingCallFunc> bindingCallMethodDispatchTable = [
  getterBindingCall,
  setterBindingCall,
  getPropertyNamesBindingCall,
  invokeBindingMethodSync,
  invokeBindingMethodAsync
];

// Dispatch the event to the binding side.
Future<void> _dispatchNomalEventToNative(Event event) async {
  await _dispatchEventToNative(event, false);
}
Future<void> _dispatchCaptureEventToNative(Event event) async {
  await _dispatchEventToNative(event, true);
}

void _handleDispatchResult(Object contextHandle, Pointer<NativeValue> returnValue) {
  _DispatchEventResultContext context = contextHandle as _DispatchEventResultContext;
  Pointer<EventDispatchResult> dispatchResult = fromNativeValue(context.controller.view, returnValue).cast<EventDispatchResult>();
  Event event = context.event;
  event.cancelable = dispatchResult.ref.canceled;
  event.propagationStopped = dispatchResult.ref.propagationStopped;
  event.defaultPrevented = dispatchResult.ref.preventDefaulted;
  event.sharedJSProps = Pointer.fromAddress(context.rawEvent.ref.bytes.elementAt(8).value);
  event.propLen = context.rawEvent.ref.bytes.elementAt(9).value;
  event.allocateLen = context.rawEvent.ref.bytes.elementAt(10).value;

  if (enableWebFCommandLog && context.stopwatch != null) {
    print('dispatch event to native side: target: ${event.target} arguments: ${context.dispatchEventArguments} time: ${context.stopwatch!.elapsedMicroseconds}us');
  }

  // Free the allocated arguments.
  malloc.free(context.rawEvent);
  malloc.free(context.method);
  malloc.free(context.allocatedNativeArguments);
  malloc.free(dispatchResult);
  malloc.free(returnValue);

  if (enableWebFProfileTracking) {
    WebFProfiler.instance.finishTrackEvaluate(context.profileOp!);
  }

  context.completer.complete();
}

class _DispatchEventResultContext {
  Completer completer;
  Stopwatch? stopwatch;
  Event event;
  Pointer<NativeValue> method;
  Pointer<NativeValue> allocatedNativeArguments;
  Pointer<RawEvent> rawEvent;
  List<dynamic> dispatchEventArguments;
  WebFController controller;
  EvaluateOpItem? profileOp;
  _DispatchEventResultContext(
    this.completer,
    this.event,
    this.method,
    this.allocatedNativeArguments,
    this.rawEvent,
    this.controller,
    this.dispatchEventArguments,
    this.stopwatch,
    this.profileOp,
  );
}

Future<void> _dispatchEventToNative(Event event, bool isCapture) async {
  Pointer<NativeBindingObject>? pointer = event.currentTarget?.pointer;
  double? contextId = event.target?.contextId;
  WebFController? controller = WebFController.getControllerOfJSContextId(contextId);

  if (controller == null || controller.view.disposed) return;

  if (contextId != null &&
      pointer != null &&
      pointer.ref.invokeBindingMethodFromDart != nullptr &&
      !isBindingObjectDisposed(event.target?.pointer) &&
      !isBindingObjectDisposed(event.currentTarget?.pointer)
  ) {
    Completer completer = Completer();

    EvaluateOpItem? currentProfileOp;
    if (enableWebFProfileTracking) {
      currentProfileOp = WebFProfiler.instance.startTrackEvaluate('_dispatchEventToNative');
    }

    BindingObject bindingObject = controller.view.getBindingObject(pointer);
    // Call methods implements at C++ side.
    DartInvokeBindingMethodsFromDart f = pointer.ref.invokeBindingMethodFromDart.asFunction();

    Pointer<RawEvent> rawEvent = event.toRaw().cast<RawEvent>();
    List<dynamic> dispatchEventArguments = [event.type, rawEvent, isCapture];

    Stopwatch? stopwatch;
    if (enableWebFCommandLog) {
      stopwatch = Stopwatch()..start();
    }

    Pointer<NativeValue> method = malloc.allocate(sizeOf<NativeValue>());
    toNativeValue(method, 'dispatchEvent');
    Pointer<NativeValue> allocatedNativeArguments = makeNativeValueArguments(bindingObject, dispatchEventArguments);

    _DispatchEventResultContext context = _DispatchEventResultContext(
      completer,
      event,
      method,
      allocatedNativeArguments,
      rawEvent,
      controller,
      dispatchEventArguments,
      stopwatch,
      currentProfileOp
    );

    Pointer<NativeFunction<NativeInvokeResultCallback>> resultCallback = Pointer.fromFunction(_handleDispatchResult);

    Future.microtask(() {
      f(pointer, currentProfileOp?.hashCode ?? 0, method, dispatchEventArguments.length, allocatedNativeArguments, context, resultCallback);
    });

    return completer.future;
  }
}

enum CreateBindingObjectType {
  createDOMMatrix,
  createPath2D,
  createDOMPoint,
  createIntersectionObserver
}

abstract class BindingBridge {
  static final Pointer<NativeFunction<InvokeBindingsMethodsFromNative>> _invokeBindingMethodFromNative =
      Pointer.fromFunction(invokeBindingMethodFromNativeImpl);

  static Pointer<NativeFunction<InvokeBindingsMethodsFromNative>> get nativeInvokeBindingMethod =>
      _invokeBindingMethodFromNative;

  static void createBindingObject(double contextId, Pointer<NativeBindingObject> pointer, CreateBindingObjectType type, Pointer<NativeValue> args, int argc) {
    WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
    List<dynamic> arguments = List.generate(argc, (index) {
      return fromNativeValue(controller.view, args.elementAt(index));
    });
    switch(type) {
      case CreateBindingObjectType.createDOMMatrix: {
        DOMMatrix domMatrix = DOMMatrix(BindingContext(controller.view, contextId, pointer), arguments);
        controller.view.setBindingObject(pointer, domMatrix);
        return;
      }
      case CreateBindingObjectType.createPath2D: {
        Path2D path2D = Path2D(context: BindingContext(controller.view, contextId, pointer), path2DInit: arguments);
        controller.view.setBindingObject(pointer, path2D);
        return;
      }
      case CreateBindingObjectType.createDOMPoint: {
        DOMPoint domPoint = DOMPoint(BindingContext(controller.view, contextId, pointer), arguments);
        controller.view.setBindingObject(pointer, domPoint);
        return;
      }
      case CreateBindingObjectType.createIntersectionObserver: {
        IntersectionObserver intersectionObserver = IntersectionObserver(BindingContext(controller.view, contextId, pointer), arguments);
        controller.view.setBindingObject(pointer, intersectionObserver);
      }
    }
  }

  // For compatible requirement, we set the WebFViewController to nullable due to the historical reason.
  // exp: We can not break the types for WidgetElement which will break all the codes for Users.
  static void _bindObject(WebFViewController? view, BindingObject object) {
    Pointer<NativeBindingObject>? nativeBindingObject = object.pointer;
    if (nativeBindingObject != null) {
      if (view != null) {
        view.setBindingObject(nativeBindingObject, object);
      }

      if (!isBindingObjectDisposed(nativeBindingObject)) {
        nativeBindingObject.ref.invokeBindingMethodFromNative = _invokeBindingMethodFromNative;
      }
    }
  }

  // For compatible requirement, we set the WebFViewController to nullable due to the historical reason.
  // exp: We can not break the types for WidgetElement which will break all the codes for Users.
  static void _unbindObject(WebFViewController? view, BindingObject object) {
    Pointer<NativeBindingObject>? nativeBindingObject = object.pointer;
    if (nativeBindingObject != null) {
      nativeBindingObject.ref.invokeBindingMethodFromNative = nullptr;
    }
  }

  static void setup() {
    BindingObject.bind = _bindObject;
    BindingObject.unbind = _unbindObject;
  }

  static void teardown() {
    BindingObject.bind = null;
    BindingObject.unbind = null;
  }

  static void listenEvent(EventTarget target, String type, {Pointer<AddEventListenerOptions>? addEventListenerOptions}) {
    bool isCapture = addEventListenerOptions != null ? addEventListenerOptions.ref.capture : false;
    if (!hasListener(target, type, isCapture: isCapture)) {
      EventListenerOptions? eventListenerOptions;
      if (addEventListenerOptions != null && addEventListenerOptions.ref.capture) {
        eventListenerOptions = EventListenerOptions(addEventListenerOptions.ref.capture, addEventListenerOptions.ref.passive, addEventListenerOptions.ref.once);
        target.addEventListener(type, _dispatchCaptureEventToNative, addEventListenerOptions: eventListenerOptions);
      } else
        target.addEventListener(type, _dispatchNomalEventToNative, addEventListenerOptions: eventListenerOptions);
    }
  }

  static void unlistenEvent(EventTarget target, String type, {bool isCapture = false}) {
    if (isCapture)
      target.removeEventListener(type, _dispatchCaptureEventToNative, isCapture: isCapture);
    else
      target.removeEventListener(type, _dispatchNomalEventToNative, isCapture: isCapture);

  }

  static bool hasListener(EventTarget target, String type, {bool isCapture = false}) {
    Map<String, List<EventHandler>> eventHandlers = isCapture ? target.getCaptureEventHandlers() : target.getEventHandlers();
    List<EventHandler>? handlers = eventHandlers[type];
    if (handlers != null) {
      return isCapture ? handlers.contains(_dispatchCaptureEventToNative) : handlers.contains(_dispatchNomalEventToNative);
    }
    return false;
  }
}
