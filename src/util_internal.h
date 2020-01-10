//
// File:        util_internal.h
// Description: Declarations internal to the utils
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#ifndef UTIL_INTERNAL_H
#define UTIL_INTERNAL_H

#include "rm.h"
#include "ix.h"
#include "sm.h"
#include <cstdlib>

// A wrapper to execute the API of RM.
inline void Try_RM(RC rm_rc)
{
    if (rm_rc)
    {
        RM_PrintError(rm_rc);
        exit(1);
    }
}

// A wrapper to execute the API of IX.
inline void Try_IX(RC ix_rc)
{
    if (ix_rc)
    {
        IX_PrintError(ix_rc);
        exit(1);
    }
}

// A wrapper to execute the API of SM.
inline void Try_SM(RC sm_rc)
{
    if (sm_rc)
    {
        SM_PrintError(sm_rc);
        exit(1);
    }
}

#endif