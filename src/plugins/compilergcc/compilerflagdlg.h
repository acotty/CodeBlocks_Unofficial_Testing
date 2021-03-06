#ifndef COMPILERFLAGDLG_H
#define COMPILERFLAGDLG_H

#include <wx/dialog.h>
class wxTextCtrl;
class wxComboBox;
class wxToggleButton;
class wxArrayString;
struct CompOption;

class CompilerFlagDlg: public wxDialog
{
    public:

        CompilerFlagDlg(wxWindow * parent, CompOption * opt, wxArrayString & categ, const wxString & selectedCategory);
        virtual ~CompilerFlagDlg() override;

        void EndModal(int retCode) override;

    protected:

    private:

        void OnAdvancedOption(wxCommandEvent & event);

        wxTextCtrl * NameText;
        wxTextCtrl * CompilerText;
        wxTextCtrl * LinkerText;
        wxComboBox * CategoryCombo;
        wxTextCtrl * AgainstText;
        wxTextCtrl * MessageText;
        wxTextCtrl * SupersedeText;
        wxToggleButton * ExclusiveToggle;

        CompOption * copt;
};

#endif
