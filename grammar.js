/**
 * @file Dagor BLK (DataBlock) text format grammar for tree-sitter
 * @author Gaijin Entertainment
 * @author Anton Zinovyev <xog3@yandex.ru>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

export default grammar({
  name: 'blk',

  externals: $ => [
    $.comment,
    $.string,
    $._parameter_value,
    $._simple_value,
    $._include_path,
    $._array_value,
    $._array_open,
    $._unexpected_line_break,
    $._quoted_type,
    $._quoted_include_keyword,
    $._text_after_nul,
    $._separator,
    $._separator_reset,
    $._error_sentinel,
  ],

  extras: $ => [/[ \t\r\n\x1a]/, $.comment, $._text_after_nul, $._separator_reset],

  word: $ => $.identifier,

  rules: {
    root_block: $ => repeat($._statement),

    _statement: $ => choice($.block, $.parameter, $.include),

    block: $ => seq($._name, '{', repeat($._statement), '}'),

    parameter: $ => seq(
      $._name,
      choice(
        seq(
          ':',
          $._type,
          '=',
          field('value', choice($.string, alias($._parameter_value, $.value))),
          optional($._value_separator),
        ),
        seq(':', $._type, '[]', '=', field('value', $.array)),
        seq('=', field('value', choice($.string, alias($._simple_value, $.value))), optional($._value_separator)),
      ),
    ),

    array: $ => seq(
      alias($._array_open, '['),
      repeat(seq(choice($.string, alias($._array_value, $.value)), optional($._value_separator))),
      ']',
    ),

    include: $ => seq(
      choice(
        alias($._include_keyword, 'include'),
        seq(alias($._quoted_include_keyword, 'include'), optional($._value_separator)),
      ),
      field('file', choice($.string, alias($._include_path, $.value))),
      optional($._value_separator),
    ),

    _name: $ => choice(
      field('name', choice($.identifier, alias($._include_keyword, $.identifier))),
      seq(field('name', $.string), optional($._value_separator)),
    ),

    _type: $ => choice(
      field('type', $.type),
      seq(field('type', alias($._quoted_type_name, $.type)), optional($._value_separator)),
    ),

    _value_separator: $ => alias($._separator, ';'),

    type: _ => choice('t', 'i', 'b', 'c', 'r', 'm', 'p2', 'p3', 'p4', 'ip2', 'ip3', 'ip4', 'i64'),

    _quoted_type_name: $ => alias($._quoted_type, $.string),

    _include_keyword: _ => /[iI][nN][cC][lL][uU][dD][eE]/,

    identifier: _ => /[A-Za-z0-9_.~-]+/,
  },
});
