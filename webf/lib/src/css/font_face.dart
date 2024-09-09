/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'package:collection/collection.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter/services.dart';
import 'package:webf/css.dart';
import 'package:webf/foundation.dart';
import 'package:webf/launcher.dart';
import 'dart:convert';

final List<String> supportedFonts = [
  'ttc',
  'ttf',
  'otf',
  'data'
];

class _Font {
  String src = '';
  String format = '';
  Uint8List content = Uint8List(0);
  _Font(this.src, this.format);
  _Font.content(Uint8List value) {
    src = '';
    format = 'data';
    content = value;
  }
}

class CSSFontFace {
  static Uri? _resolveFontSource(double contextId, String source, String? base) {
    WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
    base = base ?? controller.url;
    try {
      return controller.uriParser!.resolve(Uri.parse(base), Uri.parse(source));
    } catch (_) {
      return null;
    }
  }
  static void resolveFontFaceRules(CSSFontFaceRule fontFaceRule, double contextId, String? baseHref) async {
    CSSStyleDeclaration declaration = fontFaceRule.declarations;
    String fontFamily = declaration.getPropertyValue('fontFamily');
    String url = declaration.getPropertyValue('src');
    if (fontFamily.isNotEmpty && url.isNotEmpty && CSSFunction.isFunction(url)) {
      String? tmp_src;
      List<CSSFunctionalNotation> functions = CSSFunction.parseFunction(url);

      List<_Font> fonts = [];

      for(int i = 0; i < functions.length; i ++) {
        CSSFunctionalNotation notation = functions[i];
        if (notation.name == 'url') {
          tmp_src = notation.args[0];

          tmp_src = removeQuotationMark(tmp_src);

          if (tmp_src.startsWith('data')) {
            String tmp_content = tmp_src.split(';').last;
            if (tmp_content.startsWith('base64')) {
              String base64 = tmp_src.split(',').last;
              try {
                Uint8List decoded = base64Decode(base64);
                if (decoded.isNotEmpty) {
                  fonts.add(_Font.content(decoded));
                }
              } catch(e) {}
            }

          } else {
            String formatFromExt = tmp_src.split('.').last;
            fonts.add(_Font(tmp_src, formatFromExt));
          }

        }
      }

      _Font? targetFont = fonts.firstWhereOrNull((f) {
        return supportedFonts.contains(f.format);
      });

      if (targetFont == null) return;

      try {
        if (targetFont.content.isNotEmpty) {
          Uint8List content = targetFont.content;
          Future<ByteData> bytes = Future.value(ByteData.sublistView(content));
          FontLoader loader = FontLoader(fontFamily);
          SchedulerBinding.instance.addPostFrameCallback((timeStamp) {
            loader.addFont(bytes);
            loader.load();
          });
        } else {
          Uri? uri = _resolveFontSource(contextId, targetFont.src, baseHref);
          if (uri == null) return;
          final WebFController controller = WebFController.getControllerOfJSContextId(contextId)!;
          WebFBundle bundle = controller.getPreloadBundleFromUrl(uri.toString()) ?? WebFBundle.fromUrl(uri.toString());
          await bundle.resolve(contextId);
          assert(bundle.isResolved, 'Failed to obtain $url');
          FontLoader loader = FontLoader(removeQuotationMark(fontFamily));
          Future<ByteData> bytes = Future.value(bundle.data?.buffer.asByteData());
          SchedulerBinding.instance.addPostFrameCallback((timeStamp) {
            loader.addFont(bytes);
            loader.load();
          });
        }


      } catch(e, stack) {
        print('$e\n$stack');
        return;
      }
    }
  }
}
