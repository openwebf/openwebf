/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'package:flutter/cupertino.dart';
import 'package:flutter/rendering.dart';
import 'package:webf/css.dart';
import 'package:webf/dom.dart';
import 'package:webf/foundation.dart';
import 'package:webf/rendering.dart';
import 'package:webf/svg.dart';
import 'aspect_ratio.dart';

const DEFAULT_VIEW_BOX_TOP = 0.0;
const DEFAULT_VIEW_BOX_LEFT = 0.0;
const DEFAULT_VIEW_BOX_WIDTH = 300.0;
const DEFAULT_VIEW_BOX_HEIGHT = 150.0;
const DEFAULT_VIEW_BOX = Rect.fromLTWH(DEFAULT_VIEW_BOX_LEFT,
    DEFAULT_VIEW_BOX_TOP, DEFAULT_VIEW_BOX_WIDTH, DEFAULT_VIEW_BOX_HEIGHT);

class SVGSVGElement extends SVGGraphicsElement {
  @override
  bool get isRepaintBoundary => true;

  @override
  Map<String, dynamic> get defaultStyle => {
        DISPLAY: INLINE,
        WIDTH: DEFAULT_VIEW_BOX_WIDTH.toString(),
        HEIGHT: DEFAULT_VIEW_BOX_HEIGHT.toString(),
      };

  Rect? _viewBox;
  get viewBox => _viewBox;

  var _ratio = SVGPreserveAspectRatio();
  get ratio => _ratio;

  @override
  get presentationAttributeConfigs => super.presentationAttributeConfigs
    ..addAll([
      SVGPresentationAttributeConfig('width', property: true),
      SVGPresentationAttributeConfig('height', property: true),
    ]);

  SVGSVGElement(super.context) {}

  @override
  RenderBoxModel createRenderSVG({RenderBoxModel? previous, bool isRepaintBoundary = false}) {
    RenderSVGRoot root = RenderSVGRoot(renderStyle: renderStyle)..viewBox = viewBox..ratio = ratio;

    if (previous is RenderSVGRoot) {
      List<RenderBox> children = previous.getChildren();
      previous.removeAll();
      root.addAll(children);
    }

    return root;
  }

  @override
  void willAttachRenderer() {
    super.willAttachRenderer();
    style.addStyleChangeListener(_stylePropertyChanged);
  }

  void _stylePropertyChanged(String property, String? original, String present,
      {String? baseHref}) {
    if (property == COLOR) {
      renderBoxModel?.markNeedsPaint();
    }
  }


  @override
  void initializeAttributes(Map<String, ElementAttributeProperty> attributes) {
    super.initializeAttributes(attributes);
    attributes.addAll({
      'viewBox': ElementAttributeProperty(
          getter: () =>
              '${_viewBox?.left ?? 0} ${_viewBox?.top ?? 0} ${_viewBox?.width ?? 0} ${_viewBox?.height ?? 0}',
          setter: (val) {
            final nextViewBox = parseViewBox(val);
            _viewBox = nextViewBox;
            if (nextViewBox != (renderBoxModel as RenderSVGRoot?)?.viewBox) {
              (renderBoxModel as RenderSVGRoot?)?.viewBox = nextViewBox;
            }
          }),
      'preserveAspectRatio': ElementAttributeProperty(setter: (val) {
        final nextRatio = SVGPreserveAspectRatio.parse(val);
        if (nextRatio == null) {
          // TODO: should log error like chrome
          return;
        }
        if (nextRatio != _ratio) {
          _ratio = nextRatio;
        }
      })
    });
  }

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {
    super.initializeProperties(properties);
    properties.addAll({'viewBox': BindingObjectProperty(getter: () => {})});
  }
}

Rect? parseViewBox(String viewBoxString) {
  final array = viewBoxString.split(' ');
  final left = double.tryParse(array[0]);
  final top = double.tryParse(array[1]);
  final width = double.tryParse(array[2]);
  final height = double.tryParse(array[3]);
  if (left != null && top != null && width != null && height != null) {
    return Rect.fromLTWH(left, top, width, height);
  }
  return null;
}
