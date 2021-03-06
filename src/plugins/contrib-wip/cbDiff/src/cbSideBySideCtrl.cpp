#include "cbSideBySideCtrl.h"

#include <algorithm>

#include <manager.h>
#include <editormanager.h>
#include <configmanager.h>
#include <logmanager.h>
#include <cbstyledtextctrl.h>
#include <cbeditor.h>

#include <wx/textfile.h>
#include <wx/splitter.h>
#include "cbDiffEditor.h"

/**
    We need to create this timer,
    because if we use the Scintilla events
    the graphic get messed up.
    And its easier to handle all the
    synchronisation stuff in one function.
    Trust me or try it self. ;-)
**/
class LineChangedTimer : public wxTimer
{
        cbSideBySideCtrl * pane_;
    public:
        LineChangedTimer(cbSideBySideCtrl * pane):
            wxTimer()
        {
            pane_ = pane;
        }
        void Notify()
        {
            pane_->Synchronize();
        }
        void start()
        {
            wxTimer::Start(20);
        }
};

const int STYLE_BLUE_BACK = 55;

BEGIN_EVENT_TABLE(cbSideBySideCtrl, cbDiffCtrl)
END_EVENT_TABLE()

cbSideBySideCtrl::cbSideBySideCtrl(cbDiffEditor * parent):
    cbDiffCtrl(parent),
    lastSyncedLine_(-1),
    lastSyncedLHandle_(-1),
    lastSyncedRHandle_(-1),
    lineNumbersWidthLeft_(0),
    lineNumbersWidthRight_(0)
{
    wxBoxSizer * VBoxSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer * HBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    vScrollBar_ = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
    hScrollBar_ = new wxScrollBar(this, wxID_ANY);
    wxSplitterWindow * splitWindow = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0x0200 | 0x0100 | wxSP_THIN_SASH);
    splitWindow->SetMinimumPaneSize(20); // don't un-split
    tcLeft_ = new cbStyledTextCtrl(splitWindow, wxID_ANY);
    tcRight_ = new cbStyledTextCtrl(splitWindow, wxID_ANY);
    splitWindow->SplitVertically(tcLeft_, tcRight_);
    HBoxSizer->Add(splitWindow, 1, wxEXPAND, 0);
    HBoxSizer->Add(vScrollBar_, 0, wxEXPAND, 0);
    VBoxSizer->Add(HBoxSizer, 1, wxEXPAND, 5);
    VBoxSizer->Add(hScrollBar_, 0, wxEXPAND, 0);
    SetSizer(VBoxSizer);
    VBoxSizer->Fit(this);
    VBoxSizer->SetSizeHints(this);
    splitWindow->SetSashGravity(0.5);
    tcLeft_->SetVScrollBar(vScrollBar_);
    tcLeft_->SetHScrollBar(hScrollBar_);
    tcRight_->SetVScrollBar(vScrollBar_);
    tcRight_->SetHScrollBar(hScrollBar_);
    timer_ = new LineChangedTimer(this);
    timer_->start();
    vscrollpos_ = vScrollBar_->GetThumbPosition();
    hscrollpos_ = hScrollBar_->GetThumbPosition();
}

