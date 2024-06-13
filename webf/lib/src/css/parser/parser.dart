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

import 'dart:math' as math;

import 'package:webf/css.dart';

import 'package:source_span/source_span.dart';

part 'tree.dart';
part 'token.dart';
part 'token_kind.dart';
part 'tokenizer.dart';
part 'tokenizer_base.dart';
part 'selector.dart';
part 'visitor.dart';

enum ClauseType {
  none,
  conjunction,
  disjunction,
}

// We assume that the CSS input may contain unexpected tokens, but they will not be print out or throw unless we open it.
bool kShowCSSParseError = false;

/// Used for parser lookup ahead (used for nested selectors Less support).
class ParserState extends TokenizerState {
  final Token peekToken;
  final Token? previousToken;

  ParserState(this.peekToken, this.previousToken, Tokenizer tokenizer) : super(tokenizer);
}

bool get isChecked => true;

// CSS2.1 pseudo-elements which were defined with a single ':'.
const _legacyPseudoElements = <String>{
  'after',
  'before',
  'first-letter',
  'first-line',
};

class CSSParser {
  final Tokenizer tokenizer;

  /// File containing the source being parsed, used to report errors with
  /// source-span locations.
  // final SourceFile file;

  Token? _previousToken;
  late Token _peekToken;

  /// A string containing the baseURL used to resolve relative URLs in the stylesheet.
  String? href;

  CSSParser(String text, {int start = 0, this.href}) : tokenizer = Tokenizer(SourceFile.fromString(text), text, true, start) {
    _peekToken = tokenizer.next();
  }

  /// Main entry point for parsing an entire CSS file.
  CSSStyleSheet parse({double? windowWidth, double? windowHeight, bool? isDarkMode}) {
    final rules = parseRules(windowWidth: windowWidth, windowHeight: windowHeight, isDarkMode: isDarkMode);
    return CSSStyleSheet(rules);
  }

  Map<String, dynamic> parseInlineStyle() {
    Map<String, dynamic> style = {};
    do {
      if (TokenKind.isIdentifier(_peekToken.kind)) {
        var propertyIdent = camelize(identifier().name);
        var resetProperty = false;
        var keepGoing = true;
        while (keepGoing) {
          switch (_peek()) {
            case TokenKind.COLON:
              _eat(TokenKind.COLON);
              keepGoing = false;
              break;
            case TokenKind.SEMICOLON:
            case TokenKind.NEWLINE:
              resetProperty = true;
              _next();
              break;
            case TokenKind.IDENTIFIER:
              if (resetProperty) {
                propertyIdent = identifier().name;
              }
              break;
            default:
              keepGoing = false;
          }
        }
        var expr = processExpr();
        style[propertyIdent] = expr;
      } else if (_peekToken.kind == TokenKind.VAR_DEFINITION) {
        _next();
      } else if (_peekToken.kind == TokenKind.DIRECTIVE_INCLUDE) {
        // TODO @include mixinName in the declaration area.
      } else if (_peekToken.kind == TokenKind.DIRECTIVE_EXTEND) {
        _next();
      }
    } while (_maybeEat(TokenKind.SEMICOLON));
    return style;
  }

  List<CSSRule> parseRules({double? windowWidth, double? windowHeight, bool? isDarkMode}) {
    var rules = <CSSRule>[];
    while (!_maybeEat(TokenKind.END_OF_FILE)) {
      final data = processRule();
      if (data != null) {
        for (CSSRule cssRule in data) {
          if (cssRule is CSSMediaDirective) {
            List<CSSRule>? mediaRules = cssRule.getValidMediaRules(windowWidth, windowHeight, isDarkMode ?? false);
            if (mediaRules != null) {
              rules.addAll(mediaRules);
            }
          } else {
            rules.add(cssRule);
          }
        }
      } else {
        _next();
      }
    }
    checkEndOfFile();
    return rules;
  }

  /// Main entry point for parsing a simple selector sequence.
  List<Selector> parseSelector() {
    var productions = <Selector>[];
    while (!_maybeEat(TokenKind.END_OF_FILE) && !_peekKind(TokenKind.RBRACE)) {
      var selector = processSelector();
      if (selector != null) {
        productions.add(selector);
      } else {
        break; // Prevent infinite loop if we can't parse something.
      }
    }

    checkEndOfFile();
    return productions;
  }

  /// Generate an error if [file] has not been completely consumed.
  void checkEndOfFile() {
    if (!(_peekKind(TokenKind.END_OF_FILE) || _peekKind(TokenKind.INCOMPLETE_COMMENT))) {
      _error('premature end of file unknown CSS');
    }
  }

  /// Guard to break out of parser when an unexpected end of file is found.
  // TODO(jimhug): Failure to call this method can lead to inifinite parser
  //   loops.  Consider embracing exceptions for more errors to reduce
  //   the danger here.
  bool isPrematureEndOfFile() {
    if (_maybeEat(TokenKind.END_OF_FILE)) {
      _error('unexpected end of file');
      return true;
    } else {
      return false;
    }
  }

  ///////////////////////////////////////////////////////////////////
  // Basic support methods
  ///////////////////////////////////////////////////////////////////
  int _peek() {
    return _peekToken.kind;
  }

