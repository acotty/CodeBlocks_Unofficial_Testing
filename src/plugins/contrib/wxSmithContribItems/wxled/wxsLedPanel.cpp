#include "wxsLedPanel.h"

namespace
{
#include "images/wxLedPanel16.xpm"
#include "images/wxLedPanel32.xpm"

wxsRegisterItem<wxsLedPanel> Reg(
    _T("wxLEDPanel"),
    wxsTWidget,
    _T("wxWindows"),
    _T("Christian Graefe"),
    _T("graefe@NOSPAM!@mcs-soft.de"),
    _T("http://wxcode.sourceforge.net/components/ledpanel/"),
    _T("Led"),
    80,
    _T("LedPanel"),
    wxsCPP,
    1, 0,
    wxBitmap(wxLedPanel32_xpm),
    wxBitmap(wxLedPanel16_xpm),
    false);

#pragma push_macro("_")
#undef _
#define _(x)   L##x
static const long    Values[] = { wxLED_COLOUR_RED, wxLED_COLOUR_CYAN, wxLED_COLOUR_MAGENTA, wxLED_COLOUR_BLUE, wxLED_COLOUR_GREEN, wxLED_COLOUR_YELLOW, wxLED_COLOUR_GREY };
static const wxChar * Names[]  = { _("Red"), _("Cyan"), _("Magenta"), _("Blue"), _("Green"), _("Yellow"), _("Grey"), nullptr }; // Must end with NULL entry

static const long    ValuesDirection[] = { wxALIGN_RIGHT, wxALIGN_LEFT, wxALIGN_CENTER, wxALIGN_TOP, wxALIGN_BOTTOM };
static const wxChar * NamesDirection[]  = { _("Right"), _("Left"), _("Center"), _("Top"), _("Bottom"), nullptr };

static const long    ValuesContent[] = { wxALIGN_CENTER_VERTICAL, wxALIGN_CENTER_HORIZONTAL, wxALIGN_CENTER};
static const wxChar * NamesContent[]  = { _("Center Vertical"), _("Center Horizontal"), _("Center"), nullptr};

static const long    ValuesScroll[] = { 0, wxLEFT, wxRIGHT, wxUP, wxDOWN};
static const wxChar * NamesScroll[]  = { _("No Scroll"), _("Left"), _("Right"), _("Up"), _("Down"), nullptr};
#pragma pop_macro("_")
}

wxsLedPanel::wxsLedPanel(wxsItemResData * Data) : wxsWidget(Data, &Reg.Info, nullptr, nullptr, flId | flVariable)
{
    //ctor
    LedMatrixSize.X      = 4;
    LedMatrixSize.Y      = 4;
    LedMatrix.X          = 65;
    LedMatrix.Y          = 9;
    Space                = 0;
    Colour               = wxLED_COLOUR_RED;
    Invert               = false;
    Inactiv              = true;
    Text                 = _T("");
    TextPlace            = 1;
    Bold                 = false;
    Align                = wxALIGN_CENTER;
    ContentAlign         = wxALIGN_CENTER;
    ScrollDirection      = 0;
    ScrollSpeed          = 0;
}

wxsLedPanel::~wxsLedPanel()
{
    //dtor
}

