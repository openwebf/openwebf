/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:collection';
import 'dart:ffi';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';
import 'package:webf/css.dart';
import 'package:webf/dom.dart';
import 'package:webf/html.dart';
import 'package:webf/widget.dart';
import 'package:webf/foundation.dart';
import 'package:webf/gesture.dart';
import 'package:webf/launcher.dart';
import 'package:webf/rendering.dart';
import 'package:webf/src/css/query_selector.dart' as QuerySelector;
import 'package:webf/src/dom/element_registry.dart' as element_registry;
import 'package:webf/src/foundation/cookie_jar.dart';
import 'package:webf/src/dom/intersection_observer.dart';

/// In the document tree, there may contains WidgetElement which connected to a Flutter Elements.
/// And these flutter element will be unmounted in the end of this frame and their renderObject will call dispose() too.
/// So we can't dispose WebF Element's renderObject immediately if the WebF element are removed from document tree.
/// This class will buffering all the renderObjects who's element are removed from the document tree, and they will be disposed
/// in the end of this frame.
class _InactiveRenderObjects {
  Set<RenderObject> _renderObjects = {};
  Set<RenderObject> _nextDisposeRenderObjects = {};

  bool _isScheduled = false;

  void _scheduleFrameToFinalizeRenderObjects() {
    _isScheduled = true;

    /// We needs to wait at least 2 frames to dispose all webf managed renderObjects.
    /// All renderObjects managed by WebF should be disposed after Flutter managed renderObjects dispose.
    RendererBinding.instance.addPostFrameCallback((timeStamp) {
      /// The Flutter framework will move all deactivated elements into _InactiveElement list.
      /// They will be disposed in the next frame.
      RendererBinding.instance.addPostFrameCallback((timeStamp) {
        /// Now the renderObjects managed by Flutter framework are disposed, it's safe to dispose renderObject by our own.
        finalizeInactiveRenderObjects();
        _isScheduled = false;

        /// Some pending Objects may be added during this 2 frame waiting. Regrouping them into the waiting list.
        if (_nextDisposeRenderObjects.isNotEmpty) {
          _renderObjects = _nextDisposeRenderObjects;
          _nextDisposeRenderObjects = {};
          _scheduleFrameToFinalizeRenderObjects();
        }
      });
      RendererBinding.instance.scheduleFrame();
    });
    RendererBinding.instance.scheduleFrame();
  }

  void add(RenderObject? renderObject) {
    if (renderObject == null) return;

    if (_renderObjects.isEmpty && !_isScheduled) {
      _scheduleFrameToFinalizeRenderObjects();
    }

    assert(!renderObject.debugDisposed!, '$renderObject already disposed');

    if (!_isScheduled) {
      _renderObjects.add(renderObject);
    } else {
      _nextDisposeRenderObjects.add(renderObject);
    }
  }

  void finalizeInactiveRenderObjects() {
    for(RenderObject object in _renderObjects) {
      assert(!object.attached);
      object.dispose();
    }
    _renderObjects.clear();
  }
}

enum DocumentReadyState { loading, interactive, complete }

enum VisibilityState { visible, hidden }

class Document extends ContainerNode {
  final WebFController controller;
  final AnimationTimeline animationTimeline = AnimationTimeline();
  GestureListener? gestureListener;

  Map<String, List<Element>> elementsByID = {};
  Map<String, List<Element>> elementsByName = {};

  // Cache all the fixed children of renderBoxModel of root element.
  Set<RenderBoxModel> fixedChildren = {};

  final List<AsyncCallback> pendingPreloadingScriptCallbacks = [];

  final Set<int> _styleDirtyElements = {};

  final Set<IntersectionObserver> _intersectionObserverList = {};

  void markElementStyleDirty(Element element) {
    _styleDirtyElements.add(element.pointer!.address);
  }
  void clearElementStyleDirty(Element element) {
    _styleDirtyElements.remove(element.pointer!.address);
  }

  final NthIndexCache _nthIndexCache = NthIndexCache();