  Token _next({bool unicodeRange = false}) {
    final next = _previousToken = _peekToken;
    _peekToken = tokenizer.next(unicodeRange: unicodeRange);
    return next;
  }

  bool _peekKind(int kind) {
    return _peekToken.kind == kind;
  }

  // Is the next token a legal identifier?  This includes pseudo-keywords.
  bool _peekIdentifier() {
    return TokenKind.isIdentifier(_peekToken.kind);
  }

  /// Marks the parser/tokenizer look ahead to support Less nested selectors.
  ParserState get _mark => ParserState(_peekToken, _previousToken, tokenizer);

  /// Restores the parser/tokenizer state to state remembered by _mark.
  void _restore(ParserState markedData) {
    tokenizer.restore(markedData);
    _peekToken = markedData.peekToken;
    _previousToken = markedData.previousToken;
  }

  bool _maybeEat(int kind, {bool unicodeRange = false}) {
    if (_peekToken.kind == kind) {
      _previousToken = _peekToken;
      _peekToken = tokenizer.next(unicodeRange: unicodeRange);
      return true;
    } else {
      return false;
    }
  }

  void _eat(int kind, {bool unicodeRange = false}) {
    if (!_maybeEat(kind, unicodeRange: unicodeRange)) {
      _errorExpected(TokenKind.kindToString(kind));
    }
  }

  void _errorExpected(String expected) {
    if (!kShowCSSParseError) return;

    var tok = _next();
    String message;
    try {
      message = 'expected $expected, but found $tok';
    } catch (e) {
      message = 'parsing error expected $expected';
    }
    _error(message);
  }

  void _error(String message, {SourceSpan? location}) {
    if (!kShowCSSParseError) return;
    location ??= _peekToken.span;
    print(location.message(message, color: '\u001b[31m'));
  }

  void _warning(String message, {SourceSpan? location}) {
    if (!kShowCSSParseError) return;
    location ??= _makeSpan(_peekToken.span);
    print(location.message(message, color: '\u001b[35m'));
  }

  SourceSpan _makeSpan(FileSpan start) {
    // TODO(terry): there are places where we are creating spans before we eat
    // the tokens, so using _previousToken is not always valid.
    // TODO(nweiz): use < rather than compareTo when SourceSpan supports it.
    if (_previousToken == null || _previousToken!.span.compareTo(start) < 0) {
      return start;
    }
    return start.expand(_previousToken!.span);
  }

  ///////////////////////////////////////////////////////////////////
  // Top level productions
  ///////////////////////////////////////////////////////////////////

  CSSMediaQuery? processMediaQuery() {
    // Grammar: [ONLY | NOT]? S* media_type S*
    //          [ AND S* MediaExpr ]* | MediaExpr [ AND S* MediaExpr ]*

    var start = _peekToken.span;

    // Is it a unary media operator?
    // @media only screen
    var op = _peekToken.text; //only
    var opLen = op.length;
    var unaryOp = TokenKind.matchMediaOperator(op, 0, opLen);
    if (unaryOp != -1) {
      _next();
      start = _peekToken.span;
    }

    Identifier? type;
    // Get the media type.
    if (_peekIdentifier()) type = identifier(); // screen

    var exprs = <CSSMediaExpression>[];

    while (true) {
      // Parse AND if query has a media_type or previous expression.
      if (exprs.isEmpty && type == null) {
        op = MediaOperator.AND;
      } else {
        var andOp = exprs.isNotEmpty || type != null;
        if (andOp) {
          op = _peekToken.text; // and
          opLen = op.length;
          int matchMOP = TokenKind.matchMediaOperator(op, 0, opLen);
          if (matchMOP != TokenKind.MEDIA_OP_AND && matchMOP != TokenKind.MEDIA_OP_OR) {
            break;
          }
          _next();
        }
      }
      var expr = processMediaExpression(op);
      if (expr == null) break;

      exprs.add(expr);
    }

    if (unaryOp != -1 || type != null || exprs.isNotEmpty) {
      return CSSMediaQuery(unaryOp, type, exprs);
    }
    return null;
  }

  CSSMediaExpression? processMediaExpression([String op = MediaOperator.AND]) {
    var start = _peekToken.span;
    // Grammar: '(' S* media_feature S* [ ':' S* expr ]? ')' S*
    if (_maybeEat(TokenKind.LPAREN)) {
      if (_peekIdentifier()) {
        var feature = identifier().name;
        String text = '';
        if (_maybeEat(TokenKind.COLON)) {
          do {
            text += _next().text;
          } while(!_maybeEat(TokenKind.RPAREN));
          return CSSMediaExpression(op, {feature : text});
        }
      } else if (isChecked) {
        _warning('Missing media feature in media expression', location: _makeSpan(start));
      }
    }
    return null;
  }

