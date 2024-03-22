/*
Copyright 2013, the Dart project authors.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google LLC nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

import 'package:flutter/foundation.dart';
import 'package:webf/dom.dart';
import 'package:webf/css.dart';
import 'package:webf/html.dart';

typedef IndexCounter = int Function(Element element);

Element? querySelector(Node node, String selector) =>
    SelectorEvaluator().querySelector(node, _parseSelectorGroup(selector));

List<Element> querySelectorAll(Node node, String selector) {
  final group = _parseSelectorGroup(selector);
  final results = <Element>[];
  SelectorEvaluator().querySelectorAll(node, group, results);
  return results;
}

bool matches(Element element, String selector) =>
    SelectorEvaluator().matchSelector(_parseSelectorGroup(selector), element);

Element? closest(Node node, String selector) =>
    SelectorEvaluator().closest(node, _parseSelectorGroup(selector));


// http://dev.w3.org/csswg/selectors-4/#grouping
SelectorGroup? _parseSelectorGroup(String selector) {
  CSSParser parser = CSSParser(selector)..tokenizer.inSelector = true;
  return parser.processSelectorGroup();
}

class SelectorEvaluator extends SelectorVisitor {
  Element? _element;
  SelectorGroup? _selectorGroup;

  bool matchSelector(SelectorGroup? selectorGroup, Element? element) {
    if (selectorGroup == null || element == null) {
      return false;
    }
    _element = element;
    _selectorGroup = selectorGroup;
    _selectorGroup?.matchSpecificity = -1;
    return visitSelectorGroup(selectorGroup);
  }

  Element? querySelector(Node root, SelectorGroup? selector) {
    for (var element in root.childNodes.whereType<Element>()) {
      if (matchSelector(selector, element)) return element;
      final result = querySelector(element, selector);
      if (result != null) return result;
    }
    return null;
  }

  void querySelectorAll(Node root, SelectorGroup? selector, List<Element> results) {
    for (var element in root.childNodes.whereType<Element>()) {
      if (matchSelector(selector, element)) results.add(element);
      querySelectorAll(element, selector, results);
    }
  }

  Element? closest(Node node, SelectorGroup? selector) {
    Node? targetNode = node;
    while (targetNode != null) {
      if (targetNode is Element) {
        if (matchSelector(selector, targetNode)) return targetNode;
      }
      targetNode = targetNode.parentNode;
    }
    return null;
  }

  @override
  bool visitSelectorGroup(SelectorGroup node) => node.selectors.any(visitSelector);

  @override
  bool visitSelector(Selector node) {
    final old = _element;
    var result = true;

    // Note: evaluate selectors right-to-left as it's more efficient.
    int? combinator;
    for (var s in node.simpleSelectorSequences.reversed) {
      if (combinator == null) {
        result = s.simpleSelector.visit(this) as bool;
      } else if (combinator == TokenKind.COMBINATOR_DESCENDANT) {
        // descendant combinator
        // http://dev.w3.org/csswg/selectors-4/#descendant-combinators
        do {
          _element = _element!.parentElement;
        } while (_element != null && !(s.simpleSelector.visit(this) as bool));

        if (_element == null) result = false;
      } else if (combinator == TokenKind.COMBINATOR_TILDE) {
        // Following-sibling combinator
        // http://dev.w3.org/csswg/selectors-4/#general-sibling-combinators
        do {
          _element = _element!.previousElementSibling;
        } while (_element != null && !(s.simpleSelector.visit(this) as bool));

        if (_element == null) result = false;
      }

      if (!result) break;

      switch (s.combinator) {
        case TokenKind.COMBINATOR_PLUS:
          // Next-sibling combinator
          // http://dev.w3.org/csswg/selectors-4/#adjacent-sibling-combinators
          _element = _element!.previousElementSibling;
          break;
        case TokenKind.COMBINATOR_GREATER:
          // Child combinator
          // http://dev.w3.org/csswg/selectors-4/#child-combinators
          _element = _element!.parentElement;
          break;
        case TokenKind.COMBINATOR_DESCENDANT:
        case TokenKind.COMBINATOR_TILDE:
          // We need to iterate through all siblings or parents.
          // For now, just remember what the combinator was.
          combinator = s.combinator;
          break;
        case TokenKind.COMBINATOR_NONE:
          combinator = null;
          break;
        default:
          if (kDebugMode) throw _unsupported(node);
      }

      if (_element == null) {
        result = false;
        break;
      }
    }

    _element = old;
    if (result)
      _selectorGroup?.matchSpecificity = node.specificity;
    return result;
  }

  @override
  bool visitPseudoClassSelector(PseudoClassSelector node) {
    switch (node.name.toLowerCase()) {
      // http://dev.w3.org/csswg/selectors-4/#structural-pseudos

      // http://dev.w3.org/csswg/selectors-4/#the-root-pseudo
      case 'root':
        return _element!.nodeName == HTML && (_element!.parentElement == null || _element!.parentElement is Document);

      // http://dev.w3.org/csswg/selectors-4/#the-empty-pseudo
      case 'empty':
        return _element!.childNodes.every((n) => !(n is Element || n is TextNode && n.data.isNotEmpty));

      // http://dev.w3.org/csswg/selectors-4/#the-blank-pseudo
      case 'blank':
        return _element!.childNodes
            .any((n) => !(n is Element || n is TextNode && n.data.runes.any((r) => !isWhitespaceCC(r))));

      // http://dev.w3.org/csswg/selectors-4/#the-first-child-pseudo
      case 'first-child':
        if (_element!.parentElement != null) {
          _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByFirstChildRules);
        }
        if (_element!.previousElementSibling != null) {
          return _element!.previousElementSibling is HeadElement;
        }
        return true;

      // http://dev.w3.org/csswg/selectors-4/#the-last-child-pseudo
      case 'last-child':
        if (_element!.nextSibling != null && _element!.parentElement != null) {
          _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByLastChildRules);
        }
        return _element!.nextSibling == null;

      //http://drafts.csswg.org/selectors-4/#first-of-type-pseudo
      //http://drafts.csswg.org/selectors-4/#last-of-type-pseudo
      //http://drafts.csswg.org/selectors-4/#only-of-type-pseudo
      case 'first-of-type':
      case 'last-of-type':
      case 'only-of-type':
        var parent = _element!.parentElement;
        if (parent != null) {
          var children = parent.children.where((Element el) {
            return el.nodeName == _element!.nodeName;
          }).toList();

          var index = children.indexOf(_element!);
          var isFirst = index == 0;
          var isLast = index == children.length - 1;

          if (isFirst && node.name == 'first-of-type') {
            if (_element!.parentElement != null) {
              _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByForwardPositionalRules);
            }
            return true;
          }

          if (isLast && node.name == 'last-of-type') {
            if (_element!.parentElement != null) {
              _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByBackwardPositionalRules);
            }
            return true;
          }

          if (isFirst && isLast && node.name == 'only-of-type') {
            if (_element!.parentElement != null) {
              _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByFirstChildRules);
            }
            return true;
          }

          return false;
        }

        break;
      // http://dev.w3.org/csswg/selectors-4/#the-only-child-pseudo
      case 'only-child':
        if (_element!.parentElement != null) {
          _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByFirstChildRules);
          _element!.parentElement!.addFlag(DynamicRestyleFlag.ChildrenAffectedByLastChildRules);
        }
        if (_element!.previousSibling == null && _element!.nextSibling == null) {
          return true;
        }
        return false;
      // http://dev.w3.org/csswg/selectors-4/#link
      case 'link':
        return _element!.attributes['href'] != null;

      case 'visited':
        // Always return false since we aren't a browser. This is allowed per:
        // http://dev.w3.org/csswg/selectors-4/#visited-pseudo
        return false;
    }

    if (_isLegacyPsuedoClass(node.name)) return true;

    if (kDebugMode) throw _unimplemented(node);
    return false;
  }

  @override
  bool visitPseudoElementSelector(PseudoElementSelector node) {
    if (_isLegacyPsuedoClass(node.name)) return true;

    if (kDebugMode) throw _unimplemented(node);
    return false;
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

  @override
  bool visitPseudoElementFunctionSelector(PseudoElementFunctionSelector node) {
    if (kDebugMode) throw _unimplemented(node);
    return false;
  }

  @override
  bool visitPseudoClassFunctionSelector(PseudoClassFunctionSelector selector) {
    List<num?>? data = _parseNthExpressions(selector.expression);
    //  i = An + B
    if (data == null) {
      return false;
    }

    switch (selector.name) {
      // http://dev.w3.org/csswg/selectors-4/#the-nth-child-pseudo
      case 'nth-child':
        return _elementSatisfiesNthChildren(_element!, selector, data[0], data[1]!);
      case 'nth-of-type':
        return _elementSatisfiesNthChildrenOfType(_element!, _element!.tagName, selector, data[0], data[1]!);
      case 'nth-last-child':
        return _elementSatisfiesNthLastChildren(_element!, selector, data[0], data[1]!);
      case 'nth-last-of-type':
        return _elementSatisfiesNthLastChildrenOfType(_element!, _element!.tagName, selector, data[0], data[1]!);
    }
    if (kDebugMode) throw _unimplemented(selector);
    return false;
  }

  bool _elementSatisfies(Element element, PseudoClassFunctionSelector selector, num? a, num b, IndexCounter finder) {
    int index = 0;

    int? cacheIndex = element.ownerDocument.nthIndexCache.getChildrenIndexFromCache(_element!.parentNode!, _element!, selector.name);
    if (cacheIndex != null) {
      index = cacheIndex;
    } else {
      index = finder(element);
      element.ownerDocument.nthIndexCache.setChildrenIndexWithParentNode(_element!.parentNode!, _element!, selector.name, index);
    }

    return _indexSatisfiesEquation(index + 1, a, b);
  }

  bool _elementSatisfiesNthChildren(Element element, PseudoClassFunctionSelector selector, num? a, num b) {
    return _elementSatisfies(element, selector, a, b, (element) {
      int index = 0;
      Element? currentElement = element;
      // Traverse the list to find the index of the element
      while (currentElement?.previousElementSibling != null) {
        currentElement = currentElement!.previousElementSibling;
        index++;
      }
      return index;
    });
  }

  bool _elementSatisfiesNthChildrenOfType(Element element, String tagName, PseudoClassFunctionSelector selector, num? a, num b) {
    return _elementSatisfies(element, selector, a, b, (element) {
      int index = 0;
      Element? currentElement = element;
      // Traverse the list to find the index of the element
      while (currentElement?.previousElementSibling != null) {
        currentElement = currentElement!.previousElementSibling;
        if (currentElement?.tagName == tagName) {
          index++;
        }
      }
      return index;
    });
  }

  bool _elementSatisfiesNthLastChildren(Element element, PseudoClassFunctionSelector selector, num? a, num b) {
    return _elementSatisfies(element, selector, a, b, (element) {
      Element? currentElement = element;
      int index = 0;
      // Traverse the list to find the index of the element
      while (currentElement?.nextElementSibling != null) {
        currentElement = currentElement!.nextElementSibling;
        index++;
      }
      return index;
    });
  }

  bool _elementSatisfiesNthLastChildrenOfType(Element element, String tagName, PseudoClassFunctionSelector selector, num? a, num b) {
    return _elementSatisfies(element, selector, a, b, (element) {
      Element? currentElement = element;
      int index = 0;
      // Traverse the list to find the index of the element
      while (currentElement?.nextElementSibling != null) {
        currentElement = currentElement!.nextElementSibling;
        if (currentElement?.tagName == tagName) {
          index++;
        }
      }
      return index;
    });
  }

  bool _indexSatisfiesEquation(int index, num? a, num b) {
    if (a == null) {
      return b > 0 && index == b;
    }

    var divideResult = (index - b) / a;
    if (divideResult >= 1) {
      return divideResult % divideResult.ceil() == 0;
    } else {
      return divideResult == 0;
    }
  }

  num _countExpressionList(List<String> list) {
    String first = list[0];
    num sum = 0;
    num modulus = 1;
    if (first == '-') {
      modulus = -1;
      list = list.sublist(1);
    }
    list.forEach((String item) {
      num? value = num.tryParse(item);
      if (value != null) {
        sum += value;
      }
    });
    return sum * modulus;
  }

  List<num?>? _parseNthExpressions(List<String> exprs) {
    num? A;
    num B = 0;

    if (exprs.isNotEmpty) {
      if (exprs.length == 1) {
        String value = exprs[0];
        if (isNumeric(value)) {
          B = num.parse(value);
        } else {
          if (value == 'even') {
            A = 2;
            B = 1;
          } else if (value == 'odd') {
            A = 2;
            B = 0;
          } else if (value == 'n') {
            A = 1;
            B = 0;
          } else {
            return null;
          }
        }
      }

      List<String> bTerms = [];
      List<String> aTerms = [];
      var nIndex = exprs.indexWhere((expr) {
        return expr.toString() == 'n';
      });

      if (nIndex > -1) {
        bTerms.addAll(exprs.sublist(nIndex + 1));
        aTerms.addAll(exprs.sublist(0, nIndex));
      } else {
        bTerms.addAll(exprs);
      }

      if (bTerms.isNotEmpty) {
        B = _countExpressionList(bTerms);
      }

      if (aTerms.isNotEmpty) {
        if (aTerms.length == 1 && aTerms[0] == '-') {
          A = -1;
        } else {
          A = _countExpressionList(aTerms);
        }
      } else {
        if (nIndex == 0) {
          A = 1;
        }
      }
    }

    return [A, B];
  }

  bool isNumeric(String s) {
    return num.tryParse(s) != null;
  }

  @override
  bool visitElementSelector(ElementSelector node) => node.isWildcard || _element!.tagName == node.name.toUpperCase();

  @override
  bool visitIdSelector(IdSelector node) => _element!.id == node.name;

  @override
  bool visitClassSelector(ClassSelector node) => _element!.classList.contains(node.name);

  // TODO(jmesserly): negation should support any selectors in level 4,
  // not just simple selectors.
  // http://dev.w3.org/csswg/selectors-4/#negation
  @override
  bool visitNegationSelector(NegationSelector node) => !(node.negationArg!.visit(this) as bool);

  @override
  bool visitAttributeSelector(AttributeSelector node) {
    // Match name first
    final value = _element!.attributes[node.name.toLowerCase()];
    if (value == null) return false;

    if (node.operatorKind == TokenKind.NO_MATCH) return true;

    final select = '${node.value}';
    switch (node.operatorKind) {
      case TokenKind.EQUALS:
        return value == select;
      case TokenKind.INCLUDES:
        return value.split(' ').any((v) => v.isNotEmpty && v == select);
      case TokenKind.DASH_MATCH:
        return value.startsWith(select) && (value.length == select.length || value[select.length] == '-');
      case TokenKind.PREFIX_MATCH:
        return value.startsWith(select);
      case TokenKind.SUFFIX_MATCH:
        return value.endsWith(select);
      case TokenKind.SUBSTRING_MATCH:
        return value.contains(select);
      default:
        if (kDebugMode) throw _unsupported(node);
    }
    return false;
  }

  UnimplementedError _unimplemented(SimpleSelector selector) => UnimplementedError("'$selector' selector of type "
      '${selector.runtimeType} is not implemented');

  FormatException _unsupported(selector) => FormatException("'$selector' is not a valid selector");

  bool isWhitespaceCC(int charCode) {
    switch (charCode) {
      case TokenChar.TAB: // '\t'
      case TokenChar.NEWLINE: // '\n'
      case TokenChar.FF: // '\f'
      case TokenChar.RETURN: // '\r'
      case TokenChar.SPACE: // ' '
        return true;
    }
    return false;
  }
}
