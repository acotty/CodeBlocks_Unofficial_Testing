
#include "cbDiff.h"

#include <sdk.h>
#include <configurationpanel.h>
#include <cbproject.h>
#include <editormanager.h>
#include <logmanager.h>
#include <cbeditor.h>
#include <configmanager.h>
#include <wx/xrc/xmlres.h>

#include "cbDiffMenu.h"
#include "cbDiffEditor.h"
#include "cbDiffUtils.h"
#include "cbDiffSelectFiles.h"
#include "cbDiffConfigPanel.h"

// Register the plugin with Code::Blocks.
namespace
{
PluginRegistrant<cbDiff> reg(_T("cbDiff"));
const int ID_MENU_DIFF_FILES        = wxNewId();
const int ID_CONTEXT_DIFF_TWO_FILES = wxNewId();
const int ID_MENU_SAVE_UNIFIED_DIFF = wxNewId();

const int ID_VIEW_TABLE             = XRCID("CB_DIFF_ID_VIEW_TABLE");
const int ID_VIEW_UNIFIED           = XRCID("CB_DIFF_ID_VIEW_UNIFIED");
const int ID_VIEW_SIDEBYSIDE        = XRCID("CB_DIFF_ID_VIEW_SIDEBYSIDE");
const int ID_RELOAD_FILES           = XRCID("CB_DIFF_ID_RELOAD_FILES");
const int ID_SWAP_FILES             = XRCID("CB_DIFF_ID_SWAP_FILES");

const int ID_GOTO_NEXT_DIFFERENCE   = XRCID("CB_DIFF_GOTO_NEXT_DIFFERENCE");
const int ID_GOTO_PREV_DIFFERENCE   = XRCID("CB_DIFF_GOTO_PREVIOUS_DIFFERENCE");
const int ID_GOTO_FIRST_DIFFERENCE  = XRCID("CB_DIFF_GOTO_FIRST_DIFFERENCE");
const int ID_GOTO_LAST_DIFFERENCE   = XRCID("CB_DIFF_GOTO_LAST_DIFFERENCE");

const int ID_COPY_TO_LEFT           = XRCID("CB_DIFF_COPY_TO_LEFT");
const int ID_COPY_TO_RIGHT          = XRCID("CB_DIFF_COPY_TO_RIGHT");
const int ID_COPY_TO_LEFT_NEXT      = XRCID("CB_DIFF_COPY_TO_LEFT_NEXT");
const int ID_COPY_TO_RIGHT_NEXT     = XRCID("CB_DIFF_COPY_TO_RIGHT_NEXT");
}

/// Function for other plugins

EXPORT_FFP void DiffFiles(const wxString & firstfile, const wxString & secondfile, int viewmode, bool leftReadOnly, bool rightReadOnly)
{
    new cbDiffEditor(firstfile, secondfile, viewmode, leftReadOnly, rightReadOnly);
}

