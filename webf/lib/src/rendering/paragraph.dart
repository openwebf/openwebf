/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'dart:ui' as ui show LineMetrics, Gradient, Shader, TextBox, TextHeightBehavior;
import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';

const String _kEllipsis = '\u2026';

class _LineOffset {
  int start;
  int end;
  final double offset;

  _LineOffset(this.start, this.end, this.offset);
}

/// Forked from Flutter WebFRenderParagraph
/// Flutter's paragraph line-height calculation logic differs from web's
/// Use multiple line text painters to controll the leading of font in paint stage
/// A render object that displays a paragraph of text.
/// W3C line-height spec: https://www.w3.org/TR/css-inline-3/#inline-height
class WebFRenderParagraph extends RenderBox
    with
        ContainerRenderObjectMixin<RenderBox, TextParentData>,
        RenderBoxContainerDefaultsMixin<RenderBox, TextParentData>,
        RelayoutWhenSystemFontsChangeMixin {
  /// Creates a paragraph render object.
  ///
  /// The [text], [textAlign], [textDirection], [overflow], [softWrap], and
  /// [textScaleFactor] arguments must not be null.
  ///
  /// The [maxLines] property may be null (and indeed defaults to null), but if
  /// it is not null, it must be greater than zero.
  WebFRenderParagraph(
    InlineSpan text, {
    TextAlign textAlign = TextAlign.start,
    required TextDirection textDirection,
    bool softWrap = true,
    TextOverflow overflow = TextOverflow.clip,
    double textScaleFactor = 1.0,
    int? maxLines,
    Locale? locale,
    StrutStyle? strutStyle,
    TextWidthBasis textWidthBasis = TextWidthBasis.parent,
    ui.TextHeightBehavior? textHeightBehavior,
    List<RenderBox>? children,
    Color? selectionColor,
    SelectionRegistrar? registrar,
  })  : assert(text.debugAssertIsValid()),
        assert(maxLines == null || maxLines > 0),
        _softWrap = softWrap,
        _overflow = overflow,
        _selectionColor = selectionColor,
        _textPainter = TextPainter(
            text: text,
            textAlign: textAlign,
            textDirection: textDirection,
            textScaleFactor: textScaleFactor,
            locale: locale,
            strutStyle: strutStyle,
            textWidthBasis: textWidthBasis,
            textHeightBehavior: textHeightBehavior) {
    addAll(children);
    this.registrar = registrar;
  }

  @override
  void setupParentData(RenderBox child) {
    if (child.parentData is! TextParentData) child.parentData = TextParentData();
  }

  static final String _placeholderCharacter = String.fromCharCode(PlaceholderSpan.placeholderCodeUnit);

  final TextPainter _textPainter;

  // The text painter of each line
  late List<TextPainter> _lineTextPainters;

  // The line mertics of paragraph
  late List<ui.LineMetrics> _lineMetrics;

  // The vertical offset of each line
  late List<_LineOffset> _lineOffset;

  // The line height of paragraph
  double? _lineHeight;
  double? get lineHeight => _lineHeight;
  set lineHeight(double? value) {
    if (lineHeight == value) return;
    _lineHeight = value;
  }

  // The text to display.
  TextSpan get text => _textPainter.text as TextSpan;

  set text(TextSpan value) {
    switch (_textPainter.text!.compareTo(value)) {
      case RenderComparison.identical:
      case RenderComparison.metadata:
        return;
      case RenderComparison.paint:
      case RenderComparison.layout:
        _textPainter.text = value;
        _overflowShader = null;
        break;
    }
  }

  /// How the text should be aligned horizontally.
  TextAlign get textAlign => _textPainter.textAlign;

  set textAlign(TextAlign value) {
    if (_textPainter.textAlign == value) return;
    _textPainter.textAlign = value;
  }

  /// The directionality of the text.
  ///
  /// This decides how the [TextAlign.start], [TextAlign.end], and
  /// [TextAlign.justify] values of [textAlign] are interpreted.
  ///
  /// This is also used to disambiguate how to render bidirectional text. For
  /// example, if the [text] is an English phrase followed by a Hebrew phrase,
  /// in a [TextDirection.ltr] context the English phrase will be on the left
  /// and the Hebrew phrase to its right, while in a [TextDirection.rtl]
  /// context, the English phrase will be on the right and the Hebrew phrase on
  /// its left.
  ///
  /// This must not be null.
  TextDirection get textDirection => _textPainter.textDirection!;

  set textDirection(TextDirection value) {
    if (_textPainter.textDirection == value) return;
    _textPainter.textDirection = value;
    markNeedsLayout();
  }

  /// Whether the text should break at soft line breaks.
  ///
  /// If false, the glyphs in the text will be positioned as if there was
  /// unlimited horizontal space.
  ///
  /// If [softWrap] is false, [overflow] and [textAlign] may have unexpected
  /// effects.
  bool get softWrap => _softWrap;
  bool _softWrap;

  set softWrap(bool value) {
    if (_softWrap == value) return;
    _softWrap = value;
    markNeedsLayout();
  }

  /// How visual overflow should be handled.
  TextOverflow get overflow => _overflow;
  TextOverflow _overflow;

  set overflow(TextOverflow value) {
    if (_overflow == value) return;
    _overflow = value;
    _textPainter.ellipsis = value == TextOverflow.ellipsis ? _kEllipsis : null;
  }

  /// The number of font pixels for each logical pixel.
  ///
  /// For example, if the text scale factor is 1.5, text will be 50% larger than
  /// the specified font size.
  double get textScaleFactor => _textPainter.textScaleFactor;

  set textScaleFactor(double value) {
    if (_textPainter.textScaleFactor == value) return;
    _textPainter.textScaleFactor = value;
    _overflowShader = null;
  }

  /// An optional maximum number of lines for the text to span, wrapping if
  /// necessary. If the text exceeds the given number of lines, it will be
  /// truncated according to [overflow] and [softWrap].
  int? get maxLines => _textPainter.maxLines;

  /// The value may be null. If it is not null, then it must be greater than
  /// zero.
  set maxLines(int? value) {
    assert(value == null || value > 0);
    if (_textPainter.maxLines == value) return;
    _textPainter.maxLines = value;
    _overflowShader = null;
  }

  /// Used by this paragraph's internal [TextPainter] to select a
  /// locale-specific font.
  ///
  /// In some cases the same Unicode character may be rendered differently
  /// depending
  /// on the locale. For example the '骨' character is rendered differently in
  /// the Chinese and Japanese locales. In these cases the [locale] may be used
  /// to select a locale-specific font.
  Locale? get locale => _textPainter.locale;

  /// The value may be null.
  set locale(Locale? value) {
    if (_textPainter.locale == value) return;
    _textPainter.locale = value;
    _overflowShader = null;
  }

  /// {@macro flutter.painting.textPainter.strutStyle}
  StrutStyle? get strutStyle => _textPainter.strutStyle;

  /// The value may be null.
  set strutStyle(StrutStyle? value) {
    if (_textPainter.strutStyle == value) return;
    _textPainter.strutStyle = value;
    _overflowShader = null;
    markNeedsLayout();
  }

  /// {@macro flutter.widgets.basic.TextWidthBasis}
  TextWidthBasis get textWidthBasis => _textPainter.textWidthBasis;

  set textWidthBasis(TextWidthBasis value) {
    if (_textPainter.textWidthBasis == value) return;
    _textPainter.textWidthBasis = value;
    _overflowShader = null;
    markNeedsLayout();
  }

  /// {@macro flutter.dart:ui.textHeightBehavior}
  ui.TextHeightBehavior? get textHeightBehavior => _textPainter.textHeightBehavior;

  set textHeightBehavior(ui.TextHeightBehavior? value) {
    if (_textPainter.textHeightBehavior == value) return;
    _textPainter.textHeightBehavior = value;
    _overflowShader = null;
    markNeedsLayout();
  }

  /// The color to use when painting the selection.
  ///
  /// Ignored if the text is not selectable (e.g. if [registrar] is null).
  Color? get selectionColor => _selectionColor;
  Color? _selectionColor;
  set selectionColor(Color? value) {
    if (_selectionColor == value) {
      return;
    }
    _selectionColor = value;
    if (_lastSelectableFragments?.any((_SelectableFragment fragment) => fragment.value.hasSelection) ?? false) {
      markNeedsPaint();
    }
  }


  /// The ongoing selections in this paragraph.
  ///
  /// The selection does not include selections in [PlaceholderSpan] if there
  /// are any.
  @visibleForTesting
  List<TextSelection> get selections {
    if (_lastSelectableFragments == null) {
      return const <TextSelection>[];
    }
    final List<TextSelection> results = <TextSelection>[];
    for (final _SelectableFragment fragment in _lastSelectableFragments!) {
      if (fragment._textSelectionStart != null &&
          fragment._textSelectionEnd != null &&
          fragment._textSelectionStart!.offset != fragment._textSelectionEnd!.offset) {
        results.add(
            TextSelection(
                baseOffset: fragment._textSelectionStart!.offset,
                extentOffset: fragment._textSelectionEnd!.offset
            )
        );
      }
    }
    return results;
  }

  // Should be null if selection is not enabled, i.e. _registrar = null. The
  // paragraph splits on [PlaceholderSpan.placeholderCodeUnit], and stores each
  // fragment in this list.
  List<_SelectableFragment>? _lastSelectableFragments;

  /// The [SelectionRegistrar] this paragraph will be, or is, registered to.
  SelectionRegistrar? get registrar => _registrar;
  SelectionRegistrar? _registrar;
  set registrar(SelectionRegistrar? value) {
    if (value == _registrar) {
      return;
    }
    _removeSelectionRegistrarSubscription();
    _disposeSelectableFragments();
    _registrar = value;
    _updateSelectionRegistrarSubscription();
  }

  void _updateSelectionRegistrarSubscription() {
    if (_registrar == null) {
      return;
    }
    _lastSelectableFragments ??= _getSelectableFragments();
    _lastSelectableFragments!.forEach(_registrar!.add);
  }

  void _removeSelectionRegistrarSubscription() {
    if (_registrar == null || _lastSelectableFragments == null) {
      return;
    }
    _lastSelectableFragments!.forEach(_registrar!.remove);
  }

  List<_SelectableFragment> _getSelectableFragments() {
    final String plainText = text.toPlainText(includeSemanticsLabels: false);
    final List<_SelectableFragment> result = <_SelectableFragment>[];
    int start = 0;
    while (start < plainText.length) {
      int end = plainText.indexOf(_placeholderCharacter, start);
      if (start != end) {
        if (end == -1) {
          end = plainText.length;
        }
        result.add(_SelectableFragment(paragraph: this, range: TextRange(start: start, end: end), fullText: plainText));
        start = end;
      }
      start += 1;
    }
    return result;
  }

  void _disposeSelectableFragments() {
    if (_lastSelectableFragments == null) {
      return;
    }
    for (final _SelectableFragment fragment in _lastSelectableFragments!) {
      fragment.dispose();
    }
    _lastSelectableFragments = null;
  }

  @override
  void markNeedsLayout() {
    _lastSelectableFragments?.forEach((_SelectableFragment element) => element.didChangeParagraphLayout());
    super.markNeedsLayout();
  }

  @override
  void dispose() {
    _removeSelectionRegistrarSubscription();
    // _lastSelectableFragments may hold references to this WebFRenderParagraph.
    // Release them manually to avoid retain cycles.
    _lastSelectableFragments = null;
    _textPainter.dispose();
    super.dispose();
  }

  /// Compute distance to baseline of first text line
  double computeDistanceToFirstLineBaseline() {
    double firstLineOffset = _lineOffset[0].offset;
    ui.LineMetrics firstLineMetrics = _lineMetrics[0];

    // Use the baseline of the last line as paragraph baseline.
    return text.text == '' ? 0.0 : (firstLineOffset + firstLineMetrics.ascent);
  }

  /// Compute distance to baseline of last text line
  double computeDistanceToLastLineBaseline() {
    double lastLineOffset = _lineOffset[_lineOffset.length - 1].offset;
    ui.LineMetrics lastLineMetrics = _lineMetrics[_lineMetrics.length - 1];

    // Use the baseline of the last line as paragraph baseline.
    return text.text == '' ? 0.0 : (lastLineOffset + lastLineMetrics.ascent);
  }

  @override
  bool hitTestSelf(Offset position) => true;

  @override
  bool hitTestChildren(BoxHitTestResult result, {Offset? position}) {
    RenderBox? child = firstChild;
    while (child != null) {
      final TextParentData textParentData = child.parentData as TextParentData;
      final Matrix4 transform = Matrix4.translationValues(
        textParentData.offset.dx,
        textParentData.offset.dy,
        0.0,
      )..scale(
          textParentData.scale,
          textParentData.scale,
          textParentData.scale,
        );
      final bool isHit = result.addWithPaintTransform(
        transform: transform,
        position: position!,
        hitTest: (BoxHitTestResult result, Offset transformed) {
          assert(() {
            final Offset manualPosition = (position - textParentData.offset) / textParentData.scale!;
            return (transformed.dx - manualPosition.dx).abs() < precisionErrorTolerance &&
                (transformed.dy - manualPosition.dy).abs() < precisionErrorTolerance;
          }());
          return child!.hitTest(result, position: transformed);
        },
      );
      if (isHit) {
        return true;
      }
      child = childAfter(child);
    }
    return false;
  }

  @override
  void handleEvent(PointerEvent event, BoxHitTestEntry entry) {
    assert(debugHandleEvent(event, entry));
    if (event is! PointerDownEvent) return;
    _layoutTextWithConstraints(constraints);
    final Offset offset = entry.localPosition;
    final TextPosition position = _textPainter.getPositionForOffset(offset);
    final InlineSpan? span = _textPainter.text!.getSpanForPosition(position);
    if (span == null) {
      return;
    }
    if (span is TextSpan) {
      final TextSpan textSpan = span;
      textSpan.recognizer?.addPointer(event);
    }
  }

  bool _needsClipping = false;
  ui.Shader? _overflowShader;

  /// Whether this paragraph currently has a [dart:ui.Shader] for its overflow
  /// effect.
  ///
  /// Used to test this object. Not for use in production.
  @visibleForTesting
  bool get debugHasOverflowShader => _overflowShader != null;

  void _layoutText({double minWidth = 0.0, double maxWidth = double.infinity}) {
    final bool widthMatters = softWrap || overflow == TextOverflow.ellipsis;
    _textPainter.layout(
      minWidth: minWidth,
      maxWidth: widthMatters ? maxWidth : double.infinity,
    );
  }

  @override
  void systemFontsDidChange() {
    super.systemFontsDidChange();
    _textPainter.markNeedsLayout();
  }

  void _layoutTextWithConstraints(BoxConstraints constraints) {
    _layoutText(minWidth: constraints.minWidth, maxWidth: constraints.maxWidth);
  }

  // Get text range of each line in the paragraph.
  List<List<int>> _getLineTextRanges(TextPainter textPainter, TextSpan textSpan) {
    TextSelection selection = TextSelection(baseOffset: 0, extentOffset: textSpan.text!.length);
    List<TextBox> boxes = textPainter.getBoxesForSelection(selection);
    List<List<int>> lineTexts = [];
    int start = 0;
    int end;
    int index = -1;
    // Loop through each text box
    for (TextBox box in boxes) {
      // Text include ideographic characters such as Chinese may be counted as seperated text box
      // if font-family not specified, it needs to filter text box not started from 0 such as following:
      // TextBox.fromLTRBD(14.0, 1.7, 39.1, 18.2, TextDirection.ltr)
      if (box.left != 0) {
        continue;
      }

      index += 1;
      if (index == 0) continue;
      // Go one logical pixel within the box and get the position
      // of the character in the string.
      end = textPainter.getPositionForOffset(Offset(box.left + 1, box.top + 1)).offset;
      // add the substring to the list of lines
      lineTexts.add([start, end]);
      start = end;
    }
    // get the last substring
    lineTexts.add([start, textSpan.text!.length]);
    return lineTexts;
  }


  /// {@macro flutter.painting.textPainter.getFullHeightForCaret}
  ///
  /// Valid only after [layout].
  double? getFullHeightForCaret(TextPosition position) {
    assert(!debugNeedsLayout);
    _layoutTextWithConstraints(constraints);
    return _textPainter.getFullHeightForCaret(position, Rect.zero);
  }

  Offset _getOffsetForPosition(TextPosition position) {
    return getOffsetForCaret(position, Rect.zero) + Offset(0, getFullHeightForCaret(position) ?? 0.0);
  }

  // Compute line metrics and line offset according to line-height spec.
  // https://www.w3.org/TR/css-inline-3/#inline-height
  void _computeLineMetrics() {
    _lineMetrics = _textPainter.computeLineMetrics();
    // Leading of each line
    List<double> _lineLeading = [];

    _lineOffset = [];
    for (int i = 0; i < _lineMetrics.length; i++) {
      ui.LineMetrics lineMetric = _lineMetrics[i];
      // Do not add line height in the case of textOverflow ellipsis
      // cause height of line metric equals to 0.
      double leading = lineHeight != null && lineMetric.height != 0 ? lineHeight! - lineMetric.height : 0;
      _lineLeading.add(leading);
      // Offset of previous line
      double preLineBottom = i > 0 ? _lineOffset[i - 1].offset + _lineMetrics[i - 1].height + _lineLeading[i - 1] / 2 : 0;
      double offset = preLineBottom + leading / 2;
      _lineOffset.add(_LineOffset(-1, -1, offset));
    }
  }

  // Compute paragraph height according to line metrics.
  double _getParagraphHeight() {
    double paragraphHeight = 0;
    // Height of paragraph
    for (int i = 0; i < _lineMetrics.length; i++) {
      ui.LineMetrics lineMetric = _lineMetrics[i];
      // Do not add line height in the case of textOverflow ellipsis
      // cause height of line metric equals to 0.
      double height = lineHeight != null && lineMetric.height != 0 ? lineHeight! : lineMetric.height;
      paragraphHeight += height;
    }

    return paragraphHeight;
  }

  // Create and layout text painter of each line in the paragraph for later use
  // in the paint stage to adjust the vertical space between text painters according to
  // W3C line-height spec.
  void _relayoutMultiLineText() {
    final BoxConstraints constraints = this.constraints;
    // Get text of each line
    final originTextSpan = (_textPainter.text as TextSpan);
    List<List<int>> lineTexts = _getLineTextRanges(_textPainter, originTextSpan);

    _lineTextPainters = [];
    // Create text painter of each line and layout
    for (int i = 0; i < lineTexts.length; i++) {
      int start = lineTexts[i][0];
      int end = lineTexts[i][1];
      String lineText = originTextSpan.text!.substring(start, end);
      _lineOffset[i].start = start;
      _lineOffset[i].end = end;
      final TextSpan textSpan = TextSpan(
        text: lineText,
        style: text.style,
      );
      TextPainter _lineTextPainter = TextPainter(
          text: textSpan,
          textAlign: textAlign,
          textDirection: textDirection,
          textScaleFactor: textScaleFactor,
          ellipsis: overflow == TextOverflow.ellipsis ? _kEllipsis : null,
          locale: locale,
          strutStyle: strutStyle,
          textWidthBasis: textWidthBasis,
          textHeightBehavior: textHeightBehavior);
      _lineTextPainters.add(_lineTextPainter);

      final bool widthMatters = softWrap || overflow == TextOverflow.ellipsis;
      _lineTextPainter.layout(
        minWidth: constraints.minWidth,
        maxWidth: widthMatters ? constraints.maxWidth : double.infinity,
      );
    }
  }

  void layoutText() {
    final BoxConstraints constraints = this.constraints;
    _layoutTextWithConstraints(constraints);
    _computeLineMetrics();

    // @FIXME: Layout twice will hurt performance, ideally this logic should be done
    // in flutter text engine.
    // Layout each line of the paragraph individually to
    // place each line according to W3C line-height rule.
    if (lineHeight != null) {
      _relayoutMultiLineText();
    }
  }

  @override
  void performLayout() {
    layoutText();

    // We grab _textPainter.size and _textPainter.didExceedMaxLines here because
    // assigning to `size` will trigger us to validate our intrinsic sizes,
    // which will change _textPainter's layout because the intrinsic size
    // calculations are destructive. Other _textPainter state will also be
    // affected. See also RenderEditable which has a similar issue.
    final Size textSize = _textPainter.size;
    final bool textDidExceedMaxLines = _textPainter.didExceedMaxLines;

    double paragraphHeight = _getParagraphHeight();
    Size paragraphSize = Size(textSize.width, paragraphHeight);

    size = constraints.constrain(paragraphSize);

    final bool didOverflowHeight = size.height < textSize.height || textDidExceedMaxLines;
    final bool didOverflowWidth = size.width < textSize.width;
    // TODO(abarth): We're only measuring the sizes of the line boxes here. If*
    // the glyphs draw outside the line boxes, we might think that there isn't
    // visual overflow when there actually is visual overflow. This can become
    // a problem if we start having horizontal overflow and introduce a clip
    // that affects the actual (but undetected) vertical overflow.
    final bool hasVisualOverflow = didOverflowWidth || didOverflowHeight;
    if (hasVisualOverflow) {
      switch (_overflow) {
        case TextOverflow.visible:
          _needsClipping = false;
          _overflowShader = null;
          break;
        case TextOverflow.clip:
        case TextOverflow.ellipsis:
          _needsClipping = true;
          _overflowShader = null;
          break;
        case TextOverflow.fade:
          _needsClipping = true;
          final TextPainter fadeSizePainter = TextPainter(
            text: TextSpan(style: _textPainter.text!.style, text: '\u2026'),
            textDirection: textDirection,
            textScaleFactor: textScaleFactor,
            locale: locale,
          )..layout();
          if (didOverflowWidth) {
            double fadeEnd, fadeStart;
            switch (textDirection) {
              case TextDirection.rtl:
                fadeEnd = 0.0;
                fadeStart = fadeSizePainter.width;
                break;
              case TextDirection.ltr:
                fadeEnd = size.width;
                fadeStart = fadeEnd - fadeSizePainter.width;
                break;
            }
            _overflowShader = ui.Gradient.linear(
              Offset(fadeStart, 0.0),
              Offset(fadeEnd, 0.0),
              <Color>[const Color(0xFFFFFFFF), const Color(0x00FFFFFF)],
            );
          } else {
            final double fadeEnd = size.height;
            final double fadeStart = fadeEnd - fadeSizePainter.height / 2.0;
            _overflowShader = ui.Gradient.linear(
              Offset(0.0, fadeStart),
              Offset(0.0, fadeEnd),
              <Color>[const Color(0xFFFFFFFF), const Color(0x00FFFFFF)],
            );
          }
          break;
      }
    } else {
      _needsClipping = false;
      _overflowShader = null;
    }
  }

  @override
  void paint(PaintingContext context, Offset offset) {
    assert(() {
      if (debugRepaintTextRainbowEnabled) {
        final Paint paint = Paint()..color = debugCurrentRepaintColor.toColor();
        context.canvas.drawRect(offset & size, paint);
      }
      return true;
    }());

    if (_needsClipping) {
      final Rect bounds = offset & size;
      if (_overflowShader != null) {
        // This layer limits what the shader below blends with to be just the
        // text (as opposed to the text and its background).
        context.canvas.saveLayer(bounds, Paint());
      } else {
        context.canvas.save();
      }
      context.canvas.clipRect(bounds);
    }

    if (lineHeight != null) {
      // Adjust text paint offset of each line according to line-height.
      for (int i = 0; i < _lineTextPainters.length; i++) {
        // _lineTextPainters and _lineOffset may not have the same length in some edge cases
        // cause _lineTextPainters is computed from [getBoxesForSelection] api while
        // _lineOffset is computed from [computeLineMetrics] api.
        // Add protection here to prevent access the overflow index of _lineOffset list.
        if (i >= _lineOffset.length) continue;

        TextPainter _lineTextPainter = _lineTextPainters[i];
        Offset lineOffset = Offset(offset.dx, offset.dy + _lineOffset[i].offset);
        _lineTextPainter.paint(context.canvas, lineOffset);
      }
    } else {
      _textPainter.paint(context.canvas, offset);
    }

    if (_needsClipping) {
      if (_overflowShader != null) {
        context.canvas.translate(offset.dx, offset.dy);
        final Paint paint = Paint()
          ..blendMode = BlendMode.modulate
          ..shader = _overflowShader;
        context.canvas.drawRect(Offset.zero & size, paint);
      }
      context.canvas.restore();
    }
    if (_lastSelectableFragments != null) {
      for (final _SelectableFragment fragment in _lastSelectableFragments!) {
        // Offset lineOffset = Offset(offset.dx, offset.dy + _lineOffset[i]);
        fragment.paint(context, offset);
      }
    }
  }

  /// Returns the offset at which to paint the caret.
  ///
  /// Valid only after [layout].
  Offset getOffsetForCaret(TextPosition position, Rect caretPrototype) {
    assert(!debugNeedsLayout);
    _layoutTextWithConstraints(constraints);
    Offset offset =  _textPainter.getOffsetForCaret(position, caretPrototype);
    final lineOffset = _lineOffset.where((element) =>
    !(position.offset > element.end || position.offset < element.start)).first;
    return Offset(offset.dx, lineOffset.offset);
  }

  /// Returns a list of rects that bound the given selection.
  ///
  /// A given selection might have more than one rect if this text painter
  /// contains bidirectional text because logically contiguous text might not be
  /// visually contiguous.
  ///
  /// Valid only after [layout].
  List<ui.TextBox> getBoxesForSelection(TextSelection selection) {
    assert(!debugNeedsLayout);
    _layoutTextWithConstraints(constraints);

    final offsets = _lineOffset.where((element) =>
    !(selection.start > element.end || selection.end < element.start)).toList();
    List<ui.TextBox> boxes = _textPainter.getBoxesForSelection(selection);
    if (boxes.isEmpty) {
      return [];
    }
    if (offsets.isEmpty) {
      return boxes;
    }

    List<ui.TextBox> mergedBoxes = [];
    for (int i = 0; i < boxes.length; i++) {
      if (mergedBoxes.isNotEmpty && mergedBoxes.last.right == boxes[i].left) {
        ui.TextBox lastBox = mergedBoxes.removeLast();
        mergedBoxes.add(ui.TextBox.fromLTRBD(lastBox.left,
            math.min(lastBox.top, boxes[i].top),
            boxes[i].right,
            math.max(lastBox.bottom, boxes[i].bottom),
            lastBox.direction));
      } else {
        mergedBoxes.add(boxes[i]);
      }
    }

    List<ui.TextBox> result = [];
    for (int i = 0; i < mergedBoxes.length; i++) {
      final offset = offsets[i].offset;
      final box = mergedBoxes[i];
      result.add(ui.TextBox.fromLTRBD(box.left, offset, box.right, box.bottom + (offset - box.top), TextDirection.ltr));
    }
    return result;
  }

  /// Returns the position within the text for the given pixel offset.
  ///
  /// Valid only after [layout].
  TextPosition getPositionForOffset(Offset offset) {
    assert(!debugNeedsLayout);
    _layoutTextWithConstraints(constraints);
    return _textPainter.getPositionForOffset(offset);
  }

  /// Returns the text range of the word at the given offset. Characters not
  /// part of a word, such as spaces, symbols, and punctuation, have word breaks
  /// on both sides. In such cases, this method will return a text range that
  /// contains the given text position.
  ///
  /// Word boundaries are defined more precisely in Unicode Standard Annex #29
  /// <http://www.unicode.org/reports/tr29/#Word_Boundaries>.
  ///
  /// Valid only after [layout].
  TextRange getWordBoundary(TextPosition position) {
    assert(!debugNeedsLayout);
    _layoutTextWithConstraints(constraints);
    return _textPainter.getWordBoundary(position);
  }

  TextRange _getLineAtOffset(TextPosition position) => _textPainter.getLineBoundary(position);

  TextPosition _getTextPositionAbove(TextPosition position) {
    // -0.5 of preferredLineHeight points to the middle of the line above.
    final double preferredLineHeight = _textPainter.preferredLineHeight;
    final double verticalOffset = -0.5 * preferredLineHeight;
    return _getTextPositionVertical(position, verticalOffset);
  }

  TextPosition _getTextPositionBelow(TextPosition position) {
    // 1.5 of preferredLineHeight points to the middle of the line below.
    final double preferredLineHeight = _textPainter.preferredLineHeight;
    final double verticalOffset = 1.5 * preferredLineHeight;
    return _getTextPositionVertical(position, verticalOffset);
  }

  TextPosition _getTextPositionVertical(TextPosition position, double verticalOffset) {
    final Offset caretOffset = _textPainter.getOffsetForCaret(position, Rect.zero);
    final Offset caretOffsetTranslated = caretOffset.translate(0.0, verticalOffset);
    return _textPainter.getPositionForOffset(caretOffsetTranslated);
  }

  /// Returns the size of the text as laid out.
  ///
  /// This can differ from [size] if the text overflowed or if the [constraints]
  /// provided by the parent [RenderObject] forced the layout to be bigger than
  /// necessary for the given [text].
  ///
  /// This returns the [TextPainter.size] of the underlying [TextPainter].
  ///
  /// Valid only after [layout].
  Size get textSize {
    assert(!debugNeedsLayout);
    return _textPainter.size;
  }

  /// Collected during [describeSemanticsConfiguration], used by
  /// [assembleSemanticsNode] and [_combineSemanticsInfo].
  List<InlineSpanSemanticsInformation>? _semanticsInfo;

  @override
  void describeSemanticsConfiguration(SemanticsConfiguration config) {
    super.describeSemanticsConfiguration(config);
    _semanticsInfo = text.getSemanticsInformation();

    if (_semanticsInfo!.any((InlineSpanSemanticsInformation info) => info.recognizer != null)) {
      config.explicitChildNodes = true;
      config.isSemanticBoundary = true;
    } else {
      final StringBuffer buffer = StringBuffer();
      for (final InlineSpanSemanticsInformation info in _semanticsInfo!) {
        buffer.write(info.semanticsLabel ?? info.text);
      }
      config.label = buffer.toString();
      config.textDirection = textDirection;
    }
  }

  @override
  List<DiagnosticsNode> debugDescribeChildren() {
    return <DiagnosticsNode>[
      text.toDiagnosticsNode(
        name: 'text',
        style: DiagnosticsTreeStyle.transition,
      )
    ];
  }

  @override
  void debugFillProperties(DiagnosticPropertiesBuilder properties) {
    super.debugFillProperties(properties);
    properties.add(EnumProperty<TextAlign>('textAlign', textAlign));
    properties.add(EnumProperty<TextDirection>('textDirection', textDirection));
    properties.add(FlagProperty(
      'softWrap',
      value: softWrap,
      ifTrue: 'wrapping at box width',
      ifFalse: 'no wrapping except at line break characters',
      showName: true,
    ));
    properties.add(EnumProperty<TextOverflow>('overflow', overflow));
    properties.add(DoubleProperty(
      'textScaleFactor',
      textScaleFactor,
      defaultValue: 1.0,
    ));
    properties.add(DiagnosticsProperty<Locale>(
      'locale',
      locale,
      defaultValue: null,
    ));
    properties.add(IntProperty('maxLines', maxLines, ifNull: 'unlimited'));
  }
}



