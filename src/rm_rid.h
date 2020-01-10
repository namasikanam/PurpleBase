//
// rm_rid.h
//
//   The Record Id interface
//

#ifndef RM_RID_H
#define RM_RID_H

// We separate the interface of RID from the rest of RM because some
// components will require the use of RID but not the rest of RM.

#include "redbase.h"

//
// PageNum: uniquely identifies a page in a file
//
typedef long long PageNum;

//
// SlotNum: uniquely identifies a record in a page
//
typedef int SlotNum;

//
// RID: Record id interface
//
class RID
{
public:
    RID();
    RID(PageNum pageNum, SlotNum slotNum);
    ~RID();
    RID(const RID &rid);
    RID &operator=(const RID &rid);

    RC GetPageNum(PageNum &pageNum) const; // Return page number
    RC GetSlotNum(SlotNum &slotNum) const; // Return slot number

    bool viable;     // Viablilty flag
    PageNum pageNum; // Page number
    SlotNum slotNum; // Slot number
};

#endif
