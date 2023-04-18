/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:async';
import 'dart:io';
import 'dart:ui';

import 'package:flutter/foundation.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:webf/webf.dart';
import 'package:webf/gesture.dart';
import 'package:webf/css.dart';

typedef OnControllerCreated = void Function(WebFController controller);

class WebF extends StatefulWidget {
  // The background color for viewport, default to transparent.
  final Color? background;

  // the width of webFWidget
  final double? viewportWidth;

  // the height of webFWidget
  final double? viewportHeight;

  //  The initial bundle to load.
  final WebFBundle? bundle;

  // The animationController of Flutter Route object.
  // Pass this object to webFWidget to make sure webF execute JavaScripts scripts after route transition animation completed.
  final AnimationController? animationController;

  // The methods of the webFNavigateDelegation help you implement custom behaviors that are triggered
  // during a webf view's process of loading, and completing a navigation request.
  final WebFNavigationDelegate? navigationDelegate;

  // A method channel for receiving messaged from JavaScript code and sending message to JavaScript.
  final WebFMethodChannel? javaScriptChannel;

  // Register the RouteObserver to observer page navigation.
  // This is useful if you wants to pause webf timers and callbacks when webf widget are hidden by page route.
  // https://api.flutter.dev/flutter/widgets/RouteObserver-class.html
  final RouteObserver<ModalRoute<void>>? routeObserver;

  // Trigger when webf controller once created.
  final OnControllerCreated? onControllerCreated;

  final LoadErrorHandler? onLoadError;

  final LoadHandler? onLoad;

  final JSErrorHandler? onJSError;

  // Open a service to support Chrome DevTools for debugging.
  final DevToolsService? devToolsService;

  final GestureListener? gestureListener;

  final HttpClientInterceptor? httpClientInterceptor;

  final UriParser? uriParser;

  /// The initial cookies to set.
  final List<Cookie>? initialCookies;

  WebFController? get controller {
    return WebFController.getControllerOfName(shortHash(this));
  }

  // Set webf http cache mode.
  static void setHttpCacheMode(HttpCacheMode mode) {
    HttpCacheController.mode = mode;
    if (kDebugMode) {
      print('WebF http cache mode set to $mode.');
    }
  }

  static bool _isValidCustomElementName(localName) {
    return RegExp(r'^[a-z][.0-9_a-z]*-[\-.0-9_a-z]*$').hasMatch(localName);
  }

  static void defineCustomElement(String tagName, ElementCreator creator) {
    if (!_isValidCustomElementName(tagName)) {
      throw ArgumentError('The element name "$tagName" is not valid.');
    }
    defineElement(tagName.toUpperCase(), creator);
  }

  Future<void> load(WebFBundle bundle) async {
    await controller?.load(bundle);
  }

  Future<void> reload() async {
    await controller?.reload();
  }

  WebF(
      {Key? key,
      this.viewportWidth,
      this.viewportHeight,
      this.bundle,
      this.onControllerCreated,
      this.onLoad,
      this.navigationDelegate,
      this.javaScriptChannel,
      this.background,
      this.gestureListener,
      this.devToolsService,
      // webf's http client interceptor.
      this.httpClientInterceptor,
      this.uriParser,
      this.routeObserver,
      this.initialCookies,
      // webf's viewportWidth options only works fine when viewportWidth is equal to window.physicalSize.width / window.devicePixelRatio.
      // Maybe got unexpected error when change to other values, use this at your own risk!
      // We will fixed this on next version released. (v0.6.0)
      // Disable viewportWidth check and no assertion error report.
      bool disableViewportWidthAssertion = false,
      // webf's viewportHeight options only works fine when viewportHeight is equal to window.physicalSize.height / window.devicePixelRatio.
      // Maybe got unexpected error when change to other values, use this at your own risk!
      // We will fixed this on next version release. (v0.6.0)
      // Disable viewportHeight check and no assertion error report.
      bool disableViewportHeightAssertion = false,
      // Callback functions when loading Javascript scripts failed.
      this.onLoadError,
      this.animationController,
      this.onJSError})
      : super(key: key);

  @override
  void debugFillProperties(DiagnosticPropertiesBuilder properties) {
    super.debugFillProperties(properties);
    properties.add(DiagnosticsProperty<double>('viewportWidth', viewportWidth));
    properties.add(DiagnosticsProperty<double>('viewportHeight', viewportHeight));
  }

  @override
  WebFState createState() => WebFState();
}

class WebFState extends State<WebF> with RouteAware {
  bool _disposed = false;

  final Set<WebFWidgetElementToWidgetAdapter> customElementWidgets = {};
  void onCustomElementWidgetAdd(WebFWidgetElementToWidgetAdapter adapter) {
    Future.microtask(() {
      if (!_disposed) {
        setState(() {
          customElementWidgets.add(adapter);
        });
      }
    });
  }

