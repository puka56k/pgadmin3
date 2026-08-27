//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pastedSQL.h - Clean SQL that was copied out of program source code
//
//////////////////////////////////////////////////////////////////////////

#ifndef PASTEDSQL_H
#define PASTEDSQL_H

#include <wx/string.h>

// Turn SQL that was copied from a source file - where it lives as a
// concatenated string literal - back into plain SQL:
//
//   String sql = "SELECT a, b "          SELECT a, b
//              + "  FROM t \n"      ->     FROM t
//              + "WHERE x = 1;";        WHERE x = 1;
//
// fragments returns the number of string literals that were unwrapped. Zero
// means the text did not look like source code at all and is returned
// unchanged - callers must then fall back to a plain paste.
wxString CleanPastedSQL(const wxString &src, int &fragments);

#endif // PASTEDSQL_H
