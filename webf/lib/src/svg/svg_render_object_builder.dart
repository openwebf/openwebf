import 'dart:ffi';
import 'dart:convert' as convert;
import 'dart:ui' as ui;
import 'package:ffi/ffi.dart';
import 'package:webf/bridge.dart';
import 'package:webf/css.dart';
import 'package:webf/dom.dart';
import 'package:webf/painting.dart';
import 'package:webf/rendering.dart';
import 'package:webf/src/svg/rendering/container.dart';
import 'package:webf/svg.dart';

// Those size is get from chrome. I cannot found any specs for that. @XGHeaven
const double INTRINSIC_DEFAULT_WIDTH = 150;
const double INTRINSIC_DEFAULT_HEIGHT = 150;

class SVGRenderBoxBuilder {
  final Element target;
  final Future<ImageLoadResponse> imageLoader;
  late final Pointer<NativeGumboOutput> gumboPtr;
  late final Pointer<NativeGumboNode> root;
  ui.Rect? viewBox;
  double? width;
  double? height;

  SVGRenderBoxBuilder(this.imageLoader, {required this.target});

  ui.Size getIntrinsicSize() {
    if (width != null && height != null) {
      return ui.Size(width!, height!);
    } else if (viewBox != null) {
      if (width != null) {
        return ui.Size(width!, width! / (viewBox!.width / viewBox!.height));
      } else if (height != null) {
        return ui.Size(viewBox!.width / viewBox!.height * height!, height!);
      }
    }
    return ui.Size(INTRINSIC_DEFAULT_WIDTH, INTRINSIC_DEFAULT_HEIGHT);
  }

  Future<RenderBoxModel> decode() async {
    final resp = await imageLoader;

    final code = convert.utf8.decode(resp.bytes);

    final gumbo = parseSVGResult(code);
    final ptr = gumbo.ptr;

    Pointer<NativeGumboNode> root = nullptr;
    visitSVGTree(ptr.ref.root, (node, _) {
      final type = node.ref.type;
      if (type == GumboNodeType.GUMBO_NODE_ELEMENT) {
        final element = node.ref.v.element;
        if (element.tag_namespace == GumboNamespaceEnum.GUMBO_NAMESPACE_SVG &&
            element.tag == GumboTag.SVG) {
          // svg tag
          root = node;
          return false;
        }
      }
    });

    if (root == nullptr) {
      // IMPROVE: throw a more specific error
      throw Error();
    }

    final rootRenderObject = visitSVGTree(root, (node, parent) {
      final type = node.ref.type;
      if (type == GumboNodeType.GUMBO_NODE_ELEMENT) {
        final element = node.ref.v.element;
        final tagName = element.original_tag.data
            .toDartString(length: element.original_tag.length)
            .toUpperCase();
        final renderBox = getSVGRenderBox(tagName);
        if (renderBox == null) {
          return false;
        }
        final attributes = element.attributes;
        for (int i = 0; i < attributes.length; i++) {
          final attr = attributes.data[i] as Pointer<NativeGumboAttribute>;
          final name = attr.ref.name.toDartString();
          final value = attr.ref.value.toDartString();
          setAttribute(tagName, renderBox, name, value);
        }

        if (parent != null) {
          assert(parent is RenderSVGContainer);
          parent.insert(renderBox);
          // We need to build renderStyle tree manually.
          renderBox.renderStyle.parent = parent.renderStyle;
        }

        return renderBox;
      }
      return false;
    });

    freeSVGResult(gumbo);

    return rootRenderObject as RenderBoxModel;
  }

  RenderBoxModel? getSVGRenderBox(String tagName) {
    final Constructor = svgElementsRegistry[tagName];
    if (Constructor != null) {
      final element = Constructor(null);
      if (tagName == TAG_SVG) {
        /// See [setAttribute]
        element.renderStyle.height = CSSLengthValue.auto;
        element.renderStyle.width = CSSLengthValue.auto;
      }
      element.tagName = tagName;
      element.namespaceURI = SVG_ELEMENT_URI;
      /// These tags are only for setting properties and do not need to participate in time rendering.
      /// Such tags do not require renderBoxModel.
      if (tagName == TAG_DEFS ||
          tagName == TAG_LINEAR_GRADIENT ||
          tagName == TAG_STOP ||
          tagName == TAG_CLIP_PATH) {
        return null;
      }
      element.createRenderer();
      return element.renderBoxModel!;
    }
    print('Unknown SVG element $tagName');
    final element = SVGUnknownElement(null);
    element.tagName = tagName;
    element.namespaceURI = SVG_ELEMENT_URI;
    return element.renderBoxModel!;
  }

  void setAttribute(
      String tagName, RenderBoxModel model, String name, String value) {
    switch (tagName) {
      case TAG_SVG:
        {
          final root = model as RenderSVGRoot;
          switch (name) {
            case 'viewBox':
              {
                root.viewBox = parseViewBox(value);
                viewBox = root.viewBox;
                return;
              }
            // width/height is always fixed as 100% to match the parent size
            // IMPROVE: width/height should support unit like px/em/rem when needed in the future
            case 'width':
              {
                width = double.tryParse(value);
                return;
              }
            case 'height':
              {
                height = double.tryParse(value);
                return;
              }
          }
        }
    }
    // TODO: support base url in attribute value like background-image
    final parsed = model.renderStyle.resolveValue(name, value);
    if (parsed != null) {
      model.renderStyle.setProperty(name, parsed);
    }
  }
}

/// - You can return value in [visitor].
///   - `false` is skip the children
///   - others is treat as the [parentValue]
dynamic visitSVGTree(
    Pointer<NativeGumboNode> node,
    // ignore: avoid_annotating_with_dynamic
    dynamic Function(Pointer<NativeGumboNode>, dynamic) visitor,
    [parentValue]) {
  final currentValue = visitor(node, parentValue);
  if (currentValue == false) {
    return;
  }
  final type = node.ref.type;
  if (type == GumboNodeType.GUMBO_NODE_ELEMENT) {
    final element = node.ref.v.element;
    final children = element.children;
    for (int i = 0; i < children.length; i++) {
      final childRef = children.data[i] as Pointer<NativeGumboNode>;
      visitSVGTree(childRef, visitor, currentValue);
    }
  }

  return currentValue;
}
