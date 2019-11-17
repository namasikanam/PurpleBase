//
// File:        rm_rid.cc
// Description: RID class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "rm_rid.h"

// Default constructor
RID::RID()
{
    // Set the viability flag false
    this->viable = false;
}

// Constructor with PageNum and SlotNum as arguments
RID::RID(PageNum _pageNum, SlotNum _slotNum) : pageNum(_pageNum), slotNum(_slotNum)
{
    // Set the viablity flag true
    this->viable = true;
}

// Destructor
RID::~RID() {}

// Copy constructor
RID::RID(const RID &_) : pageNum(_.pageNum), slotNum(_.slotNum), viable(_.viable) {}

// Overload =
RID &RID::operator=(const RID &rid)
{
    // Check for self-assignment
    if (this != &rid)
    {
        // Copy contents
        this->pageNum = rid.pageNum;
        this->slotNum = rid.slotNum;
        this->viable = rid.viable;
    }

    // Return a reference to self
    return (*this);
}

//
// GetPageNum
//
// Desc:    Get page number.
// In:      An empty page number
// Out:     The page number of the RID
// Ret:     RID return code
RC RID::GetPageNum(PageNum &pageNum) const
{
    // If the RID is not viable
    if (!viable)
    {
        // Return warning
        return RID_NOT_VIABLE;
    }
    else
    {
        // Set pageNum to the page number of the RID
        pageNum = this->pageNum;

        // Return OK
        return OK_RC;
    }
}

//
// GetSlotNum
//
// Desc:    Get slot number.
// In:      An empty slot number
// Out:     The slot number of the RID
// Ret:     RID return code
RC RID::GetSlotNum(SlotNum &slotNum) const
{
    // If the RID is not viable
    if (!viable)
    {
        // Return warning
        return RID_NOT_VIABLE;
    }
    else
    {
        // Set slotNum to the slot number of the RID
        slotNum = this->slotNum;

        // Return OK
        return OK_RC;
    }
}