/// A continuous, selectable piece of paragraph.
///
/// Since the selections in [PlaceHolderSpan] are handled independently in its
/// subtree, a selection in [WebFRenderParagraph] can't continue across a
/// [PlaceHolderSpan]. The [WebFRenderParagraph] splits itself on [PlaceHolderSpan]
/// to create multiple `_SelectableFragment`s so that they can be selected
/// separately.
class _SelectableFragment with Selectable, ChangeNotifier implements TextLayoutMetrics {
  _SelectableFragment({
    required this.paragraph,
    required this.fullText,
    required this.range,
  }) : assert(range.isValid && !range.isCollapsed && range.isNormalized) {
    _selectionGeometry = _getSelectionGeometry();
  }

  final TextRange range;
  final WebFRenderParagraph paragraph;
  final String fullText;

  TextPosition? _textSelectionStart;
  TextPosition? _textSelectionEnd;

  LayerLink? _startHandleLayerLink;
  LayerLink? _endHandleLayerLink;

  double get lineHeight => paragraph.lineHeight ?? paragraph._textPainter.preferredLineHeight;

  @override
  SelectionGeometry get value => _selectionGeometry;
  late SelectionGeometry _selectionGeometry;
  void _updateSelectionGeometry() {
    final SelectionGeometry newValue = _getSelectionGeometry();
    if (_selectionGeometry == newValue) {
      return;
    }
    _selectionGeometry = newValue;
    notifyListeners();
  }