  /// Directive grammar:
  ///
  ///     import:             '@import' [string | URI] media_list?
  ///     media:              '@media' media_query_list '{' ruleset '}'
  ///     page:               '@page' [':' IDENT]? '{' declarations '}'
  ///     stylet:             '@stylet' IDENT '{' ruleset '}'
  ///     media_query_list:   IDENT [',' IDENT]
  ///     keyframes:          '@-webkit-keyframes ...' (see grammar below).
  ///     font_face:          '@font-face' '{' declarations '}'
  ///     namespace:          '@namespace name url("xmlns")
  ///     host:               '@host '{' ruleset '}'
  ///     mixin:              '@mixin name [(args,...)] '{' declarations/ruleset '}'
  ///     include:            '@include name [(@arg,@arg1)]
  ///                         '@include name [(@arg...)]
  ///     content:            '@content'
  ///     -moz-document:      '@-moz-document' [ <url> | url-prefix(<string>) |
  ///                             domain(<string>) | regexp(<string) ]# '{'
  ///                           declarations
  ///                         '}'
  ///     supports:           '@supports' supports_condition group_rule_body
  CSSRule? processDirective() {
    var tokenId = _peek();
    switch (tokenId) {
      case TokenKind.DIRECTIVE_IMPORT:
        _next();
        return null;

      case TokenKind.DIRECTIVE_MEDIA:
        _next();
        // print('processDirective CSSMediaDirective start -----  TokenKind.DIRECTIVE_MEDIA');
        CSSMediaQuery? cssMediaQuery = processMediaQuery();
        if (cssMediaQuery != null) {
          _next();
        }
        List<CSSRule>? rules = [];
        do {
          List<CSSRule>? rule = processRule();
          if (rule != null) {
            rules.addAll(rule);
          }
        } while (!_maybeEat(TokenKind.RBRACE));
        // rules.forEach((rule) {
        //   if (rule is CSSStyleRule) {
        //     print(' ----> processDirective CSSMediaDirective forEach ${rule.selectorGroup.selectorText}, color ${rule.declaration.getPropertyValue('color')}');
        //   } else {
        //     print(' ----> processDirective CSSMediaDirective forEach ${rule.runtimeType}');
        //   }
        // });
        // print('processDirective CSSMediaDirective end -----   rules ${rules.length}  TokenKind.DIRECTIVE_MEDIA');
        return CSSMediaDirective(cssMediaQuery, rules);
      case TokenKind.DIRECTIVE_HOST:
        _next();

        return null;

      case TokenKind.DIRECTIVE_PAGE:
        // @page S* IDENT? pseudo_page?
        //      S* '{' S*
        //      [ declaration | margin ]?
        //      [ ';' S* [ declaration | margin ]? ]* '}' S*
        //
        // pseudo_page :
        //      ':' [ "left" | "right" | "first" ]
        //
        // margin :
        //      margin_sym S* '{' declaration [ ';' S* declaration? ]* '}' S*
        //
        // margin_sym : @top-left-corner, @top-left, @bottom-left, etc.
        //
        // See http://www.w3.org/TR/css3-page/#CSS21
        _next();

        return null;
      case TokenKind.DIRECTIVE_CHARSET:
        // @charset S* STRING S* ';'
        _next();

        processQuotedString(false);

        return null;

      // TODO(terry): Workaround Dart2js bug continue not implemented in switch
      //              see https://code.google.com/p/dart/issues/detail?id=8270
      /*
      case TokenKind.DIRECTIVE_MS_KEYFRAMES:
        // TODO(terry): For now only IE 10 (are base level) supports @keyframes,
        // -moz- has only been optional since Oct 2012 release of Firefox, not
        // all versions of webkit support @keyframes and opera doesn't yet
        // support w/o -o- prefix.  Add more warnings for other prefixes when
        // they become optional.
        if (isChecked) {
          _warning('@-ms-keyframes should be @keyframes');
        }
        continue keyframeDirective;

      keyframeDirective:
      */
      case TokenKind.DIRECTIVE_KEYFRAMES:
      case TokenKind.DIRECTIVE_WEB_KIT_KEYFRAMES:
      case TokenKind.DIRECTIVE_MOZ_KEYFRAMES:
      case TokenKind.DIRECTIVE_O_KEYFRAMES:
      // TODO(terry): Remove workaround when bug 8270 is fixed.
      case TokenKind.DIRECTIVE_MS_KEYFRAMES:
        if (tokenId == TokenKind.DIRECTIVE_MS_KEYFRAMES && isChecked) {
          _warning('@-ms-keyframes should be @keyframes');
        }
        // TODO(terry): End of workaround.

        // Key frames grammar:
        //
        //     @[browser]? keyframes [IDENT|STRING] '{' keyframes-blocks '}';
        //
        //     browser: [-webkit-, -moz-, -ms-, -o-]
        //
        //     keyframes-blocks:
        //       [keyframe-selectors '{' declarations '}']* ;
        //
        //     keyframe-selectors:
        //       ['from'|'to'|PERCENTAGE] [',' ['from'|'to'|PERCENTAGE] ]* ;
        _next();

        String name = '';
        if (_peekIdentifier()) {
          name = identifier().name;
        }
        assert(name.isNotEmpty, 'keyframes rule name must not be null');
        _eat(TokenKind.LBRACE);

        var keyframe = CSSKeyframesRule(tokenId, name);
        do {
          List<String> selectors = [];
          do {
            var selector = _next().text;
            final text = _peekToken.text;
            // ignore unit type
            if (TokenKind.matchUnits(text, 0, text.length) != -1) {
              if (_peekToken.kind == TokenKind.PERCENT) {
                selector += text; // join selector & unit
              }
              _next();
            }
            selectors.add(selector);
          } while (_maybeEat(TokenKind.COMMA));

          final declarations = processDeclarations();
          if (declarations.last is CSSStyleDeclaration) {
            keyframe.add(KeyFrameBlock(selectors, declarations.last));
          }
        } while (!_maybeEat(TokenKind.RBRACE) && !isPrematureEndOfFile());

        return keyframe;

      case TokenKind.DIRECTIVE_FONTFACE:
        _next();
        _eat(TokenKind.LBRACE);
        List data = processDeclarations();
        assert(data.isNotEmpty);
        return CSSFontFaceRule(data[0]);
      case TokenKind.DIRECTIVE_STYLET:
        // Stylet grammar:
        //
        //     @stylet IDENT '{'
        //       ruleset
        //     '}'
        _next();

        return null;
      case TokenKind.DIRECTIVE_NAMESPACE:
        // Namespace grammar:
        //
        // @namespace S* [namespace_prefix S*]? [STRING|URI] S* ';' S*
        // namespace_prefix : IDENT
        _next();

        return null;

      case TokenKind.DIRECTIVE_MIXIN:
        return null;

      case TokenKind.DIRECTIVE_INCLUDE:
        return null;
      case TokenKind.DIRECTIVE_CONTENT:
        // TODO(terry): TBD
        _warning('@content not implemented.');
        return null;
      case TokenKind.DIRECTIVE_MOZ_DOCUMENT:
        return null;
      case TokenKind.DIRECTIVE_SUPPORTS:
        return null;
      case TokenKind.DIRECTIVE_VIEWPORT:
      case TokenKind.DIRECTIVE_MS_VIEWPORT:
        return null;
    }
    return null;
  }

