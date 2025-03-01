/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'package:flutter/foundation.dart';
import 'package:webf/html.dart';
import 'package:webf/dom.dart';
import 'package:webf/foundation.dart';

typedef EventHandler = Future<void> Function(Event event);

abstract class EventTarget extends DynamicBindingObject {
  EventTarget(BindingContext? context) : super(context);

  bool _disposed = false;
  bool get disposed => _disposed;

  @protected
  final Map<String, List<EventHandler>> _eventHandlers = {};

  @protected
  final Map<String, List<EventHandler>> _eventCaptureHandlers = {};

  Map<String, List<EventHandler>> getEventHandlers() => _eventHandlers;

  Map<String, List<EventHandler>> getCaptureEventHandlers() => _eventCaptureHandlers;

  @protected
  bool hasEventListener(String type) => _eventHandlers.containsKey(type);

  // TODO: Support addEventListener options: capture, once, passive, signal.
  @mustCallSuper
  void addEventListener(String eventType, EventHandler eventHandler, {EventListenerOptions? addEventListenerOptions}) {
    if (_disposed) return;
    bool capture = false;
    if (addEventListenerOptions != null)
      capture = addEventListenerOptions.capture;
    List<EventHandler>? existHandler = capture ? _eventCaptureHandlers[eventType] : _eventHandlers[eventType];
    if (existHandler == null) {
      if (capture)
        _eventCaptureHandlers[eventType] = existHandler = [];
      else
        _eventHandlers[eventType] = existHandler = [];
    }
    existHandler.add(eventHandler);
  }

  @mustCallSuper
  void removeEventListener(String eventType, EventHandler eventHandler, {bool isCapture = false}) {
    if (_disposed) return;

    List<EventHandler>? currentHandlers = isCapture ? _eventCaptureHandlers[eventType] : _eventHandlers[eventType];
    if (currentHandlers != null) {
      currentHandlers.remove(eventHandler);
      if (currentHandlers.isEmpty) {
        if (isCapture) {
          _eventCaptureHandlers.remove(eventType);
        } else {
          _eventHandlers.remove(eventType);
        }
      }
    }
  }

  @mustCallSuper
  Future<void> dispatchEvent(Event event) async {
    if (_disposed) return;
    if (this is PseudoElement) {
      event.target = (this as PseudoElement).parent;
    } else {
      event.target = this;
    }

    await _handlerCaptureEvent(event);
    await _dispatchEventInDOM(event);
  }
  Future<void> _handlerCaptureEvent(Event event) async {
    await parentEventTarget?._handlerCaptureEvent(event);
    String eventType = event.type;
    List<EventHandler>? existHandler = _eventCaptureHandlers[eventType];
    if (existHandler != null) {
      // Modify currentTarget before the handler call, otherwise currentTarget may be modified by the previous handler.
      event.currentTarget = this;
      // To avoid concurrent exception while prev handler modify the original handler list, causing list iteration
      // with error, copy the handlers here.
      try {
        List<EventHandler> handlers = [...existHandler];
        for (int i = handlers.length - 1; i >= 0; i --) {
          final handler = handlers[i];
          await handler(event);
        }
      } catch (e, stack) {
        print('$e\n$stack');
      }
      event.currentTarget = null;
    }
  }
  // Refs: https://github.com/WebKit/WebKit/blob/main/Source/WebCore/dom/EventDispatcher.cpp#L85
  Future<void> _dispatchEventInDOM(Event event) async {
    // TODO: Invoke capturing event listeners in the reverse order.

    String eventType = event.type;
    List<EventHandler>? existHandler = _eventHandlers[eventType];
    if (existHandler != null) {
      // Modify currentTarget before the handler call, otherwise currentTarget may be modified by the previous handler.
      event.currentTarget = this;
      // To avoid concurrent exception while prev handler modify the original handler list, causing list iteration
      // with error, copy the handlers here.
      try {
        List<EventHandler> handlers = [...existHandler];
        for (int i = handlers.length - 1; i >= 0; i --) {
          final handler = handlers[i];
          await handler(event);
        }
      } catch (e, stack) {
        print('$e\n$stack');
      }
      event.currentTarget = null;
    }

    // Invoke bubbling event listeners.
    if (event.bubbles && !event.propagationStopped) {
      await parentEventTarget?._dispatchEventInDOM(event);
    }
  }

  @override
  @mustCallSuper
  void dispose() async {
    _disposed = true;
    _eventHandlers.clear();
    super.dispose();
  }

  EventTarget? get parentEventTarget;

  List<EventTarget> get eventPath {
    List<EventTarget> path = [];
    EventTarget? current = this;
    while (current != null) {
      path.add(current);
      current = current.parentEventTarget;
    }
    return path;
  }
}
class EventListenerOptions {

  bool capture;
  bool passive;
  bool once;

  EventListenerOptions(this.capture, this.passive, this.once);
}