  SelectionGeometry _getSelectionGeometry() {
    if (_textSelectionStart == null || _textSelectionEnd == null) {
      return const SelectionGeometry(
        status: SelectionStatus.none,
        hasContent: true,
      );
    }

    final int selectionStart = _textSelectionStart!.offset;
    final int selectionEnd = _textSelectionEnd!.offset;
    final bool isReversed = selectionStart > selectionEnd;
    final Offset startOffsetInParagraphCoordinates = paragraph._getOffsetForPosition(TextPosition(offset: selectionStart));
    final Offset endOffsetInParagraphCoordinates = selectionStart == selectionEnd
        ? startOffsetInParagraphCoordinates
        : paragraph._getOffsetForPosition(TextPosition(offset: selectionEnd));
    final bool flipHandles = isReversed != (TextDirection.rtl == paragraph.textDirection);
    final Matrix4 paragraphToFragmentTransform = getTransformToParagraph()..invert();
    return SelectionGeometry(
      startSelectionPoint: SelectionPoint(
          localPosition: MatrixUtils.transformPoint(paragraphToFragmentTransform, startOffsetInParagraphCoordinates),
          lineHeight: lineHeight,
          handleType: flipHandles ? TextSelectionHandleType.right : TextSelectionHandleType.left
      ),
      endSelectionPoint: SelectionPoint(
        localPosition: MatrixUtils.transformPoint(paragraphToFragmentTransform, endOffsetInParagraphCoordinates),
        lineHeight: lineHeight,
        handleType: flipHandles ? TextSelectionHandleType.left : TextSelectionHandleType.right,
      ),
      status: _textSelectionStart!.offset == _textSelectionEnd!.offset
          ? SelectionStatus.collapsed
          : SelectionStatus.uncollapsed,
      hasContent: true,
    );
  }

