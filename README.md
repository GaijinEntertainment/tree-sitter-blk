# tree-sitter-blk

[![CI][ci]](https://github.com/GaijinEntertainment/tree-sitter-blk/actions/workflows/ci.yml)
[![crates][crates]](https://crates.io/crates/tree-sitter-blk)
[![npm][npm]](https://www.npmjs.com/package/tree-sitter-blk)
[![pypi][pypi]](https://pypi.org/project/tree-sitter-blk)

BLK grammar for [tree-sitter](https://github.com/tree-sitter/tree-sitter). BLK is the DataBlock text format of the
[Dagor Engine](https://github.com/GaijinEntertainment/DagorEngine).

The grammar follows the text parser of the engine and accepts the input of every loader mode, except the mode that
keeps comments as parameters (`DataBlock::parseCommentsAsParams`). It reads values as raw text and does not check a
value against the type of its parameter. Binary BLK files are not text, and the grammar does not read them.

## References

- [The BLK text parser](https://github.com/GaijinEntertainment/DagorEngine/blob/main/prog/engine/ioSys/dataBlock/blk_parser.cpp)
- [The DataBlock API](https://github.com/GaijinEntertainment/DagorEngine/blob/main/prog/dagorInclude/ioSys/dag_dataBlock.h)

[ci]: https://img.shields.io/github/actions/workflow/status/GaijinEntertainment/tree-sitter-blk/ci.yml?logo=github&label=CI
[crates]: https://img.shields.io/crates/v/tree-sitter-blk?logo=rust
[npm]: https://img.shields.io/npm/v/tree-sitter-blk?logo=npm
[pypi]: https://img.shields.io/pypi/v/tree-sitter-blk?logo=pypi&logoColor=ffd242
