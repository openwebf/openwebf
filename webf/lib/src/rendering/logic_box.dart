import 'dart:math' as math;
import 'dart:math';

import 'package:flutter/rendering.dart';
import 'package:webf/css.dart';
import 'package:webf/rendering.dart';
import 'package:webf/src/rendering/text.dart';

class LogicInlineBox {
  RenderBox renderObject;
  LogicLineBox? parentLine;

  LogicInlineBox({required this.renderObject, this.parentLine});

  bool get happenLineJoin {
    if(renderObject is RenderFlowLayout) {
      return (renderObject as RenderFlowLayout).happenLineJoin();
    }
    if(renderObject is RenderTextBox) {
      return (renderObject as RenderTextBox).happenLineJoin();
    }
    return false;
  }

  bool get isBlockLevel {
    if(renderObject is RenderFlowLayout) {
      return (renderObject as RenderFlowLayout).isBlockLevel(renderObject);
    }
    return false;
  }

  double get scrollableWidth {
    return _getInlineBoxScrollableSize().width;
  }

  double get scrollableHeight {
    return _getInlineBoxScrollableSize().height;
  }

  double get relativeOffsetX {
    double childOffsetX = 0;
    if (renderObject is RenderBoxModel) {
      RenderStyle childRenderStyle = (renderObject as RenderBoxModel).renderStyle;
      Offset? relativeOffset = CSSPositionedLayout.getRelativeOffset(childRenderStyle);
      if (relativeOffset != null) {
        childOffsetX += relativeOffset.dx;
      }

      // Scrollable overflow area is defined in the following spec
      // which includes margin, position and transform offset.
      // https://www.w3.org/TR/css-overflow-3/#scrollable-overflow-region

      // Add offset of margin.
      childOffsetX += childRenderStyle.marginLeft.computedValue + childRenderStyle.marginRight.computedValue;
      final Offset? transformOffset = (childRenderStyle as CSSRenderStyle).effectiveTransformOffset;
      if (transformOffset != null) {
        childOffsetX = transformOffset.dx > 0 ? childOffsetX + transformOffset.dx : childOffsetX;
      }
    }
    return childOffsetX;
  }

  double get relativeOffsetY {
    double childOffsetY = 0;
    if (renderObject is RenderBoxModel) {
      RenderStyle childRenderStyle = (renderObject as RenderBoxModel).renderStyle;
      Offset? relativeOffset = CSSPositionedLayout.getRelativeOffset(childRenderStyle);

      // Scrollable overflow area is defined in the following spec
      // which includes margin, position and transform offset.
      // https://www.w3.org/TR/css-overflow-3/#scrollable-overflow-region

      if (renderObject.parent != null && renderObject.parent is RenderFlowLayout) {
        childOffsetY += (renderObject.parent as RenderFlowLayout).getChildMarginTop(renderObject as RenderBoxModel) +
            (renderObject.parent as RenderFlowLayout).getChildMarginBottom(renderObject as RenderBoxModel);
      }

      if (relativeOffset != null) {
        childOffsetY += relativeOffset.dy;
      }
      final Offset? transformOffset = (childRenderStyle as CSSRenderStyle).effectiveTransformOffset;
      if (transformOffset != null) {
        childOffsetY = transformOffset.dy > 0 ? childOffsetY + transformOffset.dy : childOffsetY;
      }
    }
    return childOffsetY;
  }

  double get width => renderObject.size.width;

  double get height => renderObject.size.height;

  Size _getInlineBoxScrollableSize() {
    Size scrollableSize = renderObject.size;
    if (renderObject is RenderBoxModel) {
      RenderStyle childRenderStyle = (renderObject as RenderBoxModel).renderStyle;
      CSSOverflowType overflowX = childRenderStyle.effectiveOverflowX;
      CSSOverflowType overflowY = childRenderStyle.effectiveOverflowY;
      // Only non scroll container need to use scrollable size, otherwise use its own size.
      if (overflowX == CSSOverflowType.visible && overflowY == CSSOverflowType.visible) {
        scrollableSize = (renderObject as RenderBoxModel).scrollableSize;
      }
    }
    return scrollableSize;
  }

  bool isLineHeightValid() {
    if (renderObject is RenderTextBox) {
      return true;
    } else if (renderObject is RenderBoxModel) {
      CSSDisplay? childDisplay = (renderObject as RenderBoxModel).renderStyle.display;
      return childDisplay == CSSDisplay.inline ||
          childDisplay == CSSDisplay.inlineBlock ||
          childDisplay == CSSDisplay.inlineFlex;
    }
    return false;
  }

