/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'dart:async';
import 'dart:collection';
import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';
import 'dart:ui' as ui;
import 'dart:ui';

import 'package:ffi/ffi.dart';
import 'package:flutter/animation.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart'
    show AnimationController, BuildContext, ModalRoute, RouteInformation, RouteObserver, View, Widget, WidgetsBinding, WidgetsBindingObserver;
import 'package:webf/css.dart';
import 'package:webf/dom.dart';
import 'package:webf/gesture.dart';
import 'package:webf/rendering.dart';
import 'package:webf/devtools.dart';
import 'package:webf/webf.dart';
import 'package:webf/src/dom/intersection_observer.dart';

// Error handler when load bundle failed.
typedef LoadErrorHandler = void Function(FlutterError error, StackTrace stack);
typedef LoadHandler = void Function(WebFController controller);
typedef TitleChangedHandler = void Function(String title);
typedef JSErrorHandler = void Function(String message);
typedef JSLogHandler = void Function(int level, String message);
typedef PendingCallback = void Function();
typedef OnCustomElementAttached = void Function(WebFWidgetElementToWidgetAdapter newWidget);
typedef OnCustomElementDetached = void Function(WebFWidgetElementToWidgetAdapter detachedWidget);

typedef TraverseElementCallback = void Function(Element element);

// Traverse DOM element.
void traverseElement(Element element, TraverseElementCallback callback) {
  callback(element);
  for (Element el in element.children) {
    traverseElement(el, callback);
  }
}

// See http://github.com/flutter/flutter/wiki/Desktop-shells
/// If the current platform is a desktop platform that isn't yet supported by
/// TargetPlatform, override the default platform to one that is.
/// Otherwise, do nothing.
/// No need to handle macOS, as it has now been added to TargetPlatform.
void setTargetPlatformForDesktop() {
  if (Platform.isLinux || Platform.isWindows) {
    debugDefaultTargetPlatformOverride = TargetPlatform.fuchsia;
  }
}

abstract class DevToolsService {
  /// Design prevDevTool for reload page,
  /// do not use it in any other place.
  /// More detail see [InspectPageModule.handleReloadPage].
  static DevToolsService? prevDevTools;

  static final Map<double, DevToolsService> _contextDevToolMap = {};
  static DevToolsService? getDevToolOfContextId(double contextId) {
    return _contextDevToolMap[contextId];
  }

  /// Used for debugger inspector.
  UIInspector? _uiInspector;
  UIInspector? get uiInspector => _uiInspector;

  late Isolate _isolateServer;
  Isolate get isolateServer => _isolateServer;
  set isolateServer(Isolate isolate) {
    _isolateServer = isolate;
  }

  SendPort? _isolateServerPort;
  SendPort? get isolateServerPort => _isolateServerPort;
  set isolateServerPort(SendPort? value) {
    _isolateServerPort = value;
  }

  WebFController? _controller;
  WebFController? get controller => _controller;

  void init(WebFController controller) {
    _contextDevToolMap[controller.view.contextId] = this;
    _controller = controller;
    spawnIsolateInspectorServer(this, controller);
    _uiInspector = UIInspector(this);
    controller.view.debugDOMTreeChanged = uiInspector!.onDOMTreeChanged;
  }

  bool get isReloading => _reloading;
  bool _reloading = false;

  void willReload() {
    _reloading = true;
  }

  void didReload() {
    _reloading = false;
    controller!.view.debugDOMTreeChanged = _uiInspector!.onDOMTreeChanged;
    _isolateServerPort!.send(InspectorReload(_controller!.view.contextId));
  }

  void dispose() {
    _uiInspector?.dispose();
    _contextDevToolMap.remove(controller!.view.contextId);
    _controller = null;
    _isolateServerPort = null;
    _isolateServer.kill();
  }
}

enum WebFLoadingMode {
  /// The default loading mode.
  /// All associated page resources begin loading once the WebF widget is mounted into the Flutter tree.
  standard,

  /// This mode preloads remote resources into memory and begins execution when the WebF widget is mounted into the Flutter tree.
  /// If the entrypoint is an HTML file, the HTML will be parsed, and its elements will be organized into a DOM tree.
  /// CSS files loaded through `<style>` and `<link>` elements will be parsed and the calculated styles applied to the corresponding DOM elements.
  /// However, JavaScript code will not be executed in this mode.
  /// If the entrypoint is a JavaScript file, WebF only do loading until the WebF widget is mounted into the Flutter tree.
  /// Using this mode can save up to 50% of loading time, while maintaining a high level of compatibility with the standard mode.
  /// It's safe and recommended to use this mode for all types of pages.
  preloading,

  /// The `aggressive` mode is a step further than `preloading`, cutting down up to 90% of loading time for optimal performance.
  /// This mode simulates the instantaneous response of native Flutter pages but may require modifications in the existing web codes for compatibility.
  /// In this mode, all remote resources are loaded and executed similarly to the standard mode, but with an offline-like behavior.
  /// Given that JavaScript is executed in this mode, properties like `clientWidth` and `clientHeight` from the CSSOM-view spec always return 0. This is because
  /// no layout or paint processes occur during preRendering.
  /// If your application depends on CSSOM-view properties to work, ensure that the related code is placed within the `load` and `DOMContentLoaded` event callbacks of the window.
  /// These callbacks are triggered once the WebF widget is mounted into the Flutter tree.
  /// Apps optimized for this mode remain compatible with both `standard` and `preloading` modes.
  preRendering
}

enum PreloadingStatus {
  none,
  preloading,
  done,
}

enum PreRenderingStatus {
  none,
  preloading,
  evaluate,
  rendering,
  done
}

// An kraken View Controller designed for multiple kraken view control.
class WebFViewController implements WidgetsBindingObserver {
  WebFController rootController;

  // The methods of the KrakenNavigateDelegation help you implement custom behaviors that are triggered
  // during a kraken view's process of loading, and completing a navigation request.
  WebFNavigationDelegate? navigationDelegate;

  GestureListener? gestureListener;

  List<Cookie>? initialCookies;

  final List<List<UICommand>> pendingUICommands = [];

  Color? background;
  WebFThread runningThread;

  bool firstLoad = true;

  WebFViewController(
      {this.background,
      this.enableDebug = false,
      required this.rootController,
      required this.runningThread,
      this.navigationDelegate,
      this.gestureListener,
      this.initialCookies}) {
  }

  Future<void> initialize() async {
    if (enableDebug) {
      debugDefaultTargetPlatformOverride = TargetPlatform.fuchsia;
      debugPaintSizeEnabled = true;
    }

    _contextId = await initBridge(this, runningThread);

    _setupObserver();

    defineBuiltInElements();
  }

  bool _isAnimationTimelineStopped = false;
  bool get isAnimationTimelineStopped => _isAnimationTimelineStopped;
  final List<VoidCallback> _pendingAnimationTimesLines = [];

  void stopAnimationsTimeLine() {
    _isAnimationTimelineStopped = true;
  }
  void addPendingAnimationTimeline(VoidCallback callback) {
    _pendingAnimationTimesLines.add(callback);
  }
  void resumeAnimationTimeline() {
    _pendingAnimationTimesLines.forEach((callback) {
      callback();
    });
    _pendingAnimationTimesLines.clear();
    _isAnimationTimelineStopped = false;
  }

  bool _isFrameBindingAttached = false;