  NthIndexCache get nthIndexCache => _nthIndexCache;

  StyleNodeManager get styleNodeManager => _styleNodeManager;
  late StyleNodeManager _styleNodeManager;

  Set<WidgetElement> aliveWidgetElements = {};

  late RuleSet ruleSet;

  String? _domain;
  final String _compatMode = 'CSS1Compat';

  String? _readyState;
  VisibilityState _visibilityState = VisibilityState.hidden;

  @override
  bool get isConnected => true;

  Document(BindingContext context, {required this.controller, this.gestureListener, List<Cookie>? initialCookies})
      : super(NodeType.DOCUMENT_NODE, context) {
    cookie_ = CookieJar(controller.url, initialCookies: initialCookies);
    _styleNodeManager = StyleNodeManager(this);
    _scriptRunner = ScriptRunner(this, context.contextId);
    ruleSet = RuleSet(this);
  }

  // https://github.com/WebKit/WebKit/blob/main/Source/WebCore/dom/Document.h#L1898
  late ScriptRunner _scriptRunner;

  ScriptRunner get scriptRunner => _scriptRunner;

  _InactiveRenderObjects inactiveRenderObjects = _InactiveRenderObjects();

  @override
  EventTarget? get parentEventTarget => defaultView;

  RenderViewportBox? get viewport => controller.view.viewport;

  @override
  Document get ownerDocument => this;

  Element? focusedElement;

  late CookieJar cookie_;

  CookieJar get cookie => cookie_;

  // Returns the Window object of the active document.
  // https://html.spec.whatwg.org/multipage/window-object.html#dom-document-defaultview-dev
  Window get defaultView => controller.view.window;

  @override
  String get nodeName => '#document';

  @override
  RenderBox? get renderer => viewport;

  // https://github.com/WebKit/WebKit/blob/main/Source/WebCore/dom/Document.h#L770
  bool parsing = false;

  int _requestCount = 0;

  bool get hasPendingRequest => _requestCount > 0;

  void incrementRequestCount() {
    _requestCount++;
  }

  void decrementRequestCount() {
    assert(_requestCount > 0);
    _requestCount--;
  }

  // https://github.com/WebKit/WebKit/blob/main/Source/WebCore/dom/Document.h#L2091
  // Counters that currently need to delay load event, such as parsing a script.
  int _loadEventDelayCount = 0;

  bool get isDelayingLoadEvent => _loadEventDelayCount > 0;

  void incrementLoadEventDelayCount() {
    _loadEventDelayCount++;
  }

  void decrementLoadEventDelayCount() {
    _loadEventDelayCount--;

    // Try to check when the request is complete.
    if (_loadEventDelayCount == 0) {
      controller.checkCompleted();
    }
  }

  int _domContentLoadedEventDelayCount = 0;

  bool get isDelayingDOMContentLoadedEvent => _domContentLoadedEventDelayCount > 0;

  void incrementDOMContentLoadedEventDelayCount() {
    _domContentLoadedEventDelayCount++;
  }

  void decrementDOMContentLoadedEventDelayCount() {
    _domContentLoadedEventDelayCount--;

    // Try to check when the request is complete.
    if (_domContentLoadedEventDelayCount == 0) {
      controller.checkCompleted();
    }
  }

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {
    properties['cookie'] =
        BindingObjectProperty(getter: () => cookie.cookie(), setter: (value) => cookie.setCookieString(value));
    properties['compatMode'] = BindingObjectProperty(getter: () => compatMode);
    properties['domain'] = BindingObjectProperty(getter: () => domain, setter: (value) => domain = value);
    properties['readyState'] = BindingObjectProperty(getter: () => readyState);
    properties['visibilityState'] = BindingObjectProperty(getter: () => visibilityState);
    properties['hidden'] = BindingObjectProperty(getter: () => hidden);
    properties['title'] = BindingObjectProperty(
      getter: () => _title ?? '',
      setter: (value) {
        _title = value ?? '';
        ownerDocument.controller.onTitleChanged?.call(title);
      },
    );
  }

