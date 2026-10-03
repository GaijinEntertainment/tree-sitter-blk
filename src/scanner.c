#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// constraint: the order matches the externals array in grammar.js
enum TokenType {
  COMMENT,
  STRING,
  PARAMETER_VALUE,
  SIMPLE_VALUE,
  INCLUDE_PATH,
  ARRAY_VALUE,
  ARRAY_OPEN,
  UNEXPECTED_LINE_BREAK,
  QUOTED_TYPE,
  QUOTED_INCLUDE_KEYWORD,
  TEXT_AFTER_NUL,
  SEPARATOR,
  ERROR_SENTINEL,
};

// constraint: mirrors where DataBlockParser::getValue takes the `;` after the value it read
enum SeparatorContext {
  NO_SEPARATOR,
  AFTER_QUOTED,
  AFTER_TRIPLE_QUOTED,
  AFTER_UNQUOTED,
};

typedef struct {
  uint8_t separator_context;
} Scanner;

typedef struct {
  char bytes[16];
  unsigned length;
} ShortText;

static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static inline void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static inline bool is_line_end(int32_t c) { return c == '\r' || c == '\n'; }

static inline bool at_text_end(TSLexer *lexer) { return lexer->lookahead == 0; }

static inline bool accept(TSLexer *lexer, enum TokenType type) {
  lexer->mark_end(lexer);
  lexer->result_symbol = type;
  return true;
}

static void short_text_push(ShortText *text, int32_t c) {
  if (text->length < sizeof(text->bytes)) {
    text->bytes[text->length] = c > 0 && c < 0x80 ? (char)c : '\x7f';
  }
  text->length++;
}

static bool short_text_is(const ShortText *text, const char *word) {
  size_t length = strlen(word);
  return text->length == length && memcmp(text->bytes, word, length) == 0;
}

static bool is_type_name(const ShortText *text) {
  // constraint: the names match the type rule in grammar.js
  static const char *const names[] = {"t", "i", "b", "c", "r", "m", "p2", "p3", "p4", "ip2", "ip3", "ip4", "i64"};
  for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    if (short_text_is(text, names[i])) {
      return true;
    }
  }
  return false;
}

static bool is_include_keyword(const ShortText *text) {
  static const char keyword[] = "include";
  if (text->length != sizeof(keyword) - 1) {
    return false;
  }
  for (unsigned i = 0; i < text->length; i++) {
    char c = text->bytes[i];
    if ((c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c) != keyword[i]) {
      return false;
    }
  }
  return true;
}

static int32_t unescape(int32_t c) {
  switch (c) {
    case 'r':
      return '\r';
    case 'n':
      return '\n';
    case 't':
      return '\t';
    default:
      return c;
  }
}

static bool scan_line_comment(TSLexer *lexer, bool take_line_end) {
  while (!at_text_end(lexer) && !is_line_end(lexer->lookahead)) {
    advance(lexer);
  }
  if (take_line_end && !at_text_end(lexer)) {
    advance(lexer);
  }
  return accept(lexer, COMMENT);
}

static bool skip_block_comment(TSLexer *lexer, bool nested, bool *crossed_line_end) {
  unsigned depth = 1;
  while (!at_text_end(lexer)) {
    int32_t c = lexer->lookahead;
    advance(lexer);
    if (is_line_end(c)) {
      *crossed_line_end = true;
    } else if (c == '*' && lexer->lookahead == '/') {
      advance(lexer);
      if (--depth == 0) {
        return true;
      }
    } else if (nested && c == '/' && lexer->lookahead == '*') {
      advance(lexer);
      depth++;
    }
  }
  return false;
}

static bool scan_triple_quoted_tail(TSLexer *lexer, int32_t quote, ShortText *text) {
  bool blank_before_first_line_end = false;
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == '\r') {
    blank_before_first_line_end |= lexer->lookahead != '\r';
    advance(lexer);
  }
  if (lexer->lookahead == '\n') {
    advance(lexer);
  } else if (blank_before_first_line_end) {
    short_text_push(text, ' ');
  }

  while (!at_text_end(lexer)) {
    int32_t c = lexer->lookahead;
    advance(lexer);
    if (c == '~') {
      if (at_text_end(lexer)) {
        return false;
      }
      short_text_push(text, unescape(lexer->lookahead));
      advance(lexer);
    } else if (c == quote && lexer->lookahead == quote) {
      advance(lexer);
      if (lexer->lookahead == quote) {
        advance(lexer);
        lexer->mark_end(lexer);
        if (text->length > 1 && text->length <= sizeof(text->bytes) && text->bytes[text->length - 1] == '\n') {
          text->length--;
        }
        return true;
      }
      short_text_push(text, quote);
      short_text_push(text, quote);
    } else if (c != '\r') {
      short_text_push(text, c);
    }
  }
  return false;
}

