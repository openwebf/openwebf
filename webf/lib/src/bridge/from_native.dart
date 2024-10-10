/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'dart:async';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:webf/bridge.dart';
import 'package:webf/foundation.dart';
import 'package:webf/launcher.dart';
import 'package:webf/src/widget/widget_element.dart';

String uint16ToString(Pointer<Uint16> pointer, int length) {
  return String.fromCharCodes(pointer.asTypedList(length));
}

Pointer<Uint16> _stringToUint16(String string) {
  final units = string.codeUnits;
  final Pointer<Uint16> result = malloc.allocate<Uint16>(units.length * sizeOf<Uint16>());
  final Uint16List nativeString = result.asTypedList(units.length);
  nativeString.setAll(0, units);
  return result;
}

Pointer<NativeString> stringToNativeString(String string) {
  Pointer<NativeString> nativeString = malloc.allocate<NativeString>(sizeOf<NativeString>());
  nativeString.ref.string = _stringToUint16(string);
  nativeString.ref.length = string.length;
  return nativeString;
}

Pointer<Uint8> uint8ListToPointer(Uint8List data) {
  Pointer<Uint8> ptr = malloc.allocate<Uint8>(sizeOf<Uint8>() * data.length + 1);
  Uint8List dataView = ptr.asTypedList(data.length + 1);
  dataView.setAll(0, data);
  dataView[data.length] = 0;
  return ptr;
}

int doubleToUint64(double value) {
  var byteData = ByteData(8);
  byteData.setFloat64(0, value);
  return byteData.getUint64(0);
}

int doubleToInt64(double value) {
  var byteData = ByteData(8);
  byteData.setFloat64(0, value);
  return byteData.getInt64(0);
}

double uInt64ToDouble(int value) {
  var byteData = ByteData(8);
  byteData.setInt64(0, value);
  return byteData.getFloat64(0);
}

String nativeStringToString(Pointer<NativeString> pointer) {
  return uint16ToString(pointer.ref.string, pointer.ref.length);
}

void freeNativeString(Pointer<NativeString> pointer) {
  malloc.free(pointer.ref.string);
  malloc.free(pointer);
}

// Steps for using dart:ffi to call a Dart function from C:
// 1. Import dart:ffi.
// 2. Create a typedef with the FFI type signature of the Dart function.
// 3. Create a typedef for the variable that you’ll use when calling the
//    Dart function.
// 4. Open the dynamic library that register in the C.
// 5. Get a reference to the C function, and put it into a variable.
// 6. Call from C.

// Register InvokeModule
typedef NativeAsyncModuleCallback = Pointer<NativeValue> Function(
    Pointer<Void> callbackContext,
    Double contextId,
    Pointer<Utf8> errmsg,
    Pointer<NativeValue> ptr,
    Handle context,
    Pointer<NativeFunction<NativeHandleInvokeModuleResult>> handleResult);
typedef DartAsyncModuleCallback = Pointer<NativeValue> Function(
    Pointer<Void> callbackContext,
    double contextId,
    Pointer<Utf8> errmsg,
    Pointer<NativeValue> ptr,
    Object context,
    Pointer<NativeFunction<NativeHandleInvokeModuleResult>> handleResult);

typedef NativeHandleInvokeModuleResult = Void Function(Handle context, Pointer<NativeValue> result);

class InvokeModuleOptions extends Struct {
  external Pointer<NativeBindingObject> formData;
}

typedef NativeInvokeModule = Pointer<NativeValue> Function(
    Pointer<Void> callbackContext,
    Double contextId,
    Int64 profileId,
    Pointer<NativeString> module,
    Pointer<NativeString> method,
    Pointer<NativeValue> params,
    Uint32 argc,
    Pointer<NativeFunction<NativeAsyncModuleCallback>>);

class _InvokeModuleResultContext {
  Completer<dynamic> completer;
  Pointer<Utf8>? errmsgPtr;
  Stopwatch? stopwatch;
  WebFViewController currentView;
  Pointer<NativeValue>? data;
  String moduleName;
  String method;
  dynamic params;

  _InvokeModuleResultContext(this.completer, this.currentView, this.moduleName, this.method, this.params,
      {this.errmsgPtr, this.data, this.stopwatch});
}