  void flushPendingCommandsPerFrame() {
    if (disposed && _isFrameBindingAttached) return;
    _isFrameBindingAttached = true;
    flushUICommand(this, window.pointer!);
    deliverIntersectionObserver();
    SchedulerBinding.instance.addPostFrameCallback((_) => flushPendingCommandsPerFrame());
  }

  final Map<String, Widget> _hybridRouterViews = {};

  void setHybridRouterView(String path, Widget root) {
    assert(!_hybridRouterViews.containsKey(path));
    _hybridRouterViews[path] = root;
  }
  Widget? getHybridRouterView(String path) {
    return _hybridRouterViews[path];
  }
  void removeHybridRouterView(String path) {
    _hybridRouterViews.remove(path);
  }

  RenderViewportBox? _activeRouterRoot;
  RenderViewportBox? get activeRouterRoot => _activeRouterRoot;
  set activeRouterRoot(RenderViewportBox? root) {
    _activeRouterRoot = root;
  }

  final Map<int, BindingObject> _nativeObjects = {};

  T? getBindingObject<T>(Pointer pointer) {
    return _nativeObjects[pointer.address] as T?;
  }

  bool hasBindingObject(Pointer pointer) {
    return _nativeObjects.containsKey(pointer.address);
  }

  void setBindingObject(Pointer pointer, BindingObject bindingObject) {
    assert(!_nativeObjects.containsKey(pointer.address));
    _nativeObjects[pointer.address] = bindingObject;
  }

  void removeBindingObject(Pointer pointer) {
    _nativeObjects.remove(pointer.address);
  }

  // fix New version of chrome devTools castrating the last three digits of long targetId num strings and replacing them with 0
  int _nodeIdCount = 0;
  final Map<int, int> _targetIdToDevNodeIdMap = {};
  Map<int, int> get targetIdToDevNodeIdMap => _targetIdToDevNodeIdMap;

  int getTargetIdByNodeId(int? address) {
    if (address == null) {
      return 0;
    }
    int targetId = targetIdToDevNodeIdMap.keys.firstWhere((k) => targetIdToDevNodeIdMap[k] == address, orElse: () => 0);
    return targetId;
  }

  void disposeTargetIdToDevNodeIdMap(BindingObject? object) {
    _targetIdToDevNodeIdMap.remove(object?.pointer?.address);
  }

  int forDevtoolsNodeId(BindingObject object) {
    int? nativeAddress = object.pointer?.address;
    if (nativeAddress != null) {
      if (targetIdToDevNodeIdMap[nativeAddress] != null) {
        return targetIdToDevNodeIdMap[nativeAddress]!;
      }
      _nodeIdCount ++;
      targetIdToDevNodeIdMap[nativeAddress] = _nodeIdCount;
      return _nodeIdCount;
    }
    return 0;
  }
  // fix New version of chrome devTools end

  // Index value which identify javascript runtime context.
  late double _contextId;
  double get contextId => _contextId;

  // Enable print debug message when rendering.
  bool enableDebug;

  // Kraken have already disposed.
  bool _disposed = false;

  bool get disposed => _disposed;

  RenderViewportBox? viewport;
  late Document document;
  late Window window;

  void initDocument(view, Pointer<NativeBindingObject> pointer) {
    document = Document(
      BindingContext(view, _contextId, pointer),
      controller: rootController,
      gestureListener: gestureListener,
      initialCookies: initialCookies,
    );

    // Listeners need to be registered to window in order to dispatch events on demand.
    if (gestureListener != null) {
      GestureListener listener = gestureListener!;
      if (listener.onTouchStart != null) {
        document.addEventListener(EVENT_TOUCH_START, (Event event) async => listener.onTouchStart!(event as TouchEvent));
      }

      if (listener.onTouchMove != null) {
        document.addEventListener(EVENT_TOUCH_MOVE, (Event event) async => listener.onTouchMove!(event as TouchEvent));
      }

      if (listener.onTouchEnd != null) {
        document.addEventListener(EVENT_TOUCH_END, (Event event) async => listener.onTouchEnd!(event as TouchEvent));
      }

      if (listener.onDrag != null) {
        document.addEventListener(EVENT_DRAG, (Event event) async => listener.onDrag!(event as GestureEvent));
      }
    }

    firstLoad = false;
  }

  void initWindow(WebFViewController view, Pointer<NativeBindingObject> pointer) {
    window = Window(BindingContext(view, _contextId, pointer), document);
    _registerPlatformBrightnessChange();

    // 3 seconds should be enough for page loading, make sure the JavaScript GC was opened.
    Timer(Duration(seconds: 3), () {
      window.dispatchEvent(Event('gcopen'));
    });

    // Blur input element when new input focused.
    window.addEventListener(EVENT_CLICK, (event) async {
      if (event.target is Element) {
        Element? focusedElement = document.focusedElement;
        if (focusedElement != null && focusedElement != event.target) {
          document.focusedElement!.blur();
        }
        (event.target as Element).focus();
      }
    });
  }

  void setCookie(List<Cookie> cookies, [Uri? uri]) {
    document.cookie.setCookie(cookies, uri);
  }

  void clearCookie() {
    document.cookie.clearCookie();
  }

  Future<void> evaluateJavaScripts(String code) async {
    assert(!_disposed, 'WebF have already disposed');
    List<int> data = utf8.encode(code);
    await evaluateScripts(_contextId, Uint8List.fromList(data));
  }

  void _setupObserver() {
    WidgetsBinding.instance.addObserver(this);
  }

  void _teardownObserver() {
    WidgetsBinding.instance.removeObserver(this);
  }

  // Attach kraken's renderObject to an renderObject.
  void attachTo(RenderObject parent, [RenderObject? previousSibling]) {
    if (parent is ContainerRenderObjectMixin) {
      parent.insert(document.renderer!, after: previousSibling);
    } else if (parent is RenderObjectWithChildMixin) {
      parent.child = document.renderer;
    }
  }

  // Dispose controller and recycle all resources.
  Future<void> dispose() async {
    await waitingSyncTaskComplete(contextId);
    _disposed = true;
    debugDOMTreeChanged = null;

    _teardownObserver();
    _unregisterPlatformBrightnessChange();

    // Should clear previous page cached ui commands
    clearUICommand(_contextId);

    await disposePage(runningThread is FlutterUIThread, _contextId);

    clearCssLength();

    _nativeObjects.forEach((key, object) {
      object.dispose();
    });
    _nativeObjects.clear();

    document.dispose();
    window.dispose();

    _targetIdToDevNodeIdMap.clear();
  }

  VoidCallback? _originalOnPlatformBrightnessChanged;

  void _registerPlatformBrightnessChange() {
    _originalOnPlatformBrightnessChanged =
        rootController.ownerFlutterView.platformDispatcher.onPlatformBrightnessChanged;
    rootController.ownerFlutterView.platformDispatcher.onPlatformBrightnessChanged = _onPlatformBrightnessChanged;
  }

  void _unregisterPlatformBrightnessChange() {
    rootController.ownerFlutterView.platformDispatcher.onPlatformBrightnessChanged =
        _originalOnPlatformBrightnessChanged;
    _originalOnPlatformBrightnessChanged = null;
  }

  void _onPlatformBrightnessChanged() {
    if (_originalOnPlatformBrightnessChanged != null) {
      _originalOnPlatformBrightnessChanged!();
    }
    window.dispatchEvent(ColorSchemeChangeEvent(window.colorScheme));
  }

