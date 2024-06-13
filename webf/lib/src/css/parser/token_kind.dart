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

part of 'parser.dart';

// TODO(terry): Need to be consistent with tokens either they're ASCII tokens
//              e.g., ASTERISK or they're CSS e.g., PSEUDO, COMBINATOR_*.
class TokenKind {
  // Common shared tokens used in TokenizerBase.
  static const int UNUSED = 0; // Unused place holder...
  static const int END_OF_FILE = 1; // EOF
  static const int LPAREN = 2; // (
  static const int RPAREN = 3; // )
  static const int LBRACK = 4; // [
  static const int RBRACK = 5; // ]
  static const int LBRACE = 6; // {
  static const int RBRACE = 7; // }
  static const int DOT = 8; // .
  static const int SEMICOLON = 9; // ;

  // Unique tokens for CSS.
  static const int AT = 10; // @
  static const int HASH = 11; // #
  static const int PLUS = 12; // +
  static const int GREATER = 13; // >
  static const int TILDE = 14; // ~
  static const int ASTERISK = 15; // *
  static const int NAMESPACE = 16; // |
  static const int COLON = 17; // :
  static const int PRIVATE_NAME = 18; // _ prefix private class or id
  static const int COMMA = 19; // ,
  static const int SPACE = 20;
  static const int TAB = 21; // /t
  static const int NEWLINE = 22; // /n
  static const int RETURN = 23; // /r
  static const int PERCENT = 24; // %
  static const int SINGLE_QUOTE = 25; // '
  static const int DOUBLE_QUOTE = 26; // "
  static const int SLASH = 27; // /
  static const int EQUALS = 28; // =
  static const int CARET = 30; // ^
  static const int DOLLAR = 31; // $
  static const int LESS = 32; // <
  static const int BANG = 33; // !
  static const int MINUS = 34; // -
  static const int BACKSLASH = 35; // \
  static const int AMPERSAND = 36; // &

  // WARNING: Tokens from this point and above must have the corresponding ASCII
  //          character in the TokenChar list at the bottom of this file.  The
  //          order of the above tokens should be the same order as TokenChar.

  /// [TokenKind] representing integer tokens.
  static const int INTEGER = 60;

  /// [TokenKind] representing hex integer tokens.
  static const int HEX_INTEGER = 61;

  /// [TokenKind] representing double tokens.
  static const int DOUBLE = 62;

  /// [TokenKind] representing whitespace tokens.
  static const int WHITESPACE = 63;

  /// [TokenKind] representing comment tokens.
  static const int COMMENT = 64;

  /// [TokenKind] representing error tokens.
  static const int ERROR = 65;

  /// [TokenKind] representing incomplete string tokens.
  static const int INCOMPLETE_STRING = 66;

  /// [TokenKind] representing incomplete comment tokens.
  static const int INCOMPLETE_COMMENT = 67;

  static const int VAR_DEFINITION = 400; // var-NNN-NNN
  static const int VAR_USAGE = 401; // var(NNN-NNN [,default])

  // Synthesized Tokens (no character associated with TOKEN).
  static const int STRING = 500;
  static const int STRING_PART = 501;
  static const int NUMBER = 502;
  static const int HEX_NUMBER = 503;
  static const int HTML_COMMENT = 504; // <!--
  static const int IMPORTANT = 505; // !important
  static const int CDATA_START = 506; // <![CDATA[
  static const int CDATA_END = 507; // ]]>
  // U+uNumber[-U+uNumber]
  // uNumber = 0..10FFFF | ?[?]*
  static const int UNICODE_RANGE = 508;
  static const int HEX_RANGE = 509; // ? in the hex range
  static const int IDENTIFIER = 511;

  // Uniquely synthesized tokens for CSS.
  static const int SELECTOR_EXPRESSION = 512;
  static const int COMBINATOR_NONE = 513;
  static const int COMBINATOR_DESCENDANT = 514; // Space combinator
  static const int COMBINATOR_PLUS = 515; // + combinator
  static const int COMBINATOR_GREATER = 516; // > combinator
  static const int COMBINATOR_TILDE = 517; // ~ combinator

  static const int UNARY_OP_NONE = 518; // No unary operator present.

  // Attribute match types:
  static const int INCLUDES = 530; // '~='
  static const int DASH_MATCH = 531; // '|='
  static const int PREFIX_MATCH = 532; // '^='
  static const int SUFFIX_MATCH = 533; // '$='
  static const int SUBSTRING_MATCH = 534; // '*='
  static const int NO_MATCH = 535; // No operator.

