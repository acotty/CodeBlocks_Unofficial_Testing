/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef ADDTODODLG_H
#define ADDTODODLG_H

#include <bitset>

#include <wx/string.h>

#include "scrollingdialog.h"

class wxArrayString;
class wxWindow;
class wxCommandEvent;

enum ToDoPosition
{
    tdpAbove = 0,
    tdpCurrent,
    tdpBelow
};

enum ToDoCommentType
{
    tdctLine = 0,      // line comments (for example C++ style)
    tdctStream,        // Stream comments (for example C style)
    tdctDoxygenLine,   // Doxygen line comment
    tdctDoxygenStream, // Doxygen stream comment
    tdctWarning,       // compiler warning
    tdctError          // compiler error
};
// when user want to add a new todo item, this dialog will shown to let user setting properties
class AddTodoDlg : public wxScrollingDialog
{
    public:
        AddTodoDlg(wxWindow * parent, const wxArrayString & users, const wxArrayString & types, std::bitset < (int)tdctError + 1 > supportedTdcts);

        wxString GetText() const;
        wxString GetUser() const;
        bool DateRequested() const;
        int GetPriority() const;
        ToDoPosition GetPosition() const;
        wxString GetType() const;
        ToDoCommentType GetCommentType() const;

        void EndModal(int retVal) override;

    private:
        void OnAddUser(wxCommandEvent &);
        void OnDelUser(wxCommandEvent &);
        void OnAddType(wxCommandEvent &);
        void OnDelType(wxCommandEvent &);

        std::bitset < (int)tdctError + 1 > m_supportedTdcts;
        DECLARE_EVENT_TABLE()
};

#endif // ADDTODODLG_H