static bool scan_quoted(TSLexer *lexer, ShortText *text, bool *triple) {
  int32_t quote = lexer->lookahead;
  advance(lexer);
  if (lexer->lookahead == quote) {
    advance(lexer);
    if (lexer->lookahead != quote) {
      lexer->mark_end(lexer);
      return true;
    }
    advance(lexer);
    *triple = true;
    return scan_triple_quoted_tail(lexer, quote, text);
  }
  while (!at_text_end(lexer) && !is_line_end(lexer->lookahead)) {
    int32_t c = lexer->lookahead;
    advance(lexer);
    if (c == quote) {
      lexer->mark_end(lexer);
      return true;
    }
    if (c == '~') {
      if (at_text_end(lexer)) {
        return false;
      }
      c = unescape(lexer->lookahead);
      advance(lexer);
    }
    short_text_push(text, c);
  }
  return false;
}

static bool next_statement_part_follows_name(TSLexer *lexer) {
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' || lexer->lookahead == 0x1A) {
    advance(lexer);
  }
  if (lexer->lookahead == ';') {
    advance(lexer);
  }
  for (;;) {
    int32_t c = lexer->lookahead;
    if (c == ' ' || c == '\t' || c == 0x1A || is_line_end(c)) {
      advance(lexer);
    } else if (c == '/') {
      advance(lexer);
      if (lexer->lookahead == '/') {
        while (!at_text_end(lexer) && !is_line_end(lexer->lookahead)) {
          advance(lexer);
        }
      } else if (lexer->lookahead == '*') {
        advance(lexer);
        bool crossed_line_end = false;
        if (!skip_block_comment(lexer, true, &crossed_line_end)) {
          return false;
        }
      } else {
        return false;
      }
    } else {
      return c == '{' || c == ':' || c == '=';
    }
  }
}

static void scan_unquoted_value(TSLexer *lexer, enum TokenType kind) {
  lexer->mark_end(lexer);
  while (!at_text_end(lexer)) {
    int32_t c = lexer->lookahead;
    if (c == ';' || c == '}' || is_line_end(c)) {
      break;
    }
    advance(lexer);
    if (c == '/' && lexer->lookahead == '/') {
      break;
    }
    if (c == '/' && lexer->lookahead == '*') {
      advance(lexer);
      bool crossed_line_end = false;
      if (!skip_block_comment(lexer, false, &crossed_line_end) || crossed_line_end) {
        break;
      }
      continue;
    }
    if (c != ' ' && c != '\t') {
      lexer->mark_end(lexer);
    }
  }
  lexer->result_symbol = kind;
}

static bool find_value_kind(const bool *valid_symbols, enum TokenType *kind) {
  static const enum TokenType kinds[] = {PARAMETER_VALUE, SIMPLE_VALUE, INCLUDE_PATH, ARRAY_VALUE};
  for (unsigned i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
    if (valid_symbols[kinds[i]]) {
      *kind = kinds[i];
      return true;
    }
  }
  return false;
}

static bool can_start_value(enum TokenType kind, int32_t c) {
  switch (kind) {
    case PARAMETER_VALUE:
    case SIMPLE_VALUE:
      return true;
    case INCLUDE_PATH:
      return c != '{' && c != ':' && c != '=';
    case ARRAY_VALUE:
      return c != ']' && c != '}';
    default:
      return false;
  }
}