  // Unit types:
  static const int UNIT_EM = 600;
  static const int UNIT_EX = 601;
  static const int UNIT_LENGTH_PX = 602;
  static const int UNIT_LENGTH_CM = 603;
  static const int UNIT_LENGTH_MM = 604;
  static const int UNIT_LENGTH_IN = 605;
  static const int UNIT_LENGTH_PT = 606;
  static const int UNIT_LENGTH_PC = 607;
  static const int UNIT_ANGLE_DEG = 608;
  static const int UNIT_ANGLE_RAD = 609;
  static const int UNIT_ANGLE_GRAD = 610;
  static const int UNIT_ANGLE_TURN = 611;
  static const int UNIT_TIME_MS = 612;
  static const int UNIT_TIME_S = 613;
  static const int UNIT_FREQ_HZ = 614;
  static const int UNIT_FREQ_KHZ = 615;
  static const int UNIT_PERCENT = 616;
  static const int UNIT_FRACTION = 617;
  static const int UNIT_RESOLUTION_DPI = 618;
  static const int UNIT_RESOLUTION_DPCM = 619;
  static const int UNIT_RESOLUTION_DPPX = 620;
  static const int UNIT_CH = 621; // Measure of "0" U+0030 glyph.
  static const int UNIT_REM = 622; // computed value ‘font-size’ on root elem.
  static const int UNIT_VIEWPORT_VW = 623;
  static const int UNIT_VIEWPORT_VH = 624;
  static const int UNIT_VIEWPORT_VMIN = 625;
  static const int UNIT_VIEWPORT_VMAX = 626;

  // Directives (@nnnn)
  static const int DIRECTIVE_NONE = 640;
  static const int DIRECTIVE_IMPORT = 641;
  static const int DIRECTIVE_MEDIA = 642;
  static const int DIRECTIVE_PAGE = 643;
  static const int DIRECTIVE_CHARSET = 644;
  static const int DIRECTIVE_STYLET = 645;
  static const int DIRECTIVE_KEYFRAMES = 646;
  static const int DIRECTIVE_WEB_KIT_KEYFRAMES = 647;
  static const int DIRECTIVE_MOZ_KEYFRAMES = 648;
  static const int DIRECTIVE_MS_KEYFRAMES = 649;
  static const int DIRECTIVE_O_KEYFRAMES = 650;
  static const int DIRECTIVE_FONTFACE = 651;
  static const int DIRECTIVE_NAMESPACE = 652;
  static const int DIRECTIVE_HOST = 653;
  static const int DIRECTIVE_MIXIN = 654;
  static const int DIRECTIVE_INCLUDE = 655;
  static const int DIRECTIVE_CONTENT = 656;
  static const int DIRECTIVE_EXTEND = 657;
  static const int DIRECTIVE_MOZ_DOCUMENT = 658;
  static const int DIRECTIVE_SUPPORTS = 659;
  static const int DIRECTIVE_VIEWPORT = 660;
  static const int DIRECTIVE_MS_VIEWPORT = 661;

  // Media query operators
  static const int MEDIA_OP_ONLY = 665; // Unary.
  static const int MEDIA_OP_NOT = 666; // Unary.
  static const int MEDIA_OP_AND = 667; // Binary.
  static const int MEDIA_OP_OR = 668; // Binary.

  // Directives inside of a @page (margin sym).
  static const int MARGIN_DIRECTIVE_TOPLEFTCORNER = 670;
  static const int MARGIN_DIRECTIVE_TOPLEFT = 671;
  static const int MARGIN_DIRECTIVE_TOPCENTER = 672;
  static const int MARGIN_DIRECTIVE_TOPRIGHT = 673;
  static const int MARGIN_DIRECTIVE_TOPRIGHTCORNER = 674;
  static const int MARGIN_DIRECTIVE_BOTTOMLEFTCORNER = 675;
  static const int MARGIN_DIRECTIVE_BOTTOMLEFT = 676;
  static const int MARGIN_DIRECTIVE_BOTTOMCENTER = 677;
  static const int MARGIN_DIRECTIVE_BOTTOMRIGHT = 678;
  static const int MARGIN_DIRECTIVE_BOTTOMRIGHTCORNER = 679;
  static const int MARGIN_DIRECTIVE_LEFTTOP = 680;
  static const int MARGIN_DIRECTIVE_LEFTMIDDLE = 681;
  static const int MARGIN_DIRECTIVE_LEFTBOTTOM = 682;
  static const int MARGIN_DIRECTIVE_RIGHTTOP = 683;
  static const int MARGIN_DIRECTIVE_RIGHTMIDDLE = 684;
  static const int MARGIN_DIRECTIVE_RIGHTBOTTOM = 685;