  // export Uint8List bytes from rendered result.
  Future<Uint8List> toImage(double devicePixelRatio, [Pointer<Void>? eventTargetPointer]) {
    assert(!_disposed, 'WebF have already disposed');
    Completer<Uint8List> completer = Completer();
    try {
      if (eventTargetPointer != null && !hasBindingObject(eventTargetPointer)) {
        String msg = 'toImage: unknown node id: $eventTargetPointer';
        completer.completeError(Exception(msg));
        return completer.future;
      }
      var node = eventTargetPointer == null ? document.documentElement : getBindingObject(eventTargetPointer);
      if (node is Element) {
        if (!node.isRendererAttached) {
          String msg = 'toImage: the element is not attached to document tree.';
          completer.completeError(Exception(msg));
          return completer.future;
        }

        node.toBlob(devicePixelRatio: devicePixelRatio).then((Uint8List bytes) {
          completer.complete(bytes);
        }).catchError((e, stack) {
          String msg = 'toBlob: failed to export image data from element id: $eventTargetPointer. error: $e}.\n$stack';
          completer.completeError(Exception(msg));
        });
      } else {
        String msg = 'toBlob: node is not an element, id: $eventTargetPointer';
        completer.completeError(Exception(msg));
      }
    } catch (e, stack) {
      completer.completeError(e, stack);
    }
    return completer.future;
  }

  void createElement(Pointer<NativeBindingObject> nativePtr, String tagName) {
    assert(!hasBindingObject(nativePtr), 'ERROR: Can not create element with same id "$nativePtr"');
    document.createElement(tagName.toUpperCase(), BindingContext(document.controller.view, _contextId, nativePtr));
  }

  void createElementNS(Pointer<NativeBindingObject> nativePtr, String uri, String tagName) {
    assert(!hasBindingObject(nativePtr), 'ERROR: Can not create element with same id "$nativePtr"');
    document.createElementNS(
        uri, tagName.toUpperCase(), BindingContext(document.controller.view, _contextId, nativePtr));
  }

  void createTextNode(Pointer<NativeBindingObject> nativePtr, String data) {
    document.createTextNode(data, BindingContext(document.controller.view, _contextId, nativePtr));
  }

  void createComment(Pointer<NativeBindingObject> nativePtr) {
    document.createComment(BindingContext(document.controller.view, _contextId, nativePtr));
  }

  void createDocumentFragment(Pointer<NativeBindingObject> nativePtr) {
    document.createDocumentFragment(BindingContext(document.controller.view, _contextId, nativePtr));
  }

  void addIntersectionObserver(
      Pointer<NativeBindingObject> observerPointer, Pointer<NativeBindingObject> elementPointer) {
    debugPrint('Dom.IntersectionObserver.observe');
    assert(hasBindingObject(observerPointer), 'observer: $observerPointer');
    assert(hasBindingObject(elementPointer), 'element: $elementPointer');

    IntersectionObserver? observer = getBindingObject<IntersectionObserver>(observerPointer);
    Element? element = getBindingObject<Element>(elementPointer);
    if (nullptr == observer || nullptr == element) {
      return;
    }

    document.addIntersectionObserver(observer!, element!);
  }

  void removeIntersectionObserver(
      Pointer<NativeBindingObject> observerPointer, Pointer<NativeBindingObject> elementPointer) {
    assert(hasBindingObject(observerPointer), 'observer: $observerPointer');
    assert(hasBindingObject(elementPointer), 'element: $elementPointer');

    IntersectionObserver? observer = getBindingObject<IntersectionObserver>(observerPointer);
    Element? element = getBindingObject<Element>(elementPointer);
    if (nullptr == observer || nullptr == element) {
      return;
    }

    document.removeIntersectionObserver(observer!, element!);
  }

  void disconnectIntersectionObserver(Pointer<NativeBindingObject> observerPointer) {
    assert(hasBindingObject(observerPointer), 'observer: $observerPointer');

    IntersectionObserver? observer = getBindingObject<IntersectionObserver>(observerPointer);
    if (nullptr == observer) {
      return;
    }

    document.disconnectIntersectionObserver(observer!);
  }

  void deliverIntersectionObserver() {
    document.deliverIntersectionObserver();
  }

  void addEvent(Pointer<NativeBindingObject> nativePtr, String eventType,
      {Pointer<AddEventListenerOptions>? addEventListenerOptions}) {
    if (!hasBindingObject(nativePtr)) return;
    EventTarget? target = getBindingObject<EventTarget>(nativePtr);
    if (target != null) {
      BindingBridge.listenEvent(target, eventType, addEventListenerOptions: addEventListenerOptions);
    }
  }

  void removeEvent(Pointer<NativeBindingObject> nativePtr, String eventType, {bool isCapture = false}) {
    if (!hasBindingObject(nativePtr)) return;
    EventTarget? target = getBindingObject<EventTarget>(nativePtr);
    if (target != null) {
      BindingBridge.unlistenEvent(target, eventType, isCapture: isCapture);
    }
  }

  void cloneNode(Pointer<NativeBindingObject> selfPtr, Pointer<NativeBindingObject> newPtr) {
    assert(hasBindingObject(selfPtr));
    assert(hasBindingObject(newPtr));

    EventTarget? originalTarget = getBindingObject<EventTarget>(selfPtr);
    EventTarget? newTarget = getBindingObject<EventTarget>(newPtr);

    if (originalTarget == null || newTarget == null) return;


    // Current only element clone will process in dart.
    if (originalTarget is Element) {
      Element newElement = newTarget as Element;
      // Copy inline style.
      originalTarget.inlineStyle.forEach((key, value) {
        newElement.setInlineStyle(key, value);
      });
      // Copy element attributes.
      originalTarget.attributes.forEach((key, value) {
        newElement.setAttribute(key, value);
      });
      newElement.className = originalTarget.className;
      newElement.id = originalTarget.id;
    }
  }

  void removeNode(Pointer pointer) {
    assert(hasBindingObject(pointer), 'pointer: $pointer');

    Node? target = getBindingObject<Node>(pointer);
    target?.parentNode?.removeChild(target);

    _debugDOMTreeChanged();
  }

  /// <!-- beforebegin -->
  /// <p>
  ///   <!-- afterbegin -->
  ///   foo
  ///   <!-- beforeend -->
  /// </p>
  /// <!-- afterend -->
  void insertAdjacentNode(
      Pointer<NativeBindingObject> selfPointer, String position, Pointer<NativeBindingObject> newPointer) {
    assert(hasBindingObject(selfPointer), 'targetId: $selfPointer position: $position newTargetId: $newPointer');
    assert(hasBindingObject(selfPointer), 'newTargetId: $newPointer position: $position');

    Node? target = getBindingObject<Node>(selfPointer);
    Node? newNode = getBindingObject<Node>(newPointer);

    if (target == null || newNode == null) {
      return;
    }

    Node? targetParentNode = target.parentNode;

    switch (position) {
      case 'beforebegin':
        targetParentNode!.insertBefore(newNode, target);
        break;
      case 'afterbegin':
        target.insertBefore(newNode, target.firstChild!);
        break;
      case 'beforeend':
        target.appendChild(newNode);
        break;
      case 'afterend':
        if (targetParentNode!.lastChild == target) {
          targetParentNode.appendChild(newNode);
        } else {
          targetParentNode.insertBefore(newNode, target.nextSibling!);
        }
        break;
    }

    _debugDOMTreeChanged();
  }