  List<CSSRule>? processRule([SelectorGroup? selectorGroup]) {
    if (selectorGroup == null) {
      final directive = processDirective();
      if (directive != null) {
        _maybeEat(TokenKind.SEMICOLON);
        return [directive];
      }
      selectorGroup = processSelectorGroup();
    }
    if (selectorGroup != null) {
      final declarations = processDeclarations();
      CSSStyleDeclaration declaration = declarations.whereType<CSSStyleDeclaration>().last;
      Iterable childRules = declarations.whereType<CSSStyleRule>();
      CSSStyleRule rule = CSSStyleRule(selectorGroup, declaration);
      List<CSSRule> rules = [rule];
      for (CSSStyleRule childRule in childRules) {
        // child Rule
        for (Selector selector in childRule.selectorGroup.selectors) {
          // parentRule
          for (Selector parentSelector in selectorGroup.selectors) {
            List<SimpleSelectorSequence> newSelectorSequences =
                mergeNestedSelector(parentSelector.simpleSelectorSequences, selector.simpleSelectorSequences);
            selector.simpleSelectorSequences.clear();
            selector.simpleSelectorSequences.addAll(newSelectorSequences);
          }
        }
        rules.add(childRule);
      }
      return rules;
    }
    return null;
  }

  List<CSSRule> processGroupRuleBody() {
    var nodes = <CSSRule>[];
    while (!(_peekKind(TokenKind.RBRACE) || _peekKind(TokenKind.END_OF_FILE))) {
      var rules = processRule();
      if (rules != null) {
        nodes.addAll(rules);
        continue;
      }
      break;
    }
    return nodes;
  }

  /// Look ahead to see if what should be a declaration is really a selector.
  /// If it's a selector than it's a nested selector.  This support's Less'
  /// nested selector syntax (requires a look ahead). E.g.,
  ///
  ///     div {
  ///       width : 20px;
  ///       span {
  ///         color: red;
  ///       }
  ///     }
  ///
  /// Two tag name selectors div and span equivalent to:
  ///
  ///     div {
  ///       width: 20px;
  ///     }
  ///     div span {
  ///       color: red;
  ///     }
  ///
  /// Return [:null:] if no selector or [SelectorGroup] if a selector was
  /// parsed.
  SelectorGroup? _nestedSelector() {
    var markedData = _mark;

    // Look a head do we have a nested selector instead of a declaration?
    var selGroup = processSelectorGroup();

    var nestedSelector = selGroup != null && _peekKind(TokenKind.LBRACE);
    //  && messages.messages.isEmpty;

    if (!nestedSelector) {
      // Not a selector so restore the world.
      _restore(markedData);
      return null;
    } else {
      // Remember any messages from look ahead.
      return selGroup;
    }
  }

  // return list of rule || CSSStyleDeclaration
  List<dynamic> processDeclarations({bool checkBrace = true}) {
    if (checkBrace) _eat(TokenKind.LBRACE);

    var declaration = CSSStyleDeclaration();
    List list = [declaration];
    do {
      var selectorGroup = _nestedSelector();
      while (selectorGroup != null) {
        // Nested selector so process as a ruleset.
        List<CSSRule> rule = processRule(selectorGroup)!;
        list.addAll(rule);
        selectorGroup = _nestedSelector();
      }
      processDeclaration(declaration);
    } while (_maybeEat(TokenKind.SEMICOLON));

    if (checkBrace) _eat(TokenKind.RBRACE);
    return list;
  }