// events handling
BEGIN_EVENT_TABLE(cbDiff, cbPlugin)
    EVT_MENU(ID_MENU_DIFF_FILES,        cbDiff::OnMenuDiffFiles)
    EVT_MENU(ID_CONTEXT_DIFF_TWO_FILES, cbDiff::OnContextDiffFiles)
    EVT_MENU(ID_MENU_SAVE_UNIFIED_DIFF, cbDiff::OnMenuSaveAsUnifiedDiff)
    EVT_UPDATE_UI(ID_MENU_SAVE_UNIFIED_DIFF, cbDiff::OnUpdateUiSaveAsUnifiedDiff)

    EVT_MENU(ID_VIEW_TABLE,             cbDiff::OnSwitchView)
    EVT_UPDATE_UI(ID_VIEW_TABLE,             cbDiff::OnUpdateSwitchView)
    EVT_MENU(ID_VIEW_UNIFIED,           cbDiff::OnSwitchView)
    EVT_UPDATE_UI(ID_VIEW_UNIFIED,           cbDiff::OnUpdateSwitchView)
    EVT_MENU(ID_VIEW_SIDEBYSIDE,        cbDiff::OnSwitchView)
    EVT_UPDATE_UI(ID_VIEW_SIDEBYSIDE,        cbDiff::OnUpdateSwitchView)

    EVT_MENU(ID_RELOAD_FILES,           cbDiff::OnReloadFiles)
    EVT_UPDATE_UI(ID_RELOAD_FILES,           cbDiff::OnUpdateReloadFiles)
    EVT_MENU(ID_SWAP_FILES,             cbDiff::OnSwapFiles)
    EVT_UPDATE_UI(ID_SWAP_FILES,             cbDiff::OnUpdateSwapFiles)

    EVT_MENU(ID_GOTO_NEXT_DIFFERENCE,   cbDiff::OnGotoNextDifference)
    EVT_UPDATE_UI(ID_GOTO_NEXT_DIFFERENCE,   cbDiff::OnUpdateGotoNextDifference)
    EVT_MENU(ID_GOTO_PREV_DIFFERENCE,   cbDiff::OnGotoPreviousDifference)
    EVT_UPDATE_UI(ID_GOTO_PREV_DIFFERENCE,   cbDiff::OnUpdateGotoPreviousDifference)
    EVT_MENU(ID_GOTO_FIRST_DIFFERENCE,  cbDiff::OnGotoFirstDifference)
    EVT_UPDATE_UI(ID_GOTO_FIRST_DIFFERENCE,  cbDiff::OnUpdateGotoFirstDifference)
    EVT_MENU(ID_GOTO_LAST_DIFFERENCE,   cbDiff::OnGotoLastDifference)
    EVT_UPDATE_UI(ID_GOTO_LAST_DIFFERENCE,   cbDiff::OnUpdateGotoLastDifference)

    EVT_MENU(ID_COPY_TO_LEFT,           cbDiff::OnCopyToLeft)
    EVT_UPDATE_UI(ID_COPY_TO_LEFT,           cbDiff::OnUpdateCopyToLeft)
    EVT_MENU(ID_COPY_TO_RIGHT,          cbDiff::OnCopyToRight)
    EVT_UPDATE_UI(ID_COPY_TO_RIGHT,          cbDiff::OnUpdateCopyToRight)
    EVT_MENU(ID_COPY_TO_LEFT_NEXT,      cbDiff::OnCopyToLeftNext)
    EVT_UPDATE_UI(ID_COPY_TO_LEFT_NEXT,      cbDiff::OnUpdateCopyToLeftNext)
    EVT_MENU(ID_COPY_TO_RIGHT_NEXT,     cbDiff::OnCopyToRightNext)
    EVT_UPDATE_UI(ID_COPY_TO_RIGHT_NEXT,     cbDiff::OnUpdateCopyToRightNext)
END_EVENT_TABLE()

// constructor
cbDiff::cbDiff()
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if (!Manager::LoadResource(_T("cbDiff.zip")))
    {
        NotifyMissingFile(_T("cbDiff.zip"));
    }
}

void cbDiff::OnAttach()
{
    m_prevSelectionValid = false;
    wxString m_prevFileName = wxEmptyString;
    wxCmdLineParser & parser = *Manager::GetCmdLineParser();
    parser.AddOption(_T("diff1"), _T("diff1"), _T("first file to compare"));
    parser.AddOption(_T("diff2"), _T("diff2"), _T("second file to compare"));
    Manager::Get()->RegisterEventSink(cbEVT_APP_STARTUP_DONE, new cbEventFunctor<cbDiff, CodeBlocksEvent>(this, &cbDiff::OnAppDoneStartup));
    Manager::Get()->RegisterEventSink(cbEVT_APP_CMDLINE, new cbEventFunctor<cbDiff, CodeBlocksEvent>(this, &cbDiff::OnAppCmdLine));
}

bool cbDiff::BuildToolBar(wxToolBar * toolBar)
{
    //The application is offering its toolbar for your plugin,
    //to add any toolbar items you want...
    //Append any items you need on the toolbar...
    toolbar_ = toolBar;

    if (!IsAttached() || !toolBar)
    {
        return false;
    }

    Manager::Get()->AddonToolBar(toolbar_, _T("cb_diff_toolbar"));
    toolbar_->Realize();
    toolbar_->SetInitialSize();
    return true;
}
void cbDiff::OnAppDoneStartup(CodeBlocksEvent & event)
{
    EvalCmdLine();
    event.Skip();
}