void _handleInvokeModuleResult(Object handle, Pointer<NativeValue> result) {
  _InvokeModuleResultContext context = handle as _InvokeModuleResultContext;
  var returnValue = fromNativeValue(context.currentView, result);

  if (enableWebFCommandLog && context.stopwatch != null) {
    print(
        'Invoke module callback from(name: ${context.moduleName} method: ${context.method}, params: ${context.params}) '
        'return: $returnValue time: ${context.stopwatch!.elapsedMicroseconds}us');
  }

  malloc.free(result);
  if (context.errmsgPtr != null) {
    malloc.free(context.errmsgPtr!);
  } else if (context.data != null) {
    malloc.free(context.data!);
  }

  context.completer.complete(returnValue);
}

dynamic invokeModule(Pointer<Void> callbackContext, WebFController controller, String moduleName, String method, params,
    DartAsyncModuleCallback callback, { BindingOpItem? profileOp }) {
  WebFViewController currentView = controller.view;
  dynamic result;

  Stopwatch? stopwatch;
  if (enableWebFCommandLog) {
    stopwatch = Stopwatch()..start();
  }

  try {
    Future<dynamic> invokeModuleCallback({String? error, data}) {
      Completer<dynamic> completer = Completer();
      // To make sure Promise then() and catch() executed before Promise callback called at JavaScript side.
      // We should make callback always async.
      Future.microtask(() {
        if (controller.view != currentView || currentView.disposed) return;

        Pointer<NativeFunction<NativeHandleInvokeModuleResult>> handleResult =
            Pointer.fromFunction(_handleInvokeModuleResult);
        if (error != null) {
          Pointer<Utf8> errmsgPtr = error.toNativeUtf8();
          _InvokeModuleResultContext context = _InvokeModuleResultContext(
              completer, currentView, moduleName, method, params,
              errmsgPtr: errmsgPtr, stopwatch: stopwatch);
          callback(callbackContext, currentView.contextId, errmsgPtr, nullptr, context, handleResult);
        } else {
          Pointer<NativeValue> dataPtr = malloc.allocate(sizeOf<NativeValue>());
          toNativeValue(dataPtr, data);
          _InvokeModuleResultContext context = _InvokeModuleResultContext(
              completer, currentView, moduleName, method, params,
              data: dataPtr, stopwatch: stopwatch);
          callback(callbackContext, currentView.contextId, nullptr, dataPtr, context, handleResult);
        }
      });
      return completer.future;
    }

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.startTrackBindingSteps(profileOp!, 'moduleManager.invokeModule');
    }

    result = controller.module.moduleManager.invokeModule(moduleName, method, params, invokeModuleCallback);

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.finishTrackBindingSteps(profileOp!);
    }

  } catch (e, stack) {
    if (enableWebFCommandLog) {
      print('Invoke module failed: $e\n$stack');
    }
    String error = '$e\n$stack';
    callback(callbackContext, currentView.contextId, error.toNativeUtf8(), nullptr, {}, nullptr);
  }

  if (enableWebFCommandLog) {
    print('Invoke module name: $moduleName method: $method, params: $params '
        'return: $result time: ${stopwatch!.elapsedMicroseconds}us');
  }

  return result;
}

Pointer<NativeValue> _invokeModule(
    Pointer<Void> callbackContext,
    double contextId,
    int profileLinkId,
    Pointer<NativeString> module,
    Pointer<NativeString> method,
    Pointer<NativeValue> params,
    int argc,
    Pointer<NativeFunction<NativeAsyncModuleCallback>> callback) {

  BindingOpItem? currentProfileOp;
  if (enableWebFProfileTracking) {
    currentProfileOp = WebFProfiler.instance.startTrackBinding(profileLinkId);
  }

  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;

  if (enableWebFProfileTracking) {
    WebFProfiler.instance.startTrackBindingSteps(currentProfileOp!, 'fromNativeValue');
  }

  String moduleValue = nativeStringToString(module);
  String methodValue = nativeStringToString(method);
  dynamic paramsValue = fromNativeValue(controller.view, params);

  if (enableWebFProfileTracking) {
    WebFProfiler.instance.finishTrackBindingSteps(currentProfileOp!);
    WebFProfiler.instance.startTrackBindingSteps(currentProfileOp, 'invokeModule');
  }

  dynamic result = invokeModule(callbackContext, controller, moduleValue, methodValue,
      paramsValue, callback.asFunction(), profileOp: currentProfileOp);

  if (enableWebFProfileTracking) {
    WebFProfiler.instance.finishTrackBindingSteps(currentProfileOp!);
    WebFProfiler.instance.startTrackBindingSteps(currentProfileOp, 'toNativeValue');
  }

  Pointer<NativeValue> returnValue = malloc.allocate(sizeOf<NativeValue>());
  toNativeValue(returnValue, result);

  if (enableWebFProfileTracking) {
    WebFProfiler.instance.finishTrackBindingSteps(currentProfileOp!);
    WebFProfiler.instance.finishTrackBinding(profileLinkId);
  }

  return returnValue;
}