  SelectorGroup? processSelectorGroup() {
    var selectors = <Selector>[];

    tokenizer.inSelector = true;
    do {
      var selector = processSelector();
      if (selector != null) {
        selectors.add(selector);
      }
    } while (_maybeEat(TokenKind.COMMA));
    tokenizer.inSelector = false;

    if (selectors.isNotEmpty) {
      return SelectorGroup(selectors);
    }
    return null;
  }

  /// Return list of selectors
  Selector? processSelector() {
    var simpleSequences = <SimpleSelectorSequence>[];
    while (true) {
      // First item is never descendant make sure it's COMBINATOR_NONE.
      var selectorItem = simpleSelectorSequence(simpleSequences.isEmpty);
      if (selectorItem != null) {
        simpleSequences.add(selectorItem);
      } else {
        break;
      }
    }

    if (simpleSequences.isEmpty) return null;

    return Selector(simpleSequences);
  }

  /// Same as [processSelector] but reports an error for each combinator.
  ///
  /// This is a quick fix for parsing <compound-selectors> until the parser
  /// supports Selector Level 4 grammar:
  /// https://drafts.csswg.org/selectors-4/#typedef-compound-selector
  Selector? processCompoundSelector() {
    var selector = processSelector();
    if (selector != null) {
      for (var sequence in selector.simpleSelectorSequences) {
        if (!sequence.isCombinatorNone) {
          _error('compound selector can not contain combinator - ${sequence.combinatorToString}');
        }
      }
    }
    return selector;
  }

  SimpleSelectorSequence? simpleSelectorSequence(bool forceCombinatorNone) {
    var combinatorType = TokenKind.COMBINATOR_NONE;
    var thisOperator = false;

    switch (_peek()) {
      case TokenKind.PLUS:
        _eat(TokenKind.PLUS);
        combinatorType = TokenKind.COMBINATOR_PLUS;
        break;
      case TokenKind.GREATER:
        _eat(TokenKind.GREATER);
        combinatorType = TokenKind.COMBINATOR_GREATER;
        break;
      case TokenKind.TILDE:
        _eat(TokenKind.TILDE);
        combinatorType = TokenKind.COMBINATOR_TILDE;
        break;
      case TokenKind.AMPERSAND:
        _eat(TokenKind.AMPERSAND);
        thisOperator = true;
        break;
    }

    // Check if WHITESPACE existed between tokens if so we're descendent.
    if (combinatorType == TokenKind.COMBINATOR_NONE && !forceCombinatorNone) {
      if (_previousToken != null && _previousToken!.end != _peekToken.start) {
        combinatorType = TokenKind.COMBINATOR_DESCENDANT;
      }
    }

    var simpleSel = thisOperator
        ? ElementSelector(
            ThisOperator(),
          )
        : simpleSelector();
    if (simpleSel == null &&
        (combinatorType == TokenKind.COMBINATOR_PLUS ||
            combinatorType == TokenKind.COMBINATOR_GREATER ||
            combinatorType == TokenKind.COMBINATOR_TILDE)) {
      // For "+ &", "~ &" or "> &" a selector sequence with no name is needed
      // so that the & will have a combinator too.  This is needed to
      // disambiguate selector expressions:
      //    .foo&:hover     combinator before & is NONE
      //    .foo &          combinator before & is DESCDENDANT
      //    .foo > &        combinator before & is GREATER
      simpleSel = ElementSelector(Identifier(''));
    }
    if (simpleSel != null) {
      return SimpleSelectorSequence(simpleSel, combinatorType);
    }
    return null;
  }

  /// Simple selector grammar:
  ///
  ///     simple_selector_sequence
  ///        : [ type_selector | universal ]
  ///          [ HASH | class | attrib | pseudo | negation ]*
  ///        | [ HASH | class | attrib | pseudo | negation ]+
  ///     type_selector
  ///        : [ namespace_prefix ]? element_name
  ///     namespace_prefix
  ///        : [ IDENT | '*' ]? '|'
  ///     element_name
  ///        : IDENT
  ///     universal
  ///        : [ namespace_prefix ]? '*'
  ///     class
  ///        : '.' IDENT
  SimpleSelector? simpleSelector() {
    // TODO(terry): Natalie makes a good point parsing of namespace and element
    //              are essentially the same (asterisk or identifier) other
    //              than the error message for element.  Should consolidate the
    //              code.
    // TODO(terry): Need to handle attribute namespace too.
    dynamic first;
    switch (_peek()) {
      case TokenKind.ASTERISK:
        // Mark as universal namespace.
        _next();
        first = Wildcard();
        break;
      case TokenKind.IDENTIFIER:
        first = identifier();
        break;
      default:
        // Expecting simple selector.
        // TODO(terry): Could be a synthesized token like value, etc.
        if (TokenKind.isKindIdentifier(_peek())) {
          first = identifier();
        } else if (_peekKind(TokenKind.SEMICOLON)) {
          // Can't be a selector if we found a semi-colon.
          return null;
        }
        break;
    }

    if (_maybeEat(TokenKind.NAMESPACE)) {
      _next();
      return null;
    } else if (first != null) {
      return ElementSelector(first);
    } else {
      // Check for HASH | class | attrib | pseudo | negation
      return simpleSelectorTail();
    }
  }

