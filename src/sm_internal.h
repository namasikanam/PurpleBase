//
// File:        sm_internal.h
// Description: Declarations internal to the SM component
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#ifndef SM_INTERNAL_H
#define SM_INTERNAL_H

#include "rm.h"
#include "ix.h"
#include "sm.h"

// A wrapper to execute the API of RM.
inline void SM_Try_RM(RC rm_rc, RC sm_rc)
{
    if (rm_rc)
    {
        RM_PrintError(rm_rc);
        throw RC{sm_rc};
    }
}

// A wrapper to execute the API of IX.
inline void SM_Try_IX(RC ix_rc, RC sm_rc)
{
    if (ix_rc)
    {
        IX_PrintError(ix_rc);
        throw RC{sm_rc};
    }
}

inline void SM_Try_RM_Or_Close_Scan(RC rm_rc, RM_FileScan rmfs, RC succ_rc, RC fail_rc, RC aim_rm_rc = OK_RC)
{
    if (rm_rc != aim_rm_rc)
    {
        RM_PrintError(rm_rc);
        SM_Try_RM(rmfs.CloseScan(), fail_rc);
        throw RC{succ_rc};
    }
}

inline void SM_Try_IX_Or_Close_Scan(RC ix_rc, RM_FileScan rmfs, RC succ_rc, RC fail_rc)
{
    if (ix_rc)
    {
        IX_PrintError(ix_rc);
        SM_Try_IX(rmfs.CloseScan(), fail_rc);
        throw RC{succ_rc};
    }
}

#endif