import XCTest
import SwiftTreeSitter
import TreeSitterBlk

final class TreeSitterBlkTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_blk())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading BLK grammar")
    }
}