  bool _anyWhiteSpaceBeforePeekToken(int kind) {
    if (_previousToken != null && _previousToken!.kind == kind) {
      // If end of previous token isn't same as the start of peek token then
      // there's something between these tokens probably whitespace.
      return _previousToken!.end != _peekToken.start;
    }

    return false;
  }

  /// type_selector | universal | HASH | class | attrib | pseudo
  SimpleSelector? simpleSelectorTail() {
    // Check for HASH | class | attrib | pseudo | negation
    var start = _peekToken.span;
    switch (_peek()) {
      case TokenKind.HASH:
        _eat(TokenKind.HASH);

        if (_anyWhiteSpaceBeforePeekToken(TokenKind.HASH)) {
          _error('Not a valid ID selector expected #id', location: _makeSpan(start));
          return null;
        }
        return IdSelector(identifier());
      case TokenKind.DOT:
        _eat(TokenKind.DOT);

        if (_anyWhiteSpaceBeforePeekToken(TokenKind.DOT)) {
          _error('Not a valid class selector expected .className', location: _makeSpan(start));
          return null;
        }
        return ClassSelector(identifier());
      case TokenKind.COLON:
        // :pseudo-class ::pseudo-element
        return processPseudoSelector(start);
      case TokenKind.LBRACK:
        return processAttribute();
      case TokenKind.DOUBLE:
        _error('name must start with a alpha character, but found a number');
        _next();
        break;
    }
    return null;
  }

  SimpleSelector? processPseudoSelector(FileSpan start) {
    // :pseudo-class ::pseudo-element
    // TODO(terry): '::' should be token.
    _eat(TokenKind.COLON);
    var pseudoElement = _maybeEat(TokenKind.COLON);

    // TODO(terry): If no identifier specified consider optimizing out the
    //              : or :: and making this a normal selector.  For now,
    //              create an empty pseudoName.
    // TODO(jiangzhou): Forced to evade
    Identifier pseudoName;
    if (_peekIdentifier()) {
      pseudoName = identifier();
      if (pseudoName.isFunction()) {
        return null;
      }
    } else {
      return null;
    }
    var name = pseudoName.name.toLowerCase();

    // Functional pseudo?
    if (_peekToken.kind == TokenKind.LPAREN) {
      if (!pseudoElement && name == 'not') {
        _eat(TokenKind.LPAREN);

        // Negation :   ':NOT(' S* negation_arg S* ')'
        var negArg = simpleSelector();

        _eat(TokenKind.RPAREN);
        return NegationSelector(negArg);
      } else if (!pseudoElement &&
          (name == 'host' || name == 'host-context' || name == 'global-context' || name == '-acx-global-context')) {
        _eat(TokenKind.LPAREN);
        var selector = processCompoundSelector();
        if (selector == null) {
          _errorExpected('a selector argument');
          return null;
        }
        _eat(TokenKind.RPAREN);
        return PseudoClassFunctionSelector(pseudoName, selector);
      } else {
        // Special parsing for expressions in pseudo functions.  Minus is used
        // as operator not identifier.
        // TODO(jmesserly): we need to flip this before we eat the "(" as the
        // next token will be fetched when we do that. I think we should try to
        // refactor so we don't need this boolean; it seems fragile.
        tokenizer.inSelectorExpression = true;
        _eat(TokenKind.LPAREN);

        // Handle function expression.
        var expr = processSelectorExpression();

        tokenizer.inSelectorExpression = false;

        // Used during selector look-a-head if not a SelectorExpression is
        // bad.
        _eat(TokenKind.RPAREN);
        return (pseudoElement)
            ? PseudoElementFunctionSelector(pseudoName, expr)
            : PseudoClassFunctionSelector(pseudoName, expr);
      }
    }

    // Treat CSS2.1 pseudo-elements defined with pseudo class syntax as pseudo-
    // elements for backwards compatibility.
    return pseudoElement || _legacyPseudoElements.contains(name)
        ? PseudoElementSelector(pseudoName, isLegacy: !pseudoElement)
        : PseudoClassSelector(pseudoName);
  }

  /// In CSS3, the expressions are identifiers, strings, or of the form "an+b".
  ///
  ///     : [ [ PLUS | '-' | DIMENSION | NUMBER | STRING | IDENT ] S* ]+
  ///
  ///     num               [0-9]+|[0-9]*\.[0-9]+
  ///     PLUS              '+'
  ///     DIMENSION         {num}{ident}
  ///     NUMBER            {num}
  List<String> /* SelectorExpression | LiteralTerm */ processSelectorExpression() {
    var expressions = <String>[];

    Token? termToken;

    var keepParsing = true;
    while (keepParsing) {
      switch (_peek()) {
        case TokenKind.PLUS:
        case TokenKind.MINUS:
        case TokenKind.INTEGER:
        case TokenKind.PERCENT:
        case TokenKind.DOUBLE:
          termToken = _next();
          expressions.add(termToken.text);
          break;
        case TokenKind.SINGLE_QUOTE:
          final value = processQuotedString(false);
          return ["'${_escapeString(value, single: true)}'"];
        case TokenKind.DOUBLE_QUOTE:
          final value = processQuotedString(false);
          return ['"${_escapeString(value)}"'];
        case TokenKind.IDENTIFIER:
          final value = identifier(); // Snarf up the ident we'll remap, maybe.
          expressions.add(value.name);
          break;
        default:
          keepParsing = false;
      }
    }

    return expressions;
  }

