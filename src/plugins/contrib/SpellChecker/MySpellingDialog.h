/*
* This file is part of SpellChecker plugin for Code::Blocks Studio
* Copyright (C) 2009 Daniel Anselmi
*
* SpellChecker plugin is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* SpellChecker plugin is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with SpellChecker. If not, see <http://www.gnu.org/licenses/>.
*
*/
#ifndef __MY_SPELL_CHECK_DIALOG__
#define __MY_SPELL_CHECK_DIALOG__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "SpellCheckDialogInterface.h"

// modelless SpellChecker dialog
class MySpellingDialog : public wxSpellCheckDialogInterface
{
    public:
        MySpellingDialog(wxWindow * parent, wxSpellCheckEngineInterface * SpellChecker = NULL);
        ~MySpellingDialog();

        // Code handling the interface
        void OnRecheckPage(wxCommandEvent & event);
        void OnCheckWord(wxCommandEvent & event);
        void OnReplaceWord(wxCommandEvent & event);
        void OnIgnoreWord(wxCommandEvent & event);
        void OnReplaceAll(wxCommandEvent & event);
        void OnIgnoreAll(wxCommandEvent & event);
        void OnAddWordToCustomDictionary(wxCommandEvent & event);
        void OnEditCustomDictionary(wxCommandEvent & event);
        void OnInit(wxInitDialogEvent & event);
        void PopulateLanguageCombo();
        void OnChangeLanguage(wxCommandEvent & event);
        void OnChangeSuggestionSelection(wxCommandEvent & event);
        void OnDblClkSuggestionSelection(wxCommandEvent & event);
        void OnClose(wxCommandEvent & event);

        virtual void SetMisspelledWord(const wxString & strMisspelling);
        void CreateDialog();

    private:
        DECLARE_EVENT_TABLE()
};

class MyPersonalDictionaryDialog : public wxDialog
{
    public:
        MyPersonalDictionaryDialog(wxWindow * parent, wxSpellCheckEngineInterface * pEngine);
        ~MyPersonalDictionaryDialog();

        void CreateDialog();
        void PopulatePersonalWordListBox();
        void AddWordToPersonalDictionary(wxCommandEvent & event);
        void ReplaceInPersonalDictionary(wxCommandEvent & event);
        void RemoveFromPersonalDictionary(wxCommandEvent & event);
        void OnClose(wxCommandEvent & event);

    protected:
        wxSpellCheckEngineInterface * m_pSpellCheckEngine;

    private:
        DECLARE_EVENT_TABLE()
};

#endif  // __MY_SPELL_CHECK_DIALOG__
