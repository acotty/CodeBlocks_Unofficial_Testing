/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "cmd_result_tokens.h"

namespace dbg_mi
{


bool GetNextToken(wxString const & str, int pos, Token & token)
{
    while (pos < static_cast<int>(str.length()) && (str[pos] == _T(' ') || str[pos] == _T('\t')))
    {
        ++pos;
    }

    if (pos >= static_cast<int>(str.length()))
    {
        return false;
    }

    token.start = -1;
    bool in_quote = false;
    wxChar wxch = str[pos];

    switch (wxch)
    {
        case '=':
            token = Token(pos, pos + 1, Token::Equal);
            return true;

        case ',':
            token = Token(pos, pos + 1, Token::Comma);
            return true;

        case '[':
            token = Token(pos, pos + 1, Token::ListStart);
            return true;

        case ']':
            token = Token(pos, pos + 1, Token::ListEnd);
            return true;

        case '{':
            token = Token(pos, pos + 1, Token::TupleStart);
            return true;

        case '}':
            token = Token(pos, pos + 1, Token::TupleEnd);
            return true;

        case '"':
            in_quote = true;

        default:
            token.type = Token::String;
            token.start = pos;
    }

    ++pos;
    bool escape_next = false;

    while (pos < static_cast<int>(str.length()))
    {
        if ((str[pos] == ' ' || str[pos] == '\t' || str[pos] == ','
                || str[pos] == '=' || str[pos] == '{' || str[pos] == '}'
                || str[pos] == '[' || str[pos] == ']') && !in_quote)
        {
            token.end = pos;
            return true;
        }
        else
            if (str[pos] == '"' && in_quote && !escape_next)
            {
                token.end = pos + 1;
                return true;
            }
            else
                if (str[pos] == '\\')
                {
                    escape_next = true;
                }
                else
                {
                    escape_next = false;
                }

        ++pos;
    }

    if (in_quote)
    {
        token.end = -1;
        return false;
    }
    else
    {
        token.end = pos;
        return true;
    }
}


} // namespace dbg_mi