  void setAttribute(Pointer<NativeBindingObject> selfPtr, String key, String value) {
    assert(hasBindingObject(selfPtr), 'selfPtr: $selfPtr key: $key value: $value');
    Node? target = getBindingObject<Node>(selfPtr);
    if (target == null) return;

    if (target is Element) {
      // Only element has properties.
      target.setAttribute(key, value);
    } else if (target is TextNode && (key == 'data' || key == 'nodeValue')) {
      target.data = value;
    } else {
      debugPrint('Only element has properties, try setting $key to Node(#$selfPtr).');
    }
  }

  String? getAttribute(Pointer selfPtr, String key) {
    assert(hasBindingObject(selfPtr), 'targetId: $selfPtr key: $key');
    Node? target = getBindingObject<Node>(selfPtr);
    if (target == null) return null;

    if (target is Element) {
      // Only element has attributes.
      return target.getAttribute(key);
    } else if (target is TextNode && (key == 'data' || key == 'nodeValue')) {
      // @TODO: property is not attribute.
      return target.data;
    } else {
      return null;
    }
  }

  void removeAttribute(Pointer selfPtr, String key) {
    assert(hasBindingObject(selfPtr), 'targetId: $selfPtr key: $key');
    Node? target = getBindingObject<Node>(selfPtr);
    if (target == null) return;

    if (target is Element) {
      target.removeAttribute(key);
    } else if (target is TextNode && (key == 'data' || key == 'nodeValue')) {
      // @TODO: property is not attribute.
      target.data = '';
    } else {
      debugPrint('Only element has attributes, try removing $key from Node(#$selfPtr).');
    }
  }

  void setInlineStyle(Pointer selfPtr, String key, String value) {
    assert(hasBindingObject(selfPtr), 'id: $selfPtr key: $key value: $value');
    Node? target = getBindingObject<Node>(selfPtr);
    if (target == null) return;

    if (target is Element) {
      target.setInlineStyle(key, value);
    } else {
      debugPrint('Only element has style, try setting style.$key from Node(#$selfPtr).');
    }
  }

  void clearInlineStyle(Pointer selfPtr) {
    assert(hasBindingObject(selfPtr), 'id: $selfPtr');
    Node? target = getBindingObject<Node>(selfPtr);
    if (target == null) return;

    if (target is Element) {
      target.clearInlineStyle();
    } else {
      debugPrint('Only element has style, try clear style from Node(#$selfPtr).');
    }
  }

  void flushPendingStyleProperties(int address) {
    if (!hasBindingObject(Pointer.fromAddress(address))) return;
    Node? target = getBindingObject<Node>(Pointer.fromAddress(address));
    if (target == null) return;

    if (target is Element) {
      target.style.flushPendingProperties();
    } else {
      debugPrint(
          'Only element has style, try flushPendingStyleProperties from Node(#${Pointer.fromAddress(address)}).');
    }
  }

  // Hooks for DevTools.
  VoidCallback? debugDOMTreeChanged;

  void _debugDOMTreeChanged() {
    VoidCallback? f = debugDOMTreeChanged;
    if (f != null) {
      f();
    }
  }

  Future<void> handleNavigationAction(String? sourceUrl, String targetUrl, WebFNavigationType navigationType) async {
    WebFNavigationAction action = WebFNavigationAction(sourceUrl, targetUrl, navigationType);

    WebFNavigationDelegate _delegate = navigationDelegate!;

    try {
      WebFNavigationActionPolicy policy = await _delegate.dispatchDecisionHandler(action);
      if (policy == WebFNavigationActionPolicy.cancel) return;

      String targetPath = action.target;

      if (!Uri.parse(targetPath).isAbsolute) {
        String base = rootController.url;
        targetPath = rootController.uriParser!.resolve(Uri.parse(base), Uri.parse(targetPath)).toString();
      }

      if (action.target.trim().startsWith('#')) {
        String oldUrl = rootController.url;
        HistoryModule historyModule = rootController.module.moduleManager.getModule('History')!;
        historyModule.pushState(null, url: targetPath);
        await window.dispatchEvent(HashChangeEvent(newUrl: targetPath, oldUrl: oldUrl));
        return;
      }

      switch (action.navigationType) {
        case WebFNavigationType.navigate:
          await rootController.load(rootController.getPreloadBundleFromUrl(targetPath) ?? WebFBundle.fromUrl(targetPath));
          break;
        case WebFNavigationType.reload:
          await rootController.reload();
          break;
        default:
        // Navigate and other type, do nothing.
      }
    } catch (e, stack) {
      if (_delegate.errorHandler != null) {
        _delegate.errorHandler!(e, stack);
      } else {
        print('WebF navigation failed: $e\n$stack');
      }
    }
  }

  // Call from JS Bridge when the BindingObject class on the JS side had been Garbage collected.
  void disposeBindingObject(WebFViewController view, Pointer<NativeBindingObject> pointer) async {
    BindingObject? bindingObject = getBindingObject(pointer);
    bindingObject?.dispose();
    view.removeBindingObject(pointer);
    view.disposeTargetIdToDevNodeIdMap(bindingObject);
    malloc.free(pointer);
  }

  RenderBox? getRootRenderObject() {
    return document.documentElement?.renderer;
  }

  @override
  void didChangeAccessibilityFeatures() {}

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (firstLoad) return;
    switch (state) {
      case AppLifecycleState.resumed:
        document.visibilityChange(VisibilityState.visible);
        rootController.resume();
        break;
      case AppLifecycleState.hidden:
      case AppLifecycleState.paused:
        document.visibilityChange(VisibilityState.hidden);
        rootController.pause();
        break;
      case AppLifecycleState.inactive:
        break;
      case AppLifecycleState.detached:
        rootController.pause();
        break;
    }
  }

  @override
  void didChangeLocales(List<Locale>? locales) {}

  static double FOCUS_VIEWINSET_BOTTOM_OVERALL = 32;

  @override
  void didChangeMetrics() {
    final ownerView = rootController.ownerFlutterView;
    final bool resizeToAvoidBottomInsets = rootController.resizeToAvoidBottomInsets;
    final double bottomInsets;
    if (resizeToAvoidBottomInsets) {
      bottomInsets = ownerView.viewInsets.bottom / ownerView.devicePixelRatio;
    } else {
      bottomInsets = 0;
    }

    if (resizeToAvoidBottomInsets && viewport?.hasSize == true) {
      bool shouldScrollByToCenter = false;
      Element? focusedElement = document.focusedElement;
      double scrollOffset = 0;
      if (focusedElement != null) {
        RenderBox? renderer = focusedElement.renderer;
        if (renderer != null && renderer.attached && renderer.hasSize) {
          Offset focusOffset = renderer.localToGlobal(Offset.zero);
          // FOCUS_VIEWINSET_BOTTOM_OVERALL to meet border case.
          if (focusOffset.dy > viewport!.size.height - bottomInsets - FOCUS_VIEWINSET_BOTTOM_OVERALL) {
            shouldScrollByToCenter = true;
            scrollOffset =
                focusOffset.dy - (viewport!.size.height - bottomInsets) + renderer.size.height + FOCUS_VIEWINSET_BOTTOM_OVERALL;
          }
        }
      }
      // Show keyboard
      if (shouldScrollByToCenter) {
        window.scrollBy(0, scrollOffset, false);
      }
    }
    window.resizeViewportRelatedElements();
    viewport?.bottomInset = bottomInsets;
  }

  @override
  void didChangePlatformBrightness() {}

  @override
  void didChangeTextScaleFactor() {}

  @override
  void didHaveMemoryPressure() {}

  @override
  Future<bool> didPopRoute() async {
    return false;
  }

  @override
  Future<bool> didPushRoute(String route) async {
    return false;
  }

  @override
  Future<bool> didPushRouteInformation(RouteInformation routeInformation) async {
    return false;
  }

  @override
  Future<ui.AppExitResponse> didRequestAppExit() async {
    return ui.AppExitResponse.exit;
  }

  @override
  void handleCancelBackGesture() {
  }

  @override
  void handleCommitBackGesture() {
  }

  @override
  bool handleStartBackGesture(backEvent) {
    return true;
  }

  @override
  void handleUpdateBackGestureProgress(backEvent) {
  }

  @override
  void didChangeViewFocus(event) {}
}