final Pointer<NativeFunction<NativeInvokeModule>> _nativeInvokeModule = Pointer.fromFunction(_invokeModule);

// Register reloadApp
typedef NativeReloadApp = Void Function(Double contextId);

void _reloadApp(double contextId) async {
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;

  try {
    await controller.reload();
  } catch (e, stack) {
    print('Dart Error: $e\n$stack');
  }
}

final Pointer<NativeFunction<NativeReloadApp>> _nativeReloadApp = Pointer.fromFunction(_reloadApp);

typedef NativeAsyncCallback = Void Function(Pointer<Void> callbackContext, Double contextId, Pointer<Utf8> errmsg);
typedef DartAsyncCallback = void Function(Pointer<Void> callbackContext, double contextId, Pointer<Utf8> errmsg);
typedef NativeRAFAsyncCallback = Void Function(
    Pointer<Void> callbackContext, Double contextId, Double data, Pointer<Utf8> errmsg);
typedef DartRAFAsyncCallback = void Function(Pointer<Void>, double contextId, double data, Pointer<Utf8> errmsg);

// Register requestBatchUpdate
typedef NativeRequestBatchUpdate = Void Function(Double contextId);

void _requestBatchUpdate(double contextId) {
  WebFController? controller = WebFController.getControllerOfJSContextId(contextId);
  return controller?.module.requestBatchUpdate();
}

final Pointer<NativeFunction<NativeRequestBatchUpdate>> _nativeRequestBatchUpdate =
    Pointer.fromFunction(_requestBatchUpdate);

// Register setTimeout
typedef NativeSetTimeout = Void Function(Int32 newTimerId, Pointer<Void> callbackContext, Double contextId,
    Pointer<NativeFunction<NativeAsyncCallback>>, Int32);

void _setTimeout(int newTimerId, Pointer<Void> callbackContext, double contextId,
    Pointer<NativeFunction<NativeAsyncCallback>> callback, int timeout) {
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
  WebFViewController currentView = controller.view;

  controller.module.setTimeout(newTimerId, timeout, () {
    DartAsyncCallback func = callback.asFunction();
    void _runCallback() {
      if (controller.view != currentView || currentView.disposed) return;

      try {
        func(callbackContext, contextId, nullptr);
      } catch (e, stack) {
        Pointer<Utf8> nativeErrorMessage = ('Error: $e\n$stack').toNativeUtf8();
        func(callbackContext, contextId, nativeErrorMessage);
      }
    }

    // Pause if webf page paused.
    if (controller.paused) {
      controller.pushPendingCallbacks(_runCallback);
    } else {
      _runCallback();
    }
  });
}

final Pointer<NativeFunction<NativeSetTimeout>> _nativeSetTimeout = Pointer.fromFunction(_setTimeout);

// Register setInterval
typedef NativeSetInterval = Void Function(Int32 newTimerId, Pointer<Void> callbackContext, Double contextId,
    Pointer<NativeFunction<NativeAsyncCallback>>, Int32);