  // Attribute grammar:
  //
  //     attributes :
  //       '[' S* IDENT S* [ ATTRIB_MATCHES S* [ IDENT | STRING ] S* ]? ']'
  //
  //     ATTRIB_MATCHES :
  //       [ '=' | INCLUDES | DASHMATCH | PREFIXMATCH | SUFFIXMATCH | SUBSTRMATCH ]
  //
  //     INCLUDES:         '~='
  //
  //     DASHMATCH:        '|='
  //
  //     PREFIXMATCH:      '^='
  //
  //     SUFFIXMATCH:      '$='
  //
  //     SUBSTRMATCH:      '*='
  AttributeSelector? processAttribute() {
    if (_maybeEat(TokenKind.LBRACK)) {
      var attrName = identifier();

      int op;
      switch (_peek()) {
        case TokenKind.EQUALS:
        case TokenKind.INCLUDES: // ~=
        case TokenKind.DASH_MATCH: // |=
        case TokenKind.PREFIX_MATCH: // ^=
        case TokenKind.SUFFIX_MATCH: // $=
        case TokenKind.SUBSTRING_MATCH: // *=
          op = _peek();
          _next();
          break;
        default:
          op = TokenKind.NO_MATCH;
      }

      dynamic value;
      if (op != TokenKind.NO_MATCH) {
        // Operator hit so we require a value too.
        if (_peekIdentifier()) {
          value = identifier();
        } else {
          value = processQuotedString(false);
        }

        if (value == null) {
          _error('expected attribute value string or ident');
        }
      }

      _eat(TokenKind.RBRACK);

      return AttributeSelector(attrName, op, value);
    }
    return null;
  }

  //  Declaration grammar:
  //
  //  declaration:  property ':' expr prio?
  //
  //  property:  IDENT [or IE hacks]
  //  prio:      !important
  //  expr:      (see processExpr)
  //
  // Here are the ugly IE hacks we need to support:
  //   property: expr prio? \9; - IE8 and below property, /9 before semi-colon
  //   *IDENT                   - IE7 or below
  //   _IDENT                   - IE6 property (automatically a valid ident)
  void processDeclaration(CSSStyleDeclaration style) {
    // IDENT ':' expr '!important'?
    if (TokenKind.isIdentifier(_peekToken.kind)) {
      var propertyIdent = camelize(identifier().name);

      var resetProperty = false;
      var keepGoing = true;
      while (keepGoing) {
        switch (_peek()) {
          case TokenKind.COLON:
            _eat(TokenKind.COLON);
            keepGoing = false;
            break;
          case TokenKind.SEMICOLON:
          case TokenKind.NEWLINE:
            resetProperty = true;
            _next();
            break;
          case TokenKind.IDENTIFIER:
            if (resetProperty) {
              propertyIdent = identifier().name;
            }
            break;
          default:
            keepGoing = false;
        }
      }

      var expr = processExpr();
      if (expr != null) {
        // Handle !important (prio)
        var importantPriority = false;
        // handle multi-important
        while (_maybeEat(TokenKind.IMPORTANT)) {
          importantPriority = true;
        }
        style.setProperty(propertyIdent, expr, isImportant: importantPriority, baseHref: href);
      }
    } else if (_peekToken.kind == TokenKind.VAR_DEFINITION) {
      _next();
    } else if (_peekToken.kind == TokenKind.DIRECTIVE_INCLUDE) {
      // TODO @include mixinName in the declaration area.
    } else if (_peekToken.kind == TokenKind.DIRECTIVE_EXTEND) {
      _next();
    }
  }

  //  Expression grammar:
  //
  //  expression:   term [ operator? term]*
  //
  //  operator:     '/' | ','
  String? processExpr([bool ieFilter = false]) {
    var start = _peekToken.span;
    FileSpan? end;

    bool hasSynaxError = false;

    var parenCount = 0;
    while (!_maybeEat(TokenKind.END_OF_FILE)) {
      if (_peek() == TokenKind.LPAREN) {
        parenCount++;
      }
      if (_peek() == TokenKind.RPAREN) {
        parenCount--;
      }
      if (parenCount <= 0 && (_peek() == TokenKind.SEMICOLON || _peek() == TokenKind.RBRACE)) {
        break;
      }
      if (_peek() == TokenKind.IMPORTANT) {
        if (parenCount == 0) {
          break;
        } else {
          // synax error
          hasSynaxError = true;
        }
      }
      end = _next().span;
    }
    if (hasSynaxError || parenCount < 0) {
      return null;
    }
    if (end != null) {
      return start.expand(end).text;
    }
    return _peekToken.text;
  }

  static const int MAX_UNICODE = 0x10FFFF;