// An controller designed to control kraken's functional modules.
class WebFModuleController with TimerMixin, ScheduleFrameMixin {
  late ModuleManager _moduleManager;

  ModuleManager get moduleManager => _moduleManager;

  WebFModuleController(WebFController controller, double contextId) {
    _moduleManager = ModuleManager(controller, contextId);
  }

  bool _initialized = false;
  Future<void> initialize() async {
    if (_initialized) return;
    await _moduleManager.initialize();
    _initialized = true;
  }

  void dispose() {
    disposeTimer();
    disposeScheduleFrame();
    _moduleManager.dispose();
  }
}

class WebFController {
  static final Map<double, WebFController?> _controllerMap = {};
  static final Map<String, double> _nameIdMap = {};

  UriParser? uriParser;
  WebFLoadingMode mode = WebFLoadingMode.standard;

  bool get isPreLoadingOrPreRenderingComplete => preloadStatus == PreloadingStatus.done || preRenderingStatus == PreRenderingStatus.done;

  static WebFController? getControllerOfJSContextId(double? contextId) {
    if (!_controllerMap.containsKey(contextId)) {
      return null;
    }

    return _controllerMap[contextId];
  }

  static Map<double, WebFController?> getControllerMap() {
    return _controllerMap;
  }

  static WebFController? getControllerOfName(String name) {
    if (!_nameIdMap.containsKey(name)) return null;
    double? contextId = _nameIdMap[name];
    return getControllerOfJSContextId(contextId);
  }

  GestureDispatcher gestureDispatcher = GestureDispatcher();

  LoadHandler? onLoad;
  LoadHandler? onDOMContentLoaded;
  TitleChangedHandler? onTitleChanged;

  // Error handler when load bundle failed.
  LoadErrorHandler? onLoadError;

  // Error handler when got javascript error when evaluate javascript codes.
  JSErrorHandler? onJSError;

  final DevToolsService? devToolsService;
  final HttpClientInterceptor? httpClientInterceptor;

  WebFMethodChannel? _methodChannel;

  WebFMethodChannel? get methodChannel => _methodChannel;

  JSLogHandler? _onJSLog;

  JSLogHandler? get onJSLog => _onJSLog;

  set onJSLog(JSLogHandler? jsLogHandler) {
    _onJSLog = jsLogHandler;
  }

  // Internal usable. Notifications to WebF widget when custom element had changed.
  OnCustomElementAttached? onCustomElementAttached;
  OnCustomElementDetached? onCustomElementDetached;

  final List<Cookie>? initialCookies;

  final ui.FlutterView ownerFlutterView;

  List<BuildContext> buildContextStack = [];
  bool resizeToAvoidBottomInsets;

  String? _name;

  String? get name => _name;

  set name(String? value) {
    if (value == null) return;
    if (_name != null) {
      double? contextId = _nameIdMap[_name];
      if (contextId == null) return;
      _nameIdMap.remove(_name);
      _nameIdMap[value] = contextId;
    }
    _name = value;
  }

  final GestureListener? _gestureListener;

  final List<WebFBundle>? preloadedBundles;
  Map<String, WebFBundle>? _preloadBundleIndex;

  WebFBundle? getPreloadBundleFromUrl(String url) {
    return _preloadBundleIndex?[url];
  }

  void _initializePreloadBundle() {
    if (preloadedBundles == null) return;
    _preloadBundleIndex = {};
    preloadedBundles!.forEach((bundle) {
      _preloadBundleIndex![bundle.url] = bundle;
    });
  }

  /// Register the RouteObserver to observer page navigation.
  /// This is useful if you wants to pause webf timers and callbacks when webf widget are hidden by page route.
  /// https://api.flutter.dev/flutter/widgets/RouteObserver-class.html
  final RouteObserver<ModalRoute<void>>? routeObserver;

  // The kraken view entrypoint bundle.
  WebFBundle? _entrypoint;
  WebFBundle? get entrypoint => _entrypoint;

  final WebFThread runningThread;

  Completer controlledInitCompleter = Completer();
  Completer controllerPreloadingCompleter = Completer();
  Completer controllerPreRenderingCompleter = Completer();

  bool externalController;

  WebFController(BuildContext context, {
    String? name,
    double? viewportWidth,
    double? viewportHeight,
    bool showPerformanceOverlay = false,
    bool enableDebug = false,
    bool autoExecuteEntrypoint = true,
    Color? background,
    GestureListener? gestureListener,
    WebFNavigationDelegate? navigationDelegate,
    WebFMethodChannel? methodChannel,
    WebFBundle? bundle,
    WebFThread? runningThread,
    this.onCustomElementAttached,
    this.onCustomElementDetached,
    this.onLoad,
    this.onDOMContentLoaded,
    this.onLoadError,
    this.onJSError,
    this.httpClientInterceptor,
    this.devToolsService,
    this.uriParser,
    this.preloadedBundles,
    this.initialCookies,
    this.routeObserver,
    this.externalController = true,
    this.resizeToAvoidBottomInsets = true,
  })  : _name = name,
        _entrypoint = bundle,
        _gestureListener = gestureListener,
        runningThread = runningThread ?? DedicatedThread(),
        ownerFlutterView = View.of(context) {
    _initializePreloadBundle();
    if (enableWebFProfileTracking) {
      WebFProfiler.initialize();
    }

    _methodChannel = methodChannel;
    WebFMethodChannel.setJSMethodCallCallback(this);

    PaintingBinding.instance.systemFonts.addListener(_watchFontLoading);

    _view = WebFViewController(
      background: background,
      enableDebug: enableDebug,
      rootController: this,
      runningThread: this.runningThread,
      navigationDelegate: navigationDelegate ?? WebFNavigationDelegate(),
      gestureListener: _gestureListener,
      initialCookies: initialCookies
    );

    _view.initialize().then((_) {
      final double contextId = _view.contextId;

      _module = WebFModuleController(this, contextId);

      if (bundle != null) {
        HistoryModule historyModule = module.moduleManager.getModule<HistoryModule>('History')!;
        historyModule.add(bundle);
      }

      assert(!_controllerMap.containsKey(contextId), 'found exist contextId of WebFController, contextId: $contextId');
      _controllerMap[contextId] = this;
      assert(!_nameIdMap.containsKey(name), 'found exist name of WebFController, name: $name');
      if (name != null) {
        _nameIdMap[name] = contextId;
      }

      setupHttpOverrides(httpClientInterceptor, contextId: contextId);

      uriParser ??= UriParser();

      if (devToolsService != null) {
        devToolsService!.init(this);
      }

      controlledInitCompleter.complete();
    }).then((_) {
      if (externalController && _entrypoint != null) {
        preload(_entrypoint!);
      }
    });
  }

