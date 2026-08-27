//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pastedSQL.cpp - Clean SQL that was copied out of program source code
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers - keep this file free of pgAdmin headers so it can be
// compiled on its own by the unit tests.
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/wxcrt.h>

#include <vector>

// App headers
#include "utils/pastedSQL.h"

namespace
{

// Whatever sits around and between the string literals of a line is program
// code, not SQL. These are the only characters we ever expect to find there.
const wxString concatChars = wxT("+&");
const wxString suffixChars = wxT("+&.,;)");
const wxString prefixChars = wxT("+&.,=(:");


wxChar CharAt(const wxString &str, size_t pos)
{
	return (wxChar)str[pos].GetValue();
}


bool IsBlank(const wxString &str)
{
	return str.Strip(wxString::both).IsEmpty();
}


bool IsOnlyFrom(const wxString &str, const wxString &allowed)
{
	for (size_t i = 0; i < str.length(); i++)
	{
		wxChar c = CharAt(str, i);
		if (wxIsspace(c))
			continue;
		if (allowed.Find(c) == wxNOT_FOUND)
			return false;
	}
	return true;
}


// A literal may be preceded by a concat operator, an assignment, an opening
// bracket or a bare word (return "select 1";). Anything else - a couple of SQL
// keywords, for example - means we are not looking at source code at all.
bool PrefixLooksLikeCode(const wxString &prefix, bool &isAssignOrCall)
{
	isAssignOrCall = false;

	wxString str = prefix.Strip(wxString::both);
	if (str.IsEmpty())
		return true;

	wxChar last = CharAt(str, str.length() - 1);
	if (str.Find(wxT('=')) != wxNOT_FOUND || last == wxT('('))
		isAssignOrCall = true;

	if (prefixChars.Find(last) != wxNOT_FOUND)
		return true;

	for (size_t i = 0; i < str.length(); i++)
	{
		wxChar c = CharAt(str, i);
		if (!wxIsalnum(c) && c != wxT('_') && c != wxT('.'))
			return false;
	}
	return true;
}


// Returns false for an escape sequence we don't know - the caller then keeps it
// verbatim rather than guessing.
bool AppendEscaped(wxString &text, wxChar esc)
{
	switch (esc)
	{
		case wxT('n'):
			text += wxT('\n');
			return true;
		case wxT('r'):
			// dropped - the line ends we produce ourselves are enough
			return true;
		case wxT('t'):
			text += wxT('\t');
			return true;
		case wxT('"'):
			text += wxT('"');
			return true;
		case wxT('\''):
			text += wxT('\'');
			return true;
		case wxT('\\'):
			text += wxT('\\');
			return true;
	}
	return false;
}


struct fragmentLine
{
	fragmentLine() : isFragment(false), strong(false), segments(0) {}

	bool     isFragment;   // the line is one or more string literals plus code
	bool     strong;       // ... and it proves the text really is source code
	int      segments;     // number of literals unwrapped
	wxString text;         // their unescaped contents
};


fragmentLine ScanLine(const wxString &line)
{
	fragmentLine res;

	bool ok = true, sawConcat = false, sawEscape = false, isAssignOrCall = false;
	wxString outside, content;

	size_t i = 0, len = line.length();
	while (i < len)
	{
		if (CharAt(line, i) != wxT('"'))
		{
			outside += CharAt(line, i);
			i++;
			continue;
		}

		// the code in front of this literal
		if (res.segments == 0)
		{
			if (!PrefixLooksLikeCode(outside, isAssignOrCall))
				ok = false;
		}
		else if (!IsOnlyFrom(outside, concatChars))
			ok = false;

		if (outside.Find(wxT('+')) != wxNOT_FOUND || outside.Find(wxT('&')) != wxNOT_FOUND)
			sawConcat = true;
		outside.Clear();

		// the literal itself
		i++;
		bool closed = false;
		while (i < len)
		{
			wxChar c = CharAt(line, i);
			if (c == wxT('\\') && i + 1 < len)
			{
				wxChar esc = CharAt(line, i + 1);
				if (AppendEscaped(content, esc))
					sawEscape = true;
				else
				{
					content += c;
					content += esc;
				}
				i += 2;
				continue;
			}
			if (c == wxT('"'))
			{
				closed = true;
				i++;
				break;
			}
			content += c;
			i++;
		}

		if (!closed)
		{
			ok = false;
			break;
		}
		res.segments++;
	}

	if (!ok || res.segments == 0)
		return res;

	// whatever follows the last literal - a concat operator, a bracket, the
	// statement terminator
	if (!IsOnlyFrom(outside, suffixChars))
		return res;
	if (outside.Find(wxT('+')) != wxNOT_FOUND || outside.Find(wxT('&')) != wxNOT_FOUND)
		sawConcat = true;

	res.isFragment = true;
	res.text = content;

	// Plain SQL holds "quoted identifiers" too, so being a well formed literal
	// is not enough: only a concat operator, an escape sequence, or a complete
	// assignment/call statement proves this is really program source.
	res.strong = sawConcat || sawEscape ||
	             (isAssignOrCall && outside.Find(wxT(';')) != wxNOT_FOUND &&
	              content.Find(wxT(' ')) != wxNOT_FOUND);
	return res;
}


// "SELECT a \n \n" -> "SELECT a ": the empty lines an escaped line end leaves
// behind are noise, we add a line end per fragment ourselves.
wxString StripTrailingBlankLines(const wxString &text)
{
	wxString res = text;
	for (;;)
	{
		int pos = res.Find(wxT('\n'), true);
		if (pos == wxNOT_FOUND || !IsBlank(res.Mid(pos + 1)))
			break;
		res = res.Left(pos);
	}
	return res;
}


void SplitLines(const wxString &src, wxArrayString &lines)
{
	size_t start = 0;
	for (size_t i = 0; i <= src.length(); i++)
	{
		if (i != src.length() && CharAt(src, i) != wxT('\n'))
			continue;

		wxString line = src.Mid(start, i - start);
		if (!line.IsEmpty() && CharAt(line, line.length() - 1) == wxT('\r'))
			line = line.Left(line.length() - 1);
		lines.Add(line);
		start = i + 1;
	}
}

} // namespace


wxString CleanPastedSQL(const wxString &src, int &fragments)
{
	fragments = 0;
	if (src.IsEmpty())
		return src;

	wxArrayString lines;
	SplitLines(src, lines);

	std::vector<fragmentLine> scan;
	int nonEmpty = 0, fragLines = 0, segments = 0;
	bool strong = false;

	for (size_t i = 0; i < lines.GetCount(); i++)
	{
		fragmentLine res = ScanLine(lines[i]);
		if (!IsBlank(lines[i]))
			nonEmpty++;
		if (res.isFragment)
		{
			fragLines++;
			segments += res.segments;
			if (res.strong)
				strong = true;
		}
		scan.push_back(res);
	}

	// Not source code, or not enough of it to be sure - hands off.
	if (!strong || !fragLines || !nonEmpty)
		return src;
	if (fragLines * 100 < nonEmpty * 60)
		return src;

	wxString sql;
	bool first = true;
	for (size_t i = 0; i < lines.GetCount(); i++)
	{
		wxString line = scan[i].isFragment ? StripTrailingBlankLines(scan[i].text) : lines[i];
		if (IsBlank(line))
			continue;

		if (!first)
			sql += wxT("\n");
		sql += line;
		first = false;
	}

	if (sql.IsEmpty())
		return src;

	fragments = segments;
	return sql;
}