void cbDiff::OnAppCmdLine(CodeBlocksEvent & event)
{
    EvalCmdLine();
    event.Skip();
}

void cbDiff::OnRelease(bool appShutDown)
{
    if (!appShutDown)
    {
        cbDiffEditor::CloseAllEditors();
    }
}

cbConfigurationPanel * cbDiff::GetConfigurationPanel(wxWindow * parent)
{
    if (!m_IsAttached)
    {
        return NULL;
    }

    return new cbDiffConfigPanel(parent);
}

void cbDiff::BuildMenu(wxMenuBar * menuBar)
{
    int fileMenuIndex = menuBar->FindMenu(_("&File"));

    if (fileMenuIndex == wxNOT_FOUND)
    {
        return;
    }

    wxMenu * fileMenu = menuBar->GetMenu(fileMenuIndex);

    if (!fileMenu)
    {
        return;
    }

    wxMenu * diffMenu = new wxMenu;
    diffMenu->Append(ID_MENU_DIFF_FILES,        _("Diff files..."),                 _("Shows the differences between two files"));
    diffMenu->Append(ID_MENU_SAVE_UNIFIED_DIFF, _("Save as unified diff file ..."), _("Saves the current differences as unified diff file"));
    diffMenu->AppendSeparator();
    diffMenu->Append(ID_GOTO_NEXT_DIFFERENCE,   _("Next difference"),               _("Go to next difference"));
    diffMenu->Append(ID_GOTO_PREV_DIFFERENCE,   _("Previous difference"),           _("Go to previous difference"));
    diffMenu->Append(ID_COPY_TO_RIGHT,          _("Copy to right"),                 _("Copy from left to right"));
    diffMenu->Append(ID_COPY_TO_LEFT,           _("Copy to left"),                  _("Copy from right to left"));
    diffMenu->Append(ID_COPY_TO_RIGHT_NEXT,     _("Copy to right and go to next"),  _("Copy from left to right and go to next difference"));
    diffMenu->Append(ID_COPY_TO_LEFT_NEXT,      _("Copy to left and go to next"),   _("Copy from right to left and go to next difference"));
    wxMenuItemList & list = fileMenu->GetMenuItems();
    int pos = 0;

    // Search for the Recent files entry and insert the diff entry after it
    for (wxMenuItemList::iterator i = list.begin(); i != list.end(); ++i, ++pos)
    {
        wxMenuItem * item = *i;
#if wxCHECK_VERSION(2, 9, 0)
        wxString label = item->GetItemLabelText();
#else
        wxString label = item->GetLabel();
#endif
        label.Replace(_T("_"), _T(""));

        if (label.Contains(_("Recent files")))
        {
            fileMenu->InsertSeparator(pos + 1);
            fileMenu->Insert(pos + 2, wxID_ANY, _("Diff Menu"), diffMenu);
            return;
        }
    }

    fileMenu->AppendSeparator();
    fileMenu->AppendSubMenu(diffMenu, _("Diff Menu"));
}