void _setInterval(int newTimerId, Pointer<Void> callbackContext, double contextId,
    Pointer<NativeFunction<NativeAsyncCallback>> callback, int timeout) {
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
  WebFViewController currentView = controller.view;
  controller.module.setInterval(newTimerId, timeout, () {
    void _runCallbacks() {
      if (controller.view != currentView || currentView.disposed) return;

      DartAsyncCallback func = callback.asFunction();
      try {
        func(callbackContext, contextId, nullptr);
      } catch (e, stack) {
        Pointer<Utf8> nativeErrorMessage = ('Dart Error: $e\n$stack').toNativeUtf8();
        func(callbackContext, contextId, nativeErrorMessage);
      }
    }

    // Pause if webf page paused.
    if (controller.paused) {
      controller.pushPendingCallbacks(_runCallbacks);
    } else {
      _runCallbacks();
    }
  });
}

final Pointer<NativeFunction<NativeSetInterval>> _nativeSetInterval = Pointer.fromFunction(_setInterval);

// Register clearTimeout
typedef NativeClearTimeout = Void Function(Double contextId, Int32);

void _clearTimeout(double contextId, int timerId) {
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
  return controller.module.clearTimeout(timerId);
}

final Pointer<NativeFunction<NativeClearTimeout>> _nativeClearTimeout = Pointer.fromFunction(_clearTimeout);

// Register requestAnimationFrame
typedef NativeRequestAnimationFrame = Void Function(
    Int32 newFrameId, Pointer<Void> callbackContext, Double contextId, Pointer<NativeFunction<NativeRAFAsyncCallback>>);

void _requestAnimationFrame(int newFrameId, Pointer<Void> callbackContext, double contextId,
    Pointer<NativeFunction<NativeRAFAsyncCallback>> callback) {
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
  WebFViewController currentView = controller.view;
  controller.module.requestAnimationFrame(newFrameId, (double highResTimeStamp) {
    void _runCallback() {
      if (controller.view != currentView || currentView.disposed) return;
      DartRAFAsyncCallback func = callback.asFunction();
      try {
        func(callbackContext, contextId, highResTimeStamp, nullptr);
      } catch (e, stack) {
        Pointer<Utf8> nativeErrorMessage = ('Error: $e\n$stack').toNativeUtf8();
        func(callbackContext, contextId, highResTimeStamp, nativeErrorMessage);
      }
    }

    // Pause if webf page paused.
    if (controller.paused) {
      controller.pushPendingCallbacks(_runCallback);
    } else {
      _runCallback();
    }
  });
}

final Pointer<NativeFunction<NativeRequestAnimationFrame>> _nativeRequestAnimationFrame =
    Pointer.fromFunction(_requestAnimationFrame);

// Register cancelAnimationFrame
typedef NativeCancelAnimationFrame = Void Function(Double contextId, Int32 id);

void _cancelAnimationFrame(double contextId, int timerId) {
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
  controller.module.cancelAnimationFrame(timerId);
}

final Pointer<NativeFunction<NativeCancelAnimationFrame>> _nativeCancelAnimationFrame =
    Pointer.fromFunction(_cancelAnimationFrame);

typedef NativeAsyncBlobCallback = Void Function(
    Pointer<Void> callbackContext, Double contextId, Pointer<Utf8>, Pointer<Uint8>, Int32);
typedef DartAsyncBlobCallback = void Function(
    Pointer<Void> callbackContext, double contextId, Pointer<Utf8>, Pointer<Uint8>, int);
typedef NativeToBlob = Void Function(Pointer<Void> callbackContext, Double contextId,
    Pointer<NativeFunction<NativeAsyncBlobCallback>>, Pointer<Void>, Double);

void _toBlob(Pointer<Void> callbackContext, double contextId, Pointer<NativeFunction<NativeAsyncBlobCallback>> callback,
    Pointer<Void> elementPtr, double devicePixelRatio) {
  DartAsyncBlobCallback func = callback.asFunction();
  WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
  controller.view.toImage(devicePixelRatio, elementPtr).then((Uint8List bytes) {
    Pointer<Uint8> bytePtr = malloc.allocate<Uint8>(sizeOf<Uint8>() * bytes.length);
    Uint8List byteList = bytePtr.asTypedList(bytes.length);
    byteList.setAll(0, bytes);
    func(callbackContext, contextId, nullptr, bytePtr, bytes.length);
  }).catchError((error, stack) {
    Pointer<Utf8> nativeErrorMessage = ('$error\n$stack').toNativeUtf8();
    func(callbackContext, contextId, nativeErrorMessage, nullptr, 0);
  });
}

