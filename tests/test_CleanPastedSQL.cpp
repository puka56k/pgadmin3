#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include "utils/pastedSQL.h"

TEST_CASE( "CleanPastedSQL unwraps source code", "[pastedsql]" ) {
	int frags = 0;

SECTION( "Java string concatenation" ) {
	wxString src =
	    "String sql = \"SELECT a, b \"\n"
	    "           + \"  FROM t \\n\"\n"
	    "           + \"WHERE x = 1;\";";
	CHECK(CleanPastedSQL(src, frags) == "SELECT a, b \n  FROM t \nWHERE x = 1;");
	CHECK(frags == 3);
}

SECTION( "trailing escaped line ends are dropped" ) {
	wxString src =
	    "\"SELECT 1 \" +\n"
	    "\"   \\n \\n\" +\n"
	    "\"FROM t\"";
	// the whitespace-only fragment leaves nothing behind at all
	CHECK(CleanPastedSQL(src, frags) == "SELECT 1 \nFROM t");
	CHECK(frags == 3);
}

SECTION( "one line assignment" ) {
	CHECK(CleanPastedSQL("String sql = \"SELECT 1\";", frags) == "SELECT 1");
	CHECK(frags == 1);
}

SECTION( "append calls" ) {
	wxString src =
	    "sb.append(\"SELECT a\\n\");\n"
	    "sb.append(\"FROM t\");\n";
	CHECK(CleanPastedSQL(src, frags) == "SELECT a\nFROM t");
	CHECK(frags == 2);
}

SECTION( "two literals on one line" ) {
	CHECK(CleanPastedSQL("q = \"SELECT \" + \"1\";", frags) == "SELECT 1");
	CHECK(frags == 2);
}

SECTION( "escape sequences" ) {
	CHECK(CleanPastedSQL("\"a\\tb\" + \"\\\\x\" + \"\\\"q\\\"\" + \"\\qz\"", frags)
	      == "a\tb\\x\"q\"\\qz");
	CHECK(frags == 4);
}
}

TEST_CASE( "CleanPastedSQL leaves plain SQL alone", "[pastedsql]" ) {
	int frags = 0;

SECTION( "plain SQL is untouched" ) {
	wxString src = "SELECT a, b   \nFROM t\nWHERE x = 1;";
	CHECK(CleanPastedSQL(src, frags) == src);
	CHECK(frags == 0);
}

SECTION( "quoted identifiers are untouched" ) {
	wxString src = "SELECT \"Col\", \"Other\",\n  count(*)\nFROM \"MyTable\";";
	CHECK(CleanPastedSQL(src, frags) == src);
	CHECK(frags == 0);
}

SECTION( "quoted identifier in a comparison is untouched" ) {
	wxString src = "SELECT * FROM t WHERE a = \"b\";";
	CHECK(CleanPastedSQL(src, frags) == src);
	CHECK(frags == 0);
}

SECTION( "empty text" ) {
	CHECK(CleanPastedSQL("", frags) == "");
	CHECK(frags == 0);
}

SECTION( "an unterminated literal is not source code" ) {
	wxString src = "SELECT \"a + b\n";
	CHECK(CleanPastedSQL(src, frags) == src);
	CHECK(frags == 0);
}
}