void wxsLedPanel::OnBuildCreatingCode()
{
    if (LedMatrixSize.X == -1)
    {
        LedMatrixSize.X = 4;
    }

    if (LedMatrixSize.Y == -1)
    {
        LedMatrixSize.Y = 4;
    }

    if (LedMatrix.X == -1)
    {
        LedMatrix.X = 65;
    }

    if (LedMatrix.Y == -1)
    {
        LedMatrix.Y = 9;
    }

    switch (GetLanguage())
    {
        case wxsCPP:
        {
            AddHeader(_T("<wx/wxledpanel.h>"), GetInfo().ClassName);
            Codef(_T("%C(%W,%I,wxSize( %d, %d),wxSize( %d, %d),%d);\n"), static_cast<int>(LedMatrixSize.X), static_cast<int>(LedMatrixSize.Y), static_cast<int>(LedMatrix.X), static_cast<int>(LedMatrix.Y), static_cast<int>(Space));
            Codef(_T("%ASetContentAlign( %d);\n"), static_cast<int>(ContentAlign));
            Codef(_T("%ASetLEDColour( (wxLEDColour)%d);\n"), static_cast<int>(Colour));
            Codef(_T("%AShowInvertet(%b);\n"), Invert);
            Codef(_T("%AShowInactivLEDs( %b);\n"), Inactiv);

            if (ScrollDirection != 0)
            {
                Codef(_T("%ASetScrollDirection( (wxDirection)%d);\n"), static_cast<int>(ScrollDirection));
                Codef(_T("%ASetScrollSpeed( %d);\n"), static_cast<int>(ScrollSpeed));
            }

            if (Text.Len() > 0)
            {
                Codef(_T("%ASetLetterSpace( %d);\n"), static_cast<int>(TextPlace));

                if (Bold)
                {
                    Codef(_T("%ASetFontType( wxLEDFont7x7);\n"));
                }

                Codef(_T("%ASetText( _T(\"%s\"), %d);\n"), Text.wx_str(), static_cast<int>(Align));
            }

            break;
        }

        case wxsUnknownLanguage: // fall-through
        default:
            wxsCodeMarks::Unknown(_T("wxsLedPanel::OnBuildCreatingCode"), GetLanguage());
    }
}

wxObject * wxsLedPanel::OnBuildPreview(wxWindow * Parent, cb_unused long Flags)
{
    wxLEDPanel * test = new wxLEDPanel(Parent, GetId(), wxSize(LedMatrixSize.X, LedMatrixSize.X), wxSize(LedMatrix.X, LedMatrix.Y), Space);
    test->SetContentAlign(ContentAlign);
    test->SetLEDColour((wxLEDColour)Colour);
    test->ShowInvertet(Invert);
    test->ShowInactivLEDs(Inactiv);

    if (Text.Len() > 0)
    {
        test->SetLetterSpace(TextPlace);

        if (Bold)
        {
            test->SetFontType(wxLEDFont7x7);
        }

        test->SetText(Text, Align);
    }

    return test;
}

void wxsLedPanel::OnEnumWidgetProperties(cb_unused long Flags)
{
    WXS_SIZE(
        wxsLedPanel,
        LedMatrixSize,
        _("LedMatrix Size"),
        _("Matrixwidth"),
        _("Matrixheight"),
        _("Matrix in Dialog-Units"),
        _T("ledMatrix_Size"));
    WXS_SIZE(
        wxsLedPanel,
        LedMatrix,
        _("LedCtrl Size"),
        _("Ctrlwidth"),
        _("Ctrlheight"),
        _("Ctrl in Dialog-Units"),
        _T("ledCtrl_Size"));
    WXS_LONG(
        wxsLedPanel,
        Space,
        _("Space between led"),
        _T("Space"),
        0);
    WXS_ENUM(
        wxsLedPanel,
        Colour,
        _("Led Colour"),
        _T("Led_Colour"),
        Values,
        Names,
        wxLED_COLOUR_RED);
    WXS_BOOL(
        wxsLedPanel,
        Invert,
        _("Invert"),
        _T("Invert_Leds"),
        false);
    WXS_BOOL(
        wxsLedPanel,
        Inactiv,
        _("Inactive"),
        _T("Inactive_Leds"),
        true);
    WXS_FLAGS(
        wxsLedPanel,
        ContentAlign,
        _("Content Align"),
        _T("Content_Align"),
        ValuesContent,
        NamesContent,
        wxALIGN_CENTER);
    WXS_STRING(
        wxsLedPanel,
        Text,
        _("Text"),
        _T("Text"),
        _T(""),
        false);
    WXS_LONG(
        wxsLedPanel,
        TextPlace,
        _("Text Space"),
        _T("Text_Space"),
        1);
    WXS_BOOL(
        wxsLedPanel,
        Bold,
        _("Bold"),
        _T("Bold"),
        false);
    WXS_FLAGS(
        wxsLedPanel,
        Align,
        _("Align"),
        _T("Align"),
        ValuesDirection,
        NamesDirection,
        wxALIGN_CENTER);
    WXS_ENUM(
        wxsLedPanel,
        ScrollDirection,
        _("Scroll"),
        _T("Scroll"),
        ValuesScroll,
        NamesScroll,
        0);
    WXS_LONG(
        wxsLedPanel,
        ScrollSpeed,
        _("Scroll Speed"),
        _T("Scroll_Speed"),
        0);
}
