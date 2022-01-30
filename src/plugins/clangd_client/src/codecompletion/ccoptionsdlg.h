/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef CCOPTIONSDLG_H
#define CCOPTIONSDLG_H

#include <wx/intl.h>
#include "configurationpanel.h"
#include <settings.h>
#include "parsemanager.h"
#include "parser/parser.h"

class CodeCompletion;
class DocumentationHelper;

class CCOptionsDlg : public cbConfigurationPanel
{
public:
    CCOptionsDlg(wxWindow* parent, ParseManager* np, CodeCompletion* cc, DocumentationHelper* dh);
    ~CCOptionsDlg() override;

    wxString GetTitle() const override
    {
        return _("clangd_client");
    }
    wxString GetBitmapBaseName() const override
    {
        return _T("clangd_client");
    }
    void OnApply() override;
    void OnCancel() override {}

    void OnFindClangDaemonDir_Dlg(wxCommandEvent& event);
    void OnFindClangDir_Dlg(wxCommandEvent& event);

    void OnLLVM_ClangDaemon_AutoDetect(cb_unused wxCommandEvent& event);
    void OnLLVM_Clang_AutoDetect(cb_unused wxCommandEvent& event);
    void OnLLVM_ClangBoth_AutoDetect(cb_unused wxCommandEvent& event);

protected:
    void OnChooseColour(wxCommandEvent& event);
    void OnCCDelayScroll(wxScrollEvent& event);

    void OnUpdateUI(wxUpdateUIEvent& event);

private:
    void UpdateCCDelayLabel();
    bool ValidateReplacementToken(wxString& from, wxString& to);

    ParseManager*        m_ParseManager;
    CodeCompletion*      m_CodeCompletion;
    ParserBase&          m_Parser;
    DocumentationHelper* m_Documentation;

    DECLARE_EVENT_TABLE()
};

#endif // CCOPTIONSDLG_H