  late WebFViewController _view;

  WebFViewController get view {
    return _view;
  }

  late WebFModuleController _module;

  WebFModuleController get module {
    return _module;
  }

  final Queue<HistoryItem> previousHistoryStack = Queue();
  final Queue<HistoryItem> nextHistoryStack = Queue();

  final Map<String, String> sessionStorage = {};

  HistoryModule get history => _module.moduleManager.getModule('History')!;
  HistoryModule get hybridHistory => _module.moduleManager.getModule('HybridHistory')!;

  static Uri fallbackBundleUri([double? id]) {
    // The fallback origin uri, like `vm://bundle/0`
    return Uri(scheme: 'vm', host: 'bundle', path: id != null ? '$id' : null);
  }

  void setNavigationDelegate(WebFNavigationDelegate delegate) {
    _view.navigationDelegate = delegate;
  }

  bool isFontsLoading = false;
  void _watchFontLoading() {
    isFontsLoading = true;
    SchedulerBinding.instance.scheduleFrameCallback((_) {
      isFontsLoading = false;
    });
  }

  Future<void> unload() async {
    assert(!_view._disposed, 'WebF have already disposed');
    // Should clear previous page cached ui commands
    clearUICommand(_view.contextId);

    await controlledInitCompleter.future;

    // Wait for next microtask to make sure C++ native Elements are GC collected.
    Completer completer = Completer();
    Future.microtask(() async {
      _module.dispose();
      await _view.dispose();
      // RenderViewportBox will not disposed when reload, just remove all children and clean all resources.
      _view.viewport?.reload();

      double oldId = _view.contextId;

      _view = WebFViewController(
          background: _view.background,
          enableDebug: _view.enableDebug,
          rootController: this,
          navigationDelegate: _view.navigationDelegate,
          gestureListener: _view.gestureListener,
          runningThread: runningThread);

      await _view.initialize();

      _module = WebFModuleController(this, _view.contextId);

      // Reconnect the new contextId to the Controller
      _controllerMap.remove(oldId);
      _controllerMap[_view.contextId] = this;
      if (name != null) {
        _nameIdMap[name!] = _view.contextId;
      }

      completer.complete();
    });

    return completer.future;
  }

  String? get _url {
    HistoryModule historyModule = module.moduleManager.getModule<HistoryModule>('History')!;
    return historyModule.stackTop?.url;
  }

  Uri? get _uri {
    HistoryModule historyModule = module.moduleManager.getModule<HistoryModule>('History')!;
    return historyModule.stackTop?.resolvedUri;
  }

  String get url => _url ?? '';

  Uri? get uri => _uri;

  _addHistory(WebFBundle bundle) {
    HistoryModule historyModule = module.moduleManager.getModule<HistoryModule>('History')!;
    historyModule.add(bundle);
  }

  void _replaceCurrentHistory(WebFBundle bundle) {
    HistoryModule historyModule = module.moduleManager.getModule<HistoryModule>('History')!;
    previousHistoryStack.clear();
    historyModule.add(bundle);
  }

  Future<void> reload() async {
    assert(!_view._disposed, 'WebF have already disposed');

    if (devToolsService != null) {
      devToolsService!.willReload();
    }

    await controlledInitCompleter.future;

    _isComplete = false;

    RenderViewportBox rootRenderObject = view.viewport!;

    await unload();

    view.viewport = rootRenderObject;

    // Initialize document, window and the documentElement.
    flushUICommand(view, nullptr);

    Completer completer = Completer();

    SchedulerBinding.instance.addPostFrameCallback((timeStamp) async {

      // Sync viewport size to the documentElement.
      view.document.initializeRootElementSize();
      // Starting to flush ui commands every frames.
      view.flushPendingCommandsPerFrame();

      await executeEntrypoint();

      if (devToolsService != null) {
        devToolsService!.didReload();
      }

      completer.complete();
    });

    return completer.future;
  }

  Future<void> load(WebFBundle bundle) async {
    assert(!_view._disposed, 'WebF have already disposed');

    if (devToolsService != null) {
      devToolsService!.willReload();
    }

    await controlledInitCompleter.future;

    RenderViewportBox rootRenderObject = view.viewport!;

    await unload();

    view.viewport = rootRenderObject;

    // Initialize document, window and the documentElement.
    flushUICommand(view, nullptr);

    // Update entrypoint.
    _entrypoint = bundle;
    _addHistory(bundle);

    Completer completer = Completer();

    SchedulerBinding.instance.addPostFrameCallback((timeStamp) async {

      // Sync viewport size to the documentElement.
      view.document.initializeRootElementSize();
      // Starting to flush ui commands every frames.
      view.flushPendingCommandsPerFrame();

      await executeEntrypoint();

      if (devToolsService != null) {
        devToolsService!.didReload();
      }

      completer.complete();
    });

    return completer.future;
  }

  PreloadingStatus _preloadStatus = PreloadingStatus.none;
  PreloadingStatus get preloadStatus => _preloadStatus;
  VoidCallback? _onPreloadingFinished;
  int unfinishedPreloadResources = 0;

  /// Preloads remote resources into memory and begins execution when the WebF widget is mounted into the Flutter tree.
  /// If the entrypoint is an HTML file, the HTML will be parsed, and its elements will be organized into a DOM tree.
  /// CSS files loaded through `<style>` and `<link>` elements will be parsed and the calculated styles applied to the corresponding DOM elements.
  /// However, JavaScript code will not be executed in this mode.
  /// If the entrypoint is a JavaScript file, WebF only do loading until the WebF widget is mounted into the Flutter tree.
  /// Using this mode can save up to 50% of loading time, while maintaining a high level of compatibility with the standard mode.
  /// It's safe and recommended to use this mode for all types of pages.
  Future<void> preload(WebFBundle bundle, {ui.Size? viewportSize}) async {
    controllerPreloadingCompleter = Completer();

    await controlledInitCompleter.future;

    if (_preloadStatus != PreloadingStatus.none) return;
    if (_preRenderingStatus != PreRenderingStatus.none) return;

    // Update entrypoint.
    _entrypoint = bundle;
    _replaceCurrentHistory(bundle);

    mode = WebFLoadingMode.preloading;

    // Initialize document, window and the documentElement.
    flushUICommand(view, nullptr);

    // Set the status value for preloading.
    _preloadStatus = PreloadingStatus.preloading;

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.startTrackUICommand();
    }