  // Simple selector type.
  static const int CLASS_NAME = 700; // .class
  static const int ELEMENT_NAME = 701; // tagName
  static const int HASH_NAME = 702; // #elementId
  static const int ATTRIBUTE_NAME = 703; // [attrib]
  static const int PSEUDO_ELEMENT_NAME = 704; // ::pseudoElement
  static const int PSEUDO_CLASS_NAME = 705; // :pseudoClass
  static const int NEGATION = 706; // NOT

  static const List<Map<String, dynamic>> _DIRECTIVES = [
    {'type': TokenKind.DIRECTIVE_IMPORT, 'value': 'import'},
    {'type': TokenKind.DIRECTIVE_MEDIA, 'value': 'media'},
    {'type': TokenKind.DIRECTIVE_PAGE, 'value': 'page'},
    {'type': TokenKind.DIRECTIVE_CHARSET, 'value': 'charset'},
    {'type': TokenKind.DIRECTIVE_STYLET, 'value': 'stylet'},
    {'type': TokenKind.DIRECTIVE_KEYFRAMES, 'value': 'keyframes'},
    {'type': TokenKind.DIRECTIVE_WEB_KIT_KEYFRAMES, 'value': '-webkit-keyframes'},
    {'type': TokenKind.DIRECTIVE_MOZ_KEYFRAMES, 'value': '-moz-keyframes'},
    {'type': TokenKind.DIRECTIVE_MS_KEYFRAMES, 'value': '-ms-keyframes'},
    {'type': TokenKind.DIRECTIVE_O_KEYFRAMES, 'value': '-o-keyframes'},
    {'type': TokenKind.DIRECTIVE_FONTFACE, 'value': 'font-face'},
    {'type': TokenKind.DIRECTIVE_NAMESPACE, 'value': 'namespace'},
    {'type': TokenKind.DIRECTIVE_HOST, 'value': 'host'},
    {'type': TokenKind.DIRECTIVE_MIXIN, 'value': 'mixin'},
    {'type': TokenKind.DIRECTIVE_INCLUDE, 'value': 'include'},
    {'type': TokenKind.DIRECTIVE_CONTENT, 'value': 'content'},
    {'type': TokenKind.DIRECTIVE_EXTEND, 'value': 'extend'},
    {'type': TokenKind.DIRECTIVE_MOZ_DOCUMENT, 'value': '-moz-document'},
    {'type': TokenKind.DIRECTIVE_SUPPORTS, 'value': 'supports'},
    {'type': TokenKind.DIRECTIVE_VIEWPORT, 'value': 'viewport'},
    {'type': TokenKind.DIRECTIVE_MS_VIEWPORT, 'value': '-ms-viewport'},
  ];

  static const List<Map<String, dynamic>> MEDIA_OPERATORS = [
    {'type': TokenKind.MEDIA_OP_ONLY, 'value': 'only'},
    {'type': TokenKind.MEDIA_OP_NOT, 'value': 'not'},
    {'type': TokenKind.MEDIA_OP_AND, 'value': 'and'},
    {'type': TokenKind.MEDIA_OP_OR, 'value': 'or'},
    {'type': TokenKind.MEDIA_OP_OR, 'value': ','},
  ];

