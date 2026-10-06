(comment) @comment

(type) @type.builtin

"include" @keyword.import

(include
  file: (_) @string.special.path)

((block
  name: (identifier) @module)
  (#not-match? @module "^_"))

((block
  name: (identifier) @module.builtin)
  (#match? @module.builtin "^_"))

((parameter
  name: (identifier) @property)
  (#not-match? @property "^_"))

((parameter
  name: (identifier) @variable.builtin)
  (#match? @variable.builtin "^_"))

((block
  name: (string) @module)
  (#not-match? @module "^(\"|'|\"\"\"|''')[~]?[@_]"))

((parameter
  name: (string) @property)
  (#not-match? @property "^(\"|'|\"\"\"|''')[~]?[@_]"))

((block
  name: (string) @module.builtin)
  (#match? @module.builtin "^(\"|'|\"\"\"|''')[~]?_"))

((parameter
  name: (string) @variable.builtin)
  (#match? @variable.builtin "^(\"|'|\"\"\"|''')[~]?_"))

([
  (block
    name: (string) @keyword.directive)
  (parameter
    name: (string) @keyword.directive)
]
  (#match? @keyword.directive "^(\"|'|\"\"\"|''')[~]?[@]"))

(parameter
  value: (string) @string)

(array
  (string) @string)

(parameter
  !type
  value: (value) @string)

((parameter
  type: (type) @type.builtin
  value: (value) @string)
  (#match? @type.builtin "^[\"'~]*t[\"']*$"))

((parameter
  type: (type) @type.builtin
  value: (value) @boolean)
  (#match? @type.builtin "^[\"'~]*b[\"']*$"))

((parameter
  type: (type) @type.builtin
  value: (value) @number)
  (#match? @type.builtin "^[\"'~]*(i|i64|r|c|m|p2|p3|p4|ip2|ip3|ip4)[\"']*$"))

((array
  (value) @number)
  (#match? @number "^[-+.0-9]"))

((array
  (value) @boolean)
  (#any-of? @boolean "yes" "no" "true" "false" "on" "off"))

((array
  (value) @string)
  (#not-match? @string "^[-+.0-9]")
  (#not-any-of? @string "yes" "no" "true" "false" "on" "off"))

[
  "{"
  "}"
  "["
  "]"
  "[]"
] @punctuation.bracket

[
  ":"
  ";"
] @punctuation.delimiter

"=" @operator

(include
  file: (value
    "=" @string.special.path))
