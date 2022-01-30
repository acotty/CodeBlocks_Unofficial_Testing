// ----------------------------------------------------------------------------
//  JumpData.cpp
// ----------------------------------------------------------------------------
#include "JumpData.h"
#include "Version.h"

// ----------------------------------------------------------------------------
JumpData::JumpData(const wxString& filename, const long posn)
// ----------------------------------------------------------------------------
{
    //ctor
    m_Filename = filename;
    m_Posn = posn;
}

// ----------------------------------------------------------------------------
JumpData::~JumpData()
// ----------------------------------------------------------------------------
{
    //dtor
#if defined(LOGGING)
    LOGIT( _T("JumpData dtor"));
#endif
}