  @override
  SelectionResult dispatchSelectionEvent(SelectionEvent event) {
    late final SelectionResult result;
    final TextPosition? existingSelectionStart = _textSelectionStart;
    final TextPosition? existingSelectionEnd = _textSelectionEnd;
    switch (event.type) {
      case SelectionEventType.startEdgeUpdate:
      case SelectionEventType.endEdgeUpdate:
        final SelectionEdgeUpdateEvent edgeUpdate = event as SelectionEdgeUpdateEvent;
        result = _updateSelectionEdge(edgeUpdate.globalPosition, isEnd: edgeUpdate.type == SelectionEventType.endEdgeUpdate);
        break;
      case SelectionEventType.clear:
        result = _handleClearSelection();
        break;
      case SelectionEventType.selectAll:
        result = _handleSelectAll();
        break;
      case SelectionEventType.selectWord:
        final SelectWordSelectionEvent selectWord = event as SelectWordSelectionEvent;
        result = _handleSelectWord(selectWord.globalPosition);
        break;
      case SelectionEventType.granularlyExtendSelection:
        final GranularlyExtendSelectionEvent granularlyExtendSelection = event as GranularlyExtendSelectionEvent;
        result = _handleGranularlyExtendSelection(
          granularlyExtendSelection.forward,
          granularlyExtendSelection.isEnd,
          granularlyExtendSelection.granularity,
        );
        break;
      case SelectionEventType.directionallyExtendSelection:
        final DirectionallyExtendSelectionEvent directionallyExtendSelection = event as DirectionallyExtendSelectionEvent;
        result = _handleDirectionallyExtendSelection(
          directionallyExtendSelection.dx,
          directionallyExtendSelection.isEnd,
          directionallyExtendSelection.direction,
        );
        break;
    }

    if (existingSelectionStart != _textSelectionStart ||
        existingSelectionEnd != _textSelectionEnd) {
      _didChangeSelection();
    }
    return result;
  }

