package tree_sitter_blk_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_blk "github.com/GaijinEntertainment/tree-sitter-blk/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	parser := tree_sitter.NewParser()
	defer parser.Close()

	if err := parser.SetLanguage(tree_sitter.NewLanguage(tree_sitter_blk.Language())); err != nil {
		t.Errorf("Error loading BLK grammar: %v", err)
	}
}
