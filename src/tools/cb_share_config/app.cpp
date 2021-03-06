/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 */

#include "app.h"

//(*AppHeaders
#include "mainframe.h"
#include <wx/image.h>
//*)

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    //(*AppInitialize
    bool wxsOK = true;
    wxInitAllImageHandlers();

    if (wxsOK)
    {
        MainFrame * Frame = new MainFrame(0);
        Frame->Show();
        SetTopWindow(Frame);
    }

    //*)
    return wxsOK;
}