  @override
  SelectedContent? getSelectedContent() {
    if (_textSelectionStart == null || _textSelectionEnd == null) {
      return null;
    }
    final int start = math.min(_textSelectionStart!.offset, _textSelectionEnd!.offset);
    final int end = math.max(_textSelectionStart!.offset, _textSelectionEnd!.offset);
    return SelectedContent(
      plainText: fullText.substring(start, end),
    );
  }

  void _didChangeSelection() {
    paragraph.markNeedsPaint();
    _updateSelectionGeometry();
  }

  SelectionResult _updateSelectionEdge(Offset globalPosition, {required bool isEnd}) {
    _setSelectionPosition(null, isEnd: isEnd);
    final Matrix4 transform = paragraph.getTransformTo(null);
    transform.invert();
    final Offset localPosition = MatrixUtils.transformPoint(transform, globalPosition);
    if (_rect.isEmpty) {
      return SelectionUtils.getResultBasedOnRect(_rect, localPosition);
    }
    final Offset adjustedOffset = SelectionUtils.adjustDragOffset(
      _rect,
      localPosition,
      direction: paragraph.textDirection,
    );

    final TextPosition position = _clampTextPosition(paragraph.getPositionForOffset(adjustedOffset));
    _setSelectionPosition(position, isEnd: isEnd);
    if (position.offset == range.end) {
      return SelectionResult.next;
    }
    if (position.offset == range.start) {
      return SelectionResult.previous;
    }
    // TODO(chunhtai): The geometry information should not be used to determine
    // selection result. This is a workaround to WebFRenderParagraph, where it does
    // not have a way to get accurate text length if its text is truncated due to
    // layout constraint.
    return SelectionUtils.getResultBasedOnRect(_rect, localPosition);
  }