void cbDiff::BuildModuleMenu(const ModuleType type, wxMenu * menu, const FileTreeData * data)
{
    if (type == mtProjectManager && data != 0 && data->GetKind() == FileTreeData::ftdkFile)
    {
        ProjectFile * prjFile = data->GetProjectFile();

        if (prjFile)
        {
            wxMenu * diffmenu = new cbDiffMenu(this, prjFile->file.GetFullPath(), m_prevSelectionValid, m_prevFileName, menuIds_);
            menu->AppendSubMenu(diffmenu, _("Diff with"));
        }
    }
    else
        if (type == mtEditorManager && Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor())
        {
            wxString filename = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor()->GetFilename();
            wxMenu * diffmenu = new cbDiffMenu(this, filename, m_prevSelectionValid, m_prevFileName, menuIds_);
            menu->AppendSubMenu(diffmenu, _("Diff with"));
        }
        else
            if (type == mtEditorTab && Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor())
            {
                wxString filename = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor()->GetFilename();
                wxMenu * diffmenu = new cbDiffMenu(this, filename, m_prevSelectionValid, m_prevFileName, menuIds_);
                menu->AppendSubMenu(diffmenu, _("Diff with"));
            }
            else
                if (type == mtFileExplorer)
                {
                    if (data && (data->GetKind() == FileTreeData::ftdkFile))
                    {
                        wxFileName f(data->GetFolder());
                        wxString filename = f.GetFullPath();
                        wxString name = f.GetFullName();
                        menu->AppendSubMenu(new cbDiffMenu(this, name, m_prevSelectionValid, m_prevFileName, menuIds_), _("Diff with"));
                    }
                    else
                        if (data && (data->GetKind() == FileTreeData::ftdkVirtualGroup))
                        {
                            wxString paths = data->GetFolder(); //get folder contains a space separated list of the files/directories selected

                            if (paths.Find('*') != wxNOT_FOUND && paths.Find('*') == paths.Find('*', true))
                            {
                                names_.file1 = paths.BeforeFirst('*');
                                names_.file2 = paths.AfterFirst('*');

                                if (wxFileName::Exists(names_.file1) && wxFileName::Exists(names_.file2))
                                {
                                    menu->Append(ID_CONTEXT_DIFF_TWO_FILES, _("Diff"), _("Shows the differences between two files"));
                                }
                            }
                        }
                }
}

void cbDiff::OnMenuDiffFiles(wxCommandEvent & event)
{
    if (!m_IsAttached)
    {
        return;
    }

    cbDiffSelectFiles sdf(Manager::Get()->GetAppWindow());

    if (sdf.ShowModal() == wxID_OK && wxFile::Exists(sdf.GetFromFile()) && wxFile::Exists(sdf.GetToFile()))
    {
        new cbDiffEditor(sdf.GetFromFile(), sdf.GetToFile(), sdf.GetViewingMode());
    }
}

void cbDiff::OnContextDiffFiles(wxCommandEvent & event)
{
    if (wxFileName::Exists(names_.file1) && wxFileName::Exists(names_.file2))
    {
        int dm = cbDiffEditor::DEFAULT;
        ConfigManager * cfg = Manager::Get()->GetConfigManager(_T("cbdiffsettings"));

        if (cfg)
        {
            dm = cfg->ReadInt(_T("viewmode"), dm);
        }

        new cbDiffEditor(names_.file1, names_.file2, dm);
    }
}

void cbDiff::OnMenuSaveAsUnifiedDiff(wxCommandEvent & event)
{
    EditorManager * edMan = Manager::Get()->GetEditorManager();

    if (!edMan)
    {
        return;
    }

    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edMan->GetActiveEditor());

    if (!ed)
    {
        return;
    }

    ed->SaveAsUnifiedDiff();
}

void cbDiff::OnUpdateUiSaveAsUnifiedDiff(wxUpdateUIEvent & event)
{
    EditorManager * edMan = Manager::Get()->GetEditorManager();

    if (!edMan)
    {
        event.Enable(false);
        return;
    }

    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edMan->GetActiveEditor());
    event.Enable(ed != nullptr);
}

void cbDiff::OnGotoNextDifference(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->NextDifference();
        }
    }
}

void cbDiff::OnGotoPreviousDifference(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->PrevDifference();
        }
    }
}

void cbDiff::OnUpdateGotoNextDifference(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanGotoNextDiff();
    }

    event.Enable(enable);
}

void cbDiff::OnUpdateGotoPreviousDifference(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanGotoPrevDiff();
    }

    event.Enable(enable);
}

void cbDiff::OnGotoFirstDifference(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->FirstDifference();
        }
    }
}

void cbDiff::OnGotoLastDifference(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->LastDifference();
        }
    }
}

void cbDiff::OnUpdateGotoFirstDifference(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanGotoFirstDiff();
    }

    event.Enable(enable);
}

void cbDiff::OnUpdateGotoLastDifference(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanGotoLastDiff();
    }

    event.Enable(enable);
}

