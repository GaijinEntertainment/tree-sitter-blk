# tree-sitter-blk

[![CI][ci]](https://github.com/GaijinEntertainment/tree-sitter-blk/actions/workflows/ci.yml)
[![crates][crates]](https://crates.io/crates/tree-sitter-blk)
[![npm][npm]](https://www.npmjs.com/package/tree-sitter-blk)
[![pypi][pypi]](https://pypi.org/project/tree-sitter-blk)

A [tree-sitter](https://tree-sitter.github.io/) grammar for BLK text files, the DataBlock text format of the
[Dagor Engine](https://github.com/GaijinEntertainment/DagorEngine). Editors with tree-sitter support use it for syntax
highlighting, and tools use it to read the structure of a BLK file.

The grammar follows the engine's text parser, `DataBlockParser` in
[`prog/engine/ioSys/dataBlock/blk_parser.cpp`](https://github.com/GaijinEntertainment/DagorEngine/blob/main/prog/engine/ioSys/dataBlock/blk_parser.cpp).
It parses every text `.blk` file of the Dagor source tree without errors, except one mission template with `@@token@@`
names, which the engine rejects too. Binary BLK files (such as the `dx12_cache.blk` files) are not text, and the grammar
does not read them.

## What the grammar reads

- Blocks, typed parameters (`t`, `i`, `b`, `c`, `r`, `m`, `p2`, `p3`, `p4`, `ip2`, `ip3`, `ip4`, `i64`) and array
  parameters (`name:t[]=[...]`).
- Names as identifiers or as quoted strings, which includes directives such as `"@override:name"` and `"@delete:name"`.
- Single-quoted, double-quoted and triple-quoted strings with `~` escapes.
- Unquoted values. A value ends where the engine ends it: at `;`, at the end of the line, at `}`, or at `//`.
- `include` directives with a quoted or an unquoted path, and an `include` without a path, which `binblk` loads.
- The quoted forms that the engine reads like a quoted value: a quoted type (`a:"i"=5`), a quoted `include` keyword,
  and a `;` after a quoted name or type (`"name";{}`).
- A `;` after a value where the engine takes it. After a single-line quoted value, only spaces, tabs, and `0x1A` may
  come before it. After an unquoted value, block comments that close on the same line may come before it too. After a
  triple-quoted value, any whitespace and comments may come before it. In an array, a `;` that no value takes is an
  empty element, as in the engine: `name:t[]=[;x;;]` has three elements.
- Line comments and block comments. A block comment nests, except one that opens inside an unquoted value or after it
  on the same line, as in the engine.
- The simple-string form `name=value`, which daGUI and `vromfsPacker` load.
- Text in any byte encoding. Comments, strings and values can contain bytes that are not UTF-8. The text ends at the
  first NUL byte, as it does when the engine loads the file.

The grammar does not check a value against its type, for example that an `i` value is an integer. The engine does that
check after it parses the file.

This file:

```blk
include "base.blk"
lod{ range:r=70; fname:t="lod01.dag" }
```

gives this tree:

```
(document
  (include path: (string))
  (block name: (identifier)
    (parameter name: (identifier) type: (type) value: (raw_value))
    (parameter name: (identifier) type: (type) value: (string))))
```

The node types are `document`, `block`, `parameter`, `array`, `include`, `identifier`, `type`, `string`, `raw_value` and
`comment`. A `parameter` without a `type` field is the simple-string form.

## Use it in Neovim

[nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) (the `main` branch) builds the parser and
installs the highlight query. Its README lists what it needs.

1. Add this to `init.lua`:

   ```lua
   vim.api.nvim_create_autocmd('User', {
     pattern = 'TSUpdate',
     callback = function()
       require('nvim-treesitter.parsers').blk = {
         install_info = {
           url = 'https://github.com/GaijinEntertainment/tree-sitter-blk',
           queries = 'queries',
         },
       }
     end,
   })
   vim.filetype.add({ extension = { blk = 'blk' } })
   vim.api.nvim_create_autocmd('FileType', {
     pattern = 'blk',
     callback = function() vim.treesitter.start() end,
   })
   ```

2. Run `:TSInstall blk`, then restart Neovim.

To build from a local copy of the grammar, replace `url` with `path` and the directory of that copy. In a Dagor
checkout, the directory is `prog/utils/blk_grammar/tree-sitter-blk`. nvim-treesitter then links the highlight query to
that directory, and `:TSInstall! blk` builds the parser again after a change to the grammar.

Other tree-sitter editors (Helix, Zed, Emacs) use the same grammar directory and `queries/highlights.scm`. Their own
documentation tells how to register a grammar.

## Use it from code

The `bindings/` directory holds bindings for C, Go, Node.js, Python, Rust and Swift. Their package manifests are at the
top of the repository: `CMakeLists.txt` and `Makefile`, `go.mod`, `package.json` and `binding.gyp`, `pyproject.toml`
and `setup.py`, `Cargo.toml`, and `Package.swift`.

## Change the grammar

The tree-sitter CLI 0.27.0 generated the files in `src/` from `grammar.js`, except `src/scanner.c`, which is written by
hand. After you change `grammar.js`, run these commands in this directory:

```sh
tree-sitter generate
tree-sitter test
```

Commit the regenerated `src/` files together with `grammar.js`. The corpus tests are in `test/corpus/`.

Then parse every BLK file of a Dagor checkout. Run these commands from the root of the checkout, with `GRAMMAR` set to
the directory of this grammar:

```sh
git ls-files ':(glob,icase)**/*.blk' ':(exclude,glob)**/dx12_cache.blk' > /tmp/blk-files.txt
tree-sitter parse --grammar-path "$GRAMMAR" --paths /tmp/blk-files.txt --quiet --stat
```

The only file that may fail is `dm_live_mission_template.blk`, the mission template with `@@token@@` names. The engine
rejects it too, with `expected identifier` at its `@@unit_kind@@` block name.

`tree-sitter init` generated the bindings and the package manifests from `tree-sitter.json`. After a change to
`tree-sitter.json`, run `tree-sitter init --update`. To release a new version, run `tree-sitter version <version>`,
which writes the version into `tree-sitter.json` and into every package manifest.

## License

MIT. See `LICENSE`.

[ci]: https://img.shields.io/github/actions/workflow/status/GaijinEntertainment/tree-sitter-blk/ci.yml?logo=github&label=CI
[crates]: https://img.shields.io/crates/v/tree-sitter-blk?logo=rust
[npm]: https://img.shields.io/npm/v/tree-sitter-blk?logo=npm
[pypi]: https://img.shields.io/pypi/v/tree-sitter-blk?logo=pypi&logoColor=ffd242