  TextPosition _clampTextPosition(TextPosition position) {
    // Affinity of range.end is upstream.
    if (position.offset > range.end ||
        (position.offset == range.end && position.affinity == TextAffinity.downstream)) {
      return TextPosition(offset: range.end, affinity: TextAffinity.upstream);
    }
    if (position.offset < range.start) {
      return TextPosition(offset: range.start);
    }
    return position;
  }

  void _setSelectionPosition(TextPosition? position, {required bool isEnd}) {
    if (isEnd) {
      _textSelectionEnd = position;
    } else {
      _textSelectionStart = position;
    }
  }

  SelectionResult _handleClearSelection() {
    _textSelectionStart = null;
    _textSelectionEnd = null;
    return SelectionResult.none;
  }

  SelectionResult _handleSelectAll() {
    _textSelectionStart = TextPosition(offset: range.start);
    _textSelectionEnd = TextPosition(offset: range.end, affinity: TextAffinity.upstream);
    return SelectionResult.none;
  }

  SelectionResult _handleSelectWord(Offset globalPosition) {
    final TextPosition position = paragraph.getPositionForOffset(paragraph.globalToLocal(globalPosition));
    if (_positionIsWithinCurrentSelection(position)) {
      return SelectionResult.end;
    }
    final TextRange word = paragraph.getWordBoundary(position);
    assert(word.isNormalized);
    // Fragments are separated by placeholder span, the word boundary shouldn't
    // expand across fragments.
    assert(word.start >= range.start && word.end <= range.end);
    late TextPosition start;
    late TextPosition end;
    if (position.offset >= word.end) {
      start = end = TextPosition(offset: position.offset);
    } else {
      start = TextPosition(offset: word.start);
      end = TextPosition(offset: word.end, affinity: TextAffinity.upstream);
    }
    _textSelectionStart = start;
    _textSelectionEnd = end;
    return SelectionResult.end;
  }