void cbDiff::EvalCmdLine()
{
    wxString file1, file2;
    wxCmdLineParser & parser = *Manager::GetCmdLineParser();

    if (parser.Found(_T("diff1"), &file1) && parser.Found(_T("diff2"), &file2))
    {
        if (wxFile::Exists(file1) && wxFile::Exists(file2))
        {
            int dm = cbDiffEditor::DEFAULT;
            ConfigManager * cfg = Manager::Get()->GetConfigManager(_T("cbdiffsettings"));

            if (cfg)
            {
                dm = cfg->ReadInt(_T("viewmode"), dm);
            }

            new cbDiffEditor(file1, file2, dm);
        }
    }
}


void cbDiff::OnSwitchView(wxCommandEvent & event)
{
    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(Manager::Get()->GetEditorManager()->GetActiveEditor());

    if (!ed || ed->GetModified())
    {
        return;
    }

    int mode;

    if (event.GetId() == ID_VIEW_TABLE)
    {
        mode = cbDiffEditor::TABLE;
    }
    else
        if (event.GetId() == ID_VIEW_UNIFIED)
        {
            mode = cbDiffEditor::UNIFIED;
        }
        else /* ID_VIEW_SIDEBYSIDE*/
        {
            mode = cbDiffEditor::SIDEBYSIDE;
        }

    if (ed->GetMode() != mode)
    {
        ed->SetMode(mode);
        ed->Reload();
    }
}

void cbDiff::OnUpdateSwitchView(wxUpdateUIEvent & event)
{
    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(Manager::Get()->GetEditorManager()->GetActiveEditor());

    if (!ed || ed->GetAnyModified())
    {
        event.Enable(false);
        return;
    }

    int mode;

    if (event.GetId() == ID_VIEW_TABLE)
    {
        mode = cbDiffEditor::TABLE;
    }
    else
        if (event.GetId() == ID_VIEW_UNIFIED)
        {
            mode = cbDiffEditor::UNIFIED;
        }
        else /* ID_VIEW_SIDEBYSIDE*/
        {
            mode = cbDiffEditor::SIDEBYSIDE;
        }

    event.Enable(ed->GetMode() != mode);
}

void cbDiff::OnReloadFiles(wxCommandEvent & event)
{
    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(Manager::Get()->GetEditorManager()->GetActiveEditor());

    if (ed)
    {
        ed->Reload();
    }
}

void cbDiff::OnUpdateReloadFiles(wxUpdateUIEvent & event)
{
    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(Manager::Get()->GetEditorManager()->GetActiveEditor());
    event.Enable(ed != nullptr);
}

void cbDiff::OnSwapFiles(wxCommandEvent & event)
{
    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(Manager::Get()->GetEditorManager()->GetActiveEditor());

    if (ed)
    {
        ed->Swap();
    }
}

void cbDiff::OnUpdateSwapFiles(wxUpdateUIEvent & event)
{
    cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(Manager::Get()->GetEditorManager()->GetActiveEditor());
    event.Enable(ed && !ed->GetAnyModified());
}

void cbDiff::OnCopyToLeft(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->CopyToLeft();
        }
    }
}

void cbDiff::OnUpdateCopyToLeft(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanCopyToLeft();
    }

    event.Enable(enable);
}

void cbDiff::OnCopyToRight(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->CopyToRight();
        }
    }
}

void cbDiff::OnUpdateCopyToRight(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanCopyToRight();
    }

    event.Enable(enable);
}

void cbDiff::OnCopyToLeftNext(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->CopyToLeftNext();
        }
    }
}

void cbDiff::OnUpdateCopyToLeftNext(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanCopyToLeftNext();
    }

    event.Enable(enable);
}

void cbDiff::OnCopyToRightNext(wxCommandEvent & event)
{
    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);

        if (ed)
        {
            ed->CopyToRightNext();
        }
    }
}

void cbDiff::OnUpdateCopyToRightNext(wxUpdateUIEvent & event)
{
    bool enable = false;

    if (EditorManager * edman = Manager::Get()->GetEditorManager())
    {
        EditorBase * edb = edman->GetActiveEditor();
        cbDiffEditor * ed = dynamic_cast<cbDiffEditor *>(edb);
        enable = ed && ed->CanCopyToRightNext();
    }

    event.Enable(enable);
}