  static const List<Map<String, dynamic>> MARGIN_DIRECTIVES = [
    {'type': TokenKind.MARGIN_DIRECTIVE_TOPLEFTCORNER, 'value': 'top-left-corner'},
    {'type': TokenKind.MARGIN_DIRECTIVE_TOPLEFT, 'value': 'top-left'},
    {'type': TokenKind.MARGIN_DIRECTIVE_TOPCENTER, 'value': 'top-center'},
    {'type': TokenKind.MARGIN_DIRECTIVE_TOPRIGHT, 'value': 'top-right'},
    {'type': TokenKind.MARGIN_DIRECTIVE_TOPRIGHTCORNER, 'value': 'top-right-corner'},
    {'type': TokenKind.MARGIN_DIRECTIVE_BOTTOMLEFTCORNER, 'value': 'bottom-left-corner'},
    {'type': TokenKind.MARGIN_DIRECTIVE_BOTTOMLEFT, 'value': 'bottom-left'},
    {'type': TokenKind.MARGIN_DIRECTIVE_BOTTOMCENTER, 'value': 'bottom-center'},
    {'type': TokenKind.MARGIN_DIRECTIVE_BOTTOMRIGHT, 'value': 'bottom-right'},
    {'type': TokenKind.MARGIN_DIRECTIVE_BOTTOMRIGHTCORNER, 'value': 'bottom-right-corner'},
    {'type': TokenKind.MARGIN_DIRECTIVE_LEFTTOP, 'value': 'left-top'},
    {'type': TokenKind.MARGIN_DIRECTIVE_LEFTMIDDLE, 'value': 'left-middle'},
    {'type': TokenKind.MARGIN_DIRECTIVE_LEFTBOTTOM, 'value': 'right-bottom'},
    {'type': TokenKind.MARGIN_DIRECTIVE_RIGHTTOP, 'value': 'right-top'},
    {'type': TokenKind.MARGIN_DIRECTIVE_RIGHTMIDDLE, 'value': 'right-middle'},
    {'type': TokenKind.MARGIN_DIRECTIVE_RIGHTBOTTOM, 'value': 'right-bottom'},
  ];

  static const List<Map<String, dynamic>> _UNITS = [
    {'unit': TokenKind.UNIT_EM, 'value': 'em'},
    {'unit': TokenKind.UNIT_EX, 'value': 'ex'},
    {'unit': TokenKind.UNIT_LENGTH_PX, 'value': 'px'},
    {'unit': TokenKind.UNIT_LENGTH_CM, 'value': 'cm'},
    {'unit': TokenKind.UNIT_LENGTH_MM, 'value': 'mm'},
    {'unit': TokenKind.UNIT_LENGTH_IN, 'value': 'in'},
    {'unit': TokenKind.UNIT_LENGTH_PT, 'value': 'pt'},
    {'unit': TokenKind.UNIT_LENGTH_PC, 'value': 'pc'},
    {'unit': TokenKind.UNIT_ANGLE_DEG, 'value': 'deg'},
    {'unit': TokenKind.UNIT_ANGLE_RAD, 'value': 'rad'},
    {'unit': TokenKind.UNIT_ANGLE_GRAD, 'value': 'grad'},
    {'unit': TokenKind.UNIT_ANGLE_TURN, 'value': 'turn'},
    {'unit': TokenKind.UNIT_TIME_MS, 'value': 'ms'},
    {'unit': TokenKind.UNIT_TIME_S, 'value': 's'},
    {'unit': TokenKind.UNIT_FREQ_HZ, 'value': 'hz'},
    {'unit': TokenKind.UNIT_FREQ_KHZ, 'value': 'khz'},
    {'unit': TokenKind.UNIT_FRACTION, 'value': 'fr'},
    {'unit': TokenKind.UNIT_RESOLUTION_DPI, 'value': 'dpi'},
    {'unit': TokenKind.UNIT_RESOLUTION_DPCM, 'value': 'dpcm'},
    {'unit': TokenKind.UNIT_RESOLUTION_DPPX, 'value': 'dppx'},
    {'unit': TokenKind.UNIT_CH, 'value': 'ch'},
    {'unit': TokenKind.UNIT_REM, 'value': 'rem'},
    {'unit': TokenKind.UNIT_VIEWPORT_VW, 'value': 'vw'},
    {'unit': TokenKind.UNIT_VIEWPORT_VH, 'value': 'vh'},
    {'unit': TokenKind.UNIT_VIEWPORT_VMIN, 'value': 'vmin'},
    {'unit': TokenKind.UNIT_VIEWPORT_VMAX, 'value': 'vmax'},
    {'unit': TokenKind.UNIT_PERCENT, 'value': '%'},
  ];

  // Some more constants:
  static const int ASCII_UPPER_A = 65; // ASCII value for uppercase A
  static const int ASCII_UPPER_Z = 90; // ASCII value for uppercase Z

  /// Return the token that matches the unit ident found.
  static int matchList(
      Iterable<Map<String, dynamic>> identList, String tokenField, String text, int offset, int length) {
    for (final entry in identList) {
      final ident = entry['value'] as String;

      if (length == ident.length) {
        var idx = offset;
        var match = true;
        for (var i = 0; i < ident.length; i++) {
          var identChar = ident.codeUnitAt(i);
          var char = text.codeUnitAt(idx++);
          // Compare lowercase to lowercase then check if char is uppercase.
          match = match &&
              (char == identChar || ((char >= ASCII_UPPER_A && char <= ASCII_UPPER_Z) && (char + 32) == identChar));
          if (!match) {
            break;
          }
        }

        if (match) {
          // Completely matched; return the token for this unit.
          return entry[tokenField] as int;
        }
      }
    }

    return -1; // Not a unit token.
  }