cbSideBySideCtrl::~cbSideBySideCtrl()
{
    wxDELETE(timer_);
    Disconnect(tcLeft_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
    Disconnect(tcRight_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
}

void cbSideBySideCtrl::Init(cbDiffColors colset)
{
    Disconnect(tcLeft_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
    Disconnect(tcRight_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
    const wxColor marbkg = tcLeft_->StyleGetBackground(wxSCI_STYLE_LINENUMBER);
    cbEditor::ApplyStyles(tcLeft_);
    tcLeft_->SetMarginWidth(1, 16);
    tcLeft_->SetMarginType(1, wxSCI_MARGIN_SYMBOL);
    tcLeft_->SetMarginWidth(2, 0);   // to hide the change and the fold margin
    tcLeft_->SetMarginWidth(3, 0);   // made by cbEditor::ApplyStyles
    tcLeft_->MarkerDefine(MINUS_MARKER, wxSCI_MARK_MINUS, colset.removedlines_, colset.removedlines_);
    tcLeft_->MarkerDefine(EQUAL_MARKER, wxSCI_MARK_CHARACTER + 61, *wxWHITE, marbkg);
    tcLeft_->MarkerDefine(RED_BKG_MARKER, wxSCI_MARK_BACKGROUND, colset.removedlines_, colset.removedlines_);
    tcLeft_->MarkerSetAlpha(RED_BKG_MARKER, colset.removedlines_.Alpha());
    tcLeft_->MarkerDefine(BLUE_BKG_MARKER, wxSCI_MARK_BACKGROUND, colset.selectedlines_, colset.selectedlines_);
    tcLeft_->MarkerSetAlpha(BLUE_BKG_MARKER, colset.selectedlines_.Alpha());

    if (colset.caretlinetype_ == 0)
    {
        tcLeft_->MarkerDefine(CARET_LINE_MARKER, wxSCI_MARK_UNDERLINE, colset.caretline_, colset.caretline_);
    }
    else
    {
        tcLeft_->MarkerDefine(CARET_LINE_MARKER, wxSCI_MARK_BACKGROUND, colset.caretline_, colset.caretline_);
        tcLeft_->MarkerSetAlpha(CARET_LINE_MARKER, colset.caretline_.Alpha());
    }

    const auto lang = colset.hlang_;
    const bool isC = lang == "C/C++";
    theme_->Apply(theme_->GetHighlightLanguage(lang), tcLeft_, isC, true);
    wxColor defbkgl = tcLeft_->StyleGetBackground(wxSCI_STYLE_DEFAULT);
    wxColor annotBkgL(defbkgl.Red()   + (colset.selectedlines_.Red()   - defbkgl.Red())*colset.selectedlines_.Alpha() / 255,
                      defbkgl.Green() + (colset.selectedlines_.Green() - defbkgl.Green())*colset.selectedlines_.Alpha() / 255,
                      defbkgl.Blue()  + (colset.selectedlines_.Blue()  - defbkgl.Blue())*colset.selectedlines_.Alpha() / 255);
    tcLeft_->AnnotationSetStyleOffset(512);
    tcLeft_->StyleSetBackground(512 + STYLE_BLUE_BACK, annotBkgL);
    tcLeft_->SetVisiblePolicy(wxSCI_VISIBLE_STRICT, 0);
    //tcLeft_->SetViewEOL(true);
    cbEditor::ApplyStyles(tcRight_);
    tcRight_->SetMarginWidth(1, 16);
    tcRight_->SetMarginType(1, wxSCI_MARGIN_SYMBOL);
    tcRight_->SetMarginWidth(2, 0);   // to hide the change and fold margin
    tcRight_->SetMarginWidth(3, 0);   // made by cbEditor::ApplyStyles
    tcRight_->MarkerDefine(PLUS_MARKER, wxSCI_MARK_PLUS, colset.addedlines_, colset.addedlines_);
    tcRight_->MarkerDefine(EQUAL_MARKER, wxSCI_MARK_CHARACTER + 61, *wxWHITE, marbkg);
    tcRight_->MarkerDefine(GREEN_BKG_MARKER, wxSCI_MARK_BACKGROUND, colset.addedlines_, colset.addedlines_);
    tcRight_->MarkerSetAlpha(GREEN_BKG_MARKER, colset.addedlines_.Alpha());
    tcRight_->MarkerDefine(BLUE_BKG_MARKER, wxSCI_MARK_BACKGROUND, colset.selectedlines_, colset.selectedlines_);
    tcRight_->MarkerSetAlpha(BLUE_BKG_MARKER, colset.selectedlines_.Alpha());

    if (colset.caretlinetype_ == 0)
    {
        tcRight_->MarkerDefine(CARET_LINE_MARKER, wxSCI_MARK_UNDERLINE, colset.caretline_, colset.caretline_);
    }
    else
    {
        tcRight_->MarkerDefine(CARET_LINE_MARKER, wxSCI_MARK_BACKGROUND, colset.caretline_, colset.caretline_);
        tcRight_->MarkerSetAlpha(CARET_LINE_MARKER, colset.caretline_.Alpha());
    }

    theme_->Apply(theme_->GetHighlightLanguage(colset.hlang_), tcRight_, isC, true);
    wxColor defbkgr = tcRight_->StyleGetBackground(wxSCI_STYLE_DEFAULT);
    wxColor annotBkgR(defbkgr.Red()   + (colset.selectedlines_.Red()   - defbkgr.Red())*colset.selectedlines_.Alpha() / 255,
                      defbkgr.Green() + (colset.selectedlines_.Green() - defbkgr.Green())*colset.selectedlines_.Alpha() / 255,
                      defbkgr.Blue()  + (colset.selectedlines_.Blue()  - defbkgr.Blue())*colset.selectedlines_.Alpha() / 255);
    tcRight_->AnnotationSetStyleOffset(512);
    tcRight_->StyleSetBackground(512 + STYLE_BLUE_BACK, annotBkgR);
    tcRight_->SetVisiblePolicy(wxSCI_VISIBLE_STRICT, 0);
    //tcRight_->SetViewEOL(true);
}

void cbSideBySideCtrl::UpdateDiff(const wxDiff & diff)
{
    ShowDiff(diff, false);
}

void cbSideBySideCtrl::ShowDiff(const wxDiff & diff)
{
    ShowDiff(diff, true);
}

void cbSideBySideCtrl::ShowDiff(const wxDiff & diff, bool reloadFile)
{
    Disconnect(tcLeft_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
    Disconnect(tcRight_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
    std::map<long, int>  right_added    = diff.GetAddedLines();
    std::map<long, int>  right_empty    = diff.GetRightEmptyLines();
    std::map<long, int>  left_empty     = diff.GetLeftEmptyLines();
    std::map<long, int>  left_removed   = diff.GetRemovedLines();
    std::map<long, long> line_pos_left  = diff.GetLinePositionsLeft();
    std::map<long, long> line_pos_right = diff.GetLinePositionsRight();
    leftFilename_ = diff.GetLeftFilename();
    rightFilename_ = diff.GetRightFilename();
    leftReadOnly_ = diff.LeftReadOnly();
    rightReadOnly_ = diff.RightReadOnly();
    linesLeftWithDifferences_.clear();
    lastLeftMarkedDiff_ = -1;
    lastLeftMarkedEmptyDiff_ = -1;
    int leftCursorPos =   tcLeft_->GetCurrentPos();
    tcLeft_->SetReadOnly(false);

    if (reloadFile)
    {
        tcLeft_->ClearAll();

        if (cbEditor * ed = GetCbEditorIfActive(leftFilename_))
        {
            tcLeft_->SetDocPointer(ed->GetControl()->GetDocPointer());
        }
        else
        {
            tcLeft_->LoadFile(diff.GetLeftFilename());
        }
    }

    tcLeft_->AnnotationClearAll();
    tcLeft_->AnnotationSetVisible(wxSCI_ANNOTATION_STANDARD);

    for (auto itr = left_removed.begin() ; itr != left_removed.end() ; ++itr)
    {
        Block bl;
        long line_removed = itr->first;
        unsigned int removed = itr->second;
        bl.len = removed;

        for (unsigned int k = 0 ; k < removed ; ++k)
        {
            tcLeft_->MarkerAdd(line_removed + k, MINUS_MARKER);
            tcLeft_->MarkerAdd(line_removed + k, RED_BKG_MARKER);
        }

        auto it = left_empty.find(line_removed);

        if (it != left_empty.end())
        {
            long line_empty = line_removed + removed;
            unsigned int empty = it->second - removed;
            wxString annotationStr('\n', empty - 1);
            tcLeft_->AnnotationSetText(line_empty - 1, annotationStr);
            left_empty.erase(it);
            bl.empty = empty;
        }

        linesLeftWithDifferences_.push_back(line_removed);
        bl.ref = line_pos_right[line_removed];
        leftChanges_[line_removed] = bl;
    }

    for (auto itr = left_empty.begin(); itr != left_empty.end() ; ++itr)
    {
        long line = itr->first;
        unsigned int len = itr->second;
        wxString annotationStr('\n', len - 1);
        tcLeft_->AnnotationSetText(line - 1, annotationStr);
        auto i = std::lower_bound(linesLeftWithDifferences_.begin(), linesLeftWithDifferences_.end(), line);
        linesLeftWithDifferences_.insert(i, line);
        Block bl;
        bl.empty = len;
        bl.ref = line_pos_right[line];
        leftChanges_[line] = bl;
    }

    if (leftReadOnly_)
    {
        tcLeft_->SetReadOnly(true);
    }

    tcLeft_->SetMarginType(0, wxSCI_MARGIN_NUMBER);
    setLineNumberMarginWidth(tcLeft_, lineNumbersWidthLeft_);
    tcLeft_->GotoPos(leftCursorPos);
    linesRightWithDifferences_.clear();
    lastRightMarkedDiff_ = -1;
    lastRightMarkedEmptyDiff_ = -1;
    int rightCursorPos = tcRight_->GetCurrentPos();
    tcRight_->SetReadOnly(false);

    if (reloadFile)
    {
        tcRight_->ClearAll();

        if (cbEditor * ed = GetCbEditorIfActive(rightFilename_))
        {
            tcRight_->SetDocPointer(ed->GetControl()->GetDocPointer());
        }
        else
        {
            tcRight_->LoadFile(diff.GetRightFilename());
        }
    }

    tcRight_->AnnotationClearAll();
    tcRight_->AnnotationSetVisible(wxSCI_ANNOTATION_STANDARD);

    for (auto itr = right_added.begin() ; itr != right_added.end() ; ++itr)
    {
        Block bl;
        long line_added = itr->first;
        unsigned int added = itr->second;
        bl.len = added;

        for (unsigned int k = 0 ; k < added ; ++k)
        {
            tcRight_->MarkerAdd(line_added + k, PLUS_MARKER);
            tcRight_->MarkerAdd(line_added + k, GREEN_BKG_MARKER);
        }

        auto it = right_empty.find(line_added);

        if (it != right_empty.end())
        {
            long line_empty = line_added + added;
            unsigned int empty = it->second - added;
            wxString annotationStr('\n', empty - 1);
            tcRight_->AnnotationSetText(line_empty - 1, annotationStr);
            right_empty.erase(it);
            bl.empty = empty;
        }

        linesRightWithDifferences_.push_back(line_added);
        bl.ref = line_pos_left[line_added];
        rightChanges_[line_added] = bl;
    }

    for (auto itr = right_empty.begin(); itr != right_empty.end() ; ++itr)
    {
        long line = itr->first;
        unsigned int len = itr->second;
        wxString annotationStr('\n', len - 1);
        tcRight_->AnnotationSetText(line - 1, annotationStr);
        auto i = std::lower_bound(linesRightWithDifferences_.begin(), linesRightWithDifferences_.end(), line);
        linesRightWithDifferences_.insert(i, line);
        Block bl;
        bl.empty = len;
        bl.ref = line_pos_left[line];
        rightChanges_[line] = bl;
    }

    if (rightReadOnly_)
    {
        tcRight_->SetReadOnly(true);
    }

    tcRight_->SetMarginType(0, wxSCI_MARGIN_NUMBER);
    setLineNumberMarginWidth(tcRight_, lineNumbersWidthRight_);
    tcRight_->GotoPos(rightCursorPos);
    Connect(tcLeft_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
    Connect(tcRight_->GetId(), wxEVT_SCI_CHANGE, wxScintillaEventHandler(cbSideBySideCtrl::OnEditorChange));
}

void cbSideBySideCtrl::Synchronize()
{
    SynchronizeSelection();
    SynchronizeCaretline();
    SynchronizeZoom();
    SynchronizeScroll();
}

void cbSideBySideCtrl::SynchronizeSelection()
{
    int curr_left_line = tcLeft_->GetCurrentLine();
    int curr_right_line = tcRight_->GetCurrentLine();

    if (lastLeftMarkedDiff_ != -1 &&
            (((leftChanges_[lastLeftMarkedDiff_].len >  0) && (curr_left_line != lastLeftMarkedDiff_)) ||
             ((leftChanges_[lastLeftMarkedDiff_].len == 0) && (curr_left_line != lastLeftMarkedEmptyDiff_))))
    {
        unmarkSelectionDiff(lastLeftMarkedDiff_, leftChanges_, tcLeft_, lastLeftMarkedEmptyDiff_, Removed);
        lastLeftMarkedDiff_ = -1;
        lastLeftMarkedEmptyDiff_ = -1;
        unmarkSelectionDiff(lastRightMarkedDiff_, rightChanges_, tcRight_, lastRightMarkedEmptyDiff_, Added);
        lastRightMarkedDiff_ = -1;
        lastRightMarkedEmptyDiff_ = -1;
    }

    if (lastRightMarkedDiff_ != -1 &&
            (((rightChanges_[lastRightMarkedDiff_].len >  0) && (curr_right_line != lastRightMarkedDiff_)) ||
             ((rightChanges_[lastRightMarkedDiff_].len == 0) && (curr_right_line != lastRightMarkedEmptyDiff_))))
    {
        unmarkSelectionDiff(lastRightMarkedDiff_, rightChanges_, tcRight_, lastRightMarkedEmptyDiff_, Added);
        lastRightMarkedDiff_ = -1;
        lastRightMarkedEmptyDiff_ = -1;
        unmarkSelectionDiff(lastLeftMarkedDiff_, leftChanges_, tcLeft_, lastLeftMarkedEmptyDiff_, Removed);
        lastLeftMarkedDiff_ = -1;
        lastLeftMarkedEmptyDiff_ = -1;
    }
}

void cbSideBySideCtrl::SynchronizeCaretline()
{
    int curr_line = 0;

    if (tcLeft_->GetSCIFocus())  // which scintilla control has the focus?
    {
        curr_line = tcLeft_->VisibleFromDocLine(tcLeft_->GetCurrentLine());
    }

    if (tcRight_->GetSCIFocus())
    {
        curr_line = tcRight_->VisibleFromDocLine(tcRight_->GetCurrentLine());
    }

    /* Caretline background synchronisation */
    if (curr_line != lastSyncedLine_)
    {
        if (lastLeftMarkedEmptyDiff_ == -1)
        {
            tcLeft_->MarkerDeleteHandle(lastSyncedLHandle_);
            lastSyncedLHandle_ = tcLeft_->MarkerAdd(tcLeft_->DocLineFromVisible(curr_line), CARET_LINE_MARKER);
        }

        if (lastRightMarkedEmptyDiff_ == -1)
        {
            tcRight_->MarkerDeleteHandle(lastSyncedRHandle_);
            lastSyncedRHandle_ = tcRight_->MarkerAdd(tcRight_->DocLineFromVisible(curr_line), CARET_LINE_MARKER);
        }

        lastSyncedLine_ = curr_line;
        return;
    }
}

void cbSideBySideCtrl::SynchronizeZoom()
{
    int curr_scroll_focus = 0;

    // which wxcontrol has the scrollfocus?
    if (tcLeft_->GetRect().Contains(ScreenToClient(wxGetMousePosition())))
    {
        curr_scroll_focus = 1;
    }

    if (tcRight_->GetRect().Contains(ScreenToClient(wxGetMousePosition())))
    {
        curr_scroll_focus = 2;
    }

    /* Zoom synchronisation */
    if (tcRight_->GetZoom() != tcLeft_->GetZoom())
    {
        if (curr_scroll_focus == 1)
        {
            tcRight_->SetZoom(tcLeft_->GetZoom());
        }

        if (curr_scroll_focus == 2)
        {
            tcLeft_->SetZoom(tcRight_->GetZoom());
        }
    }
}

void cbSideBySideCtrl::SynchronizeScroll()
{
    if (vscrollpos_ != vScrollBar_->GetThumbPosition())
    {
        tcLeft_->SetFirstVisibleLine(vScrollBar_->GetThumbPosition());
        tcRight_->SetFirstVisibleLine(vScrollBar_->GetThumbPosition());
        vscrollpos_ = vScrollBar_->GetThumbPosition();
    }

    if (hscrollpos_ != hScrollBar_->GetThumbPosition())
    {
        tcLeft_->SetXOffset(hScrollBar_->GetThumbPosition());
        tcRight_->SetXOffset(hScrollBar_->GetThumbPosition());
        hscrollpos_ = hScrollBar_->GetThumbPosition();
    }
}

bool cbSideBySideCtrl::GetModified() const
{
    if (tcLeft_->HasFocus())
    {
        return tcLeft_->GetModify();
    }
    else
        if (tcRight_->HasFocus())
        {
            return tcRight_->GetModify();
        }
        else
        {
            return tcLeft_->GetModify() || tcRight_->GetModify();
        }
}

bool cbSideBySideCtrl::QueryClose()
{
    if (tcLeft_->GetModify() && !GetCbEditorIfActive(leftFilename_) &&
            tcRight_->GetModify() && !GetCbEditorIfActive(rightFilename_))
    {
        wxArrayString choices;
        choices.Add("none");
        choices.Add("left only");
        choices.Add("right only");
        choices.Add("both");

        switch (wxGetSingleChoiceIndex("save", "Confirm", choices))
        {
            case  0:
                tcLeft_->SetSavePoint();
                tcRight_->SetSavePoint();
                break;

            case  1:
                tcRight_->SetSavePoint();
                return SaveLeft();

            case  2:
                tcLeft_->SetSavePoint();
                return SaveRight();

            case  3:
            {
                const bool leftOk = SaveLeft();
                const bool rightOk = SaveRight();
                return rightOk && leftOk;
            }

            default:
            case -1:
                return false;
        }
    }
    else
        if (tcLeft_->GetModify() && !GetCbEditorIfActive(leftFilename_))
        {
            int answer = wxMessageBox("Save Left?", "Confirm", wxYES_NO | wxCANCEL);

            if (answer == wxCANCEL)
            {
                return false;
            }
            else
                if (answer == wxYES)
                {
                    return SaveLeft();
                }
                else
                {
                    tcLeft_->SetSavePoint();
                }
        }
        else
            if (tcRight_->GetModify() && !GetCbEditorIfActive(rightFilename_))
            {
                int answer = wxMessageBox("Save Right?", "Confirm", wxYES_NO | wxCANCEL);

                if (answer == wxCANCEL)
                {
                    return false;
                }
                else
                    if (answer == wxYES)
                    {
                        return SaveRight();
                    }
                    else
                    {
                        tcRight_->SetSavePoint();
                    }
            }

    parent_->updateTitle();
    return true;
}

bool cbSideBySideCtrl::Save()
{
    if (tcLeft_->HasFocus())
    {
        return SaveLeft();
    }
    else
        if (tcRight_->HasFocus())
        {
            return SaveRight();
        }
        else
        {
            bool sl = SaveLeft();
            bool sr = SaveRight();
            return sl && sr;
        }
}

bool cbSideBySideCtrl::SaveLeft()
{
    if (tcLeft_->GetModify())
    {
        if (!tcLeft_->SaveFile(leftFilename_))
        {
            return false;
        }
        else
        {
            parent_->updateTitle();
        }
    }

    return true;
}

bool cbSideBySideCtrl::SaveRight()
{
    if (tcRight_->GetModify())
    {
        if (!tcRight_->SaveFile(rightFilename_))
        {
            return false;
        }
        else
        {
            parent_->updateTitle();
        }
    }

    return true;
}

bool cbSideBySideCtrl::LeftModified()
{
    return tcLeft_->GetModify();
}

bool cbSideBySideCtrl::RightModified()
{
    return tcRight_->GetModify();
}

void cbSideBySideCtrl::setLineNumberMarginWidth(cbStyledTextCtrl * stc, int & currWidth)
{
    ConfigManager * cfg = Manager::Get()->GetConfigManager(_T("editor"));
    int pixelWidth = stc->TextWidth(wxSCI_STYLE_LINENUMBER, _T("9"));

    if (cfg->ReadBool(_T("/margin/dynamic_width"), false))
    {
        int lineNumChars = 1;
        int lineCount = stc->GetLineCount();

        while (lineCount >= 10)
        {
            lineCount /= 10;
            ++lineNumChars;
        }

        int lineNumWidth =  lineNumChars * pixelWidth + pixelWidth * 0.75;

        if (lineNumWidth != currWidth)
        {
            stc->SetMarginWidth(0, lineNumWidth);
            currWidth = lineNumWidth;
        }
    }
    else
    {
        stc->SetMarginWidth(0, pixelWidth * 0.75 + cfg->ReadInt(_T("/margin/width_chars"), 6) * pixelWidth);
    }
}

void cbSideBySideCtrl::OnEditorChange(wxScintillaEvent & event)
{
    parent_->updateTitle();
}

void cbSideBySideCtrl::Undo()
{
    if (!leftReadOnly_ && tcLeft_->HasFocus())
    {
        tcLeft_->Undo();
    }
    else
        if (!rightReadOnly_ && tcRight_->HasFocus())
        {
            tcRight_->Undo();
        }
}

void cbSideBySideCtrl::Redo()
{
    if (!leftReadOnly_ && tcLeft_->HasFocus())
    {
        tcLeft_->Redo();
    }
    else
        if (!rightReadOnly_ && tcRight_->HasFocus())
        {
            tcRight_->Redo();
        }
}

void cbSideBySideCtrl::ClearHistory()
{
    if (tcLeft_->HasFocus())
    {
        tcLeft_->EmptyUndoBuffer();
    }
    else
        if (tcRight_->HasFocus())
        {
            tcRight_->EmptyUndoBuffer();
        }
}

void cbSideBySideCtrl::Cut()
{
    if (!leftReadOnly_ && tcLeft_->HasFocus())
    {
        tcLeft_->Cut();
    }
    else
        if (!rightReadOnly_ && tcRight_->HasFocus())
        {
            tcRight_->Cut();
        }
}

void cbSideBySideCtrl::Copy()
{
    if (tcLeft_->HasFocus())
    {
        tcLeft_->Copy();
    }
    else
        if (tcRight_->HasFocus())
        {
            tcRight_->Copy();
        }
}

void cbSideBySideCtrl::Paste()
{
    if (!leftReadOnly_ && tcLeft_->HasFocus())
    {
        tcLeft_->Paste();
    }
    else
        if (!rightReadOnly_ && tcRight_->HasFocus())
        {
            tcRight_->Paste();
        }
}

bool cbSideBySideCtrl::CanUndo() const
{
    if (tcLeft_->HasFocus())
    {
        return !leftReadOnly_ && tcLeft_->CanUndo();
    }
    else
        if (tcRight_->HasFocus())
        {
            return !rightReadOnly_ && tcRight_->CanUndo();
        }

    return false;
}

bool cbSideBySideCtrl::CanRedo() const
{
    if (tcLeft_->HasFocus())
    {
        return !leftReadOnly_ && tcLeft_->CanRedo();
    }
    else
        if (tcRight_->HasFocus())
        {
            return !rightReadOnly_ && tcRight_->CanRedo();
        }

    return false;
}

bool cbSideBySideCtrl::HasSelection() const
{
    if (tcLeft_->HasFocus())
    {
        return tcLeft_->GetSelectionStart() != tcLeft_->GetSelectionEnd();
    }
    else
        if (tcRight_->HasFocus())
        {
            return tcRight_->GetSelectionStart() != tcRight_->GetSelectionEnd();
        }

    return false;
}

bool cbSideBySideCtrl::CanPaste() const
{
    if (tcLeft_->HasFocus())
    {
        return tcLeft_->CanPaste() && !leftReadOnly_;
    }
    else
        if (tcRight_->HasFocus())
        {
            return tcRight_->CanPaste() && !rightReadOnly_;
        }

    return false;
}

bool cbSideBySideCtrl::CanSelectAll() const
{
    if (tcLeft_->HasFocus())
    {
        return tcLeft_->GetLength() > 0;
    }
    else
        if (tcRight_->HasFocus())
        {
            return tcLeft_->GetLength() > 0;
        }

    return false;
}

void cbSideBySideCtrl::SelectAll()
{
    if (tcLeft_->HasFocus())
    {
        tcLeft_->SelectAll();
    }
    else
        if (tcRight_->HasFocus())
        {
            tcRight_->SelectAll();
        }
}

long cbSideBySideCtrl::NextDifference(const std::vector<long> & linesWithDifferences, int currline)
{
    if (linesWithDifferences.empty())
    {
        return -1;
    }

    auto itr = std::upper_bound(linesWithDifferences.begin(), linesWithDifferences.end(), currline);

    if (itr == linesWithDifferences.end())
    {
        return -1;
    }

    return *itr;
}

void cbSideBySideCtrl::NextDifference()
{
    if (tcLeft_->GetSCIFocus())
    {
        long l = NextDifference(linesLeftWithDifferences_, tcLeft_->GetCurrentLine());

        if (l != -1)
        {
            selectDiff(l, -1);
        }
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            long l = NextDifference(linesRightWithDifferences_, tcRight_->GetCurrentLine());

            if (l != -1)
            {
                selectDiff(-1, l);
            }
        }
}

long cbSideBySideCtrl::PrevDifference(int curr_line, const std::vector<long> & linesWithDifferences, std::map<long, Block> & changes)
{
    if (curr_line == 0)
    {
        return -1;
    }

    if (linesWithDifferences.empty())
    {
        return -1;
    }

    auto itr = changes.find(curr_line);

    if (itr != changes.end() && changes[curr_line].len == 0)
    {
        return curr_line;
    }
    else
    {
        auto itr = std::lower_bound(linesWithDifferences.begin(), linesWithDifferences.end(), curr_line);

        if (itr == linesWithDifferences.begin())
        {
            return -1;
        }

        return *(--itr);
    }
}

void cbSideBySideCtrl::PrevDifference()
{
    if (tcLeft_->GetSCIFocus())
    {
        long l = PrevDifference(tcLeft_->GetCurrentLine(), linesLeftWithDifferences_, leftChanges_);

        if (l != -1)
        {
            selectDiff(l, -1);
        }
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            long l = PrevDifference(tcRight_->GetCurrentLine(), linesRightWithDifferences_, rightChanges_);

            if (l != -1)
            {
                selectDiff(-1, l);
            }
        }
}

bool cbSideBySideCtrl::CanGotoNextDiff(const std::vector<long> & linesWithDifferences, int currline)
{
    if (linesWithDifferences.empty())
    {
        return false;
    }

    auto i = std::upper_bound(linesWithDifferences.begin(), linesWithDifferences.end(), currline);
    return i != linesWithDifferences.end();
}

bool cbSideBySideCtrl::CanGotoNextDiff()
{
    if (tcLeft_->GetSCIFocus())
    {
        return CanGotoNextDiff(linesLeftWithDifferences_, tcLeft_->GetCurrentLine());
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            return CanGotoNextDiff(linesRightWithDifferences_, tcRight_->GetCurrentLine());
        }

    return false;
}

bool cbSideBySideCtrl::CanGotoPrevDiff(int curr_line, std::vector<long> linesWithDifferences, std::map<long, Block> & changes)
{
    if (curr_line == 0)
    {
        return false;
    }

    if (linesWithDifferences.empty())
    {
        return false;
    }

    auto itr = changes.find(curr_line);

    if (itr != changes.end() && changes[curr_line].len == 0)
    {
        return true;
    }
    else
    {
        auto i = std::lower_bound(linesWithDifferences.begin(), linesWithDifferences.end(), curr_line);
        return i != linesWithDifferences.begin();
    }
}

bool cbSideBySideCtrl::CanGotoPrevDiff()
{
    if (tcLeft_->GetSCIFocus())
    {
        return CanGotoPrevDiff(tcLeft_->GetCurrentLine(), linesLeftWithDifferences_, leftChanges_);
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            return CanGotoPrevDiff(tcRight_->GetCurrentLine(), linesRightWithDifferences_, rightChanges_);
        }

    return false;
}

void cbSideBySideCtrl::FirstDifference()
{
    if (tcLeft_->GetSCIFocus())
    {
        if (linesLeftWithDifferences_.empty())
        {
            return;
        }

        selectDiff(linesLeftWithDifferences_.front(), -1);
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            if (linesRightWithDifferences_.empty())
            {
                return;
            }

            selectDiff(-1, linesRightWithDifferences_.front());
        }
}

void cbSideBySideCtrl::LastDifference()
{
    if (tcLeft_->GetSCIFocus())
    {
        if (linesLeftWithDifferences_.empty())
        {
            return;
        }

        selectDiff(linesLeftWithDifferences_.back(), -1);
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            if (linesRightWithDifferences_.empty())
            {
                return;
            }

            selectDiff(-1, linesRightWithDifferences_.back());
        }
}

bool cbSideBySideCtrl::CanGotoFirstDiff()
{
    if (tcLeft_->GetSCIFocus())
    {
        if (linesLeftWithDifferences_.empty())
        {
            return false;
        }

        return tcLeft_->GetCurrentLine() != linesLeftWithDifferences_.front();
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            if (linesRightWithDifferences_.empty())
            {
                return false;
            }

            return tcRight_->GetCurrentLine() != linesRightWithDifferences_.front();
        }

    return false;
}

bool cbSideBySideCtrl::CanGotoLastDiff()
{
    if (tcLeft_->GetSCIFocus())
    {
        if (linesLeftWithDifferences_.empty())
        {
            return false;
        }

        return tcLeft_->GetCurrentLine() != linesLeftWithDifferences_.back();
    }
    else
        if (tcRight_->GetSCIFocus())
        {
            if (linesRightWithDifferences_.empty())
            {
                return false;
            }

            return tcRight_->GetCurrentLine() != linesRightWithDifferences_.back();
        }

    return false;
}

bool cbSideBySideCtrl::HasDiffSelected()
{
    if (linesLeftWithDifferences_.empty() || linesRightWithDifferences_.empty())
    {
        return false;
    }

    return lastLeftMarkedDiff_ != -1 && lastRightMarkedDiff_ != -1;
}

bool cbSideBySideCtrl::CanCopyToLeft()
{
    return !leftReadOnly_ && HasDiffSelected();
}

bool cbSideBySideCtrl::CanCopyToRight()
{
    return !rightReadOnly_ && HasDiffSelected();
}

void cbSideBySideCtrl::CopyTo(bool toRight)
{
    if (linesLeftWithDifferences_.empty() || linesRightWithDifferences_.empty())
    {
        return;
    }

    auto itrl = leftChanges_.find(lastLeftMarkedDiff_);

    if (itrl == leftChanges_.end())
    {
        return;
    }

    auto itrr = rightChanges_.find(lastRightMarkedDiff_);

    if (itrr == rightChanges_.end())
    {
        return;
    }

    unmarkSelectionDiff(lastLeftMarkedDiff_, leftChanges_, tcLeft_, lastLeftMarkedEmptyDiff_, Removed);
    unmarkSelectionDiff(lastRightMarkedDiff_, rightChanges_, tcRight_, lastRightMarkedEmptyDiff_, Added);
    DeleteMarksForSelection(leftChanges_, lastLeftMarkedDiff_, tcLeft_, lastLeftMarkedEmptyDiff_, Removed);
    DeleteMarksForSelection(rightChanges_, lastRightMarkedDiff_, tcRight_, lastRightMarkedEmptyDiff_, Added);

    if (toRight)
    {
        doCopy(itrl->second, itrr->second, lastLeftMarkedDiff_, lastRightMarkedDiff_, tcLeft_, tcRight_);
    }
    else
    {
        doCopy(itrr->second, itrl->second, lastRightMarkedDiff_, lastLeftMarkedDiff_, tcRight_, tcLeft_);
    }

    auto itl = std::find(linesLeftWithDifferences_.begin(), linesLeftWithDifferences_.end(), lastLeftMarkedDiff_);

    if (itl != linesLeftWithDifferences_.end())
    {
        linesLeftWithDifferences_.erase(itl);
    }

    auto itr = std::find(linesRightWithDifferences_.begin(), linesRightWithDifferences_.end(), lastRightMarkedDiff_);

    if (itr != linesRightWithDifferences_.end())
    {
        linesRightWithDifferences_.erase(itr);
    }

    leftChanges_.erase(itrl);
    rightChanges_.erase(itrr);
    lastLeftMarkedDiff_ = -1;
    lastLeftMarkedEmptyDiff_ = -1;
    lastRightMarkedDiff_ = -1;
    lastRightMarkedEmptyDiff_ = -1;
}

void cbSideBySideCtrl::doCopy(const Block & srcBlock, const Block & dstBlock, long & lastSrcMarkedDiff, long & lastDstMarkedDiff, cbStyledTextCtrl * tcSrc, cbStyledTextCtrl * tcDst)
{
    const int pos = tcDst->PositionFromLine(lastDstMarkedDiff);
    tcDst->BeginUndoAction();

    if (dstBlock.len > 0)
    {
        int start = pos;
        int stop = tcDst->PositionFromLine(lastDstMarkedDiff + dstBlock.len);
        tcDst->DeleteRange(start, stop - start);
    }

    if (srcBlock.len > 0)
    {
        wxString str;

        for (int l = 0 ; l < srcBlock.len ; ++l)
        {
            str += tcSrc->GetLine(lastSrcMarkedDiff + l);
        }

        str.RemoveLast();
        tcDst->InsertText(pos, str);
    }

    tcDst->EndUndoAction();
}

void cbSideBySideCtrl::DeleteMarksForSelection(std::map<long, Block> & changes, const long & lastMarkedDiff, cbStyledTextCtrl * tc, const long & lastMarkedEmptyDiff, AddOrRem ar)
{
    auto itr = changes.find(lastMarkedDiff);

    for (int k = 0 ; k < itr->second.len ; ++k)
    {
        tc->MarkerDelete(lastMarkedDiff + k, (ar == Removed) ? MINUS_MARKER : PLUS_MARKER);
        tc->MarkerDelete(lastMarkedDiff + k, (ar == Removed) ? RED_BKG_MARKER : GREEN_BKG_MARKER);
    }

    if (lastMarkedEmptyDiff != -1)
    {
        tc->AnnotationClearLine(lastMarkedEmptyDiff);
    }
}

bool cbSideBySideCtrl::CanCopyToLeftNext()
{
    return CanCopyToLeft() && CanGotoNextDiff();
}

bool cbSideBySideCtrl::CanCopyToRightNext()
{
    return CanCopyToRight() && CanGotoNextDiff();
}

void cbSideBySideCtrl::CopyToLeftNext()
{
    CopyToLeft();
    NextDifference();
}

void cbSideBySideCtrl::CopyToRightNext()
{
    CopyToRight();
    NextDifference();
}

void cbSideBySideCtrl::selectDiff(long lline, long rline)
{
    if (lline == -1 && rline == -1)
    {
        return;
    }

    if (lline == -1)
    {
        lline = rightChanges_[rline].ref;
    }

    if (rline == -1)
    {
        rline = leftChanges_[lline].ref;
    }

    markSelectionDiff(lline, lastLeftMarkedDiff_, leftChanges_, tcLeft_, tcRight_, lastLeftMarkedEmptyDiff_, lastSyncedLHandle_, Removed);
    markSelectionDiff(rline, lastRightMarkedDiff_, rightChanges_, tcRight_, tcLeft_, lastRightMarkedEmptyDiff_, lastSyncedRHandle_, Added);
}

void cbSideBySideCtrl::markSelectionEmptyPart(long line, long & lastMarkedEmptyDiff, cbStyledTextCtrl * tc, std::map<long, Block> & changes)
{
    if (lastMarkedEmptyDiff != -1)
    {
        tc->AnnotationSetStyle(lastMarkedEmptyDiff, 0);
    }

    int len = changes[line].len;
    int emptyLines = changes[line].empty;

    if (line)
    {
        --line;    // annotation are one below the previous line
    }

    line += len;

    if (emptyLines)
    {
        tc->AnnotationSetStyle(line, STYLE_BLUE_BACK);
        lastMarkedEmptyDiff = line;
    }
    else
    {
        lastMarkedEmptyDiff = -1;
    }
}

void cbSideBySideCtrl::markSelectionDiff(long line, long & lastMarkedDiff, std::map<long, Block> & changes, cbStyledTextCtrl * tc, cbStyledTextCtrl * tcOther, long & lastMarkedEmptyDiff, int & lastSyncedHandle, AddOrRem ar)
{
    if (lastMarkedDiff == line)
    {
        return;
    }

    unmarkSelectionDiff(lastMarkedDiff, changes, tc, lastMarkedEmptyDiff, ar);

    for (int k = 0 ; k < changes[line].len ; ++k)
    {
        tc->MarkerDelete(line + k, (ar == Removed) ? RED_BKG_MARKER : GREEN_BKG_MARKER);
        tc->MarkerAdd(line + k, BLUE_BKG_MARKER);
    }

    if (changes[line].len == 0)
    {
        tcOther->SetFocus();

        if (line)
        {
            tc->GotoLine(line - 1);
            tc->EnsureVisibleEnforcePolicy(line - 1);
        }
    }
    else
    {
        tc->GotoLine(line);
        tc->EnsureVisibleEnforcePolicy(line);
    }

    tc->MarkerDeleteHandle(lastSyncedHandle);
    lastSyncedHandle = -1;
    lastMarkedDiff = line;
    markSelectionEmptyPart(line, lastMarkedEmptyDiff, tc, changes);
}

void cbSideBySideCtrl::unmarkSelectionDiff(const long & lastMarkedDiff, std::map<long, Block> & changes, cbStyledTextCtrl * tc, const long & lastMarkedEmptyDiff, AddOrRem ar)
{
    if (lastMarkedDiff != -1)
        for (int k = 0 ; k < changes[lastMarkedDiff].len ; ++k)
        {
            tc->MarkerDelete(lastMarkedDiff + k, BLUE_BKG_MARKER);
            tc->MarkerAdd(lastMarkedDiff + k, (ar == Removed) ? RED_BKG_MARKER : GREEN_BKG_MARKER);
        }

    if (lastMarkedEmptyDiff != -1)
    {
        tc->AnnotationSetStyle(lastMarkedEmptyDiff, 0);
    }
}

cbEditor * cbSideBySideCtrl::GetCbEditorIfActive(const wxString & filename)
{
    return nullptr; // that's not as easy as it looked like...
    // what if cbEditor gets opened after opening the diff?...
    // we need somehow the reference count of the scintilla document?
    EditorBase * eb = Manager::Get()->GetEditorManager()->IsOpen(filename);

    if (eb && eb->IsBuiltinEditor())
    {
        if (cbEditor * ed = dynamic_cast<cbEditor *>(eb))
        {
            return ed;
        }
    }

    return nullptr;
}

std::vector<std::string> * cbSideBySideCtrl::GetLines(cbStyledTextCtrl * tc)
{
    const unsigned int N = tc->GetLineCount();
    std::vector<std::string> * lines = new std::vector<std::string>;

    for (size_t l = 0 ; l < N ; ++l)
    {
        lines->push_back(tc->GetLine(l).ToStdString());    //.RemoveLast()
    }

    return lines;
}