  String processQuotedString([bool urlString = false]) {
    var start = _peekToken.span;

    // URI term sucks up everything inside of quotes(' or ") or between parens
    var stopToken = urlString ? TokenKind.RPAREN : -1;

    // Note: disable skipping whitespace tokens inside a string.
    // TODO(jmesserly): the layering here feels wrong.
    var inString = tokenizer._inString;
    tokenizer._inString = false;

    switch (_peek()) {
      case TokenKind.SINGLE_QUOTE:
        stopToken = TokenKind.SINGLE_QUOTE;
        _next(); // Skip the SINGLE_QUOTE.
        start = _peekToken.span;
        break;
      case TokenKind.DOUBLE_QUOTE:
        stopToken = TokenKind.DOUBLE_QUOTE;
        _next(); // Skip the DOUBLE_QUOTE.
        start = _peekToken.span;
        break;
      default:
        if (urlString) {
          if (_peek() == TokenKind.LPAREN) {
            _next(); // Skip the LPAREN.
            start = _peekToken.span;
          }
          stopToken = TokenKind.RPAREN;
        } else {
          _error('unexpected string', location: _makeSpan(start));
        }
        break;
    }

    // Gobble up everything until we hit our stop token.
    var stringValue = StringBuffer();
    while (_peek() != stopToken && _peek() != TokenKind.END_OF_FILE) {
      stringValue.write(_next().text);
    }

    tokenizer._inString = inString;

    // All characters between quotes is the string.
    if (stopToken != TokenKind.RPAREN) {
      _next(); // Skip the SINGLE_QUOTE or DOUBLE_QUOTE;
    }

    return stringValue.toString();
  }

  // TODO(terry): Hack to gobble up the calc expression as a string looking
  //              for the matching RPAREN the expression is not parsed into the
  //              AST.
  //
  // grammar should be:
  //
  //     <calc()> = calc( <calc-sum> )
  //     <calc-sum> = <calc-product> [ [ '+' | '-' ] <calc-product> ]*
  //     <calc-product> = <calc-value> [ '*' <calc-value> | '/' <number> ]*
  //     <calc-value> = <number> | <dimension> | <percentage> | ( <calc-sum> )
  //
  String processCalcExpression() {
    var inString = tokenizer._inString;
    tokenizer._inString = false;

    // Gobble up everything until we hit our stop token.
    var stringValue = StringBuffer();
    var left = 1;
    var matchingParens = false;
    while (_peek() != TokenKind.END_OF_FILE && !matchingParens) {
      var token = _peek();
      if (token == TokenKind.LPAREN) {
        left++;
      } else if (token == TokenKind.RPAREN) {
        left--;
      }

      matchingParens = left == 0;
      if (!matchingParens) stringValue.write(_next().text);
    }

    if (!matchingParens) {
      _error('problem parsing function expected ), ');
    }

    tokenizer._inString = inString;

    return stringValue.toString();
  }

  //  Function grammar:
  //
  //  function:     IDENT '(' expr ')'
  //
  dynamic processFunction(Identifier func) {
    var name = func.name;

    switch (name) {
      case 'rgb':
        var expr = processExpr();
        if (!_maybeEat(TokenKind.RPAREN)) {
          _error('problem parsing function expected ), ');
        }
        return 'rgb($expr)';
      case 'url':
        // URI term sucks up everything inside of quotes(' or ") or between
        // parens.
        var urlParam = processQuotedString(true);

        // TODO(terry): Better error message and checking for mismatched quotes.
        if (_peek() == TokenKind.END_OF_FILE) {
          _error('problem parsing URI');
        }

        if (_peek() == TokenKind.RPAREN) {
          _next();
        }

        return 'url($urlParam)';
      case 'var':
        // TODO(terry): Consider handling var in IE specific filter/progid.
        //              This will require parsing entire IE specific syntax
        //              e.g. `param = value` or `progid:com_id`, etc.
        //              for example:
        //
        //    var-blur: Blur(Add = 0, Direction = 225, Strength = 10);
        //    var-gradient: progid:DXImageTransform.Microsoft.gradient"
        //      (GradientType=0,StartColorStr='#9d8b83', EndColorStr='#847670');
        var expr = processExpr();
        if (!_maybeEat(TokenKind.RPAREN)) {
          _error('problem parsing var expected');
        }
        return expr;
      default:
        var expr = processExpr();
        if (!_maybeEat(TokenKind.RPAREN)) {
          _error('problem parsing function expected');
        }
        return expr;
    }
  }

  Identifier identifier() {
    var tok = _next();

    if (!TokenKind.isIdentifier(tok.kind) && !TokenKind.isKindIdentifier(tok.kind)) {
      if (isChecked) {
        String message;
        try {
          message = 'expected identifier, but found $tok';
        } catch (e) {
          message = 'parsing error expected identifier';
        }
        _warning(message, location: tok.span);
      }
      return Identifier('');
    }
    return Identifier(tok.text);
  }
}

/// Escapes [text] for use in a CSS string.
/// [single] specifies single quote `'` vs double quote `"`.
String _escapeString(String text, {bool single = false}) {
  StringBuffer? result;

  for (var i = 0; i < text.length; i++) {
    var code = text.codeUnitAt(i);
    String? replace;
    switch (code) {
      case 34 /*'"'*/ :
        if (!single) replace = r'\"';
        break;
      case 39 /*"'"*/ :
        if (single) replace = r"\'";
        break;
    }

    if (replace != null && result == null) {
      result = StringBuffer(text.substring(0, i));
    }

    if (result != null) result.write(replace ?? text[i]);
  }

  return result == null ? text : result.toString();
}
