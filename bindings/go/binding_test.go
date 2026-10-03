package tree_sitter_blk_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_blk "github.com/GaijinEntertainment/tree-sitter-blk/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_blk.Language())
	if language == nil {
		t.Errorf("Error loading BLK grammar")
	}
}
