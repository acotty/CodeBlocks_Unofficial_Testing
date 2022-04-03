/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "cmd_result_parser.h"

#include "cmd_result_tokens.h"

namespace dbg_mi
{

void find_and_replace(wxString & source, const wxString & find, wxString const & replace)
{
    for (size_t j; (j = source.find(find)) != wxString::npos;)
    {
        source.replace(j, find.length(), replace);
    }
}
bool StripEnclosingQuotes(wxString & str)
{
    if (str.length() == 0)
    {
        return true;
    }

    if (str[0] == '"' && str[str.length() - 1] == '"')
    {
        if (str.length() >= 2 && str[str.length() - 2] == '\\')
        {
            return false;
        }

        str = str.substr(1, str.length() - 2);
    }
    else
        if (str[0] == '"')
        {
            return false;
        }
        else
            if (str[str.length() - 1] == '"')
            {
                if ((str.length() >= 2) && (str[str.length() - 2] != '\\'))
                {
                    return false;
                }
            }

    find_and_replace(str, "\\\"", "\"");
    return true;
}

bool ParseTuple(wxString const & str, int & start, ResultValue & tuple, bool want_closing_brace)
{
    Token token;
    int pos = start;
    ResultValue * curr_value = NULL;
    enum Step
    {
        Nothing,
        Name,
        Equal,
        Value
    };
    Step step = Nothing;

    while (pos < static_cast<int>(str.length()))
    {
        if (!GetNextToken(str, pos, token))
        {
            return false;
        }

        switch (token.type)
        {
            case Token::String:

                // set the value
                if (curr_value)
                {
                    if (step != Equal)
                    {
                        return false;
                    }

                    step = Value;
                    curr_value->SetType(ResultValue::Simple);
                    wxString value = token.ExtractString(str);

                    if (!StripEnclosingQuotes(value))
                    {
                        return false;
                    }

                    curr_value->SetSimpleValue(value);
                }
                else
                {
                    if (step != Nothing)
                    {
                        return false;
                    }

                    step = Name;
                    curr_value = new ResultValue;
                    curr_value->SetName(token.ExtractString(str));
                }

                break;

            case Token::Equal:
                if (!curr_value)
                {
                    return false;
                }

                if (step != Name)
                {
                    delete curr_value;
                    return false;
                }

                step = Equal;
                break;

            case Token::Comma:
                if (!curr_value)
                {
                    return false;
                }

                if (tuple.GetType() == ResultValue::Array)
                {
                    if (step == Name)
                    {
                        curr_value->SetType(ResultValue::Simple);
                        curr_value->SetSimpleValue(curr_value->GetName());
                        curr_value->SetName("");
                    }
                    else
                        if (step != Value)
                        {
                            delete curr_value;
                            return false;
                        }
                }
                else
                {
                    if (step != Value)
                    {
                        delete curr_value;
                        return false;
                    }
                }

                tuple.SetTupleValue(curr_value);
                curr_value = NULL;
                step = Nothing;
                break;

            case Token::TupleStart:
                if (!curr_value)
                {
                    if (tuple.GetType() != ResultValue::Array)
                    {
                        return false;
                    }
                    else
                    {
                        curr_value = new ResultValue;
                        step = Equal;
                    }
                }

                if (step != Equal)
                {
                    delete curr_value;
                    return false;
                }

                curr_value->SetType(ResultValue::Tuple);
                pos = token.end;

                if (!ParseTuple(str, pos, *curr_value, true))
                {
                    delete curr_value;
                    return false;
                }
                else
                {
                    token.end = pos;
                    token.type = Token::TupleEnd;
                    step = Value;
                }

                break;

            case Token::ListStart:
                if (!curr_value)
                {
                    return false;
                }

                if (step != Equal)
                {
                    delete curr_value;
                    return false;
                }

                curr_value->SetType(ResultValue::Array);
                pos = token.end;

                if (!ParseTuple(str, pos, *curr_value, true))
                {
                    delete curr_value;
                    return false;
                }
                else
                {
                    token.end = pos;
                    token.type = Token::ListEnd;
                    step = Value;
                }

                break;

            case Token::TupleEnd:
                if (!curr_value)
                {
                    return false;
                }

                if (tuple.GetType() != ResultValue::Tuple || !want_closing_brace)
                {
                    delete curr_value;
                    return false;
                }

                if (step != Value)
                {
                    delete curr_value;
                    return false;
                }

                start = pos + 1;
                tuple.SetTupleValue(curr_value);
                return true;

            case Token::ListEnd:

                //            if(!curr_value)
                //                return false;
                if (tuple.GetType() != ResultValue::Array || !want_closing_brace)
                {
                    delete curr_value;
                    return false;
                }

                if (step == Name)
                {
                    curr_value->SetType(ResultValue::Simple);
                    curr_value->SetSimpleValue(curr_value->GetName());
                    curr_value->SetName("");
                    tuple.SetTupleValue(curr_value);
                }
                else
                    if (step == Value)
                    {
                        tuple.SetTupleValue(curr_value);
                    }
                    else
                        if (step != Nothing || curr_value)
                        {
                            delete curr_value;
                            return false;
                        }

                start = pos + 1;
                return true;

            default:
                assert(false);
        }

        pos = token.end;
    }

    if (curr_value)
    {
        if (step != Value)
        {
            delete curr_value;
            return false;
        }
        else
        {
            tuple.SetTupleValue(curr_value);
        }
    }

    if (token.type == Token::Comma || token.type == Token::ListStart || token.type == Token::TupleStart)
    {
        return false;
    }

    // if we are here the closing brace was not found, so we exit with error
    if (want_closing_brace)
    {
        return false;
    }

    start = pos;
    return true;
}


bool ParseValue(wxString const & str, ResultValue & results, int start)
{
    results.SetType(ResultValue::Tuple);
    return ParseTuple(str, start, results, false);
}

void ResultValue::SetType(Type type)
{
    m_type = type;
}

void ResultValue::SetTupleValue(ResultValue * value)
{
    assert(value);
    m_value.tuple.push_back(value);
}

ResultValue const * ResultValue::GetTupleValue(wxString const & key) const
{
    assert(m_type == Tuple);
    wxString::size_type pos = key.find('.');

    if (pos != wxString::npos)
    {
        wxString const & subkey = key.substr(0, pos);
        Container::const_iterator it = FindTupleValue(subkey);

        if (it != m_value.tuple.end() && (*it)->GetType() == Tuple)
        {
            return (*it)->GetTupleValue(key.substr(pos + 1, key.length() - pos - 1));
        }
    }
    else
    {
        Container::const_iterator it = FindTupleValue(key);

        if (it != m_value.tuple.end())
        {
            return *it;
        }
    }

    return NULL;
}

ResultValue const * ResultValue::GetTupleValueByIndex(int index) const
{
    Container::const_iterator it = m_value.tuple.begin();
    std::advance(it, index);

    if (it != m_value.tuple.end())
    {
        return *it;
    }
    else
    {
        return NULL;
    }
}

wxString ResultValue::MakeDebugString() const
{
    switch (m_type)
    {
        case Simple:
            if (m_name.empty())
            {
                return m_value.simple;
            }
            else
            {
                return m_name + "=" + m_value.simple;
            }

            break;

        case Tuple:
        {
            wxString s;

            if (m_name.empty())
            {
                s = "{";
            }
            else
            {
                s = m_name + "={";
            }

            bool first = true;

            for (Container::const_iterator it = m_value.tuple.begin(); it != m_value.tuple.end(); ++it)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    s += ",";
                }

                s += (*it)->MakeDebugString();
            }

            s += "}";
            return s;
        }

