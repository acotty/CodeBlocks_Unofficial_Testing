/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 */

#include <sdk.h>

#ifndef CB_PRECOMP
    #include <cctype>
    #include <queue>

    #include <wx/app.h>
    #include <wx/msgdlg.h>

    #include <cbexception.h>
    #include <globals.h>
    #include <logmanager.h>
    #include <manager.h>
#endif

#include <wx/tokenzr.h>

#include "editormanager.h"
#include "cbproject.h"

#include "LSP_symbolsparser.h"
#include "parser.h"
#include "expression.h" // used to calculate the enumerator value

#define CC_LSP_SymbolsParser_DEBUG_OUTPUT 0 //(ph 2021/04/5)

#if defined(CC_GLOBAL_DEBUG_OUTPUT)
    #if CC_GLOBAL_DEBUG_OUTPUT == 1
        #undef CC_LSP_SymbolsParser_DEBUG_OUTPUT
        #define CC_LSP_SymbolsParser_DEBUG_OUTPUT 1
    #elif CC_GLOBAL_DEBUG_OUTPUT == 2
        #undef CC_LSP_SymbolsParser_DEBUG_OUTPUT
        #define CC_LSP_SymbolsParser_DEBUG_OUTPUT 2
    #endif
#endif

#ifdef CC_PARSER_TEST
#define ADDTOKEN(format, args...) \
    CCLogger::Get()->AddToken(F(format, ##args))
#define TRACE(format, args...) \
    CCLogger::Get()->DebugLog(wxString::Format(format, ##args))
#define TRACE2(format, args...) \
    CCLogger::Get()->DebugLog(wxString::Format(format, ##args))
#else
#if CC_LSP_SymbolsParser_DEBUG_OUTPUT == 1
#define ADDTOKEN(format, args...) \
    CCLogger::Get()->AddToken(F(format, ##args))
#define TRACE(format, args...) \
    CCLogger::Get()->DebugLog(wxString::Format(format, ##args))
#define TRACE2(format, args...)
#elif CC_LSP_SymbolsParser_DEBUG_OUTPUT == 2
#define ADDTOKEN(format, args...) \
    CCLogger::Get()->AddToken(F(format, ##args))
#define TRACE(format, args...)                                              \
    do                                                                      \
    {                                                                       \
        if (g_EnableDebugTrace)                                             \
            CCLogger::Get()->DebugLog(wxString::Format(format, ##args));                   \
    }                                                                       \
    while (false)
#define TRACE2(format, args...) \
    CCLogger::Get()->DebugLog(wxString::Format(format, ##args))
#else
#define ADDTOKEN(format, args...)
#define TRACE(format, args...)
#define TRACE2(format, args...)
#endif
#endif

//#define CC_LSP_SymbolsParser_TESTDESTROY 0
//
//#if CC_LSP_SymbolsParser_TESTDESTROY
//#define IS_ALIVE IsStillAlive(wxString(__PRETTY_FUNCTION__, wxConvUTF8))
//#else
//#define IS_ALIVE !TestDestroy()
//#endif

#define IS_ALIVE true //(ph 2021/03/15) //(ph 2021/03/23)
namespace
{
bool wxFound(int result)
{
    return result != wxNOT_FOUND;
};
const char STX = '\u0002';
// ----------------------------------------------------------------------------
// Language Server symbol kind.
// ----------------------------------------------------------------------------
// defined in https://microsoft.github.io/language-server-protocol/specification
enum LSP_DocumentSymbolKind
{
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};

}
const wxString g_UnnamedSymbol = _T("__Unnamed");

namespace ParserConsts
{
// length: 0
const wxString empty(_T(""));
const wxChar   null(_T('\0'));
// length: 1
const wxChar   eol_chr(_T('\n'));
const wxString space(_T(" "));
const wxChar   space_chr(_T(' '));
const wxChar   tab_chr(_T('\t'));
const wxString equals(_T("="));
const wxChar   equals_chr(_T('='));
const wxString hash(_T("#"));
const wxChar   hash_chr(_T('#'));
const wxString plus(_T("+"));
const wxChar   plus_chr(_T('+'));
const wxString dash(_T("-"));
const wxChar   dash_chr(_T('-'));
const wxString ptr(_T("*"));
const wxChar   ptr_chr(_T('*'));
const wxString ref(_T("&"));
const wxChar   ref_chr(_T('&'));
const wxString comma(_T(","));
const wxChar   comma_chr(_T(','));
const wxString dot(_T("."));
const wxChar   dot_chr(_T('.'));
const wxString colon(_T(":"));
const wxChar   colon_chr(_T(':'));
const wxString semicolon(_T(";"));
const wxChar   semicolon_chr(_T(';'));
const wxChar   opbracket_chr(_T('('));
const wxChar   clbracket_chr(_T(')'));
const wxString opbracket(_T("("));
const wxString clbracket(_T(")"));
const wxString opbrace(_T("{"));
const wxChar   opbrace_chr(_T('{'));
const wxString clbrace(_T("}"));
const wxChar   clbrace_chr(_T('}'));
const wxString oparray(_T("["));
const wxChar   oparray_chr(_T('['));
const wxString clarray(_T("]"));
const wxChar   clarray_chr(_T(']'));
const wxString tilde(_T("~"));
const wxString lt(_T("<"));
const wxChar   lt_chr(_T('<'));
const wxString gt(_T(">"));
const wxChar   gt_chr(_T('>'));
const wxChar   underscore_chr(_T('_'));
const wxChar   question_chr(_T('?'));
// length: 2
const wxString dcolon(_T("::"));
const wxString opbracesemicolon(_T("{;"));
const wxString commaclbrace(_T(",}"));
const wxString semicolonopbrace(_T(";{"));
const wxString semicolonclbrace(_T(";}"));
const wxString gtsemicolon(_T(">;"));
const wxString quot(_T("\""));
const wxString kw_do(_T("do"));
const wxString kw_if(_T("if"));
// length: 3
const wxString spaced_colon(_T(" : "));
const wxString kw__C_(_T("\"C\""));
const wxString kw_for(_T("for"));
const wxString kw_try(_T("try"));
const wxString commasemicolonopbrace(_T(",;{"));
// length: 4
const wxString kw___at(_T("__at"));
const wxString kw_else(_T("else"));
const wxString kw_enum(_T("enum"));
const wxString kw_elif(_T("elif"));
const wxString kw_case(_T("case"));
// length: 5
const wxString kw__CPP_(_T("\"C++\""));
const wxString kw___asm(_T("__asm"));
const wxString kw_catch(_T("catch"));
const wxString kw_class(_T("class"));
const wxString kw_const(_T("const"));
const wxString kw_union(_T("union"));
const wxString kw_using(_T("using"));
const wxString kw_throw(_T("throw"));
const wxString kw_while(_T("while"));
// length: 6
const wxString kw_delete(_T("delete"));
const wxString kw_extern(_T("extern"));
const wxString kw_friend(_T("friend"));
const wxString kw_inline(_T("inline"));
const wxString kw_public(_T("public"));
const wxString kw_return(_T("return"));
const wxString kw_static(_T("static"));
const wxString kw_struct(_T("struct"));
const wxString kw_switch(_T("switch"));
// length: 7
const wxString kw_include(_T("include"));
const wxString kw_private(_T("private"));
const wxString kw_typedef(_T("typedef"));
const wxString kw_virtual(_T("virtual"));
// length: 8
const wxString kw_noexcept(_T("noexcept"));
const wxString kw_operator(_T("operator"));
const wxString kw_template(_T("template"));
const wxString kw_typename(_T("typename"));
const wxString kw_volatile(_T("volatile"));
// length: 9
const wxString kw_namespace(_T("namespace"));
const wxString kw_protected(_T("protected"));
// length: 10
const wxString kw_declspec(_T("__declspec"));
// length: 13
const wxString kw_attribute(_T("__attribute__"));
}

// ----------------------------------------------------------------------------
LSP_SymbolsParser::LSP_SymbolsParser(ParserBase     *     parent,
                                     const wxString   &   bufferOrFilename,
                                     bool                 isLocal,
                                     LSP_SymbolsParserOptions & LSP_SymbolsParserOptions,
                                     TokenTree      *     tokenTree)
// ----------------------------------------------------------------------------
    : m_Tokenizer(tokenTree),
      m_Parent(parent),
      m_TokenTree(tokenTree),
      m_LastParent(0),
      m_LastScope(tsUndefined),
      m_FileSize(0),
      m_FileIdx(0),
      m_IsLocal(isLocal),
      m_Options(LSP_SymbolsParserOptions),
      m_ParsingTypedef(false),
      m_Buffer(bufferOrFilename),
      m_StructUnionUnnamedCount(0),
      m_EnumUnnamedCount(0),
      m_SemanticTokensVec(m_Tokenizer.m_SemanticTokensVec) //(ph 2021/03/22)
{
    m_Tokenizer.SetTokenizerOption(LSP_SymbolsParserOptions.wantPreprocessor,
                                   LSP_SymbolsParserOptions.storeDocumentation);

    if (!m_TokenTree)
    {
        cbThrow(_T("m_TokenTree is a nullptr?!"));
    }
}

// ----------------------------------------------------------------------------
LSP_SymbolsParser::~LSP_SymbolsParser()
// ----------------------------------------------------------------------------
{
    // wait for file loader object to complete (can't abort it)
    if (m_Options.loader)
    {
        m_Options.loader->Sync();
        delete m_Options.loader;
    }
}

wxChar LSP_SymbolsParser::SkipToOneOfChars(const wxString & chars, bool supportNesting, bool singleCharToken)
{
    unsigned int level = m_Tokenizer.GetNestingLevel();

    while (IS_ALIVE)
    {
        wxString token = m_Tokenizer.GetToken(); // grab next token...

        if (token.IsEmpty())
        {
            return ParserConsts::null;    // eof
        }

        // if supportNesting==true, we only do a match in the same brace/nesting level,
        // thus we preserve the brace level when the function returned. But if
        // supportNesting==false, we do not consider the brace level on matching.
        if (!supportNesting || m_Tokenizer.GetNestingLevel() == level)
        {
            // only consider tokens of length one, if requested
            if (singleCharToken && token.length() > 1)
            {
                continue;
            }

            wxChar ch = token.GetChar(0);

            if (chars.Find(ch) != wxNOT_FOUND) // match one char
            {
                return ch;
            }
        }
    }

    return ParserConsts::null; // not found
}

void LSP_SymbolsParser::SkipBlock()
{
    // need to force the tokenizer _not_ skip anything
    // or else default values for template params would cause us to miss everything (because of the '=' symbol)
    TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);
    // skip tokens until we reach }
    // block nesting is taken into consideration too ;)
    // this is the nesting level we start at
    // we subtract 1 because we 're already inside the block
    // (since we 've read the {)
    unsigned int level = m_Tokenizer.GetNestingLevel() - 1;

    while (IS_ALIVE)
    {
        wxString token = m_Tokenizer.GetToken();

        if (token.IsEmpty())
        {
            break;    // eof
        }

        // if we reach the initial nesting level, we are done
        if (level == m_Tokenizer.GetNestingLevel())
        {
            break;
        }
    }

    // reset tokenizer's functionality
    m_Tokenizer.SetState(oldState);
}

void LSP_SymbolsParser::SkipAngleBraces()
{
    // need to force the tokenizer _not_ skip anything
    // or else default values for template params would cause us to miss everything (because of the '=' symbol)
    TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);
    int nestLvl = 0;

    // NOTE: only exit this loop with 'break' so the tokenizer's state can
    // be reset afterwards (i.e. don't use 'return')
    while (IS_ALIVE)
    {
        wxString tmp = m_Tokenizer.GetToken();

        if (tmp == ParserConsts::lt)
        {
            ++nestLvl;
        }
        else
            if (tmp == ParserConsts::gt)
            {
                --nestLvl;
            }
            else
                if (tmp == ParserConsts::semicolon)
                {
                    // unget token - leave ; on the stack
                    m_Tokenizer.UngetToken();
                    break;
                }
                else
                    if (tmp.IsEmpty())
                    {
                        break;
                    }

        if (nestLvl <= 0)
        {
            break;
        }
    }

    // reset tokenizer's functionality
    m_Tokenizer.SetState(oldState);
}
// ----------------------------------------------------------------------------
bool LSP_SymbolsParser::ParseBufferForNamespaces(const wxString & buffer, NameSpacesVec & result)
// ----------------------------------------------------------------------------
{
    int startTime = 0;
    RECORD_TIME(startTime);  //(ph 2021/09/7)
    m_Tokenizer.InitFromBuffer(buffer);

    if (!m_Tokenizer.IsOK())
    {
        return false;
    }

    result.clear();
    wxArrayString nsStack;
    m_Tokenizer.SetState(tsNormal);
    m_ParsingTypedef = false;

    while (m_Tokenizer.NotEOF() && IS_ALIVE)
    {
        CHECK_TIME(startTime, 1000); //(ph 2021/09/7)
        wxString token = m_Tokenizer.GetToken();

        if (token.IsEmpty())
        {
            continue;
        }

        if (token == ParserConsts::kw_using)
        {
            SkipToOneOfChars(ParserConsts::semicolonclbrace);
        }
        else
            if (token == ParserConsts::opbrace)
            {
                SkipBlock();
            }
            else
                if (token == ParserConsts::kw_namespace)
                {
                    wxString name = m_Tokenizer.GetToken();

                    if (name == ParserConsts::opbrace)
                    {
                        name = wxEmptyString;    // anonymous namespace
                    }
                    else
                    {
                        wxString next = m_Tokenizer.PeekToken();

                        if (next == ParserConsts::equals)
                        {
                            SkipToOneOfChars(ParserConsts::semicolonclbrace);
                            continue;
                        }
                        else
                            if (next == ParserConsts::opbrace)
                            {
                                m_Tokenizer.GetToken();
                                name += ParserConsts::dcolon;
                            }
                    }

                    nsStack.Add(name);
                    NameSpaces ns;

                    for (size_t i = 0; i < nsStack.Count(); ++i)
                    {
                        ns.Name << nsStack[i];
                    }

                    ns.StartLine = m_Tokenizer.GetLineNumber() - 1;
                    ns.EndLine = -1;
                    result.push_back(ns);
                }
                else
                    if (token == ParserConsts::clbrace)
                    {
                        NameSpacesVec::reverse_iterator it;

                        for (it = result.rbegin(); it != result.rend(); ++it)
                        {
                            NameSpaces & ns = *it;

                            if (ns.EndLine == -1)
                            {
                                ns.EndLine = m_Tokenizer.GetLineNumber() - 1;
                                break;
                            }
                        }

                        if (!nsStack.IsEmpty())
                        {
                            nsStack.RemoveAt(nsStack.GetCount() - 1);
                        }
                    }
    }

    return true;
}
// ----------------------------------------------------------------------------
bool LSP_SymbolsParser::ParseBufferForUsingNamespace(const wxString & buffer, wxArrayString & result)
// ----------------------------------------------------------------------------
{
    m_Tokenizer.InitFromBuffer(buffer);

    if (!m_Tokenizer.IsOK())
    {
        return false;
    }

    result.Clear();
    m_Str.Clear();
    m_LastUnnamedTokenName.Clear();
    m_ParsingTypedef = false;

    // Notice: clears the queue "m_EncounteredTypeNamespaces"
    while (!m_EncounteredTypeNamespaces.empty())
    {
        m_EncounteredTypeNamespaces.pop();
    }

    // Notice: clears the queue "m_EncounteredNamespaces"
    while (!m_EncounteredNamespaces.empty())
    {
        m_EncounteredNamespaces.pop();
    }

    while (m_Tokenizer.NotEOF() && IS_ALIVE)
    {
        wxString token = m_Tokenizer.GetToken();

        if (token.IsEmpty())
        {
            continue;
        }

        if (token == ParserConsts::kw_namespace)
        {
            // need this too
            token = m_Tokenizer.GetToken();
            SkipToOneOfChars(ParserConsts::opbrace);

            if (!token.IsEmpty())
            {
                result.Add(token);
            }
        }
        else
            if (token == ParserConsts::opbrace && m_Options.bufferSkipBlocks)
            {
                SkipBlock();
            }
            else
                if (token == ParserConsts::kw_using)
                {
                    // there are some kinds of using keyword usage
                    // (1) using namespace A;
                    // (2) using namespace A::B; // where B is a namespace
                    // (3) using A::B;           // where B is NOT a namespace
                    // (4) using A = B;          // doesn't import anything, so we don't handle this here
                    token = m_Tokenizer.GetToken();
                    wxString peek = m_Tokenizer.PeekToken();

                    if (token == ParserConsts::kw_namespace || peek == ParserConsts::dcolon)
                    {
                        if (peek == ParserConsts::dcolon) // using declaration, such as case (3)
                        {
                            m_Str << token;    // push the A to the m_Str(type stack)
                        }
                        else //handling the case (1) and (2)
                        {
                            // using directive
                            while (IS_ALIVE) // support full namespaces
                            {
                                m_Str << m_Tokenizer.GetToken();

                                if (m_Tokenizer.PeekToken() == ParserConsts::dcolon)
                                {
                                    m_Str << m_Tokenizer.GetToken();
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }

                        // m_Str must end with a namespace for CC to work
                        // now, m_Str contains "A" in case (1) and (3), and "A::B" in case (2)
                        if (!m_Str.IsEmpty())
                        {
                            result.Add(m_Str);
                        }

                        m_Str.Clear();
                    }
                    else
                    {
                        SkipToOneOfChars(ParserConsts::semicolonclbrace);
                    }
                }
    }

    return true;
}
// ----------------------------------------------------------------------------
bool LSP_SymbolsParser::InitTokenizer(json * pJson)
// ----------------------------------------------------------------------------
{
    bool ret = false;

    if (m_Buffer.empty())
    {
        TRACE(_T("InitTokenizer() : Buffer is empty."));
        return false;
    }

    if (!m_Buffer.IsEmpty())
    {
        if (not m_Options.useBuffer)
        {
            if (wxFileExists(m_Buffer))
            {
                wxFile file(m_Buffer);

                if (file.IsOpened())
                {
                    m_Filename = m_Buffer;
                    m_FileSize = file.Length();
                    TRACE(_T("InitTokenizer() : m_Filename='%s', m_FileSize=%u."), m_Filename.wx_str(), m_FileSize);
                    ret = m_Tokenizer.Init(m_Filename, m_Options.loader);

                    // must delete the loader, since it was allocated by SDK's Load() function
                    if (m_Options.loader)       //(ph 2021/03/19)
                    {
                        Delete(m_Options.loader);
                    }

                    if (!ret)
                    {
                        TRACE(_T("InitTokenizer() : Could not initialise tokenizer for file '%s'."), m_Filename.wx_str());
                        return ret;
                    }
                }
            }
            else //file does not exist
            {
                TRACE(_T("InitTokenizer() : Could not open file: '%s'."), m_Buffer.wx_str());
                return ret = false;
            }
        }
        else
        {
            // record filename for buffer parsing
            m_Filename = m_Options.fileOfBuffer;
            m_FileIdx  = m_TokenTree->InsertFileOrGetIndex(m_Filename);
            ret = m_Tokenizer.InitFromBuffer(m_Buffer, m_Filename, m_Options.initLineOfBuffer);

            if (not ret)
            {
                return ret;
            }
        }
    }//endif buffer not empty

    // move semantic legends to tokenizer
    m_Tokenizer.m_SemanticTokensTypes = m_SemanticTokensTypes;
    m_Tokenizer.m_SemanticTokensModifiers = m_SemanticTokensModifiers;
    // convert LSP semantic tokens and symbols results to usable data
    wxString jsonIDfield = pJson->at("id").get<std::string>();
    bool converted = false;

    if (jsonIDfield.StartsWith("textDocument/semanticTokens/full"))
    {
        converted = m_Tokenizer.LSP_ConvertSemanticTokens(pJson);
    }

    if (jsonIDfield.StartsWith("textDocument/documentSymbol"))
    {
        return ret;
    }

    // convenience variables
    m_SemanticTokensVec = m_Tokenizer.GetSemanticTokensVec(); //a reference
    m_SemanticTokensVecSize = m_SemanticTokensVec.size();
    m_SemanticTokensIdx = 0;
    //-testing-    size_t typeCnt = m_Tokenizer.m_SemanticTokensTypes.size();
    return ret and converted;
}
// ----------------------------------------------------------------------------
bool LSP_SymbolsParser::Parse(json * pJson, cbProject * pProject)
// ----------------------------------------------------------------------------
{
    m_pJson = pJson;

    // FIXME (ph#): get rid of the IS_ALIVE requirement, its always true.
    //if (!IS_ALIVE || !InitTokenizer())
    if (!IS_ALIVE || (not InitTokenizer(pJson)))
    {
        return false;
    }

    wxString parseIDstr;

    try
    {
        parseIDstr = pJson->at("id").get<std::string>();
    }
    catch (std::exception & e)
    {
        cbMessageBox(wxString::Format("%s() %s", __FUNCTION__, e.what()));
        return false;
    }

    if (parseIDstr.Contains("/SemanticToken"))
    {
        if (m_SemanticTokensVecSize == 0)   //(ph 2021/03/22)
        {
            wxString msg = "%s() failed to find any LSP semantic tokens., __FUNCTION__";
            cbMessageBox(msg, "Parse Failed");
            return false;
        }
    }

    TRACE(_T("Parse() : Parsing '%s'"), m_Filename.wx_str());
    bool result      = false;
    m_ParsingTypedef = false;

    do
    {
        if (!m_TokenTree || !m_Tokenizer.IsOK())
        {
            break;
        }

        if (not m_Options.useBuffer) // Parse a file
        {
            // the second parameter of ReserveFileForParsing() is false, so set it to fpsBeingParsed
            m_FileIdx = m_TokenTree->ReserveFileForParsing(m_Filename);

            if (!m_FileIdx)
            {
                break;
            }
        }

        if (parseIDstr.Contains("/documentSymbol"))
        {
            DoParseDocumentSymbols(pJson, pProject);
        }
        else
        {
            DoParseSemanticTokens();
        }

        if (!m_Options.useBuffer) // Parsing a file
        {
            m_TokenTree->FlagFileAsParsed(m_Filename);
        }

        result = true;
    } while (false);

    return result;
}
// ----------------------------------------------------------------------------
bool LSP_SymbolsParser::DoParseSemanticTokens()
// ----------------------------------------------------------------------------
{
    /// FIXME (ph#): This routine is not yet setting m_LastParent ala DoParseDocumentSymbols()
    // need to reset tokenizer's behaviour
    // don't forget to reset it if you add any early exit condition!
    TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);
    m_Str.Clear();
    m_LastToken.Clear();
    m_LastUnnamedTokenName.Clear();

    // Notice: clears the queue "m_EncounteredTypeNamespaces"
    while (!m_EncounteredTypeNamespaces.empty())
    {
        m_EncounteredTypeNamespaces.pop();
    }

    // Notice: clears the queue "m_EncounteredNamespaces"
    while (!m_EncounteredNamespaces.empty())
    {
        m_EncounteredNamespaces.pop();
    }

    while (m_Tokenizer.NotEOF() && IS_ALIVE)
    {
        wxString token = m_Tokenizer.GetToken();

        if (token.IsEmpty())
            //-continue;     //(ph 2021/03/22)
        {
            break;    //LSP    //(ph 2021/03/22)
        }

        // info                                     //(ph 2021/03/22)
        //m_Tokenizer.m_SemanticTokenIndex      == m_SemanticTokensIdx;
        //m_Tokenizer.m_SemanticTokenLineNumber == lineNum;
        //m_Tokenizer.m_SemanticTokenColumn     == colNum;
        //m_Tokenizer.m_SemanticTokenLength     == txtLen;
        //m_Tokenizer.m_SemanticTokenType       == type;
        //m_Tokenizer.m_SemanticTokenModifier   == modifier;
        TokenKind ccTokenKind = tkUndefined;
        wxString  args = wxString();
        /// FIXME (ph#): the following ccTokenKind(s) may not be correct //(ph 2021/03/22)
        size_t lspTokenTypeIdx = m_Tokenizer.m_SemanticTokenType;
        wxString lspTokenType = m_Tokenizer.m_SemanticTokensTypes[lspTokenTypeIdx];

        if (lspTokenType == "type")
        {
            ccTokenKind = tkTypedef;
        }
        else
            if (lspTokenType == "class")
            {
                ccTokenKind = tkClass;
            }
            else
                if (lspTokenType == "enum")
                {
                    ccTokenKind = tkEnum;
                }
                else
                    if (lspTokenType == "interface")
                    {
                        ccTokenKind = tkMacroDef;
                    }
                    else
                        if (lspTokenType == "struct")
                        {
                            ccTokenKind = tkMacroDef;
                        }
                        else
                            if (lspTokenType == "typeParameter")
                            {
                                ccTokenKind = tkTypedef;
                            }
                            else
                                if (lspTokenType == "parameter")
                                {
                                    ccTokenKind = tkVariable;
                                }
                                else
                                    if (lspTokenType == "variable")
                                    {
                                        ccTokenKind = tkVariable;
                                    }
                                    else
                                        if (lspTokenType == "property")
                                        {
                                            ccTokenKind = tkVariable;
                                        }
                                        else
                                            if (lspTokenType == "event")
                                            {
                                                ccTokenKind = tkUndefined;
                                            }
                                            else
                                                if (lspTokenType == "function")
                                                {
                                                    ccTokenKind = tkFunction;
                                                    args = DoHandleSemanticTokenFunction();
                                                }
                                                else
                                                    if (lspTokenType == "method")
                                                    {
                                                        ccTokenKind = tkFunction;
                                                    }
                                                    else
                                                        if (lspTokenType == "macro")
                                                        {
                                                            ccTokenKind = tkMacroDef;
                                                        }
                                                        else
                                                            if (lspTokenType == "keyword")
                                                            {
                                                                ccTokenKind = tkUndefined;
                                                            }
                                                            else
                                                                if (lspTokenType == "modifier")
                                                                {
                                                                    ccTokenKind = tkTypedef;
                                                                }
                                                                else
                                                                    if (lspTokenType == "comment")
                                                                    {
                                                                        ccTokenKind = tkUndefined;
                                                                    }
                                                                    else
                                                                        if (lspTokenType == "string")
                                                                        {
                                                                            ccTokenKind = tkVariable;
                                                                        }
                                                                        else
                                                                            if (lspTokenType == "number")
                                                                            {
                                                                                ccTokenKind = tkVariable;
                                                                            }
                                                                            else
                                                                                if (lspTokenType == "regexp")
                                                                                {
                                                                                    ccTokenKind = tkMacroDef;
                                                                                }
                                                                                else
                                                                                    if (lspTokenType == "operator")
                                                                                    {
                                                                                        ccTokenKind = tkUndefined;
                                                                                    }

        //Token* DoAddToken(TokenKind       kind,
        //                  const wxString& name,
        //                  int             line,
        //                  int             implLineStart = 0,
        //                  int             implLineEnd = 0,
        //                  const wxString& args = wxEmptyString,
        //                  bool            isOperator = false,
        //                  bool            isImpl = false);
        Token * newToken = DoAddToken(ccTokenKind, token,
                                      m_Tokenizer.m_SemanticTokenLineNumber,
                                      0, 0, // FIXME (ph#):For semanticToken, any access to Impl start/end line?
                                      args,
                                      (lspTokenType == "operator"),          //isOperator
                                      ((ccTokenKind & tkAnyFunction) != 0)   //isImpl
                                     );
        wxUnusedVar(newToken);
    }//end while

    // reset tokenizer behaviour
    m_Tokenizer.SetState(oldState);
    return true;
}
// ----------------------------------------------------------------------------
bool LSP_SymbolsParser::DoParseDocumentSymbols(json * pJson, cbProject * pProject) //(ph 2021/03/15)
// ----------------------------------------------------------------------------
{
    /// Do Not free pJson, it will be freed in CodeCompletion::LSP_Event()
    int startTime = 0;
    RECORD_TIME(startTime) //(ph 2021/09/7)
    bool debugging = false;
    // fetch filename from json id
    wxString URI;

    try
    {
        URI = pJson->at("id").get<std::string>();
    }
    catch (std::exception & e)
    {
        cbMessageBox(wxString::Format("%s() %s", __FUNCTION__, e.what()));
        return false;
    }

    URI = URI.AfterFirst(STX);
    wxFileName fnFilename = fileUtils.FilePathFromURI(URI);
    wxString filename = fnFilename.GetFullPath();

    if (not wxFileExists(filename))
    {
        return false;
    }

    // Validate that this file belongs to this projects parser

    if (not pProject)
    {
        return false;
    }

    if (not pProject->GetFileByFilename(filename, false))
    {
        return false;
    }

    EditorManager * pEdMgr = Manager::Get()->GetEditorManager();
    cbEditor * pEditor = pEdMgr->GetBuiltinEditor(filename);
    cbStyledTextCtrl * pEdCtrl =  nullptr;          //(ph 2021/04/12)

    if (pEditor)
    {
        pEdCtrl = pEditor->GetControl();
    }
    else
    {
        pEdCtrl = m_Tokenizer.m_pControl;
    }

    // most ParserThreadOptions was copied from m_Options
    LSP_SymbolsParserOptions opts;
    opts.useBuffer             = false;
    opts.bufferSkipBlocks      = false;
    opts.bufferSkipOuterBlocks = false;
    opts.followLocalIncludes   = m_Options.followLocalIncludes;
    opts.followGlobalIncludes  = m_Options.followGlobalIncludes;
    opts.wantPreprocessor      = m_Options.wantPreprocessor;
    opts.parseComplexMacros    = m_Options.parseComplexMacros;
    opts.platformCheck         = m_Options.platformCheck;
    // whether to collect doxygen style documents.
    opts.storeDocumentation    = m_Options.storeDocumentation;
    opts.loader                = nullptr; // must be 0 at this point
    bool isLocal = true;
    wxUnusedVar(isLocal);
    Token * savedLastParent = nullptr;
    LogManager * pLogMgr = Manager::Get()->GetLogManager();

    if (debugging)
    {
        pLogMgr->DebugLog("-----------------symbols----------------");
    }

    try
    {
        json result = pJson->at("result");
        size_t defcnt = result.size();
        TRACE(wxString::Format("%s() json contains %d major symbols", __FUNCTION__, defcnt));

        for (size_t symidx = 0; symidx < defcnt; ++symidx)
        {
            CHECK_TIME(startTime, 2000) //(ph 2021/09/7)
            wxString name =   result.at(symidx)["name"].get<std::string>();
            int kind =        result.at(symidx)["kind"].get<int>();
            // startLine etc is the items lines ranges, eg., function start line to end line braces.
            int startLine =   result.at(symidx)["range"]["start"]["line"].get<int>();
            int startCol =    result.at(symidx)["range"]["start"]["character"].get<int>();
            int endLine =     result.at(symidx)["range"]["end"]["line"].get<int>();
            int endCol =      result.at(symidx)["range"]["end"]["character"].get<int>();
            // selection Range is the item name range, eg. function name
            int selectionRangeStartLine = result.at(symidx)["selectionRange"]["start"]["line"].get<int>();
            int selectionRangeStartCol =  result.at(symidx)["selectionRange"]["start"]["character"].get<int>();
            int selectionRangeEndLine =   result.at(symidx)["selectionRange"]["end"]["line"].get<int>();
            int selectionRangeEndCol =    result.at(symidx)["selectionRange"]["end"]["character"].get<int>();
            size_t childcnt = 0;
            childcnt = result.at(symidx).contains("children") ? result.at(symidx)["children"].size() : 0;
            wxString lineTxt = pEdCtrl->GetLine(startLine);
            lineTxt.Trim(true);  //remove crlf

            if (debugging)   //debugging
            {
                pLogMgr->DebugLog(wxString::Format("lineTxt[%s]", lineTxt));
                pLogMgr->DebugLog(wxString::Format("name[%s] kind(%d) startLine|startCol|endLine|endCol[%d:%d:%d:%d]", name, kind, startLine, startCol, endLine, endCol));
                pLogMgr->DebugLog(wxString::Format("SelectionRange: startLine|StartCol|endLine|endCol[%d:%d:%d:%d]", selectionRangeStartLine, selectionRangeStartCol, selectionRangeEndLine, selectionRangeEndCol));
                pLogMgr->DebugLog(wxString::Format("\tchildren[%d]", childcnt));
            }

            Token * newToken = nullptr;
            wxString args =  wxString();

            //Note: start and end lines contain the whole definition/implementation code.
            //      selectionRange start and end lines are the token name only.
            switch (kind)
            {
                case LSP_DocumentSymbolKind::Class: // type 5
                {
                    int lineNum = selectionRangeStartLine;
                    newToken = DoHandleClass(ctClass, lineNum, endLine, endCol);

                    if (not newToken)
                        switch (1)
                        {
                            default:    //debugging
                                lineTxt = lineTxt.Trim(true).Trim(false);

                                // clangd is reporting typedefs as classes, eg: typedef std::vector<ScriptEntry> ScriptsVector;
                                // FIXME (ph#): There's a HandleTypedef() function. Adapt it.
                                if (lineTxt.StartsWith("typedef "))
                                {
                                    break;
                                }

                                // Eliminate forward declarations like: "class ThatClass;"
                                if (lineTxt.StartsWith("class ") and wxFound(lineTxt.Mid(4).Find(";")))
                                {
                                    break;
                                }

                                // DoHandleClass() didn't understand the syntax. Could have been expanded macro.
                                wxString msg = wxString::Format("LSP Error:DoHandleClass() did not understand line:%d %s", selectionRangeStartLine, lineTxt);
                                pLogMgr->DebugLog(msg);
                        }

                    // In case the user has ticked "show inheritance",
                    // if a base class doesn't exists (not parsed yet), add an entry for it now.
                    // It'll just point back to this derived class declaration.
                    // Am having to do this because am not parsing system included files yet.
                    // FIXME (ph#): Remove this code when system included files get parsed ?
                    if (newToken and newToken->m_AncestorsString.Length())
                    {
                        Token * pClassToken = nullptr;
                        wxString ancestor = newToken->m_AncestorsString.BeforeFirst(',');
                        pClassToken = TokenExists(ancestor, 0, tkClass);

                        if (not pClassToken)
                        {
                            pClassToken = DoAddToken(tkClass, ancestor,
                                                     selectionRangeStartLine + 1,
                                                     startLine + 1,
                                                     endLine + 1,
                                                     "", false, false);    //args, isOperator, isImpl

                            if (not pClassToken)
                            {
                                wxString msg = wxString::Format("Failed to add class Token %s():%d", __FUNCTION__, __LINE__);
                                cbMessageBox(msg, "Failed");
                            }
                        }//endif ancestor token does not exists
                    }//endif ancestor specified

                    break;
                }//endcase LSP_DocumentSymbolKind::Class

                case LSP_DocumentSymbolKind::Function:      //12
                case LSP_DocumentSymbolKind::Method:        // 6
                case LSP_DocumentSymbolKind::Constructor:   // 9
                case LSP_DocumentSymbolKind::Property:      // 7
                {
                    m_Str = wxString();
                    size_t funcNameLth = selectionRangeEndCol - selectionRangeStartCol;
                    args = DoGetDocumentSymbolFunctionArgs(lineTxt, selectionRangeStartCol, funcNameLth);

                    if (wxFound(name.Find("::")))
                    {
                        int posn = name.Find("::");
                        wxString nmspce = name.SubString(0, posn - 1).Trim(true).Trim(false);
                        m_EncounteredNamespaces.push(nmspce);
                        name = name.Mid(posn + 2).Trim(true).Trim(false);

                        if (kind == LSP_DocumentSymbolKind::Method)
                        {
                            if (wxFound(posn = lineTxt.Find(nmspce)))
                            {
                                m_Str = posn ? lineTxt.SubString(0, posn - 1) : wxString();
                                m_Str = m_Str.BeforeLast(' '); //must have blank before namespace
                            }
                        }
                    }//endif method

                    // fetch function return type
                    if (kind == LSP_DocumentSymbolKind::Function)
                    {
                        int posn;

                        if (wxFound(posn = lineTxt.Find(name)))
                        {
                            m_Str = posn ? lineTxt.SubString(0, posn - 1) : wxString();
                            m_Str = m_Str.BeforeLast(' '); //must have blank before func name
                        }
                    }
                }//endcase kind function, method, constructor, property

                // fall into default:
                default:
                {
                    //Info:
                    //Token* DoAddToken(TokenKind       kind,
                    //                  const wxString& name,
                    //                  int             line,
                    //                  int             implLineStart = 0,
                    //                  int             implLineEnd = 0,
                    //                  const wxString& args = wxEmptyString,
                    //                  bool            isOperator = false,
                    //                  bool            isImpl = false);
                    bool isImpl = FileTypeOf(filename) == ftSource;
                    TokenKind ccTokenKind = ConvertDocSymbolKindToCCTokenKind(kind);

                    // Handle tokens that arn't class declarations
                    if ((not newToken) and (kind != LSP_DocumentSymbolKind::Class))
                        newToken = DoAddToken(ccTokenKind, name,
                                              selectionRangeStartLine + 1,
                                              startLine + 1,
                                              endLine + 1,
                                              args,
                                              (kind == LSP_DocumentSymbolKind::Operator), //isOperator
                                              //((ccTokenKind & tkAnyFunction)!=0)          //isImpl
                                              isImpl
                                             );

                    if (newToken and pProject->GetFileByFilename(filename, false))
                    {
                        newToken->m_UserData = pProject;                                //(ph 2021/04/22)
                    }
                }//end default
            }//endswitch kind

            if (childcnt and newToken)                   //(ph 2021/06/12) added newToken check
            {
                savedLastParent = m_LastParent; //save current parent level
                m_LastParent = newToken;
                json jChildren = result.at(symidx)["children"];
                WalkDocumentSymbols(jChildren, filename, newToken, 1);
                m_LastParent = savedLastParent; //back to prvious parent level
            }
        }//end for
    }
    catch (std::exception & e)
    {
        wxString msg = wxString::Format("%s() Error:%s", __FUNCTION__, e.what());
        cbMessageBox(msg, "json Exception");
    }

    return true;
}
// ----------------------------------------------------------------------------
void LSP_SymbolsParser::WalkDocumentSymbols(json & jref, wxString & filename, Token * pParentToken, size_t level) //(ph 2021/03/13)
// ----------------------------------------------------------------------------
{
    bool debugging = false;
    int startTime = 0;
    Token * savedLastParent = nullptr;
    RECORD_TIME(startTime) //(ph 2021/09/7)
    size_t indentLevel = level++ ? level : 1;
    LogManager * pLogMgr = Manager::Get()->GetLogManager();
    EditorManager * pEdMgr = Manager::Get()->GetEditorManager();
    cbEditor * pEditor = pEdMgr->GetBuiltinEditor(filename);
    cbStyledTextCtrl * pEdCtrl = nullptr;       //(ph 2021/04/12)

    if (pEditor)
    {
        pEdCtrl = pEditor->GetControl();
    }
    else
    {
        pEdCtrl = m_Tokenizer.m_pControl;
    }

    try
    {
        json result = jref;
        size_t defcnt = result.size();

        for (size_t symidx = 0; symidx < defcnt; ++symidx)
        {
            wxString name =   result.at(symidx)["name"].get<std::string>();
            int kind =        result.at(symidx)["kind"].get<int>();
            int endCol =      result.at(symidx)["range"]["end"]["character"].get<int>();
            int endLine =     result.at(symidx)["range"]["end"]["line"].get<int>();
            int startCol =    result.at(symidx)["range"]["start"]["character"].get<int>();
            int startLine =   result.at(symidx)["range"]["start"]["line"].get<int>();
            int selectionRangeStartLine = result.at(symidx)["selectionRange"]["start"]["line"].get<int>();
            int selectionRangeStartCol =  result.at(symidx)["selectionRange"]["start"]["character"].get<int>();
            int selectionRangeEndLine =   result.at(symidx)["selectionRange"]["end"]["line"].get<int>();
            int selectionRangeEndCol =    result.at(symidx)["selectionRange"]["end"]["character"].get<int>();
            size_t childcnt = 0;
            childcnt = result.at(symidx).contains("children") ? result.at(symidx)["children"].size() : 0;
            wxString lineTxt = pEdCtrl->GetLine(startLine);
            lineTxt.Trim(true);  //remove crlf

            if (debugging)   //debugging
            {
                pLogMgr->DebugLog(wxString::Format("%*slineTxt[%s]", indentLevel * 4, "", lineTxt));
                pLogMgr->DebugLog(wxString::Format("%*sname[%s] kind(%d) startLine|startCol|endLine|endCol[%d:%d:%d:%d]", indentLevel * 4, "",  name, kind, startLine, startCol, endLine, endCol));
                pLogMgr->DebugLog(wxString::Format("%*sSelectionRange: startLine|StartCol|endLine|endCol[%d:%d:%d:%d]", indentLevel * 4, "",  selectionRangeStartLine, selectionRangeStartCol, selectionRangeEndLine, selectionRangeEndCol));
                pLogMgr->DebugLog(wxString::Format("%*s\tchildren[%d]", indentLevel * 4, "",  childcnt));
            }

            wxString args =  wxString();

            if ((kind == LSP_DocumentSymbolKind::Function)
                    or (kind == LSP_DocumentSymbolKind::Method)
                    or (kind == LSP_DocumentSymbolKind::Constructor)
                    or (kind == LSP_DocumentSymbolKind::Property)
                    or (kind == LSP_DocumentSymbolKind::Class)
               )
            {
                //function or method
                m_Str = wxString();
                size_t funcNameLth = selectionRangeEndCol - selectionRangeStartCol;
                args = DoGetDocumentSymbolFunctionArgs(lineTxt, selectionRangeStartCol, funcNameLth);

                if (wxFound(name.Find("::")))
                {
                    int posn = name.Find("::");
                    wxString nmspce = name.SubString(0, posn - 1).Trim(true).Trim(false);
                    m_EncounteredNamespaces.push(nmspce);
                    name = name.Mid(posn + 2).Trim(true).Trim(false);

                    if (kind == LSP_DocumentSymbolKind::Method)
                    {
                        if (wxFound(posn = lineTxt.Find(nmspce)))
                        {
                            m_Str = posn ? lineTxt.SubString(0, posn - 1) : wxString();
                            m_Str = m_Str.BeforeLast(' '); //must have blank before namespace
                        }
                    }
                }//endif method

                // fetch function return type
                if (kind == LSP_DocumentSymbolKind::Function)
                {
                    int posn;

                    if (wxFound(posn = lineTxt.Find(name)))
                    {
                        m_Str = posn ? lineTxt.SubString(0, posn - 1) : wxString();
                        m_Str = m_Str.BeforeLast(' '); //must have blank before func name
                    }
                }
            }//endif kind function, method, constructor, property, class

            //Token* DoAddToken(TokenKind       kind,
            //                  const wxString& name,
            //                  int             line,
            //                  int             implLineStart = 0,
            //                  int             implLineEnd = 0,
            //                  const wxString& args = wxEmptyString,
            //                  bool            isOperator = false,
            //                  bool            isImpl = false);
            bool isImpl = FileTypeOf(filename) == ftSource;
            TokenKind ccTokenKind = ConvertDocSymbolKindToCCTokenKind(kind);
            m_LastParent = pParentToken;
            Token * newToken = TokenExists(name, 0, ccTokenKind);

            if (not newToken)
            {
                newToken = DoAddToken(ccTokenKind, name,
                                      selectionRangeStartLine + 1,
                                      startLine + 1,
                                      endLine + 1,
                                      args,
                                      (kind == LSP_DocumentSymbolKind::Operator),    //isOperator
                                      //((ccTokenKind & tkAnyFunction)!=0)             //isImpl
                                      isImpl
                                     );
            }

            if (newToken and pParentToken)                                       //(ph 2021/06/12) added pParentToken check
            {
                newToken->m_UserData = pParentToken->m_UserData;    //(ph 2021/04/22)
            }

            CHECK_TIME(startTime, 2000) //(ph 2021/09/7)

            if (childcnt and pParentToken)                                      //(ph 2021/06/12) add pParentToken check
            {
                savedLastParent = m_LastParent;  //save current parent level
                json jChildren = result.at(symidx)["children"];
                WalkDocumentSymbols(jChildren, filename, newToken, indentLevel + 1);
                m_LastParent = savedLastParent; //restore previous parent level
            }
        }//endfor
    }
    catch (std::exception & e)
    {
        wxString msg = wxString::Format("%s() Error:%s", __FUNCTION__, e.what());
        cbMessageBox(msg, "json Exception");
    }

    return;
}
// ----------------------------------------------------------------------------
wxString LSP_SymbolsParser::DoHandleSemanticTokenFunction()
// ----------------------------------------------------------------------------
{
    // info                                     //(ph 2021/03/22)
    //m_Tokenizer.m_SemanticTokenIndex      == m_SemanticTokensIdx;
    //m_Tokenizer.m_SemanticTokenLineNumber == lineNum;
    //m_Tokenizer.m_SemanticTokenColumn     == colNum;
    //m_Tokenizer.m_SemanticTokenLength     == txtLen;
    //m_Tokenizer.m_SemanticTokenType       == type;
    //m_Tokenizer.m_SemanticTokenModifier   == modifier;
    wxString args = wxString();
    cbStyledTextCtrl * pControl = m_Tokenizer.m_pControl;
    int posn = m_Tokenizer.m_SemanticTokenColumn;
    posn += m_Tokenizer.m_SemanticTokenLength;
    wxString lineTxt = pControl->GetLine(m_Tokenizer.m_SemanticTokenLineNumber);

    if (wxFound(lineTxt.Mid(posn).Find('(')))
    {
        int stx = lineTxt.Mid(posn).Find('(');
        int etx = lineTxt.Mid(posn).Find(')');
        args = lineTxt.Mid(posn).SubString(stx, etx);
    }

    return args;
}
// ----------------------------------------------------------------------------
TokenKind LSP_SymbolsParser::ConvertDocSymbolKindToCCTokenKind(int docSymKind)
// ----------------------------------------------------------------------------
{
    /// FIXME (ph#): the following ccTokenKind(s) may not be correct //(ph 2021/03/22)
    TokenKind ccTokenKind = tkUndefined;

    switch (docSymKind)
    {
        case LSP_DocumentSymbolKind::File:
            ccTokenKind = tkUndefined;
            break;

        case LSP_DocumentSymbolKind::Module:
            ccTokenKind = tkUndefined;
            break;

        case LSP_DocumentSymbolKind::Namespace:
            ccTokenKind = tkNamespace;
            break;

        case LSP_DocumentSymbolKind::Package:
            ccTokenKind = tkUndefined;
            break;

        case LSP_DocumentSymbolKind::Class:
            ccTokenKind = tkClass;
            break;

        case LSP_DocumentSymbolKind::Method:
            ccTokenKind = tkFunction;
            break;

        case LSP_DocumentSymbolKind::Property:
            ccTokenKind = tkVariable;
            break;

        case LSP_DocumentSymbolKind::Field:
            ccTokenKind = tkVariable;
            break;

        case LSP_DocumentSymbolKind::Constructor:
            ccTokenKind = tkConstructor;
            break;

        case LSP_DocumentSymbolKind::Enum:
            ccTokenKind = tkEnum;
            break;

        case LSP_DocumentSymbolKind::Interface:
            ccTokenKind = tkClass;
            break;

        case LSP_DocumentSymbolKind::Function:
            ccTokenKind = tkFunction;
            break;

        case LSP_DocumentSymbolKind::Variable:
            ccTokenKind = tkVariable;
            break;

        case LSP_DocumentSymbolKind::Constant:
            ccTokenKind = tkVariable;
            break;

        case LSP_DocumentSymbolKind::String:
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::Number:
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::Boolean:
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::Array:
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::Object:
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::Key:
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::Null :
            ccTokenKind = tkTypedef;
            break;

        case LSP_DocumentSymbolKind::EnumMember:
            ccTokenKind = tkEnumerator;
            break;

        case LSP_DocumentSymbolKind::Struct:
            ccTokenKind = tkClass;
            break;

        case LSP_DocumentSymbolKind::Event:
            ccTokenKind = tkUndefined;
            break;

        case LSP_DocumentSymbolKind::Operator:
            ccTokenKind = tkUndefined;
            break;
    }//endswitch

    return ccTokenKind;
}
// ----------------------------------------------------------------------------
wxString LSP_SymbolsParser::DoGetDocumentSymbolFunctionArgs(wxString & lineTxt, int startCol, int length)
// ----------------------------------------------------------------------------
{
    wxString args = wxString();
    //cbStyledTextCtrl* pControl = m_Tokenizer.m_pControl;
    int posn = startCol + length;

    if (wxFound(lineTxt.Mid(posn).Find('(')))
    {
        int stx = lineTxt.Mid(posn).Find('(');
        int etx = lineTxt.Mid(posn).Find(')');

        if (etx == wxNOT_FOUND)       // if no ending ')'
        {
            args = lineTxt.Mid(posn); // use rest of line as arguments
            args.Trim(true);
            args.Append("...");
        }
        else                          // else use '(chars)' following function name
        {
            args = lineTxt.Mid(posn).SubString(stx, etx);
        }
    }

    return args;
}

Token * LSP_SymbolsParser::TokenExists(const wxString & name, const Token * parent, short int kindMask)
{
    // no critical section needed here:
    // all functions that call this, already entered a critical section.
    // Lookup in local parent or in global scope
    int foundIdx = m_TokenTree->TokenExists(name, parent ? parent->m_Index : -1, kindMask);

    if (foundIdx != wxNOT_FOUND)
    {
        return m_TokenTree->at(foundIdx);
    }

    // Lookup in included namespaces
    foundIdx = m_TokenTree->TokenExists(name, m_UsedNamespacesIds, kindMask);
    return m_TokenTree->at(foundIdx);
}

Token * LSP_SymbolsParser::TokenExists(const wxString & name, const wxString & baseArgs, const Token * parent, TokenKind kind)
{
    // no critical section needed here:
    // all functions that call this, already entered a critical section.
    // Lookup in local parent or in global scope
    int foundIdx = m_TokenTree->TokenExists(name, baseArgs, parent ? parent->m_Index : -1, kind);

    if (foundIdx != wxNOT_FOUND)
    {
        return m_TokenTree->at(foundIdx);
    }

    // Lookup in included namespaces
    foundIdx = m_TokenTree->TokenExists(name, baseArgs, m_UsedNamespacesIds, kind);
    return m_TokenTree->at(foundIdx);
}

wxString LSP_SymbolsParser::GetTokenBaseType()
{
    TRACE(_T("GetTokenBaseType() : Searching within m_Str='%s'"), m_Str.wx_str());
    // Compensate for spaces between namespaces (e.g. NAMESPACE :: SomeType)
    // which is valid C++ construct.
    // Also, spaces that follow a semicolon are removed.
    int pos = 0;

    while (pos < static_cast<int>(m_Str.Length()))
    {
        if (wxIsspace(m_Str.GetChar(pos))
                && (((pos > 0)
                     && (m_Str.GetChar(pos - 1) == ParserConsts::colon_chr))
                    || ((pos < static_cast<int>(m_Str.Length()) - 1)
                        && (m_Str.GetChar(pos + 1) == ParserConsts::colon_chr))))
        {
            m_Str.Remove(pos, 1);
        }
        else
        {
            ++pos;
        }
    }

    TRACE(_T("GetTokenBaseType() : Compensated m_Str='%s'"), m_Str.wx_str());
    // TODO (Morten#5#): Handle stuff like the following gracefully:
    // int __cdecl __MINGW_NOTHROW vscanf (const char * __restrict__, __VALIST);
    // m_Str contains the full text before the token's declaration
    // an example, for a variable Token: "const wxString& s;"
    // m_Str would be: const wxString&
    // what we do here is locate the actual return value (wxString in this example)
    // it will be needed by code completion code ;)
    // Note that generally the returned type string is the identifier like token near the variable
    // name, there may be some exceptions. E.g. "wxString const &s;", here, "const" should not be
    // returned as a type name.
    pos = m_Str.Length() - 1; // search start at the end of m_Str

    while (pos >= 0)
    {
        // we walk m_Str backwards until we find a non-space character which also is
        // not * or &
        //                        const wxString&
        // in this example, we would stop here ^
        while ((pos >= 0)
                && (wxIsspace(m_Str.GetChar(pos))
                    || (m_Str.GetChar(pos) == ParserConsts::ptr_chr)
                    || (m_Str.GetChar(pos) == ParserConsts::ref_chr)))
        {
            --pos;
        }

        if (pos >= 0)
        {
            // we have the end of the word we're interested in
            int end = pos;

            // continue walking backwards until we find the start of the word
            //                               const  wxString&
            // in this example, we would stop here ^
            while ((pos >= 0)
                    && (wxIsalnum(m_Str.GetChar(pos))
                        || (m_Str.GetChar(pos) == ParserConsts::underscore_chr)
                        || (m_Str.GetChar(pos) == ParserConsts::colon_chr)))
            {
                --pos;
            }

            wxString typeCandidate = m_Str.Mid(pos + 1, end - pos);

            // "const" should not be returned as a type name, so we try next candidate.
            if (typeCandidate.IsSameAs(ParserConsts::kw_const))
            {
                continue;
            }

            TRACE(_T("GetTokenBaseType() : Found '%s'"), typeCandidate.wx_str());
            return typeCandidate;
        }
    }

    TRACE(_T("GetTokenBaseType() : Returning '%s'"), m_Str.wx_str());
    return m_Str; // token ends at start of phrase
}

Token * LSP_SymbolsParser::FindTokenFromQueue(std::queue<wxString> & q, Token * parent, bool createIfNotExist,
                                              Token * parentIfCreated)
{
    if (q.empty())
    {
        return 0;
    }

    wxString ns = q.front();
    q.pop();
    Token * result = TokenExists(ns, parent, tkNamespace | tkClass);

    // if we can't find one in global namespace, then we check the local parent
    if (!result && parent == 0)
    {
        result = TokenExists(ns, parentIfCreated, tkNamespace | tkClass);
    }

    if (!result && createIfNotExist)
    {
        result = new Token(ns, m_FileIdx, 0, ++m_TokenTree->m_TokenTicketCount);
        result->m_TokenKind = q.empty() ? tkClass : tkNamespace;
        result->m_IsLocal = m_IsLocal;
        result->m_ParentIndex = parentIfCreated ? parentIfCreated->m_Index : -1;
        int newidx = m_TokenTree->insert(result);

        if (parentIfCreated)
        {
            parentIfCreated->AddChild(newidx);
        }

        TRACE(_T("FindTokenFromQueue() : Created unknown class/namespace %s (%d) under %s (%d)"),
              ns.wx_str(),
              newidx,
              parent ? parent->m_Name.wx_str() : _T("<globals>"),
              parent ? parent->m_Index : -1);
    }

    if (q.empty())
    {
        return result;
    }

    if (result)
    {
        result = FindTokenFromQueue(q, result, createIfNotExist, parentIfCreated);
    }

    return result;
}

// ----------------------------------------------------------------------------
Token * LSP_SymbolsParser::DoAddToken(TokenKind       kind,
                                      const wxString & name,
                                      int             line,
                                      int             implLineStart,
                                      int             implLineEnd,
                                      const wxString & args,
                                      bool            isOperator,
                                      bool            isImpl)
// ----------------------------------------------------------------------------
{
    if (name.IsEmpty())
    {
        TRACE(_T("DoAddToken() : Token name is empty!"));
        return 0; // oops!
    }

    Token * newToken = 0;
    wxString newname(name);
    m_Str.Trim(true).Trim(false);

    if (kind == tkDestructor)
    {
        // special class destructors case
        newname.Prepend(ParserConsts::tilde);
        m_Str.Clear();
    }

    wxString baseArgs;

    if (kind & tkAnyFunction)
    {
        if (!GetBaseArgs(args, baseArgs))
        {
            kind = tkVariable;
        }
    }

    Token * localParent = 0;
    // preserve m_EncounteredTypeNamespaces; needed further down this function
    std::queue<wxString> q = m_EncounteredTypeNamespaces;

    if ((kind == tkDestructor || kind == tkConstructor) && !q.empty())
    {
        // look in m_EncounteredTypeNamespaces
        localParent = FindTokenFromQueue(q, 0, true, m_LastParent);

        if (localParent)
        {
            newToken = TokenExists(newname, baseArgs, localParent, kind);
        }

        if (newToken)
        {
            TRACE(_T("DoAddToken() : Found token (ctor/dtor)."));
        }
    }

    // check for implementation member function
    if (!newToken && !m_EncounteredNamespaces.empty())
    {
        localParent = FindTokenFromQueue(m_EncounteredNamespaces, 0, true, m_LastParent);

        if (localParent)
        {
            newToken = TokenExists(newname, baseArgs, localParent, kind);
        }

        if (newToken)
        {
            TRACE(_T("DoAddToken() : Found token (member function)."));

            // Special handling function implementation here, a function declaration and its
            // function implementation share one Token. But the function implementation's arguments
            // should take precedence, as they will be used for code-completion.
            if (isImpl && (kind & tkAnyFunction))
            {
                newToken->m_Args = args;
            }
        }
    }

    // none of the above; check for token under parent
    if (!newToken)
    {
        newToken = TokenExists(newname, baseArgs, m_LastParent, kind);

        if (newToken)
        {
            TRACE(_T("DoAddToken() : Found token (parent)."));

            // Special handling function implementation, see comments above
            if (isImpl && (kind & tkAnyFunction))
            {
                newToken->m_Args = args;
            }
        }
    }

    // need to check if the current token already exists in the tokenTree
    // if newToken is valid (non zero), it points to a Token with same kind and same name, so there
    // is a chance we can update the existing Token and not create a new one. This usually happens
    // we are reparsing a header file, but some class definition Token is shared with an implementation
    // file, so the Token can be updated.
    // In some special cases, the a new Token instance is needed. E.g. token's template argument is
    // checked to support template specialization
    // eg:  template<typename T> class A {...} and template<> class A<int> {...}
    // we record them as different tokens
    if (newToken
            && (newToken->m_TemplateArgument == m_TemplateArgument)
            && (kind & tkAnyFunction
                || newToken->m_Args == args
                || kind & tkAnyContainer))
    {
        ; // nothing to do
    }
    else
    {
        newToken = new Token(newname, m_FileIdx, line, ++m_TokenTree->m_TokenTicketCount);
        TRACE(_T("DoAddToken() : Created token='%s', file_idx=%u, line=%d, ticket=%lu"), newname.wx_str(),
              m_FileIdx, line, static_cast<unsigned long>(m_TokenTree->m_TokenTicketCount));
        Token * finalParent = localParent ? localParent : m_LastParent;

        if (kind == tkVariable && m_Options.parentIdxOfBuffer != -1)
        {
            finalParent = m_TokenTree->at(m_Options.parentIdxOfBuffer);
        }

        newToken->m_ParentIndex = finalParent ? finalParent->m_Index : -1;
        newToken->m_TokenKind   = kind;
        newToken->m_Scope       = m_LastScope;
        newToken->m_BaseArgs    = baseArgs;

        if (newToken->m_TokenKind == tkClass)
        {
            newToken->m_BaseArgs = args;    // save template args
        }
        else
        {
            newToken->m_Args = args;
        }

        int newidx = m_TokenTree->insert(newToken);

        if (finalParent)
        {
            finalParent->AddChild(newidx);
        }
    }

    if (!(kind & (tkConstructor | tkDestructor)))
    {
        wxString tokenFullType = m_Str;

        if (!m_PointerOrRef.IsEmpty())
        {
            tokenFullType << m_PointerOrRef;
            m_PointerOrRef.Clear();
        }

        wxString tokenBaseType = GetTokenBaseType();

        if (tokenBaseType.Find(ParserConsts::space_chr) == wxNOT_FOUND)
        {
            // token type must contain all namespaces
            wxString prepend;

            // Notice: clears the queue "m_EncounteredTypeNamespaces", too
            while (!m_EncounteredTypeNamespaces.empty())
            {
                prepend << m_EncounteredTypeNamespaces.front() << ParserConsts::dcolon;
                m_EncounteredTypeNamespaces.pop();
            }

            TRACE(_T("DoAddToken() : Prepending '%s'"), prepend.wx_str());
            tokenBaseType.Prepend(prepend);
        }

        newToken->m_FullType = tokenFullType;
        newToken->m_BaseType = tokenBaseType;
    }

    newToken->m_IsLocal    = m_IsLocal;
    newToken->m_IsTemp     = m_Options.isTemp;
    newToken->m_IsOperator = isOperator;

    if (not isImpl)
    {
        newToken->m_FileIdx = m_FileIdx;
        newToken->m_Line    = line;
    }
    else //is Implementation
    {
        newToken->m_ImplFileIdx   = m_FileIdx;
        newToken->m_ImplLine      = line;
        newToken->m_ImplLineStart = implLineStart;
        newToken->m_ImplLineEnd   = implLineEnd;
        m_TokenTree->InsertTokenBelongToFile(newToken->m_ImplFileIdx, newToken->m_Index);
    }

    // this will append the doxygen style comments to the Token
    m_Tokenizer.SetLastTokenIdx(newToken->m_Index);
    TRACE(_T("DoAddToken() : Added/updated token '%s' (%d), kind '%s', type '%s', actual '%s'. Parent is %s (%d)"),
          name.wx_str(), newToken->m_Index, newToken->GetTokenKindString().wx_str(), newToken->m_FullType.wx_str(),
          newToken->m_BaseType.wx_str(), m_TokenTree->at(newToken->m_ParentIndex) ?
          m_TokenTree->at(newToken->m_ParentIndex)->m_Name.wx_str() : wxEmptyString,
          newToken->m_ParentIndex);
    //(ph 2021/10/30) **debugging**
    //wxString AddedTokenStr = wxString::Format(_T("DoAddToken() : Added/updated token '%s' (%d), kind '%s', type '%s', actual '%s'. Parent is %s (%d)"),
    //      name.wx_str(), newToken->m_Index, newToken->GetTokenKindString().wx_str(), newToken->m_FullType.wx_str(),
    //      newToken->m_BaseType.wx_str(), m_TokenTree->at(newToken->m_ParentIndex) ?
    //      m_TokenTree->at(newToken->m_ParentIndex)->m_Name.wx_str() : wxEmptyString,
    //      newToken->m_ParentIndex);
    //CCLogger::Get()->DebugLog(AddedTokenStr);
    ADDTOKEN(_T("Token: Index %7d Line %7d: Type: %s: -> '%s'"),
             newToken->m_Index, line, newToken->GetTokenKindString().wx_str(), name.wx_str());
    //(ph 2021/10/30) **debugging**
    //AddedTokenStr = wxString::Format(_T("Token: Index %7d Line %7d: Type: %s: -> '%s'"),
    //         newToken->m_Index, line, newToken->GetTokenKindString().wx_str(), name.wx_str());
    //CCLogger::Get()->DebugLog(AddedTokenStr);

    // Notice: clears the queue "m_EncounteredTypeNamespaces"
    while (!m_EncounteredTypeNamespaces.empty())
    {
        m_EncounteredTypeNamespaces.pop();
    }

    return newToken;
}

void LSP_SymbolsParser::HandleIncludes()
{
#if defined(cbDEBUG)
    cbAssertNonFatal(0 && "LSP_SymbolsParser::HandleIncludes() shouldnt be here!");
#endif
    return;
    //wxString filename;
    //bool isGlobal = !m_IsLocal;
    //wxString token = m_Tokenizer.GetToken();
    //
    //// now token holds something like:
    //// "someheader.h"
    //// < and will follow iostream.h, >
    //if (!token.IsEmpty())
    //{
    //    if (token.GetChar(0) == '"')
    //    {
    //        // "someheader.h"
    //        // don't use wxString::Replace(); it's too costly
    //        size_t pos = 0;
    //        while (pos < token.Length())
    //        {
    //            wxChar c = token.GetChar(pos);
    //            if (c != _T('"'))
    //                filename << c;
    //            ++pos;
    //        }
    //    }
    //    else if (token.GetChar(0) == ParserConsts::lt_chr)
    //    {
    //        isGlobal = true;
    //        // next token is filename, next is '.' (dot), next is extension
    //        // basically we'll loop until '>' (gt)
    //        while (IS_ALIVE)
    //        {
    //            token = m_Tokenizer.GetToken();
    //            if (token.IsEmpty())
    //                break;
    //            if (token.GetChar(0) != ParserConsts::gt_chr)
    //                filename << token;
    //            else
    //                break;
    //        }
    //    }
    //}
    //
    //if (ParserCommon::FileType(filename) == ParserCommon::ftOther)
    //    return;
    //
    //if (!filename.IsEmpty())
    //{
    //    TRACE(_T("HandleIncludes() : Found include file '%s'"), filename.wx_str());
    //    do
    //    {
    //        // setting all #includes as global
    //        // it's amazing how many projects use #include "..." for global headers (MSVC mainly - booh)
    //        isGlobal = true;
    //
    //        if (!(isGlobal ? m_Options.followGlobalIncludes : m_Options.followLocalIncludes))
    //        {
    //            TRACE(_T("HandleIncludes() : File '%s' not requested to parse after checking options, skipping"), filename.wx_str());
    //            break; // Nothing to do!
    //        }
    //
    //        wxString real_filename = m_Parent->GetFullFileName(m_Filename, filename, isGlobal);
    //        // Parser::GetFullFileName is thread-safe :)
    //
    //        if (real_filename.IsEmpty())
    //        {
    //            TRACE(_T("HandleIncludes() : File '%s' not found, skipping"), filename.wx_str());
    //            break; // File not found, do nothing.
    //        }
    //
    //        if (m_TokenTree->IsFileParsed(real_filename))
    //        {
    //            TRACE(_T("HandleIncludes() : File '%s' is already being parsed, skipping"), real_filename.wx_str());
    //            break; // Already being parsed elsewhere
    //        }
    //
    //        TRACE(_T("HandleIncludes() : Adding include file '%s'"), real_filename.wx_str());
    //        m_Parent->ParseFile(real_filename, isGlobal, true);
    //    }
    //    while (false);
    //}
}
// ----------------------------------------------------------------------------
void LSP_SymbolsParser::HandleNamespace()
// ----------------------------------------------------------------------------
{
    wxString ns = m_Tokenizer.GetToken();
    int line = m_Tokenizer.GetLineNumber();

    if (ns == ParserConsts::opbrace)
    {
        // parse inside anonymous namespace
        Token   *  lastParent = m_LastParent;
        TokenScope lastScope  = m_LastScope;
        DoParse();
        m_LastParent = lastParent;
        m_LastScope   = lastScope;
    }
    else
    {
        while (true)
        {
            // for namespace aliases to be parsed, we need to tell the tokenizer
            // not to skip the usually unwanted tokens. One of those tokens is the
            // "assignment" (=).
            // we just have to remember to revert this setting below, or else problems will follow
            m_Tokenizer.SetState(tsNormal);
            wxString next = m_Tokenizer.PeekToken(); // named namespace

            if (next == ParserConsts::opbrace)
            {
                m_Tokenizer.SetState(tsNormal);
                // use the existing copy (if any)
                Token * newToken = TokenExists(ns, m_LastParent, tkNamespace);

                if (!newToken)
                {
                    newToken = DoAddToken(tkNamespace, ns, line);
                }

                if (!newToken)
                {
                    TRACE(_T("HandleNamespace() : Unable to create/add new token: ") + ns);
                    return;
                }

                m_Tokenizer.GetToken(); // eat {
                int lineStart = m_Tokenizer.GetLineNumber();
                Token   *  lastParent = m_LastParent; // save status, will restore after DoParse()
                TokenScope lastScope  = m_LastScope;
                m_LastParent = newToken;
                // default scope is: public for namespaces (actually no, but emulate it)
                m_LastScope   = tsPublic;
                DoParse();
                m_LastParent = lastParent;
                m_LastScope   = lastScope;
                // update implementation file and lines of namespace.
                // this doesn't make much sense because namespaces are all over the place,
                // but do it anyway so that buffer-based parsing returns the correct values.
                newToken->m_ImplFileIdx   = m_FileIdx;
                newToken->m_ImplLine      = line;
                newToken->m_ImplLineStart = lineStart;
                newToken->m_ImplLineEnd   = m_Tokenizer.GetLineNumber();
                // the namespace body is correctly parsed
                break;
            }
            else
                if (next == ParserConsts::equals)
                {
                    // namespace alias; example from cxxabi.h:
                    //
                    // namespace __cxxabiv1
                    // {
                    // ...
                    // }
                    // namespace abi = __cxxabiv1; <-- we 're in this case now
                    m_Tokenizer.GetToken(); // eat '='
                    m_Tokenizer.SetState(tsNormal);
                    Token * lastParent = m_LastParent;
                    Token * aliasToken = NULL;

                    while (IS_ALIVE)
                    {
                        wxString aliasStr = m_Tokenizer.GetToken();
                        // use the existing copy (if any)
                        aliasToken = TokenExists(aliasStr, m_LastParent, tkNamespace);

                        if (!aliasToken)
                        {
                            aliasToken = DoAddToken(tkNamespace, aliasStr, line);
                        }

                        if (!aliasToken)
                        {
                            return;
                        }

                        if (m_Tokenizer.PeekToken() == ParserConsts::dcolon)
                        {
                            m_Tokenizer.GetToken();
                            m_LastParent = aliasToken;
                        }
                        else
                        {
                            break;
                        }
                    }

                    aliasToken->m_Aliases.Add(ns);
                    m_LastParent = lastParent;
                    // the namespace alias statement is correctly parsed
                    break;
                }
                else
                {
                    m_Tokenizer.SetState(tsNormal);
                    // probably some kind of error in code ?
                    SkipToOneOfChars(ParserConsts::semicolonopbrace);
                    // in case of the code:
                    //
                    //    # define _GLIBCXX_VISIBILITY(V) _GLIBCXX_PSEUDO_VISIBILITY(V)
                    //    namespace std _GLIBCXX_VISIBILITY(default)
                    //    {
                    //        class vector
                    //        {
                    //            size_t size();
                    //        }
                    //    }
                    // we still want to parse the body of the namespace, but skip the tokens before "{"
                    m_Tokenizer.UngetToken();
                    wxString peek = m_Tokenizer.PeekToken();

                    if (peek == ParserConsts::opbrace)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
        } // while(true)
    }
}
// ----------------------------------------------------------------------------
Token * LSP_SymbolsParser::DoHandleClass(EClassType ct, int lineNumber, int lastLineNumber, int endCol)           //(ph 2021/05/15)
// ----------------------------------------------------------------------------
{
    // need to force the tokenizer _not_ skip anything
    // as we 're manually parsing class decls
    // don't forget to reset that if you add any early exit condition!
    TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);
    m_Tokenizer.SetLineNumber(lineNumber);
    int lineNr = m_Tokenizer.GetLineNumber(); //just checking
    wxString ancestors;
    wxString lastCurrent;
    Token * newToken = nullptr; //(ph 2021/05/30)
    cbStyledTextCtrl * pTextCtrl = m_Tokenizer.m_pControl;

    // FIXME (ph#): change code to use cbStyledTextCtrl directly. Quite moving text
    // In Tokenizer init, text from cbStyledTextCtrl should have been copied to Tokenizer.m_Buffer
    if ((not pTextCtrl) or (not pTextCtrl->GetLength()))
    {
        return nullptr;    //(ph 2021/05/30)
    }

    //set current parsing position to beginning of lineNumber param
    m_Tokenizer.SetTokenIndex(pTextCtrl->PositionFromLine(lineNumber));
    m_Tokenizer.SetPeekTokenIndex(pTextCtrl->PositionFromLine(lineNumber));
    m_Tokenizer.SetPeekAvailable(false);
    int startTime = 0;
    RECORD_TIME(startTime)

    while (IS_ALIVE)
    {
        CHECK_TIME(startTime, 1000) //(ph 2021/09/8)
        wxString current = m_Tokenizer.GetToken(); // class name
        wxString next    = m_Tokenizer.PeekToken();

        // remove __attribute__ or __declspec qualifiers
        if (current == ParserConsts::kw_attribute || current == ParserConsts::kw_declspec)
        {
            TRACE(_T("HandleClass() : Skip __attribute__ or __declspec"));
            // Handle stuff like: __attribute__(( whatever ))
            m_Tokenizer.GetToken();  // eat __attribute__
            current = m_Tokenizer.GetToken();  // eat (( whatever ))
            next    = m_Tokenizer.PeekToken(); // peek again
        }

        TRACE(_T("HandleClass() : Found class '%s', next='%s'"), current.wx_str(), next.wx_str());

        if (current.IsEmpty() || next.IsEmpty())
        {
            break;
        }

        // -------------------------------------------------------------------
        if (next == ParserConsts::lt) // template specialization
            // -------------------------------------------------------------------
        {
            // eg: template<> class A<int> {...}, then we update template argument with "<int>"
            GetTemplateArgs();
            next = m_Tokenizer.PeekToken();
        }

        // -------------------------------------------------------------------
        if (next == ParserConsts::colon) // has ancestor(s)
            // -------------------------------------------------------------------
        {
            TRACE(_T("HandleClass() : Class '%s' has ancestors"), current.wx_str());
            m_Tokenizer.GetToken(); // eat ":"

            while (IS_ALIVE)
            {
                CHECK_TIME(startTime, 1000) //(ph 2021/09/8)
                wxString tmp = m_Tokenizer.GetToken();
                next = m_Tokenizer.PeekToken();

                // -----------------------------------------------------------
                if (tmp == ParserConsts::kw_public
                        || tmp == ParserConsts::kw_protected
                        || tmp == ParserConsts::kw_private)
                    // -----------------------------------------------------------
                {
                    continue;
                }

                // -----------------------------------------------------------
                if (!(tmp == ParserConsts::comma || tmp == ParserConsts::gt))
                    // -----------------------------------------------------------
                {
                    // fix for namespace usage in ancestors
                    if (tmp == ParserConsts::dcolon || next == ParserConsts::dcolon)
                    {
                        ancestors << tmp;
                    }
                    else
                    {
                        ancestors << tmp << ParserConsts::comma_chr;
                    }

                    TRACE(_T("HandleClass() : Adding ancestor ") + tmp);
                }

                // -----------------------------------------------------------
                if (next.IsEmpty()
                        || next == ParserConsts::opbrace
                        || next == ParserConsts::semicolon)
                    // -----------------------------------------------------------
                {
                    break;
                }
                // -----------------------------------------------------------
                else
                    if (next == ParserConsts::lt)
                        // -----------------------------------------------------------
                    {
                        // template class
                        //m_Tokenizer.GetToken(); // reach "<"
                        // must not "eat" the token,
                        // SkipAngleBraces() will do it to see what it must match
                        SkipAngleBraces();
                        // also need to 'unget' the last token (>)
                        // so next iteration will see the { or ; in 'next'
                        m_Tokenizer.UngetToken();
                    }
            }

            TRACE(_T("HandleClass() : Ancestors: ") + ancestors);
        }

        // -------------------------------------------------------------------
        if (current == ParserConsts::opbrace) // unnamed class/struct/union
            // -------------------------------------------------------------------
        {
            wxString unnamedTmp;
            unnamedTmp.Printf(_T("%s%s%u_%lu"),
                              g_UnnamedSymbol.wx_str(),
                              (ct == ctClass ? _T("Class") : (ct == ctUnion ? _T("Union") : _T("Struct"))),
                              m_FileIdx, static_cast<unsigned long>(m_StructUnionUnnamedCount++));
            //-Token* newToken = DoAddToken(tkClass, unnamedTmp, lineNr);
            newToken = DoAddToken(tkClass, unnamedTmp, lineNr); //(ph 2021/05/30)

            // Maybe it is a bug here. I just fixed it.
            if (!newToken)
            {
                TRACE(_T("HandleClass() : Unable to create/add new token: ") + unnamedTmp);
                // restore tokenizer's functionality
                m_Tokenizer.SetState(oldState);
                return nullptr;
            }

            newToken->m_TemplateArgument = m_TemplateArgument;
            wxArrayString formals;
            SplitTemplateFormalParameters(m_TemplateArgument, formals);
#ifdef CC_PARSER_TEST

            for (size_t i = 0; i < formals.GetCount(); ++i)
            {
                TRACE(_T("The template formal arguments are '%s'."), formals[i].wx_str());
            }

#endif
            newToken->m_TemplateType = formals;
            m_TemplateArgument.Clear();
            Token   *  lastParent     = m_LastParent;
            TokenScope lastScope      = m_LastScope;
            bool       parsingTypedef = m_ParsingTypedef;
            m_LastParent     = newToken;
            // default scope is: private for classes, public for structs, public for unions
            m_LastScope      = ct == ctClass ? tsPrivate : tsPublic;
            m_ParsingTypedef = false;
            newToken->m_ImplLine = lineNr;
            newToken->m_ImplLineStart = m_Tokenizer.GetLineNumber();
            newToken->m_IsAnonymous = true;
#if defined(cbDEBUG)
            cbAssertNonFatal(wxString::Format("Trying to DoParse recursion in %s():%d", __PRETTY_FUNCTION__, __LINE__));
#endif
            break; //(ph 2021/10/13)
            /// DoParse(); // recursion
            m_LastParent     = lastParent;
            m_LastScope      = lastScope;
            m_ParsingTypedef = parsingTypedef;
            m_LastUnnamedTokenName = unnamedTmp; // used for typedef'ing anonymous class/struct/union

            // we should now be right after the closing brace
            // no vars are defined on a typedef, only types
            // In the former example, aa is not part of the typedef.
            if (m_ParsingTypedef)
            {
                m_Str.Clear();
                TRACE(_T("HandleClass() : Unable to create/add new token: ") + current);

                if (!ReadClsNames(newToken->m_Name))
                {
                    TRACE(_T("HandleClass() : ReadClsNames returned false [1]."));
                }

                break;
            }
            else
            {
                m_Str = newToken->m_Name;

                if (!ReadVarNames())
                {
                    TRACE(_T("HandleClass() : ReadVarNames returned false [1]."));
                }

                m_Str.Clear();
                break;
            }
        }
        // -------------------------------------------------------------------
        else
            if (next == ParserConsts::opbrace)
                // -------------------------------------------------------------------
            {
                // for a template class definition like
                // template <typename x, typename y>class AAA : public BBB, CCC {;}
                // we would like to show its ancestors and template formal parameters on the tooltip,
                // so we re-used the m_Args member to store those informations, the tooltip shows like:
                // class AAA<x,y> : BBB, CCC {...} instead of class AAA {...}
                wxStringTokenizer tkz(ancestors, ParserConsts::comma);
                wxString args;

                while (tkz.HasMoreTokens())
                {
                    CHECK_TIME(startTime, 1000)     //(ph 2021/09/8)
                    const wxString & ancestor = tkz.GetNextToken();

                    if (ancestor.IsEmpty())
                    {
                        continue;
                    }

                    if (args.IsEmpty())
                    {
                        args += ParserConsts::space + ParserConsts::colon;
                    }
                    else
                    {
                        args += ParserConsts::comma;
                    }

                    args += ParserConsts::space + ancestor;
                }

                wxArrayString formals;
                SplitTemplateFormalParameters(m_TemplateArgument, formals);

                if (!formals.IsEmpty())
                {
                    args.Prepend(ParserConsts::lt + GetStringFromArray(formals, ParserConsts::comma, false) + ParserConsts::gt);
                }

                //-Token* newToken = DoAddToken(tkClass, current, lineNr, 0, 0, args);
                newToken = DoAddToken(tkClass, current, lineNr, 0, 0, args);    //(ph 2021/05/30)

                if (!newToken)
                {
                    TRACE(_T("HandleClass() : Unable to create/add new token: ") + current);
                    // restore tokenizer's functionality
                    m_Tokenizer.SetState(oldState);
                    return newToken; //(ph 2021/05/30)
                }

                newToken->m_AncestorsString = ancestors;
                m_Tokenizer.GetToken(); // eat {
                Token * lastParent = m_LastParent; // save status, and will restore after DoParse()
                TokenScope lastScope = m_LastScope;
                bool parsingTypedef = m_ParsingTypedef;
                m_LastParent = newToken;
                // default scope is: private for classes, public for structs, public for unions
                m_LastScope = ct == ctClass ? tsPrivate : tsPublic;
                m_ParsingTypedef = false;
                newToken->m_ImplLine = lineNr;
                newToken->m_ImplLineStart = m_Tokenizer.GetLineNumber();
                newToken->m_TemplateArgument = m_TemplateArgument;
#ifdef CC_PARSER_TEST

                for (size_t i = 0; i < formals.GetCount(); ++i)
                {
                    TRACE(_T("The template formal arguments are '%s'."), formals[i].wx_str());
                }

#endif
                newToken->m_TemplateType = formals;
                m_TemplateArgument.Clear();
                /// ------- Dont parse class for LSP ----------------------------------- //(ph 2021/05/27)
                //-DoParse();
                // Set Tokenizer last line number to end of Class code
                cbStyledTextCtrl * pTextCtrl = m_Tokenizer.m_pControl;
                // lastLineNumber from LSP must contain ending '}'
                m_Tokenizer.SetLineNumber(lastLineNumber);
                unsigned endOfClassTokenIndex = pTextCtrl->PositionFromLine(lastLineNumber);
                wxString endOfClassText = pTextCtrl->GetLine(lastLineNumber);
                m_Tokenizer.SetTokenIndex(endOfClassTokenIndex);
                wxString token = m_Tokenizer.GetToken();
                wxString peektoken = m_Tokenizer.PeekToken();
                // unterminated class with '}'causes loop //(ph 2021/09/8)
                //while ( (token != '}' ) and (m_Tokenizer.GetLineNumber() <= (size_t)lastLineNumber) )
                //{
                //    token = m_Tokenizer.GetToken();
                //}
                wxString classLastLine = pTextCtrl->GetLine(lastLineNumber); //(ph 2021/09/8)

                if (classLastLine[endCol ? endCol - 1 : endCol] == '}')
                {
                    token = '}';
                }
                else
                {
                    token = "";
                }

                if (token == '}')
                    ;//token = m_Tokenizer.GetToken(); //eat the '}'

                //newToken->m_ImplLineEnd = m_Tokenizer.GetLineNumber();
                newToken->m_ImplLineEnd = lastLineNumber + 1;
                m_ParsingTypedef = parsingTypedef;
                m_LastParent = lastParent;
                m_LastScope = lastScope;

                // we should now be right after the closing brace
                // no vars are defined on a typedef, only types
                // In the former example, aa is not part of the typedef.
                if (m_ParsingTypedef)
                {
                    m_Str.Clear();

                    if (!ReadClsNames(newToken->m_Name))
                    {
                        TRACE(_T("HandleClass() : ReadClsNames returned false [2]."));
                    }

                    break;
                }
                else
                {
                    m_Str = newToken->m_Name;   // pattern: class A{} b; b is a variable

                    if (!ReadVarNames())
                    {
                        TRACE(_T("HandleClass() : ReadVarNames returned false [2]."));
                    }

                    m_Str.Clear();
                    break;
                }
            }
            // -------------------------------------------------------------------
            else
                if (next == ParserConsts::semicolon)
                    // -------------------------------------------------------------------
                {
                    // e.g. struct A {}; struct B { struct A a; };
                    if (m_LastParent
                            && m_LastParent->m_TokenKind == tkClass
                            && !lastCurrent.IsEmpty())
                    {
                        m_Str << lastCurrent << ParserConsts::space_chr;
                        DoAddToken(tkVariable, current, m_Tokenizer.GetLineNumber());
                        break;
                    }
                    else
                    {
                        break;    // forward decl; we don't care
                    }
                }
                // -------------------------------------------------------------------
                else
                    if (next.GetChar(0) == ParserConsts::opbracket_chr) // function: struct xyz& DoSomething()...
                        // -------------------------------------------------------------------
                    {
                        HandleFunction(current);
                        break;
                    }
                    // -------------------------------------------------------------------
                    else
                        if ((next.GetChar(0) == ParserConsts::ptr_chr)
                                || (next.GetChar(0) == ParserConsts::ref_chr))
                            // -------------------------------------------------------------------
                        {
                            // e.g. typedef struct A * a;
                            if (next.GetChar(0) == ParserConsts::ptr_chr && m_ParsingTypedef)
                            {
                                wxString args;
                                //-Token* newToken = DoAddToken(tkClass, current, lineNr, 0, 0, args);
                                newToken = DoAddToken(tkClass, current, lineNr, 0, 0, args); //(ph 2021/05/30)

                                if (!newToken)
                                {
                                    TRACE(_T("HandleClass() : Unable to create/add new token: ") + current);
                                    // restore tokenizer's functionality
                                    m_Tokenizer.SetState(oldState);
                                    return newToken; //(ph 2021/05/30)
                                }

                                newToken->m_AncestorsString = ancestors;
                                m_PointerOrRef << m_Tokenizer.GetToken();
                                m_Str.Clear();

                                if (!ReadClsNames(newToken->m_Name))
                                {
                                    TRACE(_T("HandleClass() : ReadClsNames returned false [2]."));
                                }

                                break;
                            }
                            else
                            {
                                m_Str << current;
                            }

                            break;
                        }
                        // -------------------------------------------------------------------
                        else
                            if (next == ParserConsts::equals)
                                // -------------------------------------------------------------------
                            {
                                // some patterns like: struct AAA a = {.x = 1, .y=2};
                                // In (ANSI) C99, you can use a designated initializer to initialize a structure
                                if (!lastCurrent.IsEmpty())
                                {
                                    m_Str << lastCurrent << ParserConsts::space_chr;
                                    DoAddToken(tkVariable, current, m_Tokenizer.GetLineNumber());
                                }

                                // so we have to eat the brace pair
                                SkipToOneOfChars(ParserConsts::semicolon, /* supportNesting*/ true, /*singleCharToken*/ true);
                                break;
                            }
                            // -------------------------------------------------------------------
                            else
                                // -------------------------------------------------------------------
                            {
                                // might be instantiation, see the following
                                // e.g. struct HiddenStruct { int val; }; struct HiddenStruct yy;
                                if (m_ParsingTypedef)
                                {
                                    m_Tokenizer.UngetToken();
                                    break;
                                }

                                if (TokenExists(current, m_LastParent, tkClass))
                                {
                                    if (!TokenExists(next, m_LastParent, tkVariable))
                                    {
                                        wxString farnext;
                                        m_Tokenizer.GetToken(); // go ahead of identifier
                                        farnext = m_Tokenizer.PeekToken();

                                        //  struct Point p1, p2;
                                        //  current="Point", next="p1"
                                        if (farnext == ParserConsts::semicolon || farnext == ParserConsts::comma)
                                        {
                                            while (m_Options.handleVars
                                                    && (farnext == ParserConsts::semicolon
                                                        || farnext == ParserConsts::comma))
                                            {
                                                CHECK_TIME(startTime, 1000) //(ph 2021/09/8)

                                                if (m_Str.IsEmpty())
                                                {
                                                    m_Str = current;
                                                }

                                                DoAddToken(tkVariable, next, m_Tokenizer.GetLineNumber());

                                                if (farnext == ParserConsts::comma)
                                                {
                                                    m_Tokenizer.GetToken(); // eat the ","
                                                    next = m_Tokenizer.GetToken(); // next = "p2"
                                                    farnext = m_Tokenizer.PeekToken(); // farnext = "," or the final ";"
                                                    continue;
                                                }
                                                else // we meet a ";", so break the loop
                                                {
                                                    m_Str.Clear();
                                                    break;
                                                }
                                            }

                                            m_Tokenizer.GetToken(); // eat semi-colon
                                            break;
                                        }
                                        else
                                        {
                                            m_Tokenizer.UngetToken();    // restore the identifier
                                        }
                                    }
                                }
                            }

        lastCurrent = current;
    }

    // restore tokenizer's functionality
    m_Tokenizer.SetState(oldState);
    return newToken;    //(ph 2021/05/30)
}
void LSP_SymbolsParser::HandleClass(EClassType ct) //not used until implementing HandleTypedefs
{
    // need to force the tokenizer _not_ skip anything
    // as we 're manually parsing class decls
    // don't forget to reset that if you add any early exit condition!
    TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);
    int lineNr = m_Tokenizer.GetLineNumber();
    wxString ancestors;
    wxString lastCurrent;

    while (IS_ALIVE)
    {
        wxString current = m_Tokenizer.GetToken(); // class name
        wxString next    = m_Tokenizer.PeekToken();

        // remove __attribute__ or __declspec qualifiers
        if (current == ParserConsts::kw_attribute || current == ParserConsts::kw_declspec)
        {
            TRACE(_T("HandleClass() : Skip __attribute__ or __declspec"));
            // Handle stuff like: __attribute__(( whatever ))
            m_Tokenizer.GetToken();  // eat __attribute__
            current = m_Tokenizer.GetToken();  // eat (( whatever ))
            next    = m_Tokenizer.PeekToken(); // peek again
        }

        TRACE(_T("HandleClass() : Found class '%s', next='%s'"), current.wx_str(), next.wx_str());

        if (current.IsEmpty() || next.IsEmpty())
        {
            break;
        }

        // -------------------------------------------------------------------
        if (next == ParserConsts::lt) // template specialization
            // -------------------------------------------------------------------
        {
            // eg: template<> class A<int> {...}, then we update template argument with "<int>"
            GetTemplateArgs();
            next = m_Tokenizer.PeekToken();
        }

        // -------------------------------------------------------------------
        if (next == ParserConsts::colon) // has ancestor(s)
            // -------------------------------------------------------------------
        {
            TRACE(_T("HandleClass() : Class '%s' has ancestors"), current.wx_str());
            m_Tokenizer.GetToken(); // eat ":"

            while (IS_ALIVE)
            {
                wxString tmp = m_Tokenizer.GetToken();
                next = m_Tokenizer.PeekToken();

                // -----------------------------------------------------------
                if (tmp == ParserConsts::kw_public
                        || tmp == ParserConsts::kw_protected
                        || tmp == ParserConsts::kw_private)
                    // -----------------------------------------------------------
                {
                    continue;
                }

                // -----------------------------------------------------------
                if (!(tmp == ParserConsts::comma || tmp == ParserConsts::gt))
                    // -----------------------------------------------------------
                {
                    // fix for namespace usage in ancestors
                    if (tmp == ParserConsts::dcolon || next == ParserConsts::dcolon)
                    {
                        ancestors << tmp;
                    }
                    else
                    {
                        ancestors << tmp << ParserConsts::comma_chr;
                    }

                    TRACE(_T("HandleClass() : Adding ancestor ") + tmp);
                }

                // -----------------------------------------------------------
                if (next.IsEmpty()
                        || next == ParserConsts::opbrace
                        || next == ParserConsts::semicolon)
                    // -----------------------------------------------------------
                {
                    break;
                }
                // -----------------------------------------------------------
                else
                    if (next == ParserConsts::lt)
                        // -----------------------------------------------------------
                    {
                        // template class
                        //m_Tokenizer.GetToken(); // reach "<"
                        // must not "eat" the token,
                        // SkipAngleBraces() will do it to see what it must match
                        SkipAngleBraces();
                        // also need to 'unget' the last token (>)
                        // so next iteration will see the { or ; in 'next'
                        m_Tokenizer.UngetToken();
                    }
            }

            TRACE(_T("HandleClass() : Ancestors: ") + ancestors);
        }

        // -------------------------------------------------------------------
        if (current == ParserConsts::opbrace) // unnamed class/struct/union
            // -------------------------------------------------------------------
        {
            wxString unnamedTmp;
            unnamedTmp.Printf(_T("%s%s%u_%lu"),
                              g_UnnamedSymbol.wx_str(),
                              (ct == ctClass ? _T("Class") : (ct == ctUnion ? _T("Union") : _T("Struct"))),
                              m_FileIdx, static_cast<unsigned long>(m_StructUnionUnnamedCount++));
            Token * newToken = DoAddToken(tkClass, unnamedTmp, lineNr);

            // Maybe it is a bug here. I just fixed it.
            if (!newToken)
            {
                TRACE(_T("HandleClass() : Unable to create/add new token: ") + unnamedTmp);
                // restore tokenizer's functionality
                m_Tokenizer.SetState(oldState);
                return;
            }

            newToken->m_TemplateArgument = m_TemplateArgument;
            wxArrayString formals;
            SplitTemplateFormalParameters(m_TemplateArgument, formals);
#ifdef CC_PARSER_TEST

            for (size_t i = 0; i < formals.GetCount(); ++i)
            {
                TRACE(_T("The template formal arguments are '%s'."), formals[i].wx_str());
            }

#endif
            newToken->m_TemplateType = formals;
            m_TemplateArgument.Clear();
            Token   *  lastParent     = m_LastParent;
            TokenScope lastScope      = m_LastScope;
            bool       parsingTypedef = m_ParsingTypedef;
            m_LastParent     = newToken;
            // default scope is: private for classes, public for structs, public for unions
            m_LastScope      = ct == ctClass ? tsPrivate : tsPublic;
            m_ParsingTypedef = false;
            newToken->m_ImplLine = lineNr;
            newToken->m_ImplLineStart = m_Tokenizer.GetLineNumber();
            newToken->m_IsAnonymous = true;
            DoParse(); // recursion
            m_LastParent     = lastParent;
            m_LastScope      = lastScope;
            m_ParsingTypedef = parsingTypedef;
            m_LastUnnamedTokenName = unnamedTmp; // used for typedef'ing anonymous class/struct/union

            // we should now be right after the closing brace
            // no vars are defined on a typedef, only types
            // In the former example, aa is not part of the typedef.
            if (m_ParsingTypedef)
            {
                m_Str.Clear();
                TRACE(_T("HandleClass() : Unable to create/add new token: ") + current);

                if (!ReadClsNames(newToken->m_Name))
                {
                    TRACE(_T("HandleClass() : ReadClsNames returned false [1]."));
                }

                break;
            }
            else
            {
                m_Str = newToken->m_Name;

                if (!ReadVarNames())
                {
                    TRACE(_T("HandleClass() : ReadVarNames returned false [1]."));
                }

                m_Str.Clear();
                break;
            }
        }
        // -------------------------------------------------------------------
        else
            if (next == ParserConsts::opbrace)
                // -------------------------------------------------------------------
            {
                // for a template class definition like
                // template <typename x, typename y>class AAA : public BBB, CCC {;}
                // we would like to show its ancestors and template formal parameters on the tooltip,
                // so we re-used the m_Args member to store those informations, the tooltip shows like:
                // class AAA<x,y> : BBB, CCC {...} instead of class AAA {...}
                wxStringTokenizer tkz(ancestors, ParserConsts::comma);
                wxString args;

                while (tkz.HasMoreTokens())
                {
                    const wxString & ancestor = tkz.GetNextToken();

                    if (ancestor.IsEmpty())
                    {
                        continue;
                    }

                    if (args.IsEmpty())
                    {
                        args += ParserConsts::space + ParserConsts::colon;
                    }
                    else
                    {
                        args += ParserConsts::comma;
                    }

                    args += ParserConsts::space + ancestor;
                }

                wxArrayString formals;
                SplitTemplateFormalParameters(m_TemplateArgument, formals);

                if (!formals.IsEmpty())
                {
                    args.Prepend(ParserConsts::lt + GetStringFromArray(formals, ParserConsts::comma, false) + ParserConsts::gt);
                }

                Token * newToken = DoAddToken(tkClass, current, lineNr, 0, 0, args);

                if (!newToken)
                {
                    TRACE(_T("HandleClass() : Unable to create/add new token: ") + current);
                    // restore tokenizer's functionality
                    m_Tokenizer.SetState(oldState);
                    return;
                }

                newToken->m_AncestorsString = ancestors;
                m_Tokenizer.GetToken(); // eat {
                Token * lastParent = m_LastParent; // save status, and will restore after DoParse()
                TokenScope lastScope = m_LastScope;
                bool parsingTypedef = m_ParsingTypedef;
                m_LastParent = newToken;
                // default scope is: private for classes, public for structs, public for unions
                m_LastScope = ct == ctClass ? tsPrivate : tsPublic;
                m_ParsingTypedef = false;
                newToken->m_ImplLine = lineNr;
                newToken->m_ImplLineStart = m_Tokenizer.GetLineNumber();
                newToken->m_TemplateArgument = m_TemplateArgument;
#ifdef CC_PARSER_TEST

                for (size_t i = 0; i < formals.GetCount(); ++i)
                {
                    TRACE(_T("The template formal arguments are '%s'."), formals[i].wx_str());
                }

#endif
                newToken->m_TemplateType = formals;
                m_TemplateArgument.Clear();
                DoParse();
                newToken->m_ImplLineEnd = m_Tokenizer.GetLineNumber();
                m_ParsingTypedef = parsingTypedef;
                m_LastParent = lastParent;
                m_LastScope = lastScope;

                // we should now be right after the closing brace
                // no vars are defined on a typedef, only types
                // In the former example, aa is not part of the typedef.
                if (m_ParsingTypedef)
                {
                    m_Str.Clear();

                    if (!ReadClsNames(newToken->m_Name))
                    {
                        TRACE(_T("HandleClass() : ReadClsNames returned false [2]."));
                    }

                    break;
                }
                else
                {
                    m_Str = newToken->m_Name;   // pattern: class A{} b; b is a variable

                    if (!ReadVarNames())
                    {
                        TRACE(_T("HandleClass() : ReadVarNames returned false [2]."));
                    }

                    m_Str.Clear();
                    break;
                }
            }
            // -------------------------------------------------------------------
            else
                if (next == ParserConsts::semicolon)
                    // -------------------------------------------------------------------
                {
                    // e.g. struct A {}; struct B { struct A a; };
                    if (m_LastParent
                            && m_LastParent->m_TokenKind == tkClass
                            && !lastCurrent.IsEmpty())
                    {
                        m_Str << lastCurrent << ParserConsts::space_chr;
                        DoAddToken(tkVariable, current, m_Tokenizer.GetLineNumber());
                        break;
                    }
                    else
                    {
                        break;    // forward decl; we don't care
                    }
                }
                // -------------------------------------------------------------------
                else
                    if (next.GetChar(0) == ParserConsts::opbracket_chr) // function: struct xyz& DoSomething()...
                        // -------------------------------------------------------------------
                    {
                        HandleFunction(current);
                        break;
                    }
                    // -------------------------------------------------------------------
                    else
                        if ((next.GetChar(0) == ParserConsts::ptr_chr)
                                || (next.GetChar(0) == ParserConsts::ref_chr))
                            // -------------------------------------------------------------------
                        {
                            // e.g. typedef struct A * a;
                            if (next.GetChar(0) == ParserConsts::ptr_chr && m_ParsingTypedef)
                            {
                                wxString args;
                                Token * newToken = DoAddToken(tkClass, current, lineNr, 0, 0, args);

                                if (!newToken)
                                {
                                    TRACE(_T("HandleClass() : Unable to create/add new token: ") + current);
                                    // restore tokenizer's functionality
                                    m_Tokenizer.SetState(oldState);
                                    return;
                                }

                                newToken->m_AncestorsString = ancestors;
                                m_PointerOrRef << m_Tokenizer.GetToken();
                                m_Str.Clear();

                                if (!ReadClsNames(newToken->m_Name))
                                {
                                    TRACE(_T("HandleClass() : ReadClsNames returned false [2]."));
                                }

                                break;
                            }
                            else
                            {
                                m_Str << current;
                            }

                            break;
                        }
                        // -------------------------------------------------------------------
                        else
                            if (next == ParserConsts::equals)
                                // -------------------------------------------------------------------
                            {
                                // some patterns like: struct AAA a = {.x = 1, .y=2};
                                // In (ANSI) C99, you can use a designated initializer to initialize a structure
                                if (!lastCurrent.IsEmpty())
                                {
                                    m_Str << lastCurrent << ParserConsts::space_chr;
                                    DoAddToken(tkVariable, current, m_Tokenizer.GetLineNumber());
                                }

                                // so we have to eat the brace pair
                                SkipToOneOfChars(ParserConsts::semicolon, /* supportNesting*/ true, /*singleCharToken*/ true);
                                break;
                            }
                            // -------------------------------------------------------------------
                            else
                                // -------------------------------------------------------------------
                            {
                                // might be instantiation, see the following
                                // e.g. struct HiddenStruct { int val; }; struct HiddenStruct yy;
                                if (m_ParsingTypedef)
                                {
                                    m_Tokenizer.UngetToken();
                                    break;
                                }

                                if (TokenExists(current, m_LastParent, tkClass))
                                {
                                    if (!TokenExists(next, m_LastParent, tkVariable))
                                    {
                                        wxString farnext;
                                        m_Tokenizer.GetToken(); // go ahead of identifier
                                        farnext = m_Tokenizer.PeekToken();

                                        //  struct Point p1, p2;
                                        //  current="Point", next="p1"
                                        if (farnext == ParserConsts::semicolon || farnext == ParserConsts::comma)
                                        {
                                            while (m_Options.handleVars
                                                    && (farnext == ParserConsts::semicolon
                                                        || farnext == ParserConsts::comma))
                                            {
                                                if (m_Str.IsEmpty())
                                                {
                                                    m_Str = current;
                                                }

                                                DoAddToken(tkVariable, next, m_Tokenizer.GetLineNumber());

                                                if (farnext == ParserConsts::comma)
                                                {
                                                    m_Tokenizer.GetToken(); // eat the ","
                                                    next = m_Tokenizer.GetToken(); // next = "p2"
                                                    farnext = m_Tokenizer.PeekToken(); // farnext = "," or the final ";"
                                                    continue;
                                                }
                                                else // we meet a ";", so break the loop
                                                {
                                                    m_Str.Clear();
                                                    break;
                                                }
                                            }

                                            m_Tokenizer.GetToken(); // eat semi-colon
                                            break;
                                        }
                                        else
                                        {
                                            m_Tokenizer.UngetToken();    // restore the identifier
                                        }
                                    }
                                }
                            }

        lastCurrent = current;
    }

    // restore tokenizer's functionality
    m_Tokenizer.SetState(oldState);
}

void LSP_SymbolsParser::HandleFunction(wxString & name, bool isOperator, bool isPointer)
{
    TRACE(_T("HandleFunction() : Adding function '") + name + _T("': m_Str='") + m_Str + _T("'"));
    int lineNr = m_Tokenizer.GetLineNumber();
    wxString args = m_Tokenizer.GetToken();
    wxString peek = m_Tokenizer.PeekToken();
    TRACE(_T("HandleFunction() : name='") + name + _T("', args='") + args + _T("', peek='") + peek + _T("'"));

    // NOTE: Avoid using return, because m_Str needs to be cleared
    // at the end of this function.

    // special case for function pointers
    if (isPointer)
    {
        int pos = name.find(ParserConsts::ptr);

        // pattern: m_Str AAA (*BBB) (...);
        // pattern: m_Str AAA (*BBB) (...) = some_function;
        if (pos != wxNOT_FOUND && (peek == ParserConsts::semicolon
                                   || peek == ParserConsts::equals
                                   || peek == ParserConsts::comma))
        {
            name.RemoveLast();  // remove ")"
            name.Remove(0, pos + 1).Trim(false); // remove "(* "
            // pattern: m_Str AAA (*BBB[X][Y]) (...);
            // Trim(true) for safety, in case the name contains a trailing space
            pos = name.find(ParserConsts::oparray_chr);

            if (pos != wxNOT_FOUND)
            {
                name.Remove(pos).Trim(true);
            }

            TRACE(_T("HandleFunction() : Add token name='") + name + _T("', args='") + args + _T("', return type='") + m_Str + _T("'"));
            Token * newToken =  DoAddToken(tkFunction, name, lineNr, 0, 0, args);

            if (newToken)
            {
                newToken->m_IsConst = false;
                newToken->m_TemplateArgument = m_TemplateArgument;

                if (!m_TemplateArgument.IsEmpty() && newToken->m_TemplateMap.empty())
                {
                    ResolveTemplateArgs(newToken);
                }
            }
            else
            {
                TRACE(_T("HandleFunction() : Unable to create/add new token: ") + name);
            }

            m_TemplateArgument.Clear();
        }
    }
    else
        if (!m_Str.StartsWith(ParserConsts::kw_friend))
        {
            int lineStart = 0;
            int lineEnd = 0;
            bool isCtor = m_Str.IsEmpty();
            bool isDtor = m_Str.StartsWith(ParserConsts::tilde);
            Token * localParent = 0;

            if ((isCtor || isDtor) && !m_EncounteredTypeNamespaces.empty())
            {
                // probably a ctor/dtor
                std::queue<wxString> q = m_EncounteredTypeNamespaces; // preserve m_EncounteredTypeNamespaces; needed in DoAddToken()
                localParent = FindTokenFromQueue(q, m_LastParent);
                TRACE(_T("HandleFunction() : Ctor/Dtor '%s', m_Str='%s', localParent='%s'"),
                      name.wx_str(),
                      m_Str.wx_str(),
                      localParent ? localParent->m_Name.wx_str() : _T("<none>"));
            }
            else
            {
                std::queue<wxString> q = m_EncounteredNamespaces; // preserve m_EncounteredNamespaces; needed in DoAddToken()
                localParent = FindTokenFromQueue(q, m_LastParent);
                TRACE(_T("HandleFunction() : !(Ctor/Dtor) '%s', m_Str='%s', localParent='%s'"),
                      name.wx_str(),
                      m_Str.wx_str(),
                      localParent ? localParent->m_Name.wx_str() : _T("<none>"));
            }

            bool isCtorOrDtor = m_LastParent && name == m_LastParent->m_Name;

            if (!isCtorOrDtor)
            {
                isCtorOrDtor = localParent && name == localParent->m_Name;
            }

            if (!isCtorOrDtor && m_Options.useBuffer)
            {
                isCtorOrDtor = isCtor || isDtor;
            }

            TRACE(_T("HandleFunction() : Adding function '%s', ': m_Str='%s', enc_ns='%s'."),
                  name.wx_str(),
                  m_Str.wx_str(),
                  m_EncounteredNamespaces.size() ? m_EncounteredNamespaces.front().wx_str() : wxT("nil"));
            bool isImpl = false;
            bool isConst = false;
            bool isNoExcept = false;

            while (!peek.IsEmpty()) // !eof
            {
                if (peek == ParserConsts::colon) // probably a ctor with member initializers
                {
                    SkipToOneOfChars(ParserConsts::opbrace);
                    m_Tokenizer.UngetToken(); // leave brace there
                    peek = m_Tokenizer.PeekToken();
                    continue;
                }
                else
                    if (peek == ParserConsts::opbrace)// function implementation
                    {
                        isImpl = true;
                        m_Tokenizer.GetToken(); // eat {
                        lineStart = m_Tokenizer.GetLineNumber();
                        SkipBlock(); // skip  to matching }
                        lineEnd = m_Tokenizer.GetLineNumber();
                        break;
                    }
                    else
                        if (peek == ParserConsts::clbrace
                                || peek == ParserConsts::semicolon
                                || peek == ParserConsts::comma)
                        {
                            break;    // function decl
                        }
                        else
                            if (peek == ParserConsts::kw_const)
                            {
                                isConst = true;
                            }
                            else
                                if (peek == ParserConsts::kw_noexcept)
                                {
                                    isNoExcept = true;
                                }
                                else
                                    if (peek == ParserConsts::kw_throw)
                                    {
                                        // Handle something like: std::string MyClass::MyMethod() throw(std::exception)
                                        wxString arg = m_Tokenizer.GetToken(); // eat args ()
                                    }
                                    else
                                        if (peek == ParserConsts::kw_try)
                                        {
                                            // function-try-block pattern: AAA(...)try{}catch{}
                                            m_Tokenizer.GetToken(); // eat the try keyword

                                            if (m_Tokenizer.PeekToken() == ParserConsts::colon)
                                            {
                                                // skip ctor initialization list
                                                SkipToOneOfChars(ParserConsts::opbrace);
                                                m_Tokenizer.UngetToken(); // leave brace there
                                            }

                                            if (m_Tokenizer.PeekToken() == ParserConsts::opbrace)
                                            {
                                                isImpl = true;
                                                m_Tokenizer.GetToken(); // eat {
                                                lineStart = m_Tokenizer.GetLineNumber();
                                                SkipBlock(); // skip to matching }

                                                while (m_Tokenizer.PeekToken() == ParserConsts::kw_catch)
                                                {
                                                    m_Tokenizer.GetToken(); // eat catch
                                                    m_Tokenizer.GetToken(); // eat catch args

                                                    if (m_Tokenizer.PeekToken() == ParserConsts::opbrace)
                                                    {
                                                        m_Tokenizer.GetToken(); // eat {
                                                        SkipBlock(); // skip to matching }
                                                    }
                                                }

                                                lineEnd = m_Tokenizer.GetLineNumber();
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            TRACE(_T("HandleFunction() : Possible macro '%s' in function '%s' (file name='%s', line numer %d)."),
                                                  peek.wx_str(), name.wx_str(), m_Filename.wx_str(), m_Tokenizer.GetLineNumber());
                                            break; // darned macros that do not end with a semicolon :/
                                        }

                // if we reached here, eat the token so peek gets a new value
                m_Tokenizer.GetToken();
                peek = m_Tokenizer.PeekToken();
            } // while

            TRACE(_T("HandleFunction() : Add token name='") + name + _T("', args='") + args + _T("', return type='") + m_Str + _T("'"));
            TokenKind tokenKind = !isCtorOrDtor ? tkFunction : (isDtor ? tkDestructor : tkConstructor);
            Token * newToken =  DoAddToken(tokenKind, name, lineNr, lineStart, lineEnd, args, isOperator, isImpl);

            if (newToken)
            {
                newToken->m_IsConst = isConst;
                newToken->m_IsNoExcept = isNoExcept;
                newToken->m_TemplateArgument = m_TemplateArgument;

                if (!m_TemplateArgument.IsEmpty() && newToken->m_TemplateMap.empty())
                {
                    ResolveTemplateArgs(newToken);
                }
            }
            else
            {
                TRACE(_T("HandleFunction() : Unable to create/add new token: ") + name);
            }

            m_TemplateArgument.Clear();
        }

    // NOTE: If we peek an equals or comma, this could be a list of function
    // declarations. In that case, don't clear return type (m_Str).
    peek = m_Tokenizer.PeekToken();

    if (peek != ParserConsts::equals && peek != ParserConsts::comma)
    {
        m_Str.Clear();
    }
}

void LSP_SymbolsParser::HandleConditionalArguments()
{
    // if these aren't empty at this point, we have a syntax error
    if (!m_Str.empty())
    {
        return;
    }

    if (!m_PointerOrRef.empty())
    {
        return;
    }

    if (!m_TemplateArgument.empty())
    {
        return;
    }

    // conditional arguments can look like this:
    // (int i = 12)
    // (Foo *bar = getFooBar())
    // (var <= 12 && (getType() != 23))
    wxString args = m_Tokenizer.GetToken();

    // remove braces
    if (args.StartsWith(_T("(")))
    {
        args = args.Mid(1, args.length() - 1);
    }

    if (args.EndsWith(_T(")")))
    {
        args = args.Mid(0, args.length() - 1);
    }

    // parse small tokens inside for loop head
    TokenTree tree;
    wxString fileName = m_Tokenizer.GetFilename();
    Tokenizer smallTokenizer(&tree);
    smallTokenizer.InitFromBuffer(args, fileName, m_Tokenizer.GetLineNumber());

    while (IS_ALIVE)
    {
        wxString token = smallTokenizer.GetToken();

        if (token.empty())
        {
            break;
        }

        wxString peek = smallTokenizer.PeekToken();

        if (peek.empty())
        {
            if (!m_Str.empty())
            {
                // remove template argument if there is one
                wxString varType, templateArgs;
                RemoveTemplateArgs(m_Str, varType, templateArgs);
                m_Str = varType;
                m_TemplateArgument = templateArgs;
                Token * newToken = DoAddToken(tkVariable, token, smallTokenizer.GetLineNumber());

                if (newToken && !m_TemplateArgument.IsEmpty())
                {
                    ResolveTemplateArgs(newToken);
                }
                else
                {
                    TRACE(_T("HandleConditionalArguments() : Unable to create/add new token: ") + token);
                }
            }

            break;
        }
        else
        {
            if (token == ParserConsts::ref_chr || token == ParserConsts::ptr_chr)
            {
                m_PointerOrRef << token;
            }
            else
            {
                if (!m_Str.empty())
                {
                    m_Str << _T(" ");
                }

                m_Str << token;
            }
        }
    }

    m_Str.clear();
    m_PointerOrRef.clear();
    m_TemplateArgument.clear();
}

void LSP_SymbolsParser::HandleForLoopArguments()
{
    // if these aren't empty at this point, we have a syntax error
    if (!m_Str.empty())
    {
        return;
    }

    if (!m_PointerOrRef.empty())
    {
        return;
    }

    if (!m_TemplateArgument.empty())
    {
        return;
    }

    // for loop heads look like this:
    // ([init1 [, init2 ...] ] ; [cond1 [, cond2 ..]]; [mod1 [, mod2 ..]])
    wxString args = m_Tokenizer.GetToken();

    // remove braces
    if (args.StartsWith(_T("(")))
    {
        args = args.Mid(1, args.length() - 1);
    }

    if (args.EndsWith(_T(")")))
    {
        args = args.Mid(0, args.length() - 1);
    }

    // parse small tokens inside for loop head
    TokenTree tree;
    wxString fileName = m_Tokenizer.GetFilename();
    Tokenizer smallTokenizer(&tree);
    smallTokenizer.InitFromBuffer(args, fileName, m_Tokenizer.GetLineNumber());

    while (IS_ALIVE)
    {
        wxString token = smallTokenizer.GetToken();

        if (token.empty())
        {
            break;
        }

        // pattern  for (; ...)
        // the first token is a ';'
        if (token == ParserConsts::semicolon)
        {
            break;
        }

        wxString peek = smallTokenizer.PeekToken();
        bool createNewToken = false;
        bool finished = false;

        // pattern  for(int i = 5; ...)
        // there is a "=" after the token "i"
        if (peek == ParserConsts::equals)
        {
            // skip to ',' or ';'
            while (IS_ALIVE)
            {
                smallTokenizer.GetToken();
                peek = smallTokenizer.PeekToken();

                if (peek == ParserConsts::comma
                        || peek == ParserConsts::semicolon
                        || peek.empty())
                {
                    break;
                }
            }
        }

        if (peek == ParserConsts::comma)
        {
            smallTokenizer.GetToken(); // eat comma
            createNewToken = true;
        }
        else
            if (peek == ParserConsts::colon
                    || peek == ParserConsts::semicolon
                    || peek.empty())
            {
                createNewToken = true;
                finished = true; // after this point there will be no further declarations
            }
            else
            {
                if (token == ParserConsts::ref_chr || token == ParserConsts::ptr_chr)
                {
                    m_PointerOrRef << token;
                }
                else
                {
                    if (!m_Str.empty())
                    {
                        m_Str << _T(" ");
                    }

                    m_Str << token;
                }
            }

        if (createNewToken && !m_Str.empty())
        {
            // remove template argument if there is one
            wxString name, templateArgs;
            RemoveTemplateArgs(m_Str, name, templateArgs);
            m_Str = name;
            m_TemplateArgument = templateArgs;
            Token * newToken = DoAddToken(tkVariable, token, smallTokenizer.GetLineNumber());

            if (newToken && !m_TemplateArgument.IsEmpty())
            {
                ResolveTemplateArgs(newToken);
            }
            else
            {
                TRACE(_T("HandleForLoopArguments() : Unable to create/add new token: ") + token);
            }
        }

        if (finished)
        {
            break;
        }
    }

    m_Str.clear();
    m_PointerOrRef.clear();
    m_TemplateArgument.clear();
}

void LSP_SymbolsParser::HandleEnum()
{
    // enums have the following rough definition:
    // enum [xxx] { type1 name1 [= 1][, [type2 name2 [= 2]]] };
    bool isUnnamed = false;
    bool isEnumClass = false;
    int lineNr = m_Tokenizer.GetLineNumber();
    wxString token = m_Tokenizer.GetToken();

    // C++11 has some enhanced enumeration declaration
    // see: http://en.cppreference.com/w/cpp/language/enum
    if (token == ParserConsts::kw_class)
    {
        token = m_Tokenizer.GetToken();
        isEnumClass = true;
    }
    else
        if (token == ParserConsts::colon)
        {
            // enum : int {...}
            SkipToOneOfChars(ParserConsts::semicolonopbrace); // jump to the "{" or ";"
            // note in this case, the "{" or ";" is already eaten, so we need to go back one step
            m_Tokenizer.UngetToken();
            token = m_Tokenizer.PeekToken();
        }

    if (token.IsEmpty())
    {
        return;
    }
    else
        if (token == ParserConsts::opbrace)
        {
            // we have an un-named enum
            if (m_ParsingTypedef)
            {
                token.Printf(_T("%sEnum%u_%lu"), g_UnnamedSymbol.wx_str(), m_FileIdx, static_cast<unsigned long>(m_EnumUnnamedCount++));
                m_LastUnnamedTokenName = token;
            }
            else
            {
                token = g_UnnamedSymbol;
            }

            m_Tokenizer.UngetToken(); // return '{' back
            isUnnamed = true;
        }

    // the token is now the expected enum name
    Token * newEnum = 0L;
    unsigned int level = 0;

    if (wxIsalpha(token.GetChar(0))
            || (token.GetChar(0) == ParserConsts::underscore_chr))
    {
        // we have such pattern: enum    name     {
        //                               ^^^^
        //                               token    peek
        wxString peek = m_Tokenizer.PeekToken();

        if (peek == ParserConsts::colon) // enum    name  : type    {
        {
            m_Tokenizer.GetToken(); // eat the ":"
            SkipToOneOfChars(ParserConsts::semicolonopbrace); // jump to the "{" or ";"
            // note in this case, the "{" or ";" is already eaten, so we need to go back one step
            m_Tokenizer.UngetToken();
            peek = m_Tokenizer.PeekToken();
        }

        if (peek.GetChar(0) != ParserConsts::opbrace_chr)
        {
            // pattern:  enum E var;
            // now peek=var, so we try to see it is a variable definition
            if (TokenExists(token, m_LastParent, tkEnum))
            {
                if (!TokenExists(m_Tokenizer.PeekToken(), m_LastParent, tkVariable))
                {
                    wxString ident = m_Tokenizer.GetToken(); // go ahead of identifier

                    if (m_Tokenizer.PeekToken() == ParserConsts::semicolon)
                    {
                        if (m_Options.handleEnums)
                        {
                            m_Str = token;
                            DoAddToken(tkVariable, ident, m_Tokenizer.GetLineNumber());
                            m_Str.Clear();
                        }

                        m_Tokenizer.GetToken(); // eat semi-colon
                    }
                    else
                    {
                        // peek is not ";", mostly it is some pattern like:
                        // enum E fun (..) ;
                        // enum E fun (..) {...};
                        // so we just push the "E" to the m_Str, and return
                        // this make the just like:
                        // E fun (..) ;
                        // E fun (..) {...};
                        m_Str = token;
                        m_Tokenizer.UngetToken(); // restore the identifier
                    }
                }
            }

            return;
        }

        if (isUnnamed && !m_ParsingTypedef)
        {
            // for unnamed enums, look if we already have "Unnamed", so we don't
            // add a new one for every unnamed enum we encounter, in this scope...
            newEnum = TokenExists(token, m_LastParent, tkEnum);
        }

        if (!newEnum) // either named or first unnamed enum
        {
            newEnum = DoAddToken(tkEnum, token, lineNr);
            newEnum->m_IsAnonymous = true;
        }

        level = m_Tokenizer.GetNestingLevel();
        m_Tokenizer.GetToken(); // skip {
    }
    else
    {
        if (token.GetChar(0) != ParserConsts::opbrace_chr)
        {
            return;
        }

        level = m_Tokenizer.GetNestingLevel() - 1; // we 've already entered the { block
    }

    int lineStart = m_Tokenizer.GetLineNumber();
    //Implementation for showing Enum values: resolves expressions, preprocessor and enum tokens.
    int enumValue = 0;
    bool updateValue = true;
    const TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);

    while (IS_ALIVE)
    {
        // process enumerators
        token = m_Tokenizer.GetToken();
        wxString peek = m_Tokenizer.PeekToken();

        if (token.IsEmpty() || peek.IsEmpty())
        {
            return;    //eof
        }

        if (token == ParserConsts::clbrace && level == m_Tokenizer.GetNestingLevel())
        {
            break;
        }

        // assignments (=xxx) are ignored by the tokenizer,
        // so we don't have to worry about them here,
        // if (peek==ParserConsts::comma || peek==ParserConsts::clbrace || peek==ParserConsts::colon)
        if (peek == ParserConsts::colon)
        {
            peek = SkipToOneOfChars(ParserConsts::equals + ParserConsts::commaclbrace);
        }

        if (peek == ParserConsts::equals)
        {
            m_Tokenizer.GetToken(); //eat '='
            long result;
            updateValue = false;

            if (CalcEnumExpression(newEnum, result, peek))
            {
                enumValue = result;
                updateValue = true;
            }
        }

        if (peek == ParserConsts::comma || peek == ParserConsts::clbrace)
        {
            // this "if", avoids non-valid enumerators
            // like a comma (if no enumerators follow)
            if (wxIsalpha(token.GetChar(0))
                    || (token.GetChar(0) == ParserConsts::underscore_chr))
            {
                wxString args;

                if (updateValue)
                {
                    args << enumValue++;
                }

                Token * lastParent = m_LastParent;
                m_LastParent = newEnum;
                Token * enumerator = DoAddToken(tkEnumerator, token, m_Tokenizer.GetLineNumber(), 0, 0, args);
                enumerator->m_Scope = isEnumClass ? tsPrivate : tsPublic;
                m_LastParent = lastParent;
            }
        }
    }

    m_Tokenizer.SetState(oldState);
    newEnum->m_ImplLine      = lineNr;
    newEnum->m_ImplLineStart = lineStart;
    newEnum->m_ImplLineEnd   = m_Tokenizer.GetLineNumber();
    //    // skip to ;
    //    SkipToOneOfChars(ParserConsts::semicolon);
}

bool LSP_SymbolsParser::CalcEnumExpression(Token * tokenParent, long & result, wxString & peek)
{
    // need to force the tokenizer skip raw expression
    const TokenizerState oldState = m_Tokenizer.GetState();
    // expand macros, but don't read a single parentheses
    m_Tokenizer.SetState(tsRawExpression);
    Expression exp;
    wxString token, next;

    while (IS_ALIVE)
    {
        token = m_Tokenizer.GetToken();

        if (token.IsEmpty())
        {
            return false;
        }

        if (token == _T("\\"))
        {
            continue;
        }

        if (token == ParserConsts::comma || token == ParserConsts::clbrace)
        {
            m_Tokenizer.UngetToken();
            peek = token;
            break;
        }

        if (token == ParserConsts::dcolon)
        {
            peek = SkipToOneOfChars(ParserConsts::commaclbrace);
            m_Tokenizer.UngetToken();
            exp.Clear();
            break;
        }

        if (wxIsalpha(token[0]) || token[0] == ParserConsts::underscore_chr) // handle enum or macro
        {
            const Token * tk = m_TokenTree->at(m_TokenTree->TokenExists(token, tokenParent->m_Index, tkEnumerator));

            if (tk) // the enumerator token
            {
                if (!tk->m_Args.IsEmpty() && wxIsdigit(tk->m_Args[0]))
                {
                    token = tk->m_Args;    // add the value to exp
                }
            }
            else
            {
                peek = SkipToOneOfChars(ParserConsts::commaclbrace);
                m_Tokenizer.UngetToken();
                exp.Clear();
                break;
            }
        }

        // only remaining number now
        if (!token.StartsWith(_T("0x")))
        {
            exp.AddToInfixExpression(token);
        }
        else
        {
            long value;

            if (token.ToLong(&value, 16))
            {
                exp.AddToInfixExpression(wxString::Format(_T("%ld"), value));
            }
            else
            {
                peek = SkipToOneOfChars(ParserConsts::commaclbrace);
                exp.Clear();
                break;
            }
        }
    }

    // reset tokenizer's functionality
    m_Tokenizer.SetState(oldState);
    exp.ConvertInfixToPostfix();

    if (exp.CalcPostfix() && exp.GetStatus())
    {
        result = exp.GetResult();
        return true;
    }

    return false;
}

void LSP_SymbolsParser::HandleTypedef()
{
    // typedefs are handled as tkClass and we put the typedef'd type as the
    // class's ancestor. This way, it will work through inheritance.
    // Function pointers are a different beast and are handled differently.
    // this is going to be tough :(
    // let's see some examples:
    //
    // relatively easy:
    // typedef unsigned int uint32;
    // typedef std::map<String, StringVector> AnimableDictionaryMap;
    // typedef class|struct|enum [name] {...} type;
    //
    // special case of above:
    // typedef struct __attribute__((packed)) _PSTRUCT {...} PSTRUCT;
    //
    // harder:
    // typedef void dMessageFunction (int errnum, const char *msg, va_list ap);
    //
    // even harder:
    // typedef void (*dMessageFunction)(int errnum, const char *msg, va_list ap);
    // or
    // typedef void (MyClass::*Function)(int);
    size_t lineNr = m_Tokenizer.GetLineNumber();
    bool is_function_pointer = false;
    wxString typ;
    std::queue<wxString> components;
    // get everything on the same line
    TRACE(_T("HandleTypedef() : Typedef start"));
    wxString args;
    wxString token;
    wxString peek;
    m_ParsingTypedef = true;

    while (IS_ALIVE)
    {
        token = m_Tokenizer.GetToken();
        peek  = m_Tokenizer.PeekToken();
        TRACE(_T("HandleTypedef() : token=%s, peek=%s"), token.wx_str(), peek.wx_str());

        if (token.IsEmpty() || token == ParserConsts::semicolon)
        {
            m_Tokenizer.UngetToken();   // NOTE: preserve ';' for the next GetToken();
            break;
        }

        if (token == ParserConsts::kw_const)
        {
            continue;
        }

        if (token == ParserConsts::kw_class
                || token == ParserConsts::kw_struct
                || token == ParserConsts::kw_union)
        {
            // "typedef struct|class|union"
            TRACE(_("HandleTypedef() : Before HandleClass m_LastUnnamedTokenName='%s'"), m_LastUnnamedTokenName.wx_str());
            HandleClass(token == ParserConsts::kw_class ? ctClass :
                        token == ParserConsts::kw_union ? ctUnion :
                        ctStructure);
            token = m_LastUnnamedTokenName;
            TRACE(_("HandleTypedef() : After HandleClass m_LastUnnamedTokenName='%s'"), m_LastUnnamedTokenName.wx_str());
        }
        else
            if (token == ParserConsts::ptr || token == ParserConsts::ref)
            {
                m_PointerOrRef << token;
                continue;
            }
            else
                if (peek == ParserConsts::comma)
                {
                    m_Tokenizer.UngetToken();

                    if (components.size() != 0)
                    {
                        wxString ancestor;

                        while (components.size() > 0)
                        {
                            wxString tempToken = components.front();
                            components.pop();

                            if (!ancestor.IsEmpty())
                            {
                                ancestor << ParserConsts::space_chr;
                            }

                            ancestor << tempToken;
                        }

                        if (!ReadClsNames(ancestor))
                        {
                            TRACE(_T("HandleTypedef() : ReadClsNames returned false."));
                            m_Tokenizer.GetToken(); // eat it
                        }
                    }
                }
                else
                    if (token == ParserConsts::kw_enum)
                    {
                        // "typedef enum"
                        HandleEnum();
                        token = m_LastUnnamedTokenName;
                    }

        // keep namespaces together
        while (peek == ParserConsts::dcolon)
        {
            token << peek;
            m_Tokenizer.GetToken(); // eat it
            token << m_Tokenizer.GetToken(); // get what's next
            peek = m_Tokenizer.PeekToken();
        }

        if (token.GetChar(0) == ParserConsts::opbracket_chr)
        {
            // function pointer (probably)
            is_function_pointer = true;

            if (peek.GetChar(0) == ParserConsts::opbracket_chr)
            {
                // typedef void (*dMessageFunction)(int errnum, const char *msg, va_list ap);
                // typedef void (MyClass::*Function)(int);
                // remove parentheses and keep everything after the dereferencing symbol
                token.RemoveLast();
                int pos = token.Find(ParserConsts::ptr_chr, true);

                if (pos != wxNOT_FOUND)
                {
                    typ << ParserConsts::opbracket_chr
                        << token.Mid(1, pos)
                        << ParserConsts::clbracket_chr;
                    token.Remove(0, pos + 1);
                }
                else
                {
                    typ = _T("(*)");
                    token.Remove(0, 1); // remove opening parenthesis
                }

                args = peek;
                m_Tokenizer.GetToken(); // eat args
                TRACE(_("HandleTypedef() : Pushing component='%s' (typedef args='%s')"), token.Trim(true).Trim(false).wx_str(), args.wx_str());
                components.push(token.Trim(true).Trim(false));
            }
            else
            {
                // typedef void dMessageFunction (int errnum, const char *msg, va_list ap);
                // last component is already the name and this is the args
                args = token;
                TRACE(_("HandleTypedef() : Typedef args='%s'"), args.wx_str());
            }

            break;
        }

        TRACE(_("HandleTypedef() : Pushing component='%s', typedef args='%s'"), token.Trim(true).Trim(false).wx_str(), args.wx_str());
        components.push(token.Trim(true).Trim(false));

        // skip templates <>
        if (peek == ParserConsts::lt)
        {
            GetTemplateArgs();
            continue;
        }

        TRACE(_T(" + '%s'"), token.wx_str());
    }

    TRACE(_T("HandleTypedef() : Typedef done"));
    m_ParsingTypedef = false;

    if (components.empty())
    {
        return;    // invalid typedef
    }

    if (!is_function_pointer && components.size() <= 1)
    {
        return;    // invalid typedef
    }

    // now get the type
    wxString ancestor;
    wxString alias;

    // handle the special cases below, a template type parameter is used in typedef
    // template<typename _Tp>
    // class c2
    // {
    //    public:
    //        typedef _Tp  alise;
    //
    // };
    if ((components.size() == 2)
            && m_LastParent
            && m_LastParent->m_TokenKind == tkClass
            && (!m_LastParent->m_TemplateType.IsEmpty())
            && m_LastParent->m_TemplateType.Index(components.front()) != wxNOT_FOUND)
    {
        wxArrayString templateType = m_LastParent->m_TemplateType;
        alias = components.front();
        components.pop();
        ancestor = components.front();
    }
    else
    {
        while (components.size() > 1)
        {
            token = components.front();
            components.pop();

            if (!ancestor.IsEmpty())
            {
                ancestor << ParserConsts::space_chr;
            }

            ancestor << token;
        }
    }

    // no return type
    m_Str.Clear();
    TRACE(_("HandleTypedef() : Adding typedef: name='%s', ancestor='%s', args='%s'"), components.front().wx_str(), ancestor.wx_str(), args.wx_str());
    Token * tdef = DoAddToken(tkTypedef /*tkClass*/, components.front(), lineNr, 0, 0, args);

    if (tdef)
    {
        wxString actualAncestor = ancestor.BeforeFirst(ParserConsts::lt_chr).Trim();
        TRACE(_("HandleTypedef() : Ancestor='%s', actual ancestor='%s'"), ancestor.wx_str(), actualAncestor.wx_str());

        if (is_function_pointer)
        {
            tdef->m_FullType        = ancestor + typ; // + args;
            tdef->m_BaseType        = actualAncestor;

            if (tdef->IsValidAncestor(ancestor))
            {
                tdef->m_AncestorsString = ancestor;
            }
        }
        else
        {
            tdef->m_FullType        = ancestor;
            tdef->m_BaseType        = actualAncestor;
            tdef->m_TemplateAlias   = alias;
            TRACE(_T("The typedef alias is %s."), tdef->m_TemplateAlias.wx_str());

            if (tdef->IsValidAncestor(ancestor))
            {
                tdef->m_AncestorsString = ancestor;
            }

            if (!m_TemplateArgument.IsEmpty())
            {
                ResolveTemplateArgs(tdef);
            }
        }
    }
}

bool LSP_SymbolsParser::ReadVarNames()
{
    bool success = true; // optimistic start value

    while (IS_ALIVE)
    {
        wxString token = m_Tokenizer.GetToken();

        if (token.IsEmpty())                     // end of file / tokens
        {
            break;
        }

        if (token == ParserConsts::comma)        // another variable name
        {
            continue;
        }
        else
            if (token == ParserConsts::semicolon) // end of variable name(s)
            {
                m_PointerOrRef.Clear();
                break;
            }
            else
                if (token == ParserConsts::oparray)
                {
                    SkipToOneOfChars(ParserConsts::clarray);
                }
                else
                    if (token == ParserConsts::ptr)     // variable is a pointer
                    {
                        m_PointerOrRef << token;
                    }
                    else
                        if (wxIsalpha(token.GetChar(0))
                                || (token.GetChar(0) == ParserConsts::underscore_chr))
                        {
                            TRACE(_T("ReadVarNames() : Adding variable '%s' as '%s' to '%s'"),
                                  token.wx_str(), m_Str.wx_str(),
                                  (m_LastParent ? m_LastParent->m_Name.wx_str() : _T("<no-parent>")));

                            // Detects anonymous ancestor and gives him a name based on the first found alias.
                            if (m_Str.StartsWith(g_UnnamedSymbol))
                            {
                                RefineAnonymousTypeToken(tkUndefined, token);
                            }

                            Token * newToken = DoAddToken(tkVariable, token, m_Tokenizer.GetLineNumber());

                            if (!newToken)
                            {
                                TRACE(_T("ReadVarNames() : Unable to create/add new token: ") + token);
                                break;
                            }
                        }
                        else // unexpected
                        {
                            TRACE(F(_T("ReadVarNames() : Unexpected token '%s' for '%s', file '%s', line %d."),
                                    token.wx_str(), m_Str.wx_str(), m_Tokenizer.GetFilename().wx_str(), m_Tokenizer.GetLineNumber()));
                            CCLogger::Get()->DebugLog(wxString::Format(_T("ReadVarNames() : Unexpected token '%s' for '%s', file '%s', line %d."),
                                                                       token.wx_str(), m_Str.wx_str(), m_Tokenizer.GetFilename().wx_str(), m_Tokenizer.GetLineNumber()));
                            success = false;
                            break;
                        }
    }

    return success;
}

bool LSP_SymbolsParser::ReadClsNames(wxString & ancestor)
{
    bool success = true; // optimistic start value

    while (IS_ALIVE)
    {
        wxString token = m_Tokenizer.GetToken();

        if (token.IsEmpty())                     // end of file / tokens
        {
            break;
        }

        if (token == ParserConsts::comma)        // another class name
        {
            continue;
        }
        else
            if (token == ParserConsts::kw_attribute)
            {
                m_Tokenizer.GetToken();  // eat (( whatever ))
                continue;
            }
            else
                if (token == ParserConsts::semicolon) // end of class name(s)
                {
                    m_Tokenizer.UngetToken();
                    m_PointerOrRef.Clear();
                    break;
                }
                else
                    if (token == ParserConsts::ptr)     // variable is a pointer
                    {
                        m_PointerOrRef << token;
                    }
                    else
                        if (wxIsalpha(token.GetChar(0))
                                || (token.GetChar(0) == ParserConsts::underscore_chr))
                        {
                            TRACE(_T("ReadClsNames() : Adding variable '%s' as '%s' to '%s'"),
                                  token.wx_str(),
                                  m_Str.wx_str(),
                                  (m_LastParent ? m_LastParent->m_Name.wx_str() : _T("<no-parent>")));
                            m_Str.clear();
                            m_Str = ancestor;

                            // Detects anonymous ancestor and gives him a name based on the first found alias.
                            if (m_Str.StartsWith(g_UnnamedSymbol))
                            {
                                RefineAnonymousTypeToken(tkTypedef | tkClass, token);
                                ancestor = m_Str;
                            }

                            Token * newToken = DoAddToken(tkTypedef, token, m_Tokenizer.GetLineNumber());

                            if (!newToken)
                            {
                                TRACE(_T("ReadClsNames() : Unable to create/add new token: ") + token);
                                break;
                            }
                            else
                            {
                                newToken->m_AncestorsString = ancestor;
                            }
                        }
                        else // unexpected
                        {
                            TRACE(F(_T("ReadClsNames() : Unexpected token '%s' for '%s', file '%s', line %d."),
                                    token.wx_str(), m_Str.wx_str(), m_Tokenizer.GetFilename().wx_str(), m_Tokenizer.GetLineNumber()));
                            CCLogger::Get()->DebugLog(wxString::Format(_T("ReadClsNames() : Unexpected token '%s' for '%s', file '%s', line %d."),
                                                                       token.wx_str(), m_Str.wx_str(), m_Tokenizer.GetFilename().wx_str(), m_Tokenizer.GetLineNumber()));
                            // The following code snippet freezes CC here:
                            // typedef std::enable_if<N > 1, get_type_N<N-1, Tail...>> type;
                            m_Tokenizer.UngetToken();
                            // Note: Do NOT remove m_Tokenizer.UngetToken();, otherwise it freezes somewhere else
                            success = false;
                            break;
                        }
    }

    return success;
}

bool LSP_SymbolsParser::GetBaseArgs(const wxString & args, wxString & baseArgs)
{
    const wxChar * ptr = args.wx_str(); // pointer to current char in args string
    wxString word;             // compiled word of last arg
    bool skip = false;         // skip the next char (do not add to stripped args)
    bool sym  = false;         // current char symbol
    bool one  = true;          // only one argument
    //   ( int abc = 5 , float * def )
    //        ^
    // ptr point to the next char of "int"
    // word = "int"
    // sym = true means ptr is point to an identifier like token
    // here, if we find an identifier like token which is "int", we just skip the next token
    // until we meet a "," or ")".
    TRACE(_T("GetBaseArgs() : args='%s'."), args.wx_str());
    baseArgs.Alloc(args.Len() + 1);

    // Verify ptr is valid (still within the range of the string)
    while (*ptr != ParserConsts::null)
    {
        switch (*ptr)
        {
            case ParserConsts::eol_chr:

                // skip the "\r\n"
                while (*ptr != ParserConsts::null && *ptr <= ParserConsts::space_chr)
                {
                    ++ptr;
                }

                break;

            case ParserConsts::space_chr:

                // take care of args like:
                // - enum     my_enum the_enum_my_enum
                // - const    int     the_const_int
                // - volatile long    the_volatile_long
                if ((word == ParserConsts::kw_enum)
                        || (word == ParserConsts::kw_const)
                        || (word == ParserConsts::kw_volatile))
                {
                    skip = false;    // don't skip this (it's part of the stripped arg)
                }
                else
                {
                    skip = true;    // safely skip this as it is the args name
                }

                word = _T(""); // reset
                sym  = false;
                break;

            case ParserConsts::ptr_chr: // handle pointer args

                // handle multiple pointer like in: main (int argc, void** argv)
                // or ((int *, char ***))
                while (*(ptr + 1) != ParserConsts::null && *(ptr + 1) == ParserConsts::ptr_chr)
                {
                    baseArgs << *ptr; // append one more '*' to baseArgs
                    ptr++; // next char
                }

            // ...and fall through:
            case ParserConsts::ref_chr: // handle references
                word = _T(""); // reset
                skip = true;
                sym  = true;
                // TODO (Morten#5#): Do comment the following even more. It's still not exactly clear to me...
                // verify completeness of last stripped argument (handle nested brackets correctly)
                {
                    // extract last stripped argument from baseArgs
                    wxString lastStrippedArg;
                    int lastArgComma = baseArgs.Find(ParserConsts::comma_chr, true);

                    if (lastArgComma)
                    {
                        lastStrippedArg = baseArgs.Mid(1);
                    }
                    else
                    {
                        lastStrippedArg = baseArgs.Mid(lastArgComma);
                    }

                    // input:  (float a = 0.0, int* f1(char x, char y), void z)
                    // output: (float, int*, z)
                    // the internal "(char x, char y)" should be removed
                    // No opening brackets in last stripped arg?
                    // this means if we have function pointer in the argument
                    // input:   void foo(double (*fn)(double))
                    // we should not skip the content after '*', since the '(' before '*' is already
                    // pushed to the lastStrippedArg.
                    if (lastStrippedArg.Find(ParserConsts::opbracket_chr) == wxNOT_FOUND)
                    {
                        baseArgs << *ptr; // append to baseArgs
                        // find end
                        int brackets = 0;
                        ptr++; // next char

                        while (*ptr != ParserConsts::null)
                        {
                            if (*ptr == ParserConsts::opbracket_chr)
                            {
                                brackets++;
                            }
                            else
                                if (*ptr == ParserConsts::clbracket_chr)
                                {
                                    if (brackets == 0)
                                    {
                                        break;
                                    }

                                    brackets--;
                                }
                                else
                                    if (*ptr == ParserConsts::comma_chr && brackets == 0)
                                    {
                                        // don't stop at the inner comma char, such as '^' pointed below
                                        // (float a = 0.0, int* f1(char x, char y), void z)
                                        //                               ^
                                        skip = false;
                                        break;
                                    }

                            ptr++; // next char
                        }
                    }
                }
                break;

            case ParserConsts::colon_chr: // namespace handling like for 'std::vector'
                skip = false;
                sym  = true;
                break;

            case ParserConsts::oparray_chr: // array handling like for 'int[20]'

                // [   128   ]  ->   [128]
                // space between the [] is stripped
                while (*ptr != ParserConsts::null
                        && *ptr != ParserConsts::clarray_chr)
                {
                    if (*ptr != ParserConsts::space_chr)
                    {
                        baseArgs << *ptr;    // append to baseArgs, skipping spaces
                    }

                    ptr++; // next char
                }

                skip = true;
                sym  = true;
                break;

            case ParserConsts::lt_chr: // template arg handling like for 'vector<int>'

                // <   int   >  ->   <int>
                // space between the <> is stripped
                // note that embeded <> such as vector<vector<int>> is not handled here
                while (*ptr != ParserConsts::null
                        && *ptr != ParserConsts::gt_chr)
                {
                    if (*ptr != ParserConsts::space_chr)
                    {
                        baseArgs << *ptr;    // append to baseArgs, skipping spaces
                    }

                    ptr++; // next char
                }

                skip = true;
                sym  = true;
                break;

            case ParserConsts::comma_chr:     // fall through
            case ParserConsts::clbracket_chr: // fall through
            case ParserConsts::opbracket_chr:

                // ( int abc, .....)
                // we have just skip the "abc", and now, we see the ","
                if (skip && *ptr == ParserConsts::comma_chr)
                {
                    one = false;    // see a comma, which means we have at least two parameter!
                }

                // try to remove the __attribute__(xxx) decoration in the parameter
                // such as: int f(__attribute__(xxx) wxCommandEvent & event);
                // should be convert to : int f(wxCommandEvent & event);
                if (*ptr == ParserConsts::opbracket_chr && word == ParserConsts::kw_attribute)
                {
                    // remove the "__attribute__" keywords from the baseArgs
                    // the length of "__attribute__" is 13
                    baseArgs = baseArgs.Mid(0, baseArgs.Len() - 13);
                    // skip the next "(xxx)
                    int brackets = 1; // skip the first "(" already
                    ptr++; // next char

                    while (*ptr != ParserConsts::null)
                    {
                        if (*ptr == ParserConsts::opbracket_chr)
                        {
                            brackets++;
                        }
                        else
                            if (*ptr == ParserConsts::clbracket_chr)
                            {
                                brackets--;

                                if (brackets == 0)
                                {
                                    ptr++;
                                    break;
                                }
                            }

                        ptr++; // next char
                    }

                    // skip the spaces after the "__attribute__(xxx)"
                    while (*ptr     != ParserConsts::null
                            && *(ptr) == ParserConsts::space_chr)
                    {
                        ++ptr; // next char
                    }

                    word = _T(""); // reset
                    sym  = false;  // no symbol is added
                    skip = false;  // don't skip the next token
                    break;
                }

                word = _T(""); // reset
                sym  = true;
                skip = false;
                break;

            default:
                sym = false;
        }// switch (*ptr)

        // Now handle the char processed in this loop:
        if (!skip || sym)
        {
            // append to stripped argument and save the last word
            // (it's probably a type specifier like 'const' or alike)
            if (*ptr != ParserConsts::null)
            {
                baseArgs << *ptr; // append to baseArgs

                if (wxIsalnum(*ptr) || *ptr == ParserConsts::underscore_chr)
                {
                    word << *ptr;    // append to word
                }
            }
        }

        if (!skip && sym)
        {
            // skip white spaces and increase pointer
            while (*ptr     != ParserConsts::null
                    && *(ptr + 1) == ParserConsts::space_chr)
            {
                ++ptr; // next char
            }
        }

        if (*ptr != ParserConsts::null)
        {
            ++ptr; // next char
        }
    }

    if (one && baseArgs.Len() > 2)
    {
        const wxChar ch = baseArgs[1];

        if ((ch <= _T('9') && ch >= _T('0'))                // number, 0 ~ 9
                || baseArgs.Find(_T('"')) != wxNOT_FOUND    // string
                || baseArgs.Find(_T('\'')) != wxNOT_FOUND)  // character
        {
            return false; // not function, it should be variable
        }

        if (baseArgs == _T("(void)"))
        {
            baseArgs = _T("()");
        }
    }

    TRACE(_T("GetBaseArgs() : baseArgs='%s'."), baseArgs.wx_str());
    return true;
}

void LSP_SymbolsParser::GetTemplateArgs()
{
    // need to force the tokenizer _not_ skip anything
    // otherwise default values for template params would cause us to miss everything (because of the '=' symbol)
    TokenizerState oldState = m_Tokenizer.GetState();
    m_Tokenizer.SetState(tsNormal);
    m_TemplateArgument.clear();
    int nestLvl = 0;

    // NOTE: only exit this loop with 'break' so the tokenizer's state can
    // be reset afterwards (i.e. don't use 'return')
    while (IS_ALIVE)
    {
        wxString tmp = m_Tokenizer.GetToken();

        if (tmp == ParserConsts::lt)
        {
            ++nestLvl;
            m_TemplateArgument << tmp;
        }
        else
            if (tmp == ParserConsts::gt)
            {
                --nestLvl;
                m_TemplateArgument << tmp;
            }
            else
                if (tmp == ParserConsts::semicolon)
                {
                    // unget token - leave ; on the stack
                    m_Tokenizer.UngetToken();
                    m_TemplateArgument.clear();
                    break;
                }
                else
                    if (tmp.IsEmpty())
                    {
                        break;
                    }
                    else
                    {
                        m_TemplateArgument << tmp;
                    }

        if (nestLvl <= 0)
        {
            break;
        }
    }

    // reset tokenizer's functionality
    m_Tokenizer.SetState(oldState);
}

void LSP_SymbolsParser::ResolveTemplateArgs(Token * newToken)
{
    TRACE(_T("The variable template arguments are '%s'."), m_TemplateArgument.wx_str());
    newToken->m_TemplateArgument = m_TemplateArgument;
    wxArrayString actuals;
    SplitTemplateActualParameters(m_TemplateArgument, actuals);

    for (size_t i = 0; i < actuals.GetCount(); ++i)
    {
        TRACE(_T("The template actual arguments are '%s'."), actuals[i].wx_str());
    }

    newToken->m_TemplateType = actuals;
    // now resolve the template normal and actual map
    // wxString parentType = m_Str;
    std::map<wxString, wxString> templateMap;
    ResolveTemplateMap(newToken->m_FullType, actuals, templateMap);
    newToken->m_TemplateMap = templateMap;
}

wxArrayString LSP_SymbolsParser::GetTemplateArgArray(const wxString & templateArgs, bool remove_gt_lt, bool add_last)
{
    wxString word;
    wxString args = templateArgs;
    args.Trim(true).Trim(false);

    if (remove_gt_lt)
    {
        args.Remove(0, 1);
        args.RemoveLast();
    }

    wxArrayString container;

    for (size_t i = 0; i < args.Len(); ++i)
    {
        wxChar arg = args.GetChar(i);

        switch (arg)
        {
            case ParserConsts::space_chr:
                container.Add(word);
                word.clear();
                continue;

            case ParserConsts::lt_chr:
            case ParserConsts::gt_chr:
            case ParserConsts::comma_chr:
                container.Add(word);
                word.clear();
                container.Add(args[i]);
                continue;

            default:
                word << args[i];
        }
    }

    if (add_last && !word.IsEmpty())
    {
        container.Add(word);
    }

    return container;
}

void LSP_SymbolsParser::SplitTemplateFormalParameters(const wxString & templateArgs, wxArrayString & formals)
{
    wxArrayString container = GetTemplateArgArray(templateArgs, false, false);
    size_t n = container.GetCount();

    for (size_t j = 0; j < n; ++j)
    {
        if ((container[j] == ParserConsts::kw_typename)
                || (container[j] == ParserConsts::kw_class))
        {
            if ((j + 1) < n)
            {
                formals.Add(container[j + 1]);
                ++j; // skip
            }
        }
    }
}

void LSP_SymbolsParser::SplitTemplateActualParameters(const wxString & templateArgs, wxArrayString & actuals)
{
    wxArrayString container = GetTemplateArgArray(templateArgs, true, true);
    size_t n = container.GetCount();

    // for debug purposes only
    for (size_t j = 0; j < n; ++j)
    {
        TRACE(_T("The container elements are '%s'."), container[j].wx_str());
    }

    int level = 0;

    for (size_t j = 0; j < n; ++j)
    {
        if (container[j] == ParserConsts::lt)
        {
            ++level;

            while (level > 0 && (j + 1) < n)
            {
                if (container[j] == ParserConsts::gt)
                {
                    --level;
                }

                ++j; // skip
            }
        }
        else
            if (container[j] == ParserConsts::comma)
            {
                ++j; // skip
                continue;
            }
            else
            {
                actuals.Add(container[j]);
            }

        ++j; // skip
    }
}

bool LSP_SymbolsParser::ResolveTemplateMap(const wxString & typeStr, const wxArrayString & actuals,
                                           std::map<wxString, wxString> & results)
{
    // Check if type is an alias template. If it is, then we use the actual type's template map.
    // For example, given:
    // template <class T> using AAA = BBB<T>;
    // AAA<MyClass> obj;
    // When handling obj, typeStr would equal AAA, but we would use BBB's template map.
    wxString tokenFullType = typeStr;
    TokenIdxSet fullTypeMatches;
    size_t matchesCount = m_TokenTree->FindMatches(tokenFullType, fullTypeMatches, true, false, tkTypedef);

    if (matchesCount > 0)
    {
        for (TokenIdxSet::const_iterator it = fullTypeMatches.begin(); it != fullTypeMatches.end(); ++it)
        {
            int id = (*it);
            Token * token = m_TokenTree->at(id);

            if (token->m_TokenKind == tkTypedef)
            {
                tokenFullType = token->m_FullType;

                // we are only interested in the type name, so remove the scope qualifiers
                if (tokenFullType.Find(_T("::")) != wxNOT_FOUND)
                {
                    tokenFullType = tokenFullType.substr(tokenFullType.Find(_T("::")) + 2);
                }

                break;
            }
        }
    }

    wxString parentType = tokenFullType;
    parentType.Trim(true).Trim(false);
    // Note that we only search by the type name, and we don't care about the scope qualifiers
    // I add this for temporary support of templates under std, I will write better code later.
    TokenIdxSet parentResult;
    size_t tokenCounts = m_TokenTree->FindMatches(parentType, parentResult, true, false, tkClass);

    if (tokenCounts > 0)
    {
        for (TokenIdxSet::const_iterator it = parentResult.begin(); it != parentResult.end(); ++it)
        {
            int id = (*it);
            Token * normalToken = m_TokenTree->at(id);

            if (normalToken)
            {
                // Get the formal template argument lists
                wxArrayString formals =  normalToken->m_TemplateType;

                for (size_t i = 0; i < formals.GetCount(); ++i)
                {
                    TRACE(_T("ResolveTemplateMap get the formal template arguments are '%s'."), formals[i].wx_str());
                }

                size_t n = formals.GetCount() < actuals.GetCount() ? formals.GetCount() : actuals.GetCount();

                for (size_t i = 0; i < n; ++i)
                {
                    results[formals[i]] = actuals[i];
                    TRACE(_T("In ResolveTemplateMap function the normal is '%s',the actual is '%s'."), formals[i].wx_str(), actuals[i].wx_str());
                }
            }
        }

        return (results.size() > 0) ? true : false;
    }
    else
    {
        return false;
    }
}

void LSP_SymbolsParser::RemoveTemplateArgs(const wxString & exp, wxString & expNoArgs, wxString & templateArgs)
{
    expNoArgs.clear();
    templateArgs.clear();
    int nestLvl = 0;

    for (unsigned int i = 0; i < exp.length(); i++)
    {
        wxChar c = exp[i];

        if (c == ParserConsts::lt_chr)
        {
            nestLvl++;
            templateArgs << c;
            continue;
        }

        if (c == ParserConsts::gt_chr)
        {
            nestLvl--;
            templateArgs << c;
            continue;
        }

        if (nestLvl == 0)
        {
            expNoArgs << c;
        }
        else
        {
            bool wanted = true;

            // don't add unwanted whitespaces, i.e. ws around '<' and '>'
            if (c == ParserConsts::space)
            {
                wxChar last = 0;
                wxChar next = 0;

                if (i > 0)
                {
                    last = exp[i - 1];
                }

                if (i < exp.length() - 1)
                {
                    next = exp[i + 1];
                }

                if (last == ParserConsts::gt || last == ParserConsts::lt)
                {
                    wanted = false;
                }

                if (next == ParserConsts::gt || next == ParserConsts::lt)
                {
                    wanted = false;
                }
            }

            if (wanted == true)
            {
                templateArgs << c;
            }
        }
    }
}

bool LSP_SymbolsParser::IsStillAlive(cb_unused const wxString & funcInfo)
{
    //const bool alive = !TestDestroy(); //(ph 2021/03/15)
    const bool alive = IS_ALIVE;

    if (!alive)
    {
        TRACE(_T("IsStillAlive() : %s "), funcInfo.wx_str());
        free(0);
    }

    return alive;
}

void LSP_SymbolsParser::RefineAnonymousTypeToken(short int typeMask, wxString alias)
{
    // we expect the m_Str are storing the unnamed type token, like UnnamedClassAA_BBB
    // AA is the file index, BBB is the unnamed token index
    // now, we are going to rename its name to classAA_CCC, CCC is the alias name
    Token * unnamedAncestor = TokenExists(m_Str, m_LastParent, typeMask);

    if (unnamedAncestor && unnamedAncestor->m_IsAnonymous) // Unnamed ancestor found - rename it to something useful.
    {
        if (m_Str.Contains(_T("Union")))
        {
            m_Str = _T("union");
        }
        else
            if (m_Str.Contains(_T("Struct")))
            {
                m_Str = _T("struct");
            }
            else
            {
                m_Str = _T("tag");
            }

        m_Str << m_FileIdx << _T("_") << alias;
        m_TokenTree->RenameToken(unnamedAncestor, m_Str);
    }
}

wxString LSP_SymbolsParser::ReadAngleBrackets()
{
    wxString str = m_Tokenizer.GetToken();

    if (str != wxT("<"))
    {
        return wxEmptyString;
    }

    int level = 1; // brace level of '<' and '>'

    while (m_Tokenizer.NotEOF())
    {
        wxString token = m_Tokenizer.GetToken();

        if (token == _T("<"))
        {
            ++level;
            str << token;
        }
        else
            if (token == _T(">"))
            {
                --level;
                str << token;

                if (level == 0)
                {
                    break;
                }
            }
            else
                if (token == _T("*") || token == _T("&") || token == _T(","))
                {
                    str << token;
                }
                else
                {
                    if (str.Last() == _T('<')) // there is no space between '(' and the following token
                    {
                        str << token;
                    }
                    else                       // otherwise, a space is needed
                    {
                        str << _T(" ") << token;
                    }
                }

        if (level == 0)
        {
            break;
        }
    }//while (NotEOF())

    return str;
}
// ----------------------------------------------------------------------------
void LSP_SymbolsParser::DoParse()
// ----------------------------------------------------------------------------
{
    /// deprecated - not used for LSP clangd
#if defined(cbDEBUG)
    cbAssertNonFatal(0 && "This DoParse() is deprecated.");
#endif
    return ;
    //// need to reset tokenizer's behaviour
    //// don't forget to reset that if you add any early exit condition!
    //TokenizerState oldState = m_Tokenizer.GetState();
    //m_Tokenizer.SetState(tsNormal);
    //
    //m_Str.Clear();
    //m_LastToken.Clear();
    //m_LastUnnamedTokenName.Clear();
    //
    //// Notice: clears the queue "m_EncounteredTypeNamespaces"
    //while (!m_EncounteredTypeNamespaces.empty())
    //    m_EncounteredTypeNamespaces.pop();
    //
    //// Notice: clears the queue "m_EncounteredNamespaces"
    //while (!m_EncounteredNamespaces.empty())
    //    m_EncounteredNamespaces.pop();
    //
    //while (m_Tokenizer.NotEOF() && IS_ALIVE)
    //{
    //    wxString token = m_Tokenizer.GetToken();
    //    if (token.IsEmpty())
    //        continue;
    //
    //    TRACE(_T("DoParse() : Loop:m_Str='%s', token='%s'"), m_Str.wx_str(), token.wx_str());
    //
    //    bool switchHandled = true;
    //    switch (token.Length())
    //    {
    //        // ---------------------------------------------------------------
    //        // token length of 1
    //        // ---------------------------------------------------------------
    //        case 1:
    //        switch (static_cast<wxChar>(token[0]))
    //        {
    //        case ParserConsts::semicolon_chr:
    //            {
    //                m_Str.Clear();
    //                m_PointerOrRef.Clear();
    //                // Notice: clears the queue "m_EncounteredTypeNamespaces"
    //                while (!m_EncounteredTypeNamespaces.empty())
    //                    m_EncounteredTypeNamespaces.pop();
    //                m_TemplateArgument.Clear();
    //            }
    //            break;
    //
    //        case ParserConsts::dot_chr:
    //            {
    //                m_Str.Clear();
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace);
    //            }
    //            break;
    //
    //        case ParserConsts::gt_chr:
    //            {
    //                if (m_LastToken == ParserConsts::dash)
    //                {
    //                    m_Str.Clear();
    //                    SkipToOneOfChars(ParserConsts::semicolonclbrace);
    //                }
    //                else
    //                    switchHandled = false;
    //            }
    //            break;
    //
    //        case ParserConsts::opbrace_chr:
    //            {
    //                if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                    SkipBlock();
    //                m_Str.Clear();
    //            }
    //            break;
    //
    //        case ParserConsts::clbrace_chr:
    //            {
    //                m_LastParent = 0L;
    //                m_LastScope = tsUndefined;
    //                m_Str.Clear();
    //                // the only time we get to find a } is when recursively called by e.g. HandleClass
    //                // we have to return now...
    //                if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                {
    //                    m_Tokenizer.SetState(oldState); // This uses the top-level oldState (renamed shadowed versions below)
    //                    return;
    //                }
    //            }
    //            break;
    //
    //        case ParserConsts::colon_chr:
    //            {
    //                if      (m_LastToken == ParserConsts::kw_public)
    //                    m_LastScope = tsPublic;
    //                else if (m_LastToken == ParserConsts::kw_protected)
    //                    m_LastScope = tsProtected;
    //                else if (m_LastToken == ParserConsts::kw_private)
    //                    m_LastScope = tsPrivate;
    //                m_Str.Clear();
    //            }
    //            break;
    //
    //        case ParserConsts::hash_chr:
    //            {
    //                token = m_Tokenizer.GetToken();
    //                // only the ptOthers kinds of preprocessor directives will be passed here
    //                // see details in: Tokenizer::SkipPreprocessorBranch()
    //                // those could be: "#include" or "#warning" or "#xxx" and more
    //                if (token == ParserConsts::kw_include)
    //                    HandleIncludes();
    //                else // handle "#warning" or "#xxx" and more, just skip them
    //                    m_Tokenizer.SkipToEOL();
    //
    //                m_Str.Clear();
    //            }
    //            break;
    //
    //        case ParserConsts::ptr_chr:
    //        case ParserConsts::ref_chr:
    //            {
    //                m_PointerOrRef << token;
    //            }
    //            break;
    //
    //        case ParserConsts::equals_chr:
    //            {
    //                // pattern int a = 3;
    //                // m_Str.Clear();
    //                SkipToOneOfChars(ParserConsts::commasemicolonopbrace, true);
    //                m_Tokenizer.UngetToken();
    //            }
    //            break;
    //
    //        case ParserConsts::question_chr:
    //            {
    //                m_Str.Clear();
    //                SkipToOneOfChars(ParserConsts::semicolonopbrace, true);
    //            }
    //            break;
    //
    //        case ParserConsts::plus_chr:
    //            {
    //                m_Str.Clear();
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace);
    //            }
    //            break;
    //
    //        case ParserConsts::dash_chr:
    //            {
    //                if (m_LastToken == ParserConsts::dash)
    //                {
    //                    m_Str.Clear();
    //                    SkipToOneOfChars(ParserConsts::semicolonclbrace);
    //                }
    //            }
    //            break;
    //
    //        case ParserConsts::oparray_chr:
    //            {
    //                SkipToOneOfChars(ParserConsts::clarray);
    //            }
    //            break;
    //
    //        case ParserConsts::comma_chr:
    //            break;
    //
    //        default:
    //            switchHandled = false;
    //            break;
    //        }
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 2
    //        // ---------------------------------------------------------------
    //        case 2:
    //        if (token == ParserConsts::kw_if || token == ParserConsts::kw_do)
    //        {
    //            if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            else
    //                HandleConditionalArguments();
    //
    //            m_Str.Clear();
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 3
    //        // ---------------------------------------------------------------
    //        case 3:
    //        if (token == ParserConsts::kw_for)
    //        {
    //            if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            else
    //                HandleForLoopArguments();
    //
    //            m_Str.Clear();
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 4
    //        // ---------------------------------------------------------------
    //        case 4:
    //        if (token == ParserConsts::kw_else)
    //        {
    //            if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_enum)
    //        {
    //            m_Str.Clear();
    //            if (m_Options.handleEnums)
    //                HandleEnum();
    //            else
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //        }
    //        else if (token == ParserConsts::kw_case)
    //        {
    //            m_Str.Clear();
    //            SkipToOneOfChars(ParserConsts::colon, true, true);
    //        }
    //        else if (token == ParserConsts::kw___at)
    //        {
    //            m_Tokenizer.GetToken(); // skip arguments
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 5
    //        // ---------------------------------------------------------------
    //        case 5:
    //        if (token == ParserConsts::kw_while || token == ParserConsts::kw_catch)
    //        {
    //            if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            else
    //                HandleConditionalArguments();
    //
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_const)
    //        {
    //            m_Str << token << _T(" ");
    //        }
    //        else if (token==ParserConsts::kw_using)
    //        {
    //            // there are some kinds of using keyword usage
    //            // (1) using namespace A;
    //            // (2) using namespace A::B;
    //            // (3) using A::B;
    //            // (4) using A = B;
    //            token = m_Tokenizer.GetToken();
    //            wxString peek = m_Tokenizer.PeekToken();
    //            if (peek == ParserConsts::kw_namespace)
    //            {
    //                while (true) // support full namespaces
    //                {
    //                    m_Str << m_Tokenizer.GetToken();
    //                    if (m_Tokenizer.PeekToken() == ParserConsts::dcolon)
    //                        m_Str << m_Tokenizer.GetToken();
    //                    else
    //                        break;
    //                }
    //                if (    !m_Str.IsEmpty()
    //                     && m_LastParent != 0L
    //                     && m_LastParent->m_Index != -1
    //                     && m_LastParent->m_TokenKind == tkNamespace )
    //                {
    //                    if (m_LastParent->m_AncestorsString.IsEmpty())
    //                        m_LastParent->m_AncestorsString << m_Str;
    //                    else
    //                        m_LastParent->m_AncestorsString << ParserConsts::comma_chr << m_Str;
    //                }
    //                else if (   !m_Str.IsEmpty()
    //                         && (m_LastParent == 0 || m_LastParent->m_Index == -1) )
    //                {
    //                    // using namespace in global scope
    //                    // "using namespace first::second::third;"
    //
    //                    Token* foundNsToken = nullptr;
    //                    wxStringTokenizer tokenizer(m_Str, ParserConsts::dcolon);
    //                    while (tokenizer.HasMoreTokens())
    //                    {
    //                        std::queue<wxString> nsQuqe;
    //                        nsQuqe.push(tokenizer.GetNextToken());
    //                        foundNsToken = FindTokenFromQueue(nsQuqe, foundNsToken, true, foundNsToken);
    //                        foundNsToken->m_TokenKind = tkNamespace;
    //                    }
    //                    m_UsedNamespacesIds.insert(foundNsToken->m_Index);
    //                }
    //            }
    //            else if (peek == ParserConsts::equals)
    //            {
    //                // Type alias pattern: using AAA = BBB::CCC;
    //                // Handle same as a typedef
    //                wxString args;
    //                size_t lineNr = m_Tokenizer.GetLineNumber();
    //                Token* tdef = DoAddToken(tkTypedef, token, lineNr, 0, 0, args);
    //
    //                m_Tokenizer.GetToken(); // eat equals
    //                wxString type;
    //
    //                while (IS_ALIVE) // support full namespaces
    //                {
    //                    type << m_Tokenizer.GetToken();
    //                    if (m_Tokenizer.PeekToken() == ParserConsts::dcolon)
    //                        type << m_Tokenizer.GetToken();
    //                    else
    //                        break;
    //                }
    //
    //                if (tdef)
    //                {
    //                    tdef->m_FullType = type;
    //                    tdef->m_BaseType = type;
    //                    if (tdef->IsValidAncestor(type))
    //                        tdef->m_AncestorsString = type;
    //                }
    //            }
    //            else
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace);
    //
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_class)
    //        {
    //            m_Str.Clear();
    //            if (m_Options.handleClasses)
    //                HandleClass(ctClass);
    //            else
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //        }
    //        else if (token == ParserConsts::kw_union)
    //        {
    //            m_Str.Clear();
    //            if (m_Options.handleClasses)
    //                HandleClass(ctUnion);
    //            else
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 6
    //        // ---------------------------------------------------------------
    //        case 6:
    //        if (token == ParserConsts::kw_delete)
    //        {
    //            m_Str.Clear();
    //            SkipToOneOfChars(ParserConsts::semicolonclbrace);
    //        }
    //        else if (token == ParserConsts::kw_switch)
    //        {
    //            if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            else
    //                m_Tokenizer.GetToken(); // skip arguments
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_return)
    //        {
    //            SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_extern)
    //        {
    //            // check for "C", "C++"
    //            m_Str = m_Tokenizer.GetToken();
    //            if (m_Str == ParserConsts::kw__C_ || m_Str == ParserConsts::kw__CPP_)
    //            {
    //                if (m_Tokenizer.PeekToken() == ParserConsts::opbrace)
    //                {
    //                    m_Tokenizer.GetToken(); // "eat" {
    //                    DoParse(); // time for recursion ;)
    //                }
    //            }
    //            else
    //            {
    //                // do nothing, just skip keyword "extern", otherwise uncomment:
    //                //SkipToOneOfChars(ParserConsts::semicolon); // skip externs
    //                m_Tokenizer.UngetToken();
    //            }
    //            m_Str.Clear();
    //        }
    //        else if (   token == ParserConsts::kw_static
    //                 || token == ParserConsts::kw_inline )
    //        {
    //            // do nothing, just skip keyword "static" / "inline"
    //        }
    //        else if (token == ParserConsts::kw_friend)
    //        {
    //            // friend methods can be either the decl only or an inline implementation
    //            SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_struct)
    //        {
    //            m_Str.Clear();
    //            if (m_Options.handleClasses)
    //                HandleClass(ctStructure);
    //            else
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 7
    //        // ---------------------------------------------------------------
    //        case 7:
    //        if (token == ParserConsts::kw_typedef)
    //        {
    //            if (m_Options.handleTypedefs)
    //                HandleTypedef();
    //            else
    //                SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //            m_Str.Clear();
    //        }
    //        else if (token == ParserConsts::kw_virtual)
    //        {
    //            // do nothing, just skip keyword "virtual"
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 8
    //        // ---------------------------------------------------------------
    //        case 8:
    //        if (token == ParserConsts::kw_template)
    //        {
    //            // There are some template definitions that are not working like
    //            // within gcc headers (NB: This syntax is a GNU extension):
    //            // extern template
    //            //    const codecvt<char, char, mbstate_t>&
    //            //    use_facet<codecvt<char, char, mbstate_t> >(const locale&);
    //            // read <> as a whole token
    //            m_TemplateArgument = ReadAngleBrackets();
    //            TRACE(_T("DoParse() : Template argument='%s'"), m_TemplateArgument.wx_str());
    //            m_Str.Clear();
    //            if (m_Tokenizer.PeekToken() != ParserConsts::kw_class)
    //                m_TemplateArgument.clear();
    //        }
    //        else if (token == ParserConsts::kw_noexcept)
    //        {
    //            m_Str << token << _T(" ");
    //        }
    //        else if (token == ParserConsts::kw_operator)
    //        {
    //            wxString func = token;
    //            while (IS_ALIVE)
    //            {
    //                token = m_Tokenizer.GetToken();
    //                if (!token.IsEmpty())
    //                {
    //                    if (token.GetChar(0) == ParserConsts::opbracket_chr)
    //                    {
    //                        // check for operator()()
    //                        wxString peek = m_Tokenizer.PeekToken();
    //                        if (  !peek.IsEmpty()
    //                            && peek.GetChar(0) != ParserConsts::opbracket_chr)
    //                            m_Tokenizer.UngetToken();
    //                        else
    //                            func << token;
    //                        break;
    //                    }
    //                    else
    //                        func << token;
    //                }
    //                else
    //                    break;
    //            }
    //            HandleFunction(func, true);
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 9
    //        // ---------------------------------------------------------------
    //        case 9:
    //        if (token == ParserConsts::kw_namespace)
    //        {
    //            m_Str.Clear();
    //            HandleNamespace();
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 10
    //        // ---------------------------------------------------------------
    //        case 10:
    //        if (token == ParserConsts::kw_declspec)
    //        {
    //            // Handle stuff like:  int __declspec ((whatever)) fun();
    //            //  __declspec already be eat
    //            m_Tokenizer.GetToken();  // eat (( whatever ))
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // ---------------------------------------------------------------
    //        // token length of 13
    //        // ---------------------------------------------------------------
    //        case 13:
    //        if (token == ParserConsts::kw_attribute)
    //        {
    //            // Handle stuff like:  int __attribute__((whatever)) fun();
    //            //  __attribute__ already be eat
    //            m_Tokenizer.GetToken();  // eat (( whatever ))
    //        }
    //        else
    //            switchHandled = false;
    //        break;
    //
    //        // token length of other than 1 .. 13
    //        default:
    //            switchHandled = false;
    //        break;
    //    }
    //
    //    if (token.StartsWith(ParserConsts::kw___asm))
    //    {
    //        // Handle: __asm assembly-instruction [ ; ] OR
    //        //         __asm { assembly-instruction-list } [ ; ] OR
    //        //         __asm __volatile("assembly-instruction-list");
    //        // TODO: You can also put __asm in front of each assembly instruction:
    //        //       __asm mov al, 2
    //        //       __asm mov dx, 0xD007
    //        // OR:   __asm mov al, 2   __asm mov dx, 0xD007
    //        SkipToOneOfChars(ParserConsts::semicolon, true, true);
    //    }
    //    else if (!switchHandled)
    //    {
    //        // since we can't recognize the pattern by token, then the token
    //        // is normally an identifier style lexeme, now we try to peek the next token
    //        wxString peek = m_Tokenizer.PeekToken();
    //        if (!peek.IsEmpty())
    //        {
    //
    //            if (   (peek.GetChar(0) == ParserConsts::opbracket_chr)
    //                     && m_Options.handleFunctions )
    //            {
    //                if (   m_Str.IsEmpty()
    //                    && m_EncounteredNamespaces.empty()
    //                    && m_EncounteredTypeNamespaces.empty()
    //                    && (!m_LastParent || m_LastParent->m_Name != token) ) // if func has same name as current scope (class)
    //                {
    //                    // see what is inside the (...)
    //                    wxString arg = m_Tokenizer.GetToken(); // eat args ()
    //                    // try to see whether the peek pattern is (* BBB)
    //                    int pos = peek.find(ParserConsts::ptr);
    //                    if (pos != wxNOT_FOUND)
    //                    {
    //                        peek = m_Tokenizer.PeekToken();
    //                        // see whether there is a (...) after (* BBB)
    //                        if (peek.GetChar(0) == ParserConsts::opbracket_chr)
    //                        {
    //                            // pattern: AAA (* BBB) (...)
    //                            // where peek is (...) and arg is (* BBB)
    //
    //                            // NOTE: support func ptr in local block, show return type.
    //                            // if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                            //     HandleFunction(arg); // function
    //                            // AAA now becomes the last element of stacked type string
    //                            // which is the return type of function ptr
    //                            m_Str << token << ParserConsts::space_chr;
    //                            // BBB is now the function ptr's name
    //                            HandleFunction(/*function name*/ arg,
    //                                           /*isOperator*/    false,
    //                                           /*isPointer*/     true);
    //                        }
    //                    }
    //                    else // wxString arg = m_Tokenizer.GetToken(); // eat args ()
    //                        m_Str = token + arg;
    //                }
    //                // NOTE: support some more cases..., such as m_Str is not empty
    //                // if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                //     HandleFunction(token); // function
    //                // else
    //                //     m_Tokenizer.GetToken(); // eat args when parsing block
    //
    //                // list of function ptrs
    //                // eg: void (*fun1)(void), (*fun2)(size_t size);
    //                // where, m_Str=void, token=(*fun2), peek=(size_t size)
    //
    //                // function ptr with pointer return type
    //                // eg: void *(*Alloc)(void *p, size_t size);
    //                // where, m_Str=void, token=(*Alloc), peek=(void *p, size_t size)
    //                else if (token.GetChar(0) == ParserConsts::opbracket_chr)
    //                {
    //                    int pos = token.find(ParserConsts::ptr);
    //                    if (pos != wxNOT_FOUND)
    //                    {
    //                        wxString arg = token;
    //                        HandleFunction(/*function name*/ arg,
    //                                       /*isOperator*/    false,
    //                                       /*isPointer*/     true);
    //                    }
    //                }
    //                else if (   m_Options.useBuffer
    //                         && m_Str.GetChar(0) == ParserConsts::opbracket_chr
    //                         && m_Str.GetChar(m_Str.Len() - 2) == ParserConsts::clbracket_chr)
    //                {
    //                    // pattern: (void) fun (...)
    //                    // m_Str="(void) " token="fun" peek="(...)"
    //                    // this is a return value cast, we should reset the m_Str with fun
    //                    m_Str = token;
    //                }
    //                else
    //                {
    //                    // pattern unsigned int (*getClientLibVersion)(char** result);
    //                    // currently, m_Str = unsigned, token = int, peek = (*getClientLibVersion)
    //                    // this may be a function pointer declaration, we can guess without
    //                    // reading the next token, if "peek" has a ptr char and only 1 argument
    //                    // in it.
    //
    //                    // see what is inside the (...)
    //                    // try to see whether the peek pattern is (* BBB)
    //                    if (peek.GetChar(1) == ParserConsts::ptr)
    //                    {
    //                        wxString arg = peek;
    //                        m_Str << token;
    //                        token = m_Tokenizer.GetToken(); //consume the peek
    //                        // BBB is now the function ptr's name
    //                        HandleFunction(/*function name*/ arg,
    //                                       /*isOperator*/    false,
    //                                       /*isPointer*/     true);
    //                    }
    //                    else if (!m_Options.useBuffer || m_Options.bufferSkipBlocks)
    //                    {
    //                        // pattern AAA BBB (...) in global namespace (not in local block)
    //                        // so, this is mostly like a function declaration, but in-fact this
    //                        // can also be a global variable initialized with ctor, but for
    //                        // simplicity, we drop the later case
    //                        HandleFunction(token); // function
    //                    }
    //                    else
    //                    {
    //                        // local variables initialized with ctor
    //                        if (!m_Str.IsEmpty() && m_Options.handleVars)
    //                        {
    //                            Token* newToken = DoAddToken(tkVariable, token, m_Tokenizer.GetLineNumber());
    //                            if (newToken && !m_TemplateArgument.IsEmpty())
    //                                ResolveTemplateArgs(newToken);
    //                        }
    //                        m_Tokenizer.GetToken(); // eat args when parsing block
    //                    }
    //                }
    //            }
    //            else if (   (peek  == ParserConsts::colon)
    //                     && (token != ParserConsts::kw_private)
    //                     && (token != ParserConsts::kw_protected)
    //                     && (token != ParserConsts::kw_public) )
    //            {
    //                // example decl to encounter a colon is when defining a bitfield: int x:1,y:1,z:1;
    //                // token should hold the var (x/y/z)
    //                // m_Str should hold the type (int)
    //                if (m_Options.handleVars)
    //                    DoAddToken(tkVariable, token, m_Tokenizer.GetLineNumber());
    //
    //                m_Tokenizer.GetToken(); // skip colon
    //                m_Tokenizer.GetToken(); // skip bitfield
    //            }
    //            else if (peek==ParserConsts::comma)
    //            {
    //                // example decl to encounter a comma: int x,y,z;
    //                // token should hold the var (x/y/z)
    //                // m_Str should hold the type (int)
    //                if (!m_Str.IsEmpty() && m_Options.handleVars)
    //                {
    //                    Token* newToken = DoAddToken(tkVariable, token, m_Tokenizer.GetLineNumber());
    //                    if (newToken && !m_TemplateArgument.IsEmpty())
    //                        ResolveTemplateArgs(newToken);
    //                }
    //
    //                // else it's a syntax error; let's hope we can recover from this...
    //                // skip comma (we had peeked it)
    //                m_Tokenizer.GetToken();
    //            }
    //            else if (peek==ParserConsts::lt)
    //            {
    //                // a template, e.g. someclass<void>::memberfunc
    //                // we have to skip <>, so we 're left with someclass::memberfunc
    //                // about 'const' handle, e.g.
    //                /* template<typename T> class A{};
    //                   const A<int> var; */
    //                if (m_Str.IsEmpty() || m_Str.StartsWith(ParserConsts::kw_const))
    //                    GetTemplateArgs();
    //                else
    //                    SkipAngleBraces();
    //                peek = m_Tokenizer.PeekToken();
    //                if (peek==ParserConsts::dcolon)
    //                {
    //                    TRACE(_T("DoParse() : peek='::', token='") + token + _T("', m_LastToken='") + m_LastToken + _T("', m_Str='") + m_Str + _T("'"));
    //                    if (m_Str.IsEmpty())
    //                        m_EncounteredTypeNamespaces.push(token); // it's a type's namespace
    //                    else
    //                        m_EncounteredNamespaces.push(token);
    //                    m_Tokenizer.GetToken(); // eat ::
    //                }
    //                else // case like, std::map<int, int> somevar;
    //                    m_Str << token << ParserConsts::space_chr;
    //            }
    //            else if (peek==ParserConsts::dcolon)
    //            {
    //                wxString str_stripped(m_Str); str_stripped.Trim(true).Trim(false);
    //                if (   str_stripped.IsEmpty()
    //                    || str_stripped.IsSameAs(ParserConsts::kw_const)
    //                    || str_stripped.IsSameAs(ParserConsts::kw_volatile) ) // what else?!
    //                    m_EncounteredTypeNamespaces.push(token); // it's a type's namespace
    //                else
    //                    m_EncounteredNamespaces.push(token);
    //                m_Tokenizer.GetToken(); // eat ::
    //            }
    //            // NOTE: opbracket_chr already handled above
    //            else if (   peek==ParserConsts::semicolon
    //                     || peek==ParserConsts::oparray_chr
    //                     || peek==ParserConsts::equals_chr)
    //            {
    //                if (   !m_Str.IsEmpty()
    //                    && (    wxIsalpha(token.GetChar(0))
    //                        || (token.GetChar(0) == ParserConsts::underscore_chr) ) )
    //                {
    //                    // pattern: m_Str AAA;
    //                    // pattern: m_Str AAA[X][Y];
    //                    // pattern: m_Str AAA = BBB;
    //                    // where AAA is the variable name, m_Str contains type string
    //                    if (m_Options.handleVars)
    //                    {
    //                        Token* newToken = DoAddToken(tkVariable, token, m_Tokenizer.GetLineNumber());
    //                        if (newToken && !m_TemplateArgument.IsEmpty())
    //                            ResolveTemplateArgs(newToken);
    //                    }
    //                    else
    //                        SkipToOneOfChars(ParserConsts::semicolonclbrace, true, true);
    //                }
    //
    //                if (peek==ParserConsts::oparray_chr)
    //                    SkipToOneOfChars(ParserConsts::clarray);
    //                else if (peek==ParserConsts::equals_chr)
    //                {
    //                    SkipToOneOfChars(ParserConsts::commasemicolonopbrace, true);
    //                    m_Tokenizer.UngetToken();
    //                }
    //            }
    //            else if (!m_EncounteredNamespaces.empty())
    //            {
    //                // Notice: clears the queue "m_EncounteredNamespaces", too
    //                while (!m_EncounteredNamespaces.empty())
    //                {
    //                    m_EncounteredTypeNamespaces.push( m_EncounteredNamespaces.front() );
    //                    m_EncounteredNamespaces.pop();
    //                }
    //                m_Str = token;
    //            }
    //            else
    //                m_Str << token << ParserConsts::space_chr;
    //        }
    //    }
    //
    //    m_LastToken = token;
    //}
    //
    //// reset tokenizer behaviour
    //m_Tokenizer.SetState(oldState);
}//end DoParse()