    // Manually initialize the root element and create renderObjects for each elements.
    view.document.documentElement!.applyStyle(view.document.documentElement!.style);
    view.document.documentElement!.createRenderer();
    view.document.documentElement!.ensureChildAttached();

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.finishTrackUICommand();
    }

    await Future.wait([
      _resolveEntrypoint(),
      module.initialize()
    ]);

    if (_entrypoint!.isJavascript || _entrypoint!.isBytecode) {
      // Convert the JavaScript code into bytecode.
      if (_entrypoint!.isJavascript) {
        await _entrypoint!.preProcessing(view.contextId);
      }
      _preloadStatus = PreloadingStatus.done;
      controllerPreloadingCompleter.complete();
    } else if (_entrypoint!.isHTML) {
      EvaluateOpItem? evaluateOpItem;
      if (enableWebFProfileTracking) {
        evaluateOpItem = WebFProfiler.instance.startTrackEvaluate('parseHTML');
      }

      // Evaluate the HTML entry point, and loading the stylesheets and scripts.
      await parseHTML(view.contextId, _entrypoint!.data!, profileOp: evaluateOpItem);

      if (enableWebFProfileTracking) {
        WebFProfiler.instance.finishTrackEvaluate(evaluateOpItem!);
      }

      // Initialize document, window and the documentElement.
      flushUICommand(view, view.window.pointer!);

      if (view.document.scriptRunner.hasPreloadScripts()) {
        _onPreloadingFinished = () {
          _preloadStatus = PreloadingStatus.done;
          controllerPreloadingCompleter.complete();
        };
      } else {
        _preloadStatus = PreloadingStatus.done;
        controllerPreloadingCompleter.complete();
      }
    }

    return controllerPreloadingCompleter.future;
  }

  bool get shouldBlockingFlushingResolvedStyleProperties {
    if (mode != WebFLoadingMode.preRendering) return false;

    RenderBox? rootRenderObject = view.getRootRenderObject();

    if (rootRenderObject == null || !rootRenderObject.attached) return true;

    return preRenderingStatus.index < PreRenderingStatus.done.index;
  }

  PreRenderingStatus _preRenderingStatus = PreRenderingStatus.none;
  PreRenderingStatus get preRenderingStatus => _preRenderingStatus;
  /// The `aggressive` mode is a step further than `preloading`, cutting down up to 90% of loading time for optimal performance.
  /// This mode simulates the instantaneous response of native Flutter pages but may require modifications in the existing web codes for compatibility.
  /// In this mode, all remote resources are loaded and executed similarly to the standard mode, but with an offline-like behavior.
  /// Given that JavaScript is executed in this mode, properties like `clientWidth` and `clientHeight` from the viewModule always return 0. This is because
  /// no layout or paint processes occur during preRendering.
  /// If your application depends on viewModule properties, ensure that the related code is placed within the `load` and `DOMContentLoaded` or `prerendered` event callbacks of the window.
  /// These callbacks are triggered once the WebF widget is mounted into the Flutter tree.
  /// Apps optimized for this mode remain compatible with both `standard` and `preloading` modes.
  Future<void> preRendering(WebFBundle bundle) async {
    controllerPreRenderingCompleter = Completer();

    await controlledInitCompleter.future;

    if (_preRenderingStatus != PreRenderingStatus.none) return;
    if (_preloadStatus != PreloadingStatus.none) return;

    // Update entrypoint.
    _entrypoint = bundle;
    _replaceCurrentHistory(bundle);

    mode = WebFLoadingMode.preRendering;

    // Initialize document, window and the documentElement.
    flushUICommand(view, nullptr);

    // Set the status value for preloading.
    _preRenderingStatus = PreRenderingStatus.preloading;

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.startTrackUICommand();
    }

    // Manually initialize the root element and create renderObjects for each elements.
    view.document.documentElement!.applyStyle(view.document.documentElement!.style);
    view.document.documentElement!.createRenderer();
    view.document.documentElement!.ensureChildAttached();

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.finishTrackUICommand();
    }

    // Preparing the entrypoint
    await Future.wait([
      _resolveEntrypoint(),
      module.initialize()
    ]);

    // Stop the animation frame
    module.pauseAnimationFrame();

    // Pause the animation timeline.
    view.stopAnimationsTimeLine();

    view.window.addEventListener(EVENT_LOAD, (event) async {
      _preRenderingStatus = PreRenderingStatus.done;
    });

    if (_entrypoint!.isJavascript || _entrypoint!.isBytecode) {
      // Convert the JavaScript code into bytecode.
      if (_entrypoint!.isJavascript) {
        await _entrypoint!.preProcessing(view.contextId);
      }
    }

    _preRenderingStatus = PreRenderingStatus.evaluate;

    // Evaluate the entry point, and loading the stylesheets and scripts.
    await evaluateEntrypoint();

    view.flushPendingCommandsPerFrame();

    // If there are no <script /> elements, finish this prerendering process.
    if (!view.document.scriptRunner.hasPendingScripts()) {
      controllerPreRenderingCompleter.complete();
      return;
    }

    return controllerPreRenderingCompleter.future;
  }

  String? getResourceContent(String? url) {
    WebFBundle? entrypoint = _entrypoint;
    if (url == this.url && entrypoint != null && entrypoint.isResolved) {
      return utf8.decode(entrypoint.data!);
    }
    return null;
  }

  bool _paused = false;

  bool get paused => _paused;

  final List<PendingCallback> _pendingCallbacks = [];

  void pushPendingCallbacks(PendingCallback callback) {
    _pendingCallbacks.add(callback);
  }

  void flushPendingCallbacks() {
    for (int i = 0; i < _pendingCallbacks.length; i++) {
      _pendingCallbacks[i]();
    }
    _pendingCallbacks.clear();
  }

  final List<WebFWidgetElementToWidgetAdapter> pendingWidgetElements = [];

  void flushPendingUnAttachedWidgetElements() {
    assert(onCustomElementAttached != null);
    for (int i = 0; i < pendingWidgetElements.length; i ++) {
      onCustomElementAttached!(pendingWidgetElements[i]);
    }
    pendingWidgetElements.clear();
  }

  void reactiveWidgetElements() {

  }

  // Pause all timers and callbacks if kraken page are invisible.
  void pause() {
    if (_paused) return;
    _paused = true;
    module.pauseTimer();
    module.pauseAnimationFrame();
    view.stopAnimationsTimeLine();
  }

  // Resume all timers and callbacks if kraken page now visible.
  void resume() {
    if (!_paused) return;

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.startTrackUICommand();
    }

    _paused = false;
    flushPendingCallbacks();
    module.resumeTimer();
    module.resumeAnimationFrame();
    view.resumeAnimationTimeline();
    SchedulerBinding.instance.scheduleFrame();

    if (enableWebFProfileTracking) {
      WebFProfiler.instance.finishTrackUICommand();
    }
  }

  bool _disposed = false;

  bool get disposed => _disposed;
  Future<void> dispose() async {
    _module.dispose();
    PaintingBinding.instance.systemFonts.removeListener(_watchFontLoading);
    await _view.dispose();
    _controllerMap[_view.contextId] = null;
    _controllerMap.remove(_view.contextId);
    _nameIdMap.remove(name);
    // To release entrypoint bundle memory.
    _entrypoint?.dispose();

    devToolsService?.dispose();
    _disposed = true;
  }

  String get origin {
    Uri uri = Uri.parse(url);
    return '${uri.scheme}://${uri.host}:${uri.port}';
  }

  Future<void> executeEntrypoint(
      {bool shouldResolve = true, bool shouldEvaluate = true, AnimationController? animationController}) async {
    if (_entrypoint != null && shouldResolve) {
      await controlledInitCompleter.future;
      await Future.wait([
        _resolveEntrypoint(),
        _module.initialize()
      ]);
      if (_entrypoint!.isResolved && shouldEvaluate) {
        await evaluateEntrypoint(animationController: animationController);
      } else {
        throw FlutterError('Unable to resolve $_entrypoint');
      }
    } else {
      throw FlutterError('Entrypoint is empty.');
    }
  }

  // Resolve the entrypoint bundle.
  // In general you should use executeEntrypoint, which including resolving and evaluating.
  Future<void> _resolveEntrypoint() async {
    assert(!_view._disposed, 'WebF have already disposed');

    WebFBundle? bundleToLoad = _entrypoint;
    if (bundleToLoad == null) {
      // Do nothing if bundle is null.
      return;
    }

    // Resolve the bundle, including network download or other fetching ways.
    try {
      await bundleToLoad.resolve(baseUrl: url, uriParser: uriParser);
      await bundleToLoad.obtainData(view.contextId);
    } catch (e, stack) {
      if (onLoadError != null) {
        onLoadError!(FlutterError(e.toString()), stack);
      }
      // Not to dismiss this error.
      rethrow;
    }
  }

  // Execute the content from entrypoint bundle.
  Future<void> evaluateEntrypoint({AnimationController? animationController}) async {
    // @HACK: Execute JavaScript scripts will block the Flutter UI Threads.
    // Listen for animationController listener to make sure to execute Javascript after route transition had completed.
    if (animationController != null) {
      animationController.addStatusListener((AnimationStatus status) async {
        if (status == AnimationStatus.completed) {
          await evaluateEntrypoint();
        }
      });
      return;
    }

    assert(!_view._disposed, 'WebF have already disposed');
    if (_entrypoint != null) {
      EvaluateOpItem? evaluateOpItem;
      if (enableWebFProfileTracking) {
        evaluateOpItem = WebFProfiler.instance.startTrackEvaluate('WebFController.evaluateEntrypoint');
      }

      WebFBundle entrypoint = _entrypoint!;
      double contextId = _view.contextId;
      assert(entrypoint.isResolved, 'The webf bundle $entrypoint is not resolved to evaluate.');

      // entry point start parse.
      _view.document.parsing = true;

      Uint8List data = entrypoint.data!;
      if (entrypoint.isJavascript) {
        assert(isValidUTF8String(data), 'The JavaScript codes should be in UTF-8 encoding format');
        // Prefer sync decode in loading entrypoint.
        await evaluateScripts(contextId, data, url: url, profileOp: evaluateOpItem);
      } else if (entrypoint.isBytecode) {
        await evaluateQuickjsByteCode(contextId, data, profileOp: evaluateOpItem);
      } else if (entrypoint.isHTML) {
        assert(isValidUTF8String(data), 'The HTML codes should be in UTF-8 encoding format');
        await parseHTML(contextId, data, profileOp: evaluateOpItem);
      } else if (entrypoint.contentType.primaryType == 'text') {
        // Fallback treating text content as JavaScript.
        try {
          assert(isValidUTF8String(data), 'The JavaScript codes should be in UTF-8 encoding format');
          await evaluateScripts(contextId, data, url: url, profileOp: evaluateOpItem);
        } catch (error) {
          print('Fallback to execute JavaScript content of $url');
          rethrow;
        }
      } else {
        // The resource type can not be evaluated.
        throw FlutterError('Can\'t evaluate content of $url');
      }

      // entry point end parse.
      _view.document.parsing = false;

      // Should check completed when parse end.
      SchedulerBinding.instance.addPostFrameCallback((_) {
        // UICommand list is read in the next frame, so we need to determine whether there are labels
        // such as images and scripts after it to check is completed.
        checkCompleted();
      });
      SchedulerBinding.instance.scheduleFrame();

      if (enableWebFProfileTracking) {
        WebFProfiler.instance.finishTrackEvaluate(evaluateOpItem!);
      }
    }
  }

  // https://github.com/WebKit/WebKit/blob/main/Source/WebCore/loader/FrameLoader.h#L470
  bool _isComplete = false;
  bool get isComplete => _isComplete;

  bool _evaluated = false;
  bool get evaluated => _evaluated;
  set evaluated(value) {
    _evaluated = value;
  }

  // https://github.com/WebKit/WebKit/blob/main/Source/WebCore/loader/FrameLoader.cpp#L840
  // Check whether the document has been loaded, such as html has parsed (main of JS has evaled) and images/scripts has loaded.
  void checkCompleted() {
    if (_isComplete) return;

    // Are we still parsing?
    if (_view.document.parsing) return;

    // Are all script element complete?
    if (_view.document.isDelayingDOMContentLoadedEvent) return;

    if (mode == WebFLoadingMode.standard || mode == WebFLoadingMode.preloading) {
      _view.document.readyState = DocumentReadyState.interactive;
      dispatchDOMContentLoadedEvent();
    }

    // Still waiting for images/scripts?
    if (_view.document.hasPendingRequest) return;

    // Still waiting for elements that don't go through a FrameLoader?
    if (_view.document.isDelayingLoadEvent) return;

    // Any frame that hasn't completed yet?
    // TODO:

    _isComplete = true;

    if (mode == WebFLoadingMode.standard || mode == WebFLoadingMode.preloading) {
      dispatchWindowLoadEvent();
      _view.document.readyState = DocumentReadyState.complete;
    } else if (mode == WebFLoadingMode.preRendering) {
      if (!controllerPreRenderingCompleter.isCompleted) {
        controllerPreRenderingCompleter.complete();
      }
    }
  }

  // Check whether the load was complete in preload mode.
  void checkPreloadCompleted() {
    if (unfinishedPreloadResources == 0 && _onPreloadingFinished != null) {
      _onPreloadingFinished!();
    }
  }

  bool _domContentLoadedEventDispatched = false;
  void dispatchDOMContentLoadedEvent() {
    if (_domContentLoadedEventDispatched) return;

    _domContentLoadedEventDispatched = true;

    SchedulerBinding.instance.addPostFrameCallback((_) {
      Event event = Event(EVENT_DOM_CONTENT_LOADED);
      EventTarget window = view.window;
      window.dispatchEvent(event);
      _view.document.dispatchEvent(event);
      if (onDOMContentLoaded != null) {
        onDOMContentLoaded!(this);
      }
    });
    SchedulerBinding.instance.scheduleFrame();
  }

  bool _loadEventDispatched = false;
  void dispatchWindowLoadEvent() {
    if (_loadEventDispatched) return;
    _loadEventDispatched = true;
    SchedulerBinding.instance.addPostFrameCallback((_) {
      // DOM element are created at next frame, so we should trigger onload callback in the next frame.
      Event event = Event(EVENT_LOAD);
      _view.window.dispatchEvent(event);

      if (onLoad != null) {
        onLoad!(this);
      }
    });
    SchedulerBinding.instance.scheduleFrame();
  }

  bool _preloadEventDispatched = false;
  void dispatchWindowPreloadedEvent() {
    if (_preloadEventDispatched) return;
    _preloadEventDispatched = true;
    Event event = Event(EVENT_PRELOADED);
    _view.window.dispatchEvent(event);
  }

  bool _preRenderedEventDispatched = false;
  void dispatchWindowPreRenderedEvent() {
    if (_preRenderedEventDispatched) return;
    _preRenderedEventDispatched = true;
    Event event = Event(EVENT_PRERENDERED);
    _view.window.dispatchEvent(event);
  }

  Future<void> dispatchWindowResizeEvent() async {
    Event event = Event(EVENT_RESIZE);
    await _view.window.dispatchEvent(event);
  }
}