  double getCrossAxisExtent(CSSLengthValue? lineHeightFromParent, double? marginVertical) {
    double? lineHeight = isLineHeightValid() ? getLineHeight(lineHeightFromParent) : 0;
    double marginV = 0;

    if (renderObject is RenderBoxModel) {
      marginV = marginVertical ?? 0;
    }
    Size childSize = getChildSize() ?? Size.zero;

    return lineHeight != null ? math.max(lineHeight, childSize.height) + marginV : childSize.height + marginV;
  }

  Size? getChildSize() {
    if (renderObject is RenderBoxModel) {
      return (renderObject as RenderBoxModel).boxSize;
    } else if (renderObject is RenderPositionPlaceholder) {
      return (renderObject as RenderPositionPlaceholder).boxSize;
    } else if (renderObject.hasSize) {
      // child is WidgetElement.
      return renderObject.size;
    }
    return null;
  }

  double? getLineHeight(CSSLengthValue? lineHeightFromParent) {
    CSSLengthValue? lineHeight;
    if (renderObject is RenderTextBox || renderObject is RenderBoxModel) {
      lineHeight = lineHeightFromParent;
    } else if (renderObject is RenderPositionPlaceholder) {
      lineHeight = (renderObject as RenderPositionPlaceholder).positioned!.renderStyle.lineHeight;
    }
    if (lineHeight != null && lineHeight.type != CSSLengthType.NORMAL) {
      return lineHeight.computedValue;
    }
    return null;
  }

  double getChildAscent(double marginTop, double marginBottom) {
    // Distance from top to baseline of child.
    double? childAscent = renderObject.getDistanceToBaseline(TextBaseline.alphabetic, onlyReal: true);
    Size? childSize = getChildSize();
    double baseline = renderObject.parent is RenderFlowLayout
        ? marginTop + childSize!.height + marginBottom
        : marginTop + childSize!.height;
    // When baseline of children not found, use boundary of margin bottom as baseline.
    double extentAboveBaseline = childAscent ?? baseline;
    extentAboveBaseline = max(extentAboveBaseline, 0);
    return extentAboveBaseline;
  }

  void applyRelativeOffset(Offset? relativeOffset, double outLineMainSize, double leadingSpace) {
    RenderLayoutParentData? boxParentData = renderObject.parentData as RenderLayoutParentData?;

    if (boxParentData != null) {
      Offset? styleOffset;
      // Text node does not have relative offset
      if (renderObject is RenderBoxModel) {
        styleOffset = CSSPositionedLayout.getRelativeOffset((renderObject as RenderBoxModel).renderStyle);
      }

      if (relativeOffset != null) {
        if (styleOffset != null) {
          boxParentData.offset = relativeOffset.translate(styleOffset.dx, styleOffset.dy);
        } else {
          boxParentData.offset = relativeOffset;
        }
      } else {
        boxParentData.offset = styleOffset!;
      }
    }
  }
}

class LogicTextInlineBox extends LogicInlineBox {
  Rect logicRect;

  LogicTextInlineBox({required this.logicRect, required RenderTextBox renderObject, parentLine, isDirty = true})
      : super(renderObject: renderObject, parentLine: parentLine);

  get isLineBreak {
    return false;
  }

  @override
  double get scrollableWidth {
    return logicRect.width;
  }

  @override
  double get scrollableHeight {
    return logicRect.height;
  }

  @override
  double get relativeOffsetX {
    return 0;
  }

  @override
  double get relativeOffsetY {
    return 0;
  }

  @override
  double get width => logicRect.width;

  @override
  double get height => logicRect.height;

  @override
  Size? getChildSize() {
    return logicRect.size;
  }

  @override
  double getChildAscent(double marginTop, double marginBottom) {
    double? childAscent = (renderObject as RenderTextBox)
        .getLineAscent((renderObject as RenderTextBox).textInLineBoxes.findIndex(this))![0];
    return childAscent;
  }

  @override
  void applyRelativeOffset(Offset? relativeOffset, double outLineMainSize, double leadingSpace) {
    RenderLayoutParentData? boxParentData = renderObject.parentData as RenderLayoutParentData?;
    if (boxParentData != null) {
      int index = (renderObject as RenderTextBox).textInLineBoxes.findIndex(this);
      if (relativeOffset != null) {
        // set RenderTextBox offset when update first LogicTextInlineBox offset
        if (index == 0) {
          boxParentData.offset = Offset(outLineMainSize, relativeOffset.dy);
        }
        // paint logic not contain leading space, text paint use this offset, need - parent.offset.dy
        (renderObject as RenderTextBox)
            .updateRenderTextLineOffset(index, Offset(leadingSpace, relativeOffset.dy - boxParentData.offset.dy));
      }
    }
  }
}