  SelectionResult _handleDirectionallyExtendSelection(double horizontalBaseline, bool isExtent, SelectionExtendDirection movement) {
    final Matrix4 transform = paragraph.getTransformTo(null);
    if (transform.invert() == 0.0) {
      switch(movement) {
        case SelectionExtendDirection.previousLine:
        case SelectionExtendDirection.backward:
          return SelectionResult.previous;
        case SelectionExtendDirection.nextLine:
        case SelectionExtendDirection.forward:
          return SelectionResult.next;
      }
    }
    final double baselineInParagraphCoordinates = MatrixUtils.transformPoint(transform, Offset(horizontalBaseline, 0)).dx;
    assert(!baselineInParagraphCoordinates.isNaN);
    final TextPosition newPosition;
    final SelectionResult result;
    switch(movement) {
      case SelectionExtendDirection.previousLine:
      case SelectionExtendDirection.nextLine:
        assert(_textSelectionEnd != null && _textSelectionStart != null);
        final TextPosition targetedEdge = isExtent ? _textSelectionEnd! : _textSelectionStart!;
        final MapEntry<TextPosition, SelectionResult> moveResult = _handleVerticalMovement(
          targetedEdge,
          horizontalBaselineInParagraphCoordinates: baselineInParagraphCoordinates,
          below: movement == SelectionExtendDirection.nextLine,
        );
        newPosition = moveResult.key;
        result = moveResult.value;
        break;
      case SelectionExtendDirection.forward:
      case SelectionExtendDirection.backward:
        _textSelectionEnd ??= movement == SelectionExtendDirection.forward
            ? TextPosition(offset: range.start)
            : TextPosition(offset: range.end, affinity: TextAffinity.upstream);
        _textSelectionStart ??= _textSelectionEnd;
        final TextPosition targetedEdge = isExtent ? _textSelectionEnd! : _textSelectionStart!;
        final Offset edgeOffsetInParagraphCoordinates = paragraph._getOffsetForPosition(targetedEdge);
        final Offset baselineOffsetInParagraphCoordinates = Offset(
          baselineInParagraphCoordinates,
          // Use half of line height to point to the middle of the line.
          edgeOffsetInParagraphCoordinates.dy - lineHeight / 2,
        );
        newPosition = paragraph.getPositionForOffset(baselineOffsetInParagraphCoordinates);
        result = SelectionResult.end;
        break;
    }
    if (isExtent) {
      _textSelectionEnd = newPosition;
    } else {
      _textSelectionStart = newPosition;
    }
    return result;
  }

  SelectionResult _handleGranularlyExtendSelection(bool forward, bool isExtent, TextGranularity granularity) {
    _textSelectionEnd ??= forward
        ? TextPosition(offset: range.start)
        : TextPosition(offset: range.end, affinity: TextAffinity.upstream);
    _textSelectionStart ??= _textSelectionEnd;
    final TextPosition targetedEdge = isExtent ? _textSelectionEnd! : _textSelectionStart!;
    if (forward && (targetedEdge.offset == range.end)) {
      return SelectionResult.next;
    }
    if (!forward && (targetedEdge.offset == range.start)) {
      return SelectionResult.previous;
    }
    final SelectionResult result;
    final TextPosition newPosition;
    switch (granularity) {
      case TextGranularity.character:
        final String text = range.textInside(fullText);
        newPosition = _getNextPosition(CharacterBoundary(text), targetedEdge, forward);
        result = SelectionResult.end;
        break;
      case TextGranularity.word:
        final String text = range.textInside(fullText);
        newPosition = _getNextPosition(WhitespaceBoundary(text) + WordBoundary(this), targetedEdge, forward);
        result = SelectionResult.end;
        break;
      case TextGranularity.line:
        newPosition = _getNextPosition(LineBreak(this), targetedEdge, forward);
        result = SelectionResult.end;
        break;
      case TextGranularity.document:
        final String text = range.textInside(fullText);
        newPosition = _getNextPosition(DocumentBoundary(text), targetedEdge, forward);
        if (forward && newPosition.offset == range.end) {
          result = SelectionResult.next;
        } else if (!forward && newPosition.offset == range.start) {
          result = SelectionResult.previous;
        } else {
          result = SelectionResult.end;
        }
        break;
    }

    if (isExtent) {
      _textSelectionEnd = newPosition;
    } else {
      _textSelectionStart = newPosition;
    }
    return result;
  }

  TextPosition _getNextPosition(TextBoundary boundary, TextPosition position, bool forward) {
    if (forward) {
      return _clampTextPosition(
          (PushTextPosition.forward + boundary).getTrailingTextBoundaryAt(position)
      );
    }
    return _clampTextPosition(
      (PushTextPosition.backward + boundary).getLeadingTextBoundaryAt(position),
    );
  }

