//
// File:        rm_internal.h
// Description: Declarations internal to the RM component
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

#include "rm.h"

// Some wrappers for code-convenient
void RM_ChangeRC(RC pf_rc, RC rm_rc);
void RM_TryElseUnpin(RC pf_rc, RC unpin_rc, RC rm_rc, const PF_FileHandle &file, const PageNum &pageNum);

// Some functions for convenience
void nextSlot(PageNum &pageNum, SlotNum &slotNum, SlotNum slotNumPerPage);
int strcmp(int length, char *s1, char *s2);

#endif