  @override
  void initializeMethods(Map<String, BindingObjectMethod> methods) {
    methods['querySelectorAll'] = BindingObjectMethodSync(call: (args) => querySelectorAll(args));
    methods['querySelector'] = BindingObjectMethodSync(call: (args) => querySelector(args));
    methods['getElementById'] = BindingObjectMethodSync(call: (args) => getElementById(args));
    methods['getElementsByClassName'] = BindingObjectMethodSync(call: (args) => getElementsByClassName(args));
    methods['getElementsByTagName'] = BindingObjectMethodSync(call: (args) => getElementsByTagName(args));
    methods['getElementsByName'] = BindingObjectMethodSync(call: (args) => getElementsByName(args));
    methods['elementFromPoint'] = BindingObjectMethodSync(
        call: (args) => elementFromPoint(castToType<double>(args[0]), castToType<double>(args[1])));
    if (kDebugMode || kProfileMode) {
      methods['___clear_cookies__'] = BindingObjectMethodSync(call: (args) => debugClearCookies(args));
    }
  }

  get readyState {
    _readyState ??= 'loading';
    return _readyState;
  }

  set readyState(value) {
    if (value is DocumentReadyState) {
      String readyStateValue = resolveReadyState(value);
      if (readyStateValue != _readyState) {
        _readyState = readyStateValue;
        _dispatchReadyStateChangeEvent();
      }
    }
  }

  get visibilityState {
    return _visibilityState.name;
  }

  get hidden {
    return _visibilityState == VisibilityState.hidden;
  }

  void visibilityChange(VisibilityState state) {
    _visibilityState = state;
    ownerDocument.dispatchEvent(Event('visibilitychange'));
  }

  void _dispatchReadyStateChangeEvent() {
    Event event = Event(EVENT_READY_STATE_CHANGE);
    defaultView.dispatchEvent(event);
  }

  String resolveReadyState(DocumentReadyState documentReadyState) {
    switch (documentReadyState) {
      case DocumentReadyState.loading:
        return 'loading';
      case DocumentReadyState.interactive:
        return 'interactive';
      case DocumentReadyState.complete:
        return 'complete';
    }
  }

  dynamic get compatMode => _compatMode;

  String get title => _title ?? '';
  String? _title;

  get domain {
    Uri uri = Uri.parse(controller.url);
    _domain ??= uri.host;
    return _domain;
  }

  set domain(value) {
    _domain = value;
  }

  dynamic debugClearCookies(List<dynamic> args) {
    cookie.clearAllCookies();
  }

  dynamic querySelector(List<dynamic> args) {
    if (args[0].runtimeType == String && (args[0] as String).isEmpty) return null;
    return QuerySelector.querySelector(this, args.first);
  }

  dynamic elementFromPoint(double x, double y) {
    documentElement?.flushLayout();
    return HitTestPoint(x, y);
  }

  Element? HitTestPoint(double x, double y) {
    HitTestResult hitTestResult = HitTestInDocument(x, y);
    Iterable<HitTestEntry> hitTestEntrys = hitTestResult.path;
    if (hitTestResult.path.isNotEmpty) {
      if (hitTestEntrys.first.target is RenderBoxModel) {
        return (hitTestEntrys.first.target as RenderBoxModel).renderStyle.target;
      }
    }
    return null;
  }

  HitTestResult HitTestInDocument(double x, double y) {
    BoxHitTestResult boxHitTestResult = BoxHitTestResult();
    Offset offset = Offset(x, y);
    documentElement?.renderer?.hitTest(boxHitTestResult, position: offset);
    return boxHitTestResult;
  }

  dynamic querySelectorAll(List<dynamic> args) {
    if (args[0].runtimeType == String && (args[0] as String).isEmpty) return [];
    return QuerySelector.querySelectorAll(this, args.first);
  }

  dynamic getElementById(List<dynamic> args) {
    if (args[0].runtimeType == String && (args[0] as String).isEmpty) return null;
    final elements = elementsByID[args.first];
    if (elements == null || elements.isEmpty) {
      return null;
    }
    if (elements.length == 1) {
      return elements.last;
    } else if (elements.length > 1) {
      Queue<Node> queue = Queue();
      queue.add(this);
      while (queue.isNotEmpty) {
        Node node = queue.removeFirst();
        if (elements.contains(node)) {
          return node;
        }
        if (node.childNodes.isNotEmpty) {
          for (Node child in node.childNodes) {
            queue.add(child);
          }
        }
      }
    }
    return null;
  }

  dynamic getElementsByClassName(List<dynamic> args) {
    if (args[0].runtimeType == String && (args[0] as String).isEmpty) return [];
    String selector = (args.first as String).split(classNameSplitRegExp).map((e) => '.' + e).join('');
    return QuerySelector.querySelectorAll(this, selector);
  }

  dynamic getElementsByTagName(List<dynamic> args) {
    if (args[0].runtimeType == String && (args[0] as String).isEmpty) return [];
    return QuerySelector.querySelectorAll(this, args.first);
  }

  dynamic getElementsByName(List<dynamic> args) {
    if (args[0].runtimeType == String && (args[0] as String).isEmpty) return [];
    return elementsByName[args.first];
  }

  Element? _documentElement;

  Element? get documentElement => _documentElement;

  set documentElement(Element? element) {
    if (_documentElement == element) {
      return;
    }

    bool documentElementChanged = element != null && element != _documentElement;
    // When document is disposed, viewport is null.
    if (viewport != null) {
      if (element != null) {
        element.attachTo(this);
        _visibilityState = VisibilityState.visible;
      } else {
        // Detach document element.
        viewport!.removeAll();
      }
    }

    _documentElement = element;

    if (viewport?.hasSize == true && documentElementChanged) {
      initializeRootElementSize();
    }
  }

  void initializeRootElementSize() {
    assert(documentElement != null);
    assert(viewport!.hasSize);
    documentElement!.renderStyle.width = CSSLengthValue(viewport!.viewportSize.width, CSSLengthType.PX);
    documentElement!.renderStyle.height = CSSLengthValue(viewport!.viewportSize.height, CSSLengthType.PX);
    if (enableWebFProfileTracking) {
      WebFProfiler.instance.startTrackUICommand();
    }
    documentElement!.setRenderStyleProperty(OVERFLOW_X, CSSOverflowType.scroll);
    documentElement!.setRenderStyleProperty(OVERFLOW_Y, CSSOverflowType.scroll);
    if (enableWebFProfileTracking) {
      WebFProfiler.instance.finishTrackUICommand();
    }
  }

  @override
  Node appendChild(Node child) {
    if (child is Element) {
      documentElement ??= child;
    } else {
      throw UnsupportedError('Only Element can be appended to Document');
    }
    return super.appendChild(child);
  }

  @override
  String toString() {
    return 'Document($hashCode)';
  }

  @override
  Node insertBefore(Node child, Node referenceNode) {
    if (child is Element) {
      documentElement ??= child;
    } else {
      throw UnsupportedError('Only Element can be inserted to Document');
    }
    return super.insertBefore(child, referenceNode);
  }

  @override
  Node? removeChild(Node child) {
    Node? result = super.removeChild(child);
    if (documentElement == child) {
      documentElement = null;
      ruleSet.reset();
      styleSheets.clear();
    }
    return result;
  }

  @override
  Node? replaceChild(Node newNode, Node oldNode) {
    if (documentElement == oldNode) {
      documentElement = newNode is Element ? newNode : null;
    }
    return super.replaceChild(newNode, oldNode);
  }

  Element createElement(String type, BindingContext context) {
    Element element = element_registry.createElement(type, context);
    return element;
  }

  Element createElementNS(String uri, String type, BindingContext context) {
    Element element = element_registry.createElementNS(uri, type, context);
    return element;
  }

  TextNode createTextNode(String data, BindingContext context) {
    TextNode textNode = TextNode(data, context);
    return textNode;
  }

