/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:async';
import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:webf/foundation.dart';
import 'package:webf/bridge.dart';
import 'package:webf/launcher.dart';
import 'element.dart';
import 'intersection_observer_entry.dart';
import 'package:flutter/foundation.dart';

class _IntersectionObserverDeliverContext {
  Completer completer;
  Stopwatch? stopwatch;

  Pointer<NativeValue> allocatedNativeArguments;
  Pointer<NativeIntersectionObserverEntry> rawEntries;
  WebFController controller;
  EvaluateOpItem? profileOp;

  _IntersectionObserverDeliverContext(
    this.completer,
    this.stopwatch,
    this.allocatedNativeArguments,
    this.rawEntries,
    this.controller,
    this.profileOp,
  );
}

void _handleDeliverResult(Object handle, Pointer<NativeValue> returnValue) {
  _IntersectionObserverDeliverContext context = handle as _IntersectionObserverDeliverContext;

  if (enableWebFCommandLog && context.stopwatch != null) {
    debugPrint('deliver IntersectionObserverEntry to native side, time: ${context.stopwatch!.elapsedMicroseconds}us');
  }

  // Free the allocated arguments.
  malloc.free(context.allocatedNativeArguments);
  malloc.free(context.rawEntries);
  malloc.free(returnValue);

  if (enableWebFProfileTracking) {
    WebFProfiler.instance.finishTrackEvaluate(context.profileOp!);
  }

  context.completer.complete();
}

class IntersectionObserver extends DynamicBindingObject {
  IntersectionObserver(BindingContext? context, List<dynamic> thresholds_) : super(context) {
    if (null != thresholds_) {
      debugPrint('Dom.IntersectionObserver.Constructor thresholds_:$thresholds_');
      _thresholds = thresholds_.map((e) => e as double).toList();
    }
  }

  @override
  void initializeMethods(Map<String, BindingObjectMethod> methods) {}

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {}

  void observe(Element element) {
    // debugPrint('Dom.IntersectionObserver.observe element:$element');
    if (!element.addIntersectionObserver(this, _thresholds)) {
      return;
    }
    _elementList.add(element);
  }

  void unobserve(Element element) {
    // debugPrint('Dom.IntersectionObserver.unobserve');
    _elementList.remove(element);
    element.removeIntersectionObserver(this);
  }

  void disconnect() {
    if (_elementList.isEmpty) return;
    for (var element in _elementList) {
      element!.removeIntersectionObserver(this);
    }
    _elementList.clear();
  }

  bool HasObservations() {
    return _elementList.isNotEmpty;
  }

  void addEntry(DartIntersectionObserverEntry entry) {
    _entries.add(entry);
  }

  List<DartIntersectionObserverEntry> takeRecords() {
    List<DartIntersectionObserverEntry> entries = _entries.map((entry) => entry.copy()).toList();
    _entries.clear();
    return entries;
  }

  Future<void> deliver(WebFController controller) async {
    if (pointer == null) return;

    List<DartIntersectionObserverEntry> entries = takeRecords();
    if (entries.isEmpty) {
      return;
    }
    // debugPrint('Dom.IntersectionObserver.deliver size:${entries.length}');
    Completer completer = Completer();

    EvaluateOpItem? currentProfileOp;
    if (enableWebFProfileTracking) {
      currentProfileOp = WebFProfiler.instance.startTrackEvaluate('_dispatchEventToNative');
    }

    BindingObject bindingObject = controller.view.getBindingObject(pointer!);
    // Call methods implements at C++ side.
    DartInvokeBindingMethodsFromDart? invokeBindingMethodsFromDart =
        pointer!.ref.invokeBindingMethodFromDart.asFunction();

    Stopwatch? stopwatch;
    if (enableWebFCommandLog) {
      stopwatch = Stopwatch()..start();
    }

    // Allocate an chunk of memory for an list of NativeIntersectionObserverEntry
    Pointer<NativeIntersectionObserverEntry> nativeEntries =
        malloc.allocate(sizeOf<NativeIntersectionObserverEntry>() * entries.length);

    // Write the native memory from dart objects.
    for (int i = 0; i < entries.length; i++) {
      (nativeEntries + i).ref.isIntersecting = entries[i].isIntersecting ? 1 : 0;
      (nativeEntries + i).ref.intersectionRatio = entries[i].intersectionRatio;
      (nativeEntries + i).ref.element = entries[i].element.pointer!;
    }

    List<dynamic> dispatchEntryArguments = [nativeEntries, entries.length];
    Pointer<NativeValue> allocatedNativeArguments = makeNativeValueArguments(bindingObject, dispatchEntryArguments);

    _IntersectionObserverDeliverContext context = _IntersectionObserverDeliverContext(
        completer, stopwatch, allocatedNativeArguments, nativeEntries, controller, currentProfileOp);

    Pointer<NativeFunction<NativeInvokeResultCallback>> resultCallback = Pointer.fromFunction(_handleDeliverResult);

    Future.microtask(() {
      invokeBindingMethodsFromDart(pointer!, currentProfileOp?.hashCode ?? 0, nullptr, dispatchEntryArguments.length,
          allocatedNativeArguments, context, resultCallback);
    });
    // debugPrint('Dom.IntersectionObserver.deliver this:$pointer end');
    return completer.future;
  }

  final List<DartIntersectionObserverEntry> _entries = [];
  final List<Element> _elementList = [];
  List<double> _thresholds = [0.0];
}