final Pointer<NativeFunction<NativeToBlob>> _nativeToBlob = Pointer.fromFunction(_toBlob);

typedef NativeFlushUICommand = Void Function(Double contextId, Pointer<NativeBindingObject> selfPointer);
typedef DartFlushUICommand = void Function(double contextId, Pointer<NativeBindingObject> selfPointer);

void _flushUICommand(double contextId, Pointer<NativeBindingObject> selfPointer) {
  flushUICommandWithContextId(contextId, selfPointer);
}

final Pointer<NativeFunction<NativeFlushUICommand>> _nativeFlushUICommand = Pointer.fromFunction(_flushUICommand);

typedef NativeCreateBindingObject = Void Function(Double contextId, Pointer<NativeBindingObject> nativeBindingObject,
    Int32 type, Pointer<NativeValue> args, Int32 argc);
typedef DartCreateBindingObject = void Function(
    double contextId, Pointer<NativeBindingObject> nativeBindingObject, int type, Pointer<NativeValue> args, int argc);

void _createBindingObject(
    double contextId, Pointer<NativeBindingObject> nativeBindingObject, int type, Pointer<NativeValue> args, int argc) {
  BindingBridge.createBindingObject(contextId, nativeBindingObject, CreateBindingObjectType.values[type], args, argc);
}

final Pointer<NativeFunction<NativeCreateBindingObject>> _nativeCreateBindingObject =
    Pointer.fromFunction(_createBindingObject);

typedef NativeGetWidgetElementShape = Int8 Function(Double contextId, Pointer<NativeBindingObject> nativeBindingObject, Pointer<NativeValue> result);

int _getWidgetElementShape(double contextId, Pointer<NativeBindingObject> nativeBindingObject, Pointer<NativeValue> result) {
  try {
    WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
    DynamicBindingObject object = controller.view.getBindingObject<DynamicBindingObject>(nativeBindingObject)!;

    if (object is WidgetElement) {
      object.nativeGetPropertiesAndMethods(result);
      return 1;
    }
  } catch (e, stack) {
    print('$e\n$stack');
  }
  return 0;
}

final Pointer<NativeFunction<NativeGetWidgetElementShape>> _nativeGetWidgetElementShape = Pointer.fromFunction(_getWidgetElementShape, 0);

typedef NativeJSError = Void Function(Double contextId, Pointer<Utf8>);

void _onJSError(double contextId, Pointer<Utf8> charStr) {
  WebFController? controller = WebFController.getControllerOfJSContextId(contextId);
  JSErrorHandler? handler = controller?.onJSError;
  if (handler != null) {
    String msg = charStr.toDartString();
    handler(msg);
  }
  malloc.free(charStr);
}

final Pointer<NativeFunction<NativeJSError>> _nativeOnJsError = Pointer.fromFunction(_onJSError);

typedef NativeJSLog = Void Function(Double contextId, Int32 level, Pointer<Utf8>);

void _onJSLog(double contextId, int level, Pointer<Utf8> charStr) {
  String msg = charStr.toDartString();
  WebFController? controller = WebFController.getControllerOfJSContextId(contextId);
  if (controller != null) {
    JSLogHandler? jsLogHandler = controller.onJSLog;
    if (jsLogHandler != null) {
      jsLogHandler(level, msg);
    }
  }
  malloc.free(charStr);
}

final Pointer<NativeFunction<NativeJSLog>> _nativeOnJsLog = Pointer.fromFunction(_onJSLog);

final List<int> _dartNativeMethods = [
  _nativeInvokeModule.address,
  _nativeRequestBatchUpdate.address,
  _nativeReloadApp.address,
  _nativeSetTimeout.address,
  _nativeSetInterval.address,
  _nativeClearTimeout.address,
  _nativeRequestAnimationFrame.address,
  _nativeCancelAnimationFrame.address,
  _nativeToBlob.address,
  _nativeFlushUICommand.address,
  _nativeCreateBindingObject.address,
  _nativeGetWidgetElementShape.address,
  _nativeOnJsError.address,
  _nativeOnJsLog.address,
];

List<int> makeDartMethodsData() {
  return _dartNativeMethods;
}
