/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk_precomp.h"
#include "autodetectcompilers.h"

#ifndef CB_PRECOMP
    #include <wx/button.h>
    #include <wx/checkbox.h>
    #include <wx/filename.h>
    #include <wx/intl.h>
    #include <wx/listctrl.h>
    #include <wx/radiobox.h>
    #include <wx/stattext.h>
    #include <wx/string.h>
    #include <wx/xrc/xmlres.h>

    #include "compiler.h"
    #include "compilerfactory.h"
    #include "configmanager.h"
    #include "logmanager.h"
    #include "manager.h"
    #include "macrosmanager.h"
#endif
#include <wx/tooltip.h>

#include "infowindow.h"

BEGIN_EVENT_TABLE(AutoDetectCompilers, wxScrollingDialog)
    EVT_UPDATE_UI(-1, AutoDetectCompilers::OnUpdateUI)
    EVT_BUTTON(XRCID("btnSetDefaultCompiler"), AutoDetectCompilers::OnDefaultClick)
    EVT_RADIOBOX(XRCID("rbCompilerShowOptions"), AutoDetectCompilers::OnCompilerDisplayOptionClickRadioButton)
    EVT_CHECKBOX(XRCID("cbShowCompilerPath"), AutoDetectCompilers::OnCompilerPathDisplayOptionClickCheckBox)
END_EVENT_TABLE()

AutoDetectCompilers::AutoDetectCompilers(wxWindow* parent)
{
    //ctor
    wxXmlResource::Get()->LoadObject(this, parent, _T("dlgAutoDetectCompilers"),_T("wxScrollingDialog"));
    XRCCTRL(*this, "wxID_OK", wxButton)->SetDefault();

    wxListCtrl* list = XRCCTRL(*this, "lcCompilers", wxListCtrl);
    if (list)
    {
        list->Connect(wxEVT_MOTION, wxMouseEventHandler(AutoDetectCompilers::OnMouseMotion));

        for (size_t i = 0; i < CompilerFactory::GetCompilersCount(); ++i)
        {
            Compiler* compiler = CompilerFactory::GetCompiler(i);
            if (!compiler)
                continue;

            CompilerItem cItem;
            cItem.wxsCompilerName = compiler->GetName();
            cItem.wxsStatus = _("Unknown");     // Default to Unknown
            cItem.wxsCompilerPath = _("");      // Default to empty string
            cItem.bDetected = false;            // Default to false

            wxString path = compiler->GetMasterPath();
            wxString path_no_macros = compiler->GetMasterPath();
            Manager::Get()->GetMacrosManager()->ReplaceMacros(path_no_macros);

            if (path.IsEmpty() && Manager::Get()->GetConfigManager(wxT("compiler"))->Exists(wxT("/sets/") + compiler->GetID() + wxT("/name")))
            {
                Manager::Get()->GetLogManager()->LogError(wxString::Format(_("CompilerName : '%s'   , path : '%s' , path_no_macros : '%s'"), cItem.wxsCompilerName, path, path_no_macros ));
                Manager::Get()->GetLogManager()->LogError(wxString::Format(_("Manager::Get()->GetConfigManager(wxT(\"compiler\"))->Exists('%s') : %s"), wxT("/sets/") + compiler->GetID() + wxT("/name") ,
                                          Manager::Get()->GetConfigManager(wxT("compiler"))->Exists(wxT("/sets/") + compiler->GetID() + wxT("/name"))?_("True"):_("False") ));

                // Here, some user-interaction is required not to show this
                // dialog again on each new start-up of C::B.
                cItem.wxsStatus = _("Invalid");
                cItem.bDetected = false;
                // So we better clearly HIGHLIGHT this entry:
                cItem.iHighlight = 1;
            }
            else // The compiler is *probably* invalid, but at least a master-path is set
            {
                cItem.wxsStatus = _("Not found");
                cItem.bDetected = false;
                cItem.iHighlight = -1;
            }

            // Inspect deeper and probably try to auto-detect invalid compilers:
            if (compiler->GetParentID().IsEmpty()) // built-in compiler
            {
                // Try auto-detection (which is for built-in compilers only)
                bool detected = compiler->AutoDetectInstallationDir() == adrDetected;
                wxString pathDetected( compiler->GetMasterPath() );

                // In case auto-detection was successful:
                if (detected)
                {
                    // No path setup before OR path detected as it was setup before
                    if (path.IsEmpty() || path == pathDetected || path_no_macros == pathDetected)
                    {
                        cItem.wxsStatus = _("Detected");
                        cItem.bDetected = true;
                    }
                    else
                    {
                        cItem.wxsStatus = _("User-defined detected");
                        cItem.bDetected = false;
                    }
                    cItem.wxsCompilerPath = pathDetected;
                    cItem.iHighlight = 0;
                }
                // In case auto-detection failed but a path was setup before:
                else if ( !path.IsEmpty() )
                {
                    // Check, if the master path is valid:
                    if ( wxFileName::DirExists(path_no_macros) && !(path == pathDetected || path_no_macros == pathDetected) )
                    {
                        cItem.wxsStatus = _("User-defined not detected");
                        cItem.bDetected = false;
                        cItem.iHighlight = 0;
                    }

                    // Assume the user did the setup on purpose, so reset the old settings anyways:
                    compiler->SetMasterPath(path);
                }
            }
            else // no built-in, but user-defined (i.e. copied) compiler
            {
                // Check, if the master path is valid:
                if ( !path.IsEmpty() && wxFileName::DirExists(path_no_macros) )
                {
                    cItem.wxsStatus = _("User-defined masterpath issue");
                    cItem.bDetected = false;
                    cItem.iHighlight = 0;
                }
            }

            CompilerList.push_back(cItem);
        }
        updateCompilerDisplayList();
    }

    XRCCTRL(*this, "lblDefCompiler", wxStaticText)->SetLabel(CompilerFactory::GetDefaultCompiler()->GetName());
}

