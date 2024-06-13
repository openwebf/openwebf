/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'package:webf/css.dart';
import 'package:quiver/core.dart';

abstract class StyleSheet {}

const String _CSSStyleSheetType = 'text/css';

// https://drafts.csswg.org/cssom-1/#cssstylesheet
class CSSStyleSheet implements StyleSheet, Comparable {
  String type = _CSSStyleSheetType;

  /// A Boolean indicating whether the stylesheet is disabled. False by default.
  bool disabled = false;

  /// A string containing the baseURL used to resolve relative URLs in the stylesheet.
  String? href;

  final List<CSSRule> cssRules;

  CSSStyleSheet(this.cssRules, {this.disabled = false, this.href});

  insertRule(String text, int index, {required double windowWidth, required double windowHeight, required bool isDarkMode}) {
    List<CSSRule> rules = CSSParser(text).parseRules(windowWidth: windowWidth, windowHeight: windowHeight, isDarkMode: isDarkMode);
    cssRules.addAll(rules);
  }

  /// Removes a rule from the stylesheet object.
  deleteRule(int index) {
    cssRules.removeAt(index);
  }

  /// Synchronously replaces the content of the stylesheet with the content passed into it.
  replaceSync(String text, {required double windowWidth, required double windowHeight, required bool isDarkMode}) {
    cssRules.clear();
    List<CSSRule> rules = CSSParser(text).parseRules(windowWidth: windowWidth, windowHeight: windowHeight, isDarkMode: isDarkMode);
    cssRules.addAll(rules);
  }

  Future replace(String text, {required double windowWidth, required double windowHeight, required bool isDarkMode}) async {
    return Future(() {
      replaceSync(text, windowWidth: windowWidth, windowHeight: windowHeight, isDarkMode: isDarkMode);
    });
  }

  @override
  bool operator ==(Object other) {
    return hashCode == other.hashCode;
  }

  @override
  int get hashCode => hashObjects(cssRules);

  CSSStyleSheet clone() {
    CSSStyleSheet sheet = CSSStyleSheet(List.from(cssRules), disabled: disabled, href: href);
    return sheet;
  }

  @override
  int compareTo(other) {
    if (other is! CSSStyleSheet) {
      return 0;
    }
    return hashCode.compareTo(other.hashCode);
  }
}