class LogicLineBox {
  LogicLineBox({
    required this.renderObject,
    this.breakForExtentShort = false,
    required double mainAxisExtent,
    required this.crossAxisExtent,
    required this.baselineExtent,
    required this.baselineBelowExtent,
  }) {
    _mainAxisExtent = mainAxisExtent;
  }
  bool? isFirst;
  RenderBox renderObject;
  List<LogicInlineBox> inlineBoxes = [];
  double crossAxisExtent;
  double baselineExtent;
  double baselineBelowExtent;

  // TODO this value effect need optimize
  double firstLineLeftExtent = 0;
  bool breakForExtentShort;

  double _mainAxisExtent = 0;

  int get innerLineLength {
    if (inlineBoxes.isEmpty) {
      return 0;
    }

    /// innerLineLength is dynamic calculate, if this is firstLine and firstLineLeftExtent > 0
    /// and content size is small which will join to outer pre-line
    bool singleInlineBoxSmallSize = inlineBoxes.length == 1
        && !inlineBoxes.first.happenLineJoin
        && !inlineBoxes.first.isBlockLevel;

    if((singleInlineBoxSmallSize || (inlineBoxes.length > 1)) && firstLineLeftExtent > 0 && (isFirst ?? false)) {
      return 0;
    }

    List<int> list = inlineBoxes.map<int>((box) {
      if (box.renderObject is RenderFlowLayout) {
        RenderFlowLayout render =  (box.renderObject as RenderFlowLayout);
        int length = render.lineBoxes.innerLineLength;
        if(render.happenLineJoin()) {
          return render.lineBoxes.innerLineLength - 1;
        }
        return length;
      }
      return 1;
    }).toList();

    return list.reduce(max);
  }

  LogicInlineBox? get first {
    return inlineBoxes.isNotEmpty ? inlineBoxes.first : null;
  }

  LogicInlineBox? get last {
    return inlineBoxes.isNotEmpty ? inlineBoxes.last : null;
  }

  // This value use for calculate RenderObject size.
  double get mainAxisExtent {
    return _mainAxisExtent + firstLineLeftExtent;
  }

  bool get happenLineJoin {
    if(inlineBoxes.isEmpty) {
      return false;
    }

    return inlineBoxes.first.happenLineJoin;
  }

  void set mainAxisExtent(double value) {
    _mainAxisExtent = value;
  }

  // This value use for calculate break line、line join、set children offset
  double get mainAxisExtentWithoutLineJoin {
    return _mainAxisExtent;
  }

  double defaultLastLineMainExtent() {
    double lineMainExtent = mainAxisExtent;
    if (last?.renderObject != null &&
        last?.renderObject is RenderFlowLayout &&
        (last!.renderObject as RenderFlowLayout).happenLineJoin()) {
      lineMainExtent = mainAxisExtentWithoutLineJoin;
    }
    return lineMainExtent;
  }

  double findLastLineRenderMainExtent() {
    LogicLineBox? nextLineBox = null;
    double mainAxisExtentUse = mainAxisExtent;
    if (inlineBoxes.isEmpty) {
      return mainAxisExtentUse;
    }
    nextLineBox = this;
    do {
      RenderObject theLineLastRender = nextLineBox!.inlineBoxes.last.renderObject;
      if (nextLineBox.inlineBoxes.length == 1 && theLineLastRender is RenderFlowLayout) {
        nextLineBox = theLineLastRender.lineBoxes.last;
        continue;
      }
      if (theLineLastRender is RenderFlowLayout && theLineLastRender.happenLineJoin()) {
        mainAxisExtentUse = nextLineBox.mainAxisExtentWithoutLineJoin;
        break;
      }
      if (theLineLastRender is RenderTextBox && theLineLastRender.textInLineBoxes.length > 1) {
        mainAxisExtentUse = theLineLastRender.textInLineBoxes.lastChild.width;
        break;
      }
      mainAxisExtentUse = nextLineBox.mainAxisExtent;
      break;
    } while (true);

    return mainAxisExtentUse;
  }

  int get length {
    return inlineBoxes.length;
  }