  /// Return the token that matches the directive name found.
  static int matchDirectives(String text, int offset, int length) {
    return matchList(_DIRECTIVES, 'type', text, offset, length);
  }

  /// Return the token that matches the margin directive name found.
  static int matchMarginDirectives(String text, int offset, int length) {
    return matchList(MARGIN_DIRECTIVES, 'type', text, offset, length);
  }

  /// Return the token that matches the media operator found.
  static int matchMediaOperator(String text, int offset, int length) {
    return matchList(MEDIA_OPERATORS, 'type', text, offset, length);
  }

  /// Return the token that matches the unit ident found.
  static int matchUnits(String text, int offset, int length) {
    return matchList(_UNITS, 'unit', text, offset, length);
  }

  static String? idToValue(Iterable<Object?> identList, int tokenId) {
    for (var entry in identList) {
      entry as Map<String, Object?>;
      if (tokenId == entry['type']) {
        return entry['value'] as String?;
      }
    }

    return null;
  }

  /// Return RGB value as [int] from a color entry in _EXTENDED_COLOR_NAMES.
  static int colorValue(Map entry) {
    return entry['value'] as int;
  }

  static String decimalToHex(int number, [int minDigits = 1]) {
    final _HEX_DIGITS = '0123456789abcdef';

    var result = <String>[];

    var dividend = number >> 4;
    var remain = number % 16;
    result.add(_HEX_DIGITS[remain]);
    while (dividend != 0) {
      remain = dividend % 16;
      dividend >>= 4;
      result.add(_HEX_DIGITS[remain]);
    }

    var invertResult = StringBuffer();
    var paddings = minDigits - result.length;
    while (paddings-- > 0) {
      invertResult.write('0');
    }
    for (var i = result.length - 1; i >= 0; i--) {
      invertResult.write(result[i]);
    }

    return invertResult.toString();
  }

  static String kindToString(int kind) {
    switch (kind) {
      case TokenKind.UNUSED:
        return 'ERROR';
      case TokenKind.END_OF_FILE:
        return 'end of file';
      case TokenKind.LPAREN:
        return '(';
      case TokenKind.RPAREN:
        return ')';
      case TokenKind.LBRACK:
        return '[';
      case TokenKind.RBRACK:
        return ']';
      case TokenKind.LBRACE:
        return '{';
      case TokenKind.RBRACE:
        return '}';
      case TokenKind.DOT:
        return '.';
      case TokenKind.SEMICOLON:
        return ';';
      case TokenKind.AT:
        return '@';
      case TokenKind.HASH:
        return '#';
      case TokenKind.PLUS:
        return '+';
      case TokenKind.GREATER:
        return '>';
      case TokenKind.TILDE:
        return '~';
      case TokenKind.ASTERISK:
        return '*';
      case TokenKind.NAMESPACE:
        return '|';
      case TokenKind.COLON:
        return ':';
      case TokenKind.PRIVATE_NAME:
        return '_';
      case TokenKind.COMMA:
        return ',';
      case TokenKind.SPACE:
        return ' ';
      case TokenKind.TAB:
        return '\t';
      case TokenKind.NEWLINE:
        return '\n';
      case TokenKind.RETURN:
        return '\r';
      case TokenKind.PERCENT:
        return '%';
      case TokenKind.SINGLE_QUOTE:
        return "'";
      case TokenKind.DOUBLE_QUOTE:
        return '"';
      case TokenKind.SLASH:
        return '/';
      case TokenKind.EQUALS:
        return '=';
      case TokenKind.CARET:
        return '^';
      case TokenKind.DOLLAR:
        return '\$';
      case TokenKind.LESS:
        return '<';
      case TokenKind.BANG:
        return '!';
      case TokenKind.MINUS:
        return '-';
      case TokenKind.BACKSLASH:
        return '\\';
      default:
        throw 'Unknown TOKEN';
    }
  }

  static bool isKindIdentifier(int kind) {
    switch (kind) {
      // Synthesized tokens.
      case TokenKind.DIRECTIVE_IMPORT:
      case TokenKind.DIRECTIVE_MEDIA:
      case TokenKind.DIRECTIVE_PAGE:
      case TokenKind.DIRECTIVE_CHARSET:
      case TokenKind.DIRECTIVE_STYLET:
      case TokenKind.DIRECTIVE_KEYFRAMES:
      case TokenKind.DIRECTIVE_WEB_KIT_KEYFRAMES:
      case TokenKind.DIRECTIVE_MOZ_KEYFRAMES:
      case TokenKind.DIRECTIVE_MS_KEYFRAMES:
      case TokenKind.DIRECTIVE_O_KEYFRAMES:
      case TokenKind.DIRECTIVE_FONTFACE:
      case TokenKind.DIRECTIVE_NAMESPACE:
      case TokenKind.DIRECTIVE_HOST:
      case TokenKind.DIRECTIVE_MIXIN:
      case TokenKind.DIRECTIVE_INCLUDE:
      case TokenKind.DIRECTIVE_CONTENT:
        return true;
      default:
        return false;
    }
  }

  static bool isIdentifier(int kind) {
    return kind == IDENTIFIER;
  }
}

// Note: these names should match TokenKind names
class TokenChar {
  static const int UNUSED = -1;
  static const int END_OF_FILE = 0;
  static const int LPAREN = 0x28; // "(".codeUnitAt(0)
  static const int RPAREN = 0x29; // ")".codeUnitAt(0)
  static const int LBRACK = 0x5b; // "[".codeUnitAt(0)
  static const int RBRACK = 0x5d; // "]".codeUnitAt(0)
  static const int LBRACE = 0x7b; // "{".codeUnitAt(0)
  static const int RBRACE = 0x7d; // "}".codeUnitAt(0)
  static const int DOT = 0x2e; // ".".codeUnitAt(0)
  static const int SEMICOLON = 0x3b; // ";".codeUnitAt(0)
  static const int AT = 0x40; // "@".codeUnitAt(0)
  static const int HASH = 0x23; // "#".codeUnitAt(0)
  static const int PLUS = 0x2b; // "+".codeUnitAt(0)
  static const int GREATER = 0x3e; // ">".codeUnitAt(0)
  static const int TILDE = 0x7e; // "~".codeUnitAt(0)
  static const int ASTERISK = 0x2a; // "*".codeUnitAt(0)
  static const int NAMESPACE = 0x7c; // "|".codeUnitAt(0)
  static const int COLON = 0x3a; // ":".codeUnitAt(0)
  static const int PRIVATE_NAME = 0x5f; // "_".codeUnitAt(0)
  static const int COMMA = 0x2c; // ",".codeUnitAt(0)
  static const int SPACE = 0x20; // " ".codeUnitAt(0)
  static const int TAB = 0x9; // "\t".codeUnitAt(0)
  static const int NEWLINE = 0xa; // "\n".codeUnitAt(0)
  static const int RETURN = 0xd; // "\r".codeUnitAt(0)
  static const int BACKSPACE = 0x8; // "/b".codeUnitAt(0)
  static const int FF = 0xc; // "/f".codeUnitAt(0)
  static const int VT = 0xb; // "/v".codeUnitAt(0)
  static const int PERCENT = 0x25; // "%".codeUnitAt(0)
  static const int SINGLE_QUOTE = 0x27; // "'".codeUnitAt(0)
  static const int DOUBLE_QUOTE = 0x22; // '"'.codeUnitAt(0)
  static const int SLASH = 0x2f; // "/".codeUnitAt(0)
  static const int EQUALS = 0x3d; // "=".codeUnitAt(0)
  static const int OR = 0x7c; // "|".codeUnitAt(0)
  static const int CARET = 0x5e; // "^".codeUnitAt(0)
  static const int DOLLAR = 0x24; // "\$".codeUnitAt(0)
  static const int LESS = 0x3c; // "<".codeUnitAt(0)
  static const int BANG = 0x21; // "!".codeUnitAt(0)
  static const int MINUS = 0x2d; // "-".codeUnitAt(0)
  static const int BACKSLASH = 0x5c; // "\".codeUnitAt(0)
  static const int AMPERSAND = 0x26; // "&".codeUnitAt(0)
}

class MediaType {
  static const String ALL = 'all';
  static const String PRINT = 'print';
  static const String SCREEN = 'screen';
  static const String SPEECH = 'speech';
}
class MediaOperator {
  static const String NOT = 'not';
  static const String ONLY = 'only';
  static const String AND = 'and';
  static const String OR = 'or';
  static const String OR2 = ',';
}