static bool scan_quoted_token(Scanner *scanner, TSLexer *lexer, const bool *valid_symbols, bool in_error_recovery) {
  bool type_expected = !in_error_recovery && valid_symbols[QUOTED_TYPE];
  if (!type_expected && !valid_symbols[STRING]) {
    return false;
  }
  ShortText text = {{0}, 0};
  bool triple = false;
  if (!scan_quoted(lexer, &text, &triple)) {
    return false;
  }
  scanner->separator_context = triple ? AFTER_TRIPLE_QUOTED : AFTER_QUOTED;
  if (type_expected) {
    lexer->result_symbol = QUOTED_TYPE;
    return is_type_name(&text);
  }
  bool include_possible = !in_error_recovery && valid_symbols[QUOTED_INCLUDE_KEYWORD];
  bool is_include = include_possible && is_include_keyword(&text) && !next_statement_part_follows_name(lexer);
  lexer->result_symbol = is_include ? QUOTED_INCLUDE_KEYWORD : STRING;
  return true;
}

static bool scan_comment(Scanner *scanner, TSLexer *lexer, enum SeparatorContext context, bool same_line_after_equals) {
  if (lexer->lookahead == '/') {
    advance(lexer);
    if (context == AFTER_TRIPLE_QUOTED) {
      scanner->separator_context = context;
    }
    return scan_line_comment(lexer, same_line_after_equals);
  }
  advance(lexer);
  bool crossed_line_end = false;
  if (!skip_block_comment(lexer, context != AFTER_UNQUOTED, &crossed_line_end)) {
    return false;
  }
  if (context == AFTER_TRIPLE_QUOTED || (context == AFTER_UNQUOTED && !crossed_line_end)) {
    scanner->separator_context = context;
  }
  return accept(lexer, COMMENT);
}

void *tree_sitter_blk_external_scanner_create(void) { return ts_calloc(1, sizeof(Scanner)); }

void tree_sitter_blk_external_scanner_destroy(void *payload) { ts_free(payload); }

unsigned tree_sitter_blk_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *scanner = payload;
  buffer[0] = (char)scanner->separator_context;
  return 1;
}

void tree_sitter_blk_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = payload;
  scanner->separator_context = length > 0 ? (uint8_t)buffer[0] : NO_SEPARATOR;
}

bool tree_sitter_blk_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *scanner = payload;
  enum SeparatorContext context = scanner->separator_context;
  scanner->separator_context = NO_SEPARATOR;

  bool in_error_recovery = valid_symbols[ERROR_SENTINEL];
  enum TokenType value_kind = PARAMETER_VALUE;
  bool value_expected = !in_error_recovery && find_value_kind(valid_symbols, &value_kind);
  bool array_open_expected = !in_error_recovery && valid_symbols[ARRAY_OPEN];
  bool same_line_after_equals = (value_expected && value_kind == PARAMETER_VALUE) || array_open_expected;

  bool crossed_line_end = false;
  for (;;) {
    int32_t c = lexer->lookahead;
    if (c == ' ' || c == '\t' || c == 0x1A || (is_line_end(c) && !same_line_after_equals)) {
      crossed_line_end |= is_line_end(c);
      skip(lexer);
    } else {
      break;
    }
  }
  if (crossed_line_end && context != AFTER_TRIPLE_QUOTED) {
    context = NO_SEPARATOR;
  }
  if (lexer->eof(lexer)) {
    return false;
  }

  int32_t c = lexer->lookahead;
  if (c == 0) {
    while (!lexer->eof(lexer)) {
      advance(lexer);
    }
    return valid_symbols[TEXT_AFTER_NUL] && accept(lexer, TEXT_AFTER_NUL);
  }
  if (c == ';' && !in_error_recovery && valid_symbols[SEPARATOR] && context != NO_SEPARATOR) {
    advance(lexer);
    return accept(lexer, SEPARATOR);
  }
  if (same_line_after_equals && is_line_end(c)) {
    advance(lexer);
    return accept(lexer, UNEXPECTED_LINE_BREAK);
  }
  if (array_open_expected && c == '[') {
    advance(lexer);
    return accept(lexer, ARRAY_OPEN);
  }
  if (c == '"' || c == '\'') {
    return scan_quoted_token(scanner, lexer, valid_symbols, in_error_recovery);
  }
  if (c == '/') {
    advance(lexer);
    if (lexer->lookahead == '/' || lexer->lookahead == '*') {
      return valid_symbols[COMMENT] && scan_comment(scanner, lexer, context, same_line_after_equals);
    }
  } else if (!can_start_value(value_kind, c)) {
    return false;
  }
  if (!value_expected) {
    return false;
  }
  scan_unquoted_value(lexer, value_kind);
  scanner->separator_context = AFTER_UNQUOTED;
  return true;
}
