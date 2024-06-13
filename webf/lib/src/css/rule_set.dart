/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'dart:collection';

import 'package:webf/css.dart';
import 'package:webf/dom.dart';

typedef CSSMap = HashMap<String, List<CSSRule>>;

class RuleSet {
  final Document ownerDocument;
  RuleSet(this.ownerDocument);

  bool get isEmpty =>
      idRules.isEmpty &&
      classRules.isEmpty &&
      attributeRules.isEmpty &&
      tagRules.isEmpty &&
      universalRules.isEmpty &&
      pseudoRules.isEmpty &&
      keyframesRules.isEmpty;

  final CSSMap idRules = HashMap();
  final CSSMap classRules = HashMap();
  final CSSMap attributeRules = HashMap();
  final CSSMap tagRules = HashMap();
  final List<CSSRule> universalRules = [];
  final List<CSSRule> pseudoRules = [];

  final Map<String, CSSKeyframesRule> keyframesRules = {};

  int _lastPosition = 0;

  void addRules(List<CSSRule> rules, { required String? baseHref }) {
    for (CSSRule rule in rules) {
      addRule(rule, baseHref: baseHref);
    }
  }

  void addRule(CSSRule rule, { required String? baseHref }) {
    rule.position = _lastPosition++;
    if (rule is CSSStyleRule) {
      for (final selector in rule.selectorGroup.selectors) {
        findBestRuleSetAndAdd(selector, rule);
      }
    } else if (rule is CSSKeyframesRule) {
      keyframesRules[rule.name] = rule;
    } else if (rule is CSSFontFaceRule) {
      CSSFontFace.resolveFontFaceRules(rule, ownerDocument.contextId!, baseHref);
    } else if (rule is CSSMediaDirective) {
      // doNothing
    } else {
      assert(false, 'Unsupported rule type: ${rule.runtimeType}');
    }
  }

  void reset() {
    idRules.clear();
    classRules.clear();
    attributeRules.clear();
    tagRules.clear();
    universalRules.clear();
    pseudoRules.clear();
  }

  // indexed by selectorText
  void findBestRuleSetAndAdd(Selector selector, CSSRule rule) {
    String? id, className, attributeName, tagName, pseudoName;

    for (final simpleSelectorSequence in selector.simpleSelectorSequences.reversed) {
      final simpleSelector = simpleSelectorSequence.simpleSelector;
      if (simpleSelector.runtimeType == IdSelector) {
        id = simpleSelector.name;
      } else if (simpleSelector.runtimeType == ClassSelector) {
        className = simpleSelector.name;
      } else if (simpleSelector.runtimeType == AttributeSelector) {
        attributeName = simpleSelector.name;
      } else if (simpleSelector.runtimeType == ElementSelector) {
        if (simpleSelector.isWildcard) {
          break;
        }
        tagName = simpleSelector.name;
      } else if (simpleSelector.runtimeType == PseudoClassSelector ||
          simpleSelector.runtimeType == PseudoElementSelector ||
          simpleSelector.runtimeType == PseudoClassFunctionSelector) {
        pseudoName = simpleSelector.name;
      }

      if (id != null || className != null || attributeName != null || tagName != null || pseudoName != null) {
        break;
      }
    }

    void insertRule(String key, CSSRule rule, CSSMap map) {
      List<CSSRule>? rules = map[key] ?? [];
      rules.add(rule);
      map[key] = rules;
    }

    if (id != null && id.isNotEmpty == true) {
      insertRule(id, rule, idRules);
      return;
    }

    if (className != null && className.isNotEmpty == true) {
      insertRule(className, rule, classRules);
      return;
    }

    if (attributeName != null && attributeName.isNotEmpty == true) {
      insertRule(attributeName.toUpperCase(), rule, attributeRules);
      return;
    }

    if (tagName != null && tagName.isNotEmpty == true) {
      insertRule(tagName.toUpperCase(), rule, tagRules);
      return;
    }

    if (pseudoName != null && _isLegacyPsuedoClass(pseudoName)) {
      pseudoRules.add(rule);
      return;
    }

    universalRules.add(rule);
  }

  static bool _isLegacyPsuedoClass(String name) {
    // TODO: :first-letter/line match elements.
    switch (name) {
      case 'before':
      case 'after':
        return true;
      case 'first-line':
      case 'first-letter':
      default:
        return false;
    }
  }
}
