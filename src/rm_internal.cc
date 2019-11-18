//
// File:        rm_internal.cc
// Description: Implementations of declarations in rm_internal.h
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "rm.h"

//
// If the first rc is not equal to 0,
// then print error, and return the next rc.
//
void RM_ChangeRC(RC pf_rc, RC rm_rc)
{
    if (pf_rc)
    {
        PF_PrintError(pf_rc);
        throw RC{rm_rc};
    }
}

//
// Try to do something, if fail,
// then try to unpin some page
//
void RM_TryElseUnpin(RC pf_rc, RC unpin_rc, RC rm_rc, const PF_FileHandle &file, const PageNum &pageNum)
{
    if (pf_rc)
    {
        PF_PrintError(pf_rc);
        RM_ChangeRC(file.UnpinPage(pageNum), unpin_rc);
        throw RC{rm_rc};
    }
}

//
// Calculate next slot
// Availibility doesn't count here
//
void nextSlot(PageNum &pageNum, SlotNum &slotNum, SlotNum slotNumPerPage) {
    if (slotNum + 1 == slotNumPerPage) {
        ++pageNum;
        slotNum = 0;
    }
    else
        ++slotNum;
}

//
// A imitater of strcmp
// the limitation of length is added
//
int strcmp(int length, char *s1, char *s2) {
    for (int i = 0; i < length; ++i)
        if (s1[i] != s2[i])
            return s1[i] < s2[i] ? -1 : 1;
    return 0;
}