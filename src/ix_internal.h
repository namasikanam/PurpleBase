//
// File:        ix_internal.h
// Description: Declarations internal to the IX component
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H

#include "ix.h"
#include "rm_rid.h"
#include <algorithm>

// A wrapper to execute the API of PF.
inline void IX_Try(RC pf_rc, RC ix_rc)
{
    if (pf_rc)
    {
        PF_PrintError(pf_rc);
        throw RC{ix_rc};
    }
}

// Try and if fail, unpin
void IX_TryElseUnpin(RC pf_rc, RC unpin_rc, RC ix_rc, const PF_FileHandle &file, const PageNum &pageNum)
{
    if (pf_rc)
    {
        PF_PrintError(pf_rc);
        IX_Try(file.UnpinPage(pageNum), unpin_rc);
        throw RC{ix_rc};
    }
}

// Calculate degree
inline int IX_CalDegree(int attrLength)
{
    return (PF_PAGE_SIZE - sizeof(int)) / (attrLength + std::max(sizeof(PageNum), sizeof(RID)));
}

#endif