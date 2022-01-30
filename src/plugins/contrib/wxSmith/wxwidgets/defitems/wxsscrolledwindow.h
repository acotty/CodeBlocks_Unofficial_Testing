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
* $Revision: 12072 $
* $Id: wxsscrolledwindow.h 12072 2020-04-22 22:27:12Z bluehazzard $
* $HeadURL: https://svn.code.sf.net/p/codeblocks/code/trunk/src/plugins/contrib/wxSmith/wxwidgets/defitems/wxsscrolledwindow.h $
*/

#ifndef WXSSCROLLEDWINDOW_H
#define WXSSCROLLEDWINDOW_H

#include "../wxscontainer.h"
#include "../../properties/wxstwolongproperty.h"

class wxsScrolledWindow : public wxsContainer
{
public:

    wxsScrolledWindow(wxsItemResData* Data);

protected:

    virtual wxObject* OnBuildPreview(wxWindow* Parent,long Flags);
    virtual void OnBuildCreatingCode();
    virtual void OnEnumContainerProperties(long Flags);

    wxsTwoLongData m_scrollRate;
};

#endif