AutoDetectCompilers::~AutoDetectCompilers()
{
    //dtor
}

void AutoDetectCompilers::OnDefaultClick(cb_unused wxCommandEvent& event)
{
    wxListCtrl* list = XRCCTRL(*this, "lcCompilers", wxListCtrl);
    int idx = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx != -1)
    {
        CompilerFactory::SetDefaultCompiler(idx);
        XRCCTRL(*this, "lblDefCompiler", wxStaticText)->SetLabel(CompilerFactory::GetDefaultCompiler()->GetName());
    }
}

void AutoDetectCompilers::OnCompilerDisplayOptionClickRadioButton(cb_unused wxCommandEvent& event)
{
    updateCompilerDisplayList();
}

void AutoDetectCompilers::OnCompilerPathDisplayOptionClickCheckBox(cb_unused wxCommandEvent& event)
{
    updateCompilerDisplayList();
}

void AutoDetectCompilers::updateCompilerDisplayList()
{
    bool bShowCompilerPath= XRCCTRL(*this,"cbShowCompilerPath", wxCheckBox)->GetValue();
    bool bShowAllCompilerOptions = (XRCCTRL(*this, "rbCompilerShowOptions", wxRadioBox)->GetSelection() == 1);

    wxListCtrl* list = XRCCTRL(*this, "lcCompilers", wxListCtrl);
    if (list)
    {
        list->ClearAll();
        list->InsertColumn(eCompilerNameColumn, _("Compiler"), wxLIST_FORMAT_LEFT, 380);
        list->InsertColumn(eStatusColumn, _("Status"), wxLIST_FORMAT_LEFT, 100);

        if (bShowCompilerPath)
        {
            list->InsertColumn(eCompilerPathColumn, _("Compiler Path"), wxLIST_FORMAT_LEFT, 200);
        }

        for (auto itCL = std::begin(CompilerList); itCL != std::end(CompilerList); ++itCL)
        {
            if (bShowAllCompilerOptions && !itCL->bDetected)
            {
                continue;
            }

            int idx = list->GetItemCount();
            list->InsertItem(idx, itCL->wxsCompilerName);
            list->SetItem(idx, eStatusColumn, itCL->wxsStatus);
            if (bShowCompilerPath)
            {
                list->SetItem(idx, eCompilerPathColumn, itCL->wxsCompilerPath);
            }
            switch(itCL->iHighlight)
            {
                case 1:
                    list->SetItemBackgroundColour(idx, *wxRED);
                    break;
                case -1:
                    list->SetItemTextColour(idx, *wxLIGHT_GREY);
                    break;
            }
        }

        // Resize columns so one can read the whole stuff:
        list->SetColumnWidth(eCompilerNameColumn, wxLIST_AUTOSIZE);
        list->SetColumnWidth(eStatusColumn, wxLIST_AUTOSIZE);
        if (bShowCompilerPath)
        {
            list->SetColumnWidth(eCompilerPathColumn, wxLIST_AUTOSIZE);
        }
    }
}

void AutoDetectCompilers::OnMouseMotion(wxMouseEvent& event)
{
    wxListCtrl* list = XRCCTRL(*this, "lcCompilers", wxListCtrl);
    int flags = 0;
    int idx = list->HitTest(event.GetPosition(), flags);
    wxString txt = wxEmptyString;
    if (idx != wxNOT_FOUND)
    {
        wxListItem itm;
        itm.m_itemId = idx;
        itm.m_col = eStatusColumn;
        itm.m_mask = wxLIST_MASK_TEXT;
        if (list->GetItem(itm))
            txt = itm.m_text;
    }
    if (txt == wxT("Detected") || txt == wxT("User-defined"))
    {
        wxListItem itm;
        itm.m_itemId = idx;
        itm.m_col = eCompilerNameColumn;
        itm.m_mask = wxLIST_MASK_TEXT;
        if (list->GetItem(itm))
        {
            Compiler* compiler = CompilerFactory::GetCompilerByName(itm.m_text);
            idx = CompilerFactory::GetCompilerIndex(compiler);
        }
        txt = CompilerFactory::GetCompiler(idx)->GetMasterPath();
    }
    else
        txt = wxEmptyString;
    if (list->GetToolTip())
    {
        if (txt.IsEmpty())
            list->UnsetToolTip();
        else if (txt != list->GetToolTip()->GetTip())
            list->SetToolTip(txt);
    }
    else if (!txt.IsEmpty())
        list->SetToolTip(txt);
}

void AutoDetectCompilers::OnUpdateUI(wxUpdateUIEvent& event)
{
    wxListCtrl* list = XRCCTRL(*this, "lcCompilers", wxListCtrl);
    bool en = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED) != -1;
    XRCCTRL(*this, "btnSetDefaultCompiler", wxButton)->Enable(en);

    wxString sDefaultCompiler = CompilerFactory::GetDefaultCompiler()->GetName();
    for (auto itCL = std::begin(CompilerList); itCL != std::end(CompilerList); ++itCL)
    {
        // Find default compiler in the list.
        if (itCL->wxsCompilerName.IsSameAs(sDefaultCompiler))
        {
            // Only enable the ok button if the default compiler has been detected
            XRCCTRL(*this, "wxID_OK", wxButton)->Enable(itCL->bDetected);
            break; // Exit already found default compiler.
        }
    }
    event.Skip();
}
