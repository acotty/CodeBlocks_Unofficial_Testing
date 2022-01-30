/*
* This file is part of wxSmith plugin for Code::Blocks Studio
* Copyright (C) 2006-2007  Bartlomiej Swiecki
*
* wxSmith is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* wxSmith is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with wxSmith. If not, see <http://www.gnu.org/licenses/>.
*
* $Revision: 7109 $
* $Id: wxsbitmapbutton.h 7109 2011-04-15 11:53:16Z mortenmacfly $
* $HeadURL: https://svn.code.sf.net/p/codeblocks/code/trunk/src/plugins/contrib/wxSmith/wxwidgets/defitems/wxsbitmapbutton.h $
*/

#ifndef WXSBITMAPBUTTON_H
#define WXSBITMAPBUTTON_H

#include "../wxswidget.h"

/** \brief Class for wxButton widget */
class wxsBitmapButton: public wxsWidget
{
public:

    wxsBitmapButton(wxsItemResData* Data);

private:

    virtual void OnBuildCreatingCode();
    virtual wxObject* OnBuildPreview(wxWindow* Parent,long Flags);
    virtual void OnEnumWidgetProperties(long Flags);

    bool IsDefault;
    wxsBitmapData BitmapLabel;
    wxsBitmapData BitmapDisabled;
    wxsBitmapData BitmapSelected;
    wxsBitmapData BitmapFocus;
};

#endif