  MapEntry<TextPosition, SelectionResult> _handleVerticalMovement(TextPosition position, {required double horizontalBaselineInParagraphCoordinates, required bool below}) {
    final List<ui.LineMetrics> lines = paragraph._lineMetrics;
    final Offset offset = paragraph.getOffsetForCaret(position, Rect.zero);
    int currentLine = lines.length - 1;
    for (final ui.LineMetrics lineMetrics in lines) {
      if (lineMetrics.baseline > offset.dy) {
        currentLine = lineMetrics.lineNumber;
        break;
      }
    }
    final TextPosition newPosition;
    if (below && currentLine == lines.length - 1) {
      newPosition = TextPosition(offset: range.end, affinity: TextAffinity.upstream);
    } else if (!below && currentLine == 0) {
      newPosition = TextPosition(offset: range.start);
    } else {
      final int newLine = below ? currentLine + 1 : currentLine - 1;
      newPosition = _clampTextPosition(
          paragraph.getPositionForOffset(Offset(horizontalBaselineInParagraphCoordinates, lines[newLine].baseline))
      );
    }
    final SelectionResult result;
    if (newPosition.offset == range.start) {
      result = SelectionResult.previous;
    } else if (newPosition.offset == range.end) {
      result = SelectionResult.next;
    } else {
      result = SelectionResult.end;
    }
    assert(result != SelectionResult.next || below);
    assert(result != SelectionResult.previous || !below);
    return MapEntry<TextPosition, SelectionResult>(newPosition, result);
  }

  /// Whether the given text position is contained in current selection
  /// range.
  ///
  /// The parameter `start` must be smaller than `end`.
  bool _positionIsWithinCurrentSelection(TextPosition position) {
    if (_textSelectionStart == null || _textSelectionEnd == null) {
      return false;
    }
    // Normalize current selection.
    late TextPosition currentStart;
    late TextPosition currentEnd;
    if (_compareTextPositions(_textSelectionStart!, _textSelectionEnd!) > 0) {
      currentStart = _textSelectionStart!;
      currentEnd = _textSelectionEnd!;
    } else {
      currentStart = _textSelectionEnd!;
      currentEnd = _textSelectionStart!;
    }
    return _compareTextPositions(currentStart, position) >= 0 && _compareTextPositions(currentEnd, position) <= 0;
  }

  /// Compares two text positions.
  ///
  /// Returns 1 if `position` < `otherPosition`, -1 if `position` > `otherPosition`,
  /// or 0 if they are equal.
  static int _compareTextPositions(TextPosition position, TextPosition otherPosition) {
    if (position.offset < otherPosition.offset) {
      return 1;
    } else if (position.offset > otherPosition.offset) {
      return -1;
    } else if (position.affinity == otherPosition.affinity){
      return 0;
    } else {
      return position.affinity == TextAffinity.upstream ? 1 : -1;
    }
  }

  Matrix4 getTransformToParagraph() {
    return Matrix4.translationValues(_rect.left, _rect.top, 0.0);
  }

  @override
  Matrix4 getTransformTo(RenderObject? ancestor) {
    return getTransformToParagraph()..multiply(paragraph.getTransformTo(ancestor));
  }

  @override
  void pushHandleLayers(LayerLink? startHandle, LayerLink? endHandle) {
    if (!paragraph.attached) {
      assert(startHandle == null && endHandle == null, 'Only clean up can be called.');
      return;
    }
    if (_startHandleLayerLink != startHandle) {
      _startHandleLayerLink = startHandle;
      paragraph.markNeedsPaint();
    }
    if (_endHandleLayerLink != endHandle) {
      _endHandleLayerLink = endHandle;
      paragraph.markNeedsPaint();
    }
  }

  Rect get _rect {
    if (_cachedRect == null) {
      final List<TextBox> boxes = paragraph.getBoxesForSelection(
        TextSelection(baseOffset: range.start, extentOffset: range.end),
      );
      if (boxes.isNotEmpty) {
        Rect result = boxes.first.toRect();
        for (int index = 1; index < boxes.length; index += 1) {
          result = result.expandToInclude(boxes[index].toRect());
        }
        _cachedRect = result;
      } else {
        final Offset offset = paragraph._getOffsetForPosition(TextPosition(offset: range.start));
        _cachedRect = Rect.fromPoints(offset, offset.translate(0, - lineHeight));
      }
    }
    return _cachedRect!;
  }
  Rect? _cachedRect;

  void didChangeParagraphLayout() {
    _cachedRect = null;
  }

  @override
  Size get size {
    return _rect.size;
  }

  void paint(PaintingContext context, Offset offset) {
    if (_textSelectionStart == null || _textSelectionEnd == null) {
      return;
    }
    if (paragraph.selectionColor != null) {
      final TextSelection selection = TextSelection(
        baseOffset: _textSelectionStart!.offset,
        extentOffset: _textSelectionEnd!.offset,
      );
      final Paint selectionPaint = Paint()
        ..style = PaintingStyle.fill
        ..color = paragraph.selectionColor!;
      for (final TextBox textBox in paragraph.getBoxesForSelection(selection)) {
        context.canvas.drawRect(
            textBox.toRect().shift(offset), selectionPaint);
      }
    }
    final Matrix4 transform = getTransformToParagraph();
    if (_startHandleLayerLink != null && value.startSelectionPoint != null) {
      context.pushLayer(
        LeaderLayer(
          link: _startHandleLayerLink!,
          offset: offset + MatrixUtils.transformPoint(transform, value.startSelectionPoint!.localPosition),
        ),
            (PaintingContext context, Offset offset) { },
        Offset.zero,
      );
    }
    if (_endHandleLayerLink != null && value.endSelectionPoint != null) {
      context.pushLayer(
        LeaderLayer(
          link: _endHandleLayerLink!,
          offset: offset + MatrixUtils.transformPoint(transform, value.endSelectionPoint!.localPosition),
        ),
            (PaintingContext context, Offset offset) { },
        Offset.zero,
      );
    }
  }

  @override
  TextSelection getLineAtOffset(TextPosition position) {
    final TextRange line = paragraph._getLineAtOffset(position);
    final int start = line.start.clamp(range.start, range.end); // ignore_clamp_double_lint
    final int end = line.end.clamp(range.start, range.end); // ignore_clamp_double_lint
    return TextSelection(baseOffset: start, extentOffset: end);
  }

  @override
  TextPosition getTextPositionAbove(TextPosition position) {
    return _clampTextPosition(paragraph._getTextPositionAbove(position));
  }

  @override
  TextPosition getTextPositionBelow(TextPosition position) {
    return _clampTextPosition(paragraph._getTextPositionBelow(position));
  }

  @override
  TextRange getWordBoundary(TextPosition position) => paragraph.getWordBoundary(position);
}