  DocumentFragment createDocumentFragment(BindingContext context) {
    DocumentFragment documentFragment = DocumentFragment(context);
    return documentFragment;
  }

  Comment createComment(BindingContext context) {
    Comment comment = Comment(context);
    return comment;
  }

  // TODO: https://wicg.github.io/construct-stylesheets/#using-constructed-stylesheets
  List<CSSStyleSheet> adoptedStyleSheets = [];

  // The styleSheets attribute is readonly attribute.
  final List<CSSStyleSheet> styleSheets = [];

  void handleStyleSheets(List<CSSStyleSheet> sheets) {
    styleSheets.clear();
    styleSheets.addAll(sheets.map((e) => e.clone()));
    ruleSet.reset();
    for (var sheet in sheets) {
      ruleSet.addRules(sheet.cssRules, baseHref: sheet.href);
    }
  }

  bool _recalculating = false;

  void updateStyleIfNeeded() {
    if (!styleNodeManager.hasPendingStyleSheet && !styleNodeManager.isStyleSheetCandidateNodeChanged) {
      return;
    }
    if (_recalculating) {
      return;
    }
    _recalculating = true;
    if (styleSheets.isEmpty && styleNodeManager.hasPendingStyleSheet) {
      flushStyle(rebuild: true);
      return;
    }
    flushStyle();
  }

  void flushStyle({bool rebuild = false}) {
    if (enableWebFProfileTracking) {
      WebFProfiler.instance.startTrackUICommandStep('Document.flushStyle');
    }
    if (_styleDirtyElements.isEmpty) {
      _recalculating = false;
      if (enableWebFProfileTracking) {
        WebFProfiler.instance.finishTrackUICommandStep();
      }
      return;
    }
    if (!styleNodeManager.updateActiveStyleSheets(rebuild: rebuild)) {
      _recalculating = false;
      _styleDirtyElements.clear();
      if (enableWebFProfileTracking) {
        WebFProfiler.instance.finishTrackUICommandStep();
      }
      return;
    }
    if (_styleDirtyElements.any((address) {
          BindingObject bindingObject = ownerView.getBindingObject(Pointer.fromAddress(address));
          return bindingObject is HeadElement || bindingObject is HTMLElement;
        }) ||
        rebuild) {
      documentElement?.recalculateStyle(rebuildNested: true);
    } else {
      for (int address in _styleDirtyElements) {
        Element? element = ownerView.getBindingObject(Pointer.fromAddress(address)) as Element?;
        element?.recalculateStyle();
      }
    }
    _styleDirtyElements.clear();
    _recalculating = false;
    if (enableWebFProfileTracking) {
      WebFProfiler.instance.finishTrackUICommandStep();
    }
  }

  void reactiveWidgetElements() {
    for (WidgetElement widgetElement in aliveWidgetElements) {
      widgetElement.reactiveRenderer();
    }
  }

  @override
  Future<void> dispose() async {
    gestureListener = null;
    styleSheets.clear();
    nthIndexCache.clearAll();
    adoptedStyleSheets.clear();
    cookie.clearCookie();
    _styleDirtyElements.clear();
    fixedChildren.clear();
    pendingPreloadingScriptCallbacks.clear();
    super.dispose();
  }

  void addIntersectionObserver(IntersectionObserver observer, Element element) {
    observer.observe(element);
    _intersectionObserverList.add(observer);
  }

  void removeIntersectionObserver(IntersectionObserver observer, Element element) {
    observer.unobserve(element);
    if (!observer.HasObservations()) {
      _intersectionObserverList.remove(observer);
    }
  }

  void disconnectIntersectionObserver(IntersectionObserver observer) {
    observer.disconnect();
    _intersectionObserverList.remove(observer);
  }

  void deliverIntersectionObserver() {
    if (_intersectionObserverList.isEmpty) {
      return;
    }
    for (var observer in _intersectionObserverList) {
      observer.deliver(controller);
    }
  }
}