  void onCustomElementWidgetRemove(WebFWidgetElementToWidgetAdapter adapter) {
    Future.microtask(() {
      if (!_disposed) {
        setState(() {
          customElementWidgets.remove(adapter);
        });
      }
    });
  }

  bool _flutterScreenIsReady = false;

  watchWindowIsReady() {
    double viewportWidth = window.physicalSize.width / window.devicePixelRatio;
    double viewportHeight = window.physicalSize.height / window.devicePixelRatio;

    if (viewportWidth == 0.0 && viewportHeight == 0.0) {
      // window.physicalSize are Size.zero when app first loaded. This only happened on Android and iOS physical devices with release build.
      // We should wait for onMetricsChanged when window.physicalSize get updated from Flutter Engine.
      VoidCallback? _ordinaryOnMetricsChanged = window.onMetricsChanged;
      window.onMetricsChanged = () async {
        if (window.physicalSize == Size.zero) {
          return;
        }
        setState(() {
          _flutterScreenIsReady = true;
        });

        // Should proxy to ordinary window.onMetricsChanged callbacks.
        if (_ordinaryOnMetricsChanged != null) {
          _ordinaryOnMetricsChanged();
          // Recover ordinary callback to window.onMetricsChanged
          window.onMetricsChanged = _ordinaryOnMetricsChanged;
        }
      };
    } else {
      _flutterScreenIsReady = true;
    }
  }

  @override
  void initState() {
    super.initState();
    watchWindowIsReady();
  }

  @override
  Widget build(BuildContext context) {
    if (!_flutterScreenIsReady) {
      return SizedBox(width: 0, height: 0);
    }

    Widget _defaultContextMenuBuilder(BuildContext context, SelectableRegionState selectableRegionState) {
      return AdaptiveTextSelectionToolbar.selectableRegion(
        selectableRegionState: selectableRegionState,
      );
    }

    TextSelectionControls? controls;
    switch (Theme.of(context).platform) {
      case TargetPlatform.android:
      case TargetPlatform.fuchsia:
        controls ??= materialTextSelectionHandleControls;
        break;
      case TargetPlatform.iOS:
        controls ??= cupertinoTextSelectionHandleControls;
        break;
      case TargetPlatform.linux:
      case TargetPlatform.windows:
        controls ??= desktopTextSelectionHandleControls;
        break;
      case TargetPlatform.macOS:
        controls ??= cupertinoDesktopTextSelectionHandleControls;
        break;
    }

    return RepaintBoundary(
      child:
      SelectableRegion(
        focusNode: FocusNode(),
        selectionControls: controls,
        contextMenuBuilder: _defaultContextMenuBuilder,
        child: WebFContext(
        child: WebFRootRenderObjectWidget(
          widget,
          onCustomElementAttached: onCustomElementWidgetAdd,
          onCustomElementDetached: onCustomElementWidgetRemove,
          children: customElementWidgets.toList(),
        ),
      ),
      )
    );
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (widget.routeObserver != null) {
      widget.routeObserver!.subscribe(this, ModalRoute.of(context)!);
    }
  }

  // Resume call timer and callbacks when webf widget change to visible.
  @override
  void didPopNext() {
    assert(widget.controller != null);
    widget.controller!.resume();
  }

  // Pause all timer and callbacks when webf widget has been invisible.
  @override
  void didPushNext() {
    assert(widget.controller != null);
    widget.controller!.pause();
  }

  @override
  void dispose() {
    if (widget.routeObserver != null) {
      widget.routeObserver!.unsubscribe(this);
    }
    super.dispose();
    _disposed = true;
  }

  @override
  void deactivate() {
    super.deactivate();
  }
}

class WebFContext extends InheritedWidget {
  WebFContext({required super.child});

  @override
  bool updateShouldNotify(covariant InheritedWidget oldWidget) {
    return false;
  }

  @override
  InheritedElement createElement() {
    return WebFContextInheritElement(this);
  }
}

class WebFContextInheritElement extends InheritedElement {
  WebFContextInheritElement(super.widget);

  WebFController? controller;
}

class WebFRootRenderObjectWidget extends MultiChildRenderObjectWidget {
  final OnCustomElementAttached onCustomElementAttached;
  final OnCustomElementDetached onCustomElementDetached;

  // Creates a widget that visually hides its child.
  WebFRootRenderObjectWidget(
    WebF widget, {
    Key? key,
    required List<Widget> children,
    required this.onCustomElementAttached,
    required this.onCustomElementDetached,
  })  : _webfWidget = widget,
        super(key: key, children: children);

  final WebF _webfWidget;