  double findLastLineJoinCrossAxisExtent() {
    LogicLineBox? nextLineBox = null;
    double crossAxisExtentLast = crossAxisExtent;
    if (inlineBoxes.isEmpty) {
      return crossAxisExtentLast;
    }
    nextLineBox = this;
    do {
      RenderObject theLineLastRender = nextLineBox!.inlineBoxes.last.renderObject;
      if (theLineLastRender is RenderFlowLayout) {
        RenderFlowLayout ref = theLineLastRender;
        if (ref.isInlineBlockOrInlineFlex(ref)) {
          crossAxisExtentLast = ref.lineBoxes.last.crossAxisExtent;
          break;
        }
        if (ref.lineBoxes.isEmpty) {
          if (ref.boxSize != Size.zero && (ref.boxSize?.height ?? 0) > 0) {
            crossAxisExtentLast = ref.boxSize?.height ?? 0;
          } else {
            // the last render size == zero, need use the line cross extent
            crossAxisExtentLast = nextLineBox.crossAxisExtent;
          }
          break;
        }

        if (ref.lineBoxes.length >= 1) {
          nextLineBox = ref.lineBoxes.last;
          continue;
        }
      }
      if (theLineLastRender is RenderTextBox) {
        if (theLineLastRender.textInLineBoxes.length > 1) {
          crossAxisExtentLast = theLineLastRender.textInLineBoxes.lastChild.height;
        } else {
          crossAxisExtentLast = nextLineBox.crossAxisExtent;
        }
        break;
      }
      crossAxisExtentLast = nextLineBox.crossAxisExtent;
      break;
    } while (true);
    return crossAxisExtentLast;
  }

  appendInlineBox(
      LogicInlineBox box, double childMainAxisExtent, double childCrossAxisExtent, List<double>? baselineSize) {
    box.parentLine = this;
    inlineBoxes.add(box);
    mainAxisExtent = _mainAxisExtent + childMainAxisExtent;
    List<double> newCrossAxisSize = calculateMaxCrossAxisExtent(
        crossAxisExtent, baselineExtent, baselineBelowExtent, childCrossAxisExtent, baselineSize);
    crossAxisExtent = newCrossAxisSize[0];
    baselineExtent = newCrossAxisSize[1];
    baselineBelowExtent = newCrossAxisSize[2];
  }

  List<double> calculateMaxCrossAxisExtent(double lastCrossAxisExtent, double lastAboveBaseLine,
      double lastBelowBaseLine, double childCrossAxisExtent, List<double>? baselineSize) {
    double newAboveBaseLine = lastAboveBaseLine;
    double newBelowBaseLine = lastBelowBaseLine;

    if (baselineSize != null) {
      newAboveBaseLine = math.max(
        baselineSize[0],
        newAboveBaseLine,
      );
      newBelowBaseLine = math.max(
        baselineSize[1],
        newBelowBaseLine,
      );
      return [math.max(lastCrossAxisExtent, newAboveBaseLine + newBelowBaseLine), newAboveBaseLine, newBelowBaseLine];
    } else {
      return [math.max(lastCrossAxisExtent, childCrossAxisExtent), newAboveBaseLine, newBelowBaseLine];
    }
  }

  bool get isNotEmpty {
    return inlineBoxes.isNotEmpty;
  }

  double get lineMainAxisAutoSize {
    double runMainExtent = 0;
    for (int i = 0; i < inlineBoxes.length; i++) {
      LogicInlineBox box = inlineBoxes[i];
      RenderBox render = box.renderObject;
      double runChildMainSize = box.renderObject.size.width;
      if (render is RenderTextBox) {
        runChildMainSize = render.minContentWidth;
      }
      // Should add horizontal margin of child to the main axis auto size of parent.
      if (render is RenderBoxModel) {
        double childMarginLeft = render.renderStyle.marginLeft.computedValue;
        double childMarginRight = render.renderStyle.marginRight.computedValue;
        runChildMainSize += childMarginLeft + childMarginRight;
      }
      runMainExtent += runChildMainSize;
    }
    return runMainExtent;
  }

  double get lineCrossAxisAutoSize {
    double runCrossExtent = 0;
    for (int i = 0; i < inlineBoxes.length; i++) {
      LogicInlineBox box = inlineBoxes[i];
      RenderBox render = box.renderObject;

      double runChildCrossSize = render.size.height;
      if (render is RenderTextBox && box is LogicTextInlineBox) {
        runChildCrossSize = box.logicRect.height;
      }
      if (runChildCrossSize > runCrossExtent) {
        runCrossExtent = runChildCrossSize;
      }
    }
    return runCrossExtent;
  }

  double get maxMainAxisScrollableSizeOnLine {
    double preSiblingsMainSize = 0;
    double maxWidth = 0;
    for (int i = 0; i < inlineBoxes.length; i++) {
      LogicInlineBox box = inlineBoxes[i];
      double width = preSiblingsMainSize + box.scrollableWidth + box.relativeOffsetX;
      if (width > maxWidth) {
        maxWidth = width;
      }
      preSiblingsMainSize += box.width;
    }
    return maxWidth;
  }

  double get maxCrossAxisScrollableSizeOnLine {
    double maxHeight = 0;
    for (int i = 0; i < inlineBoxes.length; i++) {
      LogicInlineBox box = inlineBoxes[i];
      double height = box.scrollableHeight + box.relativeOffsetY;
      if (height > maxHeight) {
        maxHeight = height;
      }
    }
    return maxHeight;
  }

  void dispose() {
    inlineBoxes.clear();
  }
}