        case Array:
        {
            wxString s;

            if (m_name.empty())
            {
                s = "[";
            }
            else
            {
                s = m_name + "=[";
            }

            bool first = true;

            for (Container::const_iterator it = m_value.tuple.begin(); it != m_value.tuple.end(); ++it)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    s += ",";
                }

                s += (*it)->MakeDebugString();
            }

            s += "]";
            return s;
        }

        default:
            return "not_initialized";
    }
}

bool ResultParser::Parse(wxString const & s)
{
    m_type = ParseType(s);
    m_class = ClassUnknown;
    wxString str = s.substr(1, s.length() - 1);

    if (str.length() == 0)
    {
        return false;
    }

    wxString::size_type after_class_index = 0;

    if (m_type == NotifyAsyncOutput)
    {
        after_class_index = str.find(',');

        if (after_class_index == wxString::npos)
        {
            m_async_type = str;
            return true;
        }
        else
        {
            m_async_type = str.substr(0, after_class_index);
        }
    }
    else
    {
        if (str.StartsWith("done"))
        {
            m_class = ClassDone;
            after_class_index = 4;
        }
        else
            if (str.StartsWith("stopped"))
            {
                m_class = ClassStopped;
                after_class_index = 7;
            }
            else
                if (str.StartsWith("running"))
                {
                    m_class = ClassRunning;
                    after_class_index = 7;
                }
                else
                    if (str.StartsWith("connected"))
                    {
                        m_class = ClassConnected;
                        after_class_index = 9;
                    }
                    else
                        if (str.StartsWith("error"))
                        {
                            m_class = ClassError;
                            after_class_index = 5;
                        }
                        else
                            if (str.StartsWith("exit"))
                            {
                                m_class = ClassExit;
                                after_class_index = 4;
                            }
                            else
                            {
                                return false;
                            }
    }

    if (str[after_class_index] == ',')
    {
        return ParseValue(str, m_value, after_class_index + 1);
    }
    else
        if (after_class_index != str.length())
        {
            return false;
        }
        else
        {
            return true;
        }
}

ResultParser::Type ResultParser::ParseType(wxString const & str)
{
    if (str.empty())
    {
        return TypeUnknown;
    }

    wxChar wxch = str[0];

    switch (wxch)
    {
        case '^': // result record
            return ResultParser::Result;

        case '*':
            return ResultParser::ExecAsyncOutput;

        case '+':
            return ResultParser::StatusAsyncOutput;

        case '=':
            return ResultParser::NotifyAsyncOutput;

        default:
            return TypeUnknown;
    }
}

wxString ResultParser::MakeDebugString() const
{
    wxString s("type: ");

    switch (m_type)
    {
        case Result:
            s += "result";
            break;

        case ExecAsyncOutput:
            s += "exec-async-ouput";
            break;

        case StatusAsyncOutput:
            s += "status-async-ouput";
            break;

        case NotifyAsyncOutput:
            s += "notify-async-ouput";
            break;

        default:
            s += "unknown";
    }

    switch (m_class)
    {
        case ClassDone:
            s += " , ClassDone";
            break;

        case ClassRunning:
            s += " , ClassRunning";
            break;

        case ClassConnected:
            s += " , ClassConnected";
            break;

        case ClassError:
            s += " , ClassError";
            break;

        case ClassExit:
            s += " , ClassExit";
            break;

        case ClassStopped:
            s += " , ClassStopped";
            break;

        default:
            s += " , class unknown";
    }

    wxString valueString = m_value.MakeDebugString();

    if (!valueString.empty())
    {
        s += "  , { m_value results: " + valueString + " }";
    }

    return s;
}

} // namespace dbg_mi