  @override
  RenderObject createRenderObject(BuildContext context) {
    if (kProfileMode) {
      PerformanceTiming.instance().mark(PERF_CONTROLLER_INIT_START);
    }

    double viewportWidth = _webfWidget.viewportWidth ?? window.physicalSize.width / window.devicePixelRatio;
    double viewportHeight = _webfWidget.viewportHeight ?? window.physicalSize.height / window.devicePixelRatio;

    ThemeData theme = Theme.of(context);
    final Color effectiveSelectionColor = theme.textSelectionTheme.selectionColor ?? theme.colorScheme.primary.withOpacity(0.40);
    final SelectionRegistrarScope? scope = context.dependOnInheritedWidgetOfExactType<SelectionRegistrarScope>();

    WebFController controller = WebFController(shortHash(_webfWidget), viewportWidth, viewportHeight,
        background: _webfWidget.background,
        showPerformanceOverlay: Platform.environment[ENABLE_PERFORMANCE_OVERLAY] != null,
        entrypoint: _webfWidget.bundle,
        // Execute entrypoint when mount manually.
        autoExecuteEntrypoint: false,
        onLoad: _webfWidget.onLoad,
        onLoadError: _webfWidget.onLoadError,
        onJSError: _webfWidget.onJSError,
        methodChannel: _webfWidget.javaScriptChannel,
        gestureListener: _webfWidget.gestureListener,
        navigationDelegate: _webfWidget.navigationDelegate,
        devToolsService: _webfWidget.devToolsService,
        httpClientInterceptor: _webfWidget.httpClientInterceptor,
        onCustomElementAttached: onCustomElementAttached,
        onCustomElementDetached: onCustomElementDetached,
        initialCookies: _webfWidget.initialCookies,
        uriParser: _webfWidget.uriParser,
        registrar: scope?.registrar,
        selectionColor: effectiveSelectionColor);

    if (kProfileMode) {
      PerformanceTiming.instance().mark(PERF_CONTROLLER_INIT_END);
    }

    (context as _WebFRenderObjectElement).controller = controller;

    OnControllerCreated? onControllerCreated = _webfWidget.onControllerCreated;
    if (onControllerCreated != null) {
      onControllerCreated(controller);
    }

    return controller.view.getRootRenderObject();
  }

  @override
  void updateRenderObject(BuildContext context, covariant RenderObject renderObject) {
    super.updateRenderObject(context, renderObject);
    WebFController controller = (context as _WebFRenderObjectElement).controller!;
    controller.name = shortHash(_webfWidget);

    bool viewportWidthHasChanged = controller.view.viewportWidth != _webfWidget.viewportWidth;
    bool viewportHeightHasChanged = controller.view.viewportHeight != _webfWidget.viewportHeight;

    double viewportWidth = _webfWidget.viewportWidth ?? window.physicalSize.width / window.devicePixelRatio;
    double viewportHeight = _webfWidget.viewportHeight ?? window.physicalSize.height / window.devicePixelRatio;

    if (controller.view.document.documentElement == null) return;

    if (viewportWidthHasChanged) {
      controller.view.viewportWidth = viewportWidth;
      controller.view.document.documentElement!.renderStyle.width = CSSLengthValue(viewportWidth, CSSLengthType.PX);
    }

    if (viewportHeightHasChanged) {
      controller.view.viewportHeight = viewportHeight;
      controller.view.document.documentElement!.renderStyle.height = CSSLengthValue(viewportHeight, CSSLengthType.PX);
    }
  }

  @override
  _WebFRenderObjectElement createElement() {
    return _WebFRenderObjectElement(this);
  }
}

class _WebFRenderObjectElement extends MultiChildRenderObjectElement {
  _WebFRenderObjectElement(WebFRootRenderObjectWidget widget) : super(widget);

  WebFController? controller;

  @override
  void mount(Element? parent, Object? newSlot) async {
    super.mount(parent, newSlot);
    assert(parent is WebFContextInheritElement);
    assert(controller != null);
    (parent as WebFContextInheritElement).controller = controller;
    await controller!.executeEntrypoint(animationController: widget._webfWidget.animationController);
  }

  @override
  void unmount() {
    super.unmount();
    controller?.dispose();
  }

  // RenderObjects created by webf are manager by webf itself. There are no needs to operate renderObjects on _WebFRenderObjectElement.
  @override
  void insertRenderObjectChild(RenderObject child, Object? slot) {}

  @override
  void moveRenderObjectChild(RenderObject child, Object? oldSlot, Object? newSlot) {}

  @override
  void removeRenderObjectChild(RenderObject child, Object? slot) {}

  @override
  WebFRootRenderObjectWidget get widget => super.widget as WebFRootRenderObjectWidget;
}
