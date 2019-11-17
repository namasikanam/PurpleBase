//
// File:        rm_record.cc
// Description: RM_Record class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <bits/stdc++.h>
#include "rm.h"
#include "rm_rid.h"

// Default constructor
RM_Record::RM_Record() : viable(false) {}

// Destructor
RM_Record::~RM_Record()
{
    // Delete the data if it is a valid record
    releaseData();
}

// Copy constructor
RM_Record::RM_Record(const RM_Record &rec) : rid(rec.rid), viable(rec.viable), dataSize(rec.dataSize)
{
    // Copy data
    copyData(rec);
}

// Overload =
RM_Record &RM_Record::operator=(const RM_Record &rec)
{
    // Check for self-assignment
    if (this != &rec)
    {
        // Copy the data
        copyData(rec);

        // Copy the rid, viability flag and record size
        rid = rec.rid, dataSize = rec.dataSize;
    }

    // Return a reference to this
    return *this;
}

//
// GetData
//
// Desc:    Get record data.
// In:      An empty char*
// Out:     The data of the record
// Ret:     RM return code
RC RM_Record::GetData(char *&pData) const
{
    // If the record is not viable
    if (!viable)
    {
        // Return warning
        return RM_RECORD_NOT_VIABLE;
    }
    else
    {
        // Get data
        pData = this->pData;

        // Return OK
        return OK_RC;
    }
}

//
// GetRid
//
// Desc:    Get rid of the record.
// In:      An empty rid
// Out:     The rid of the record
// Ret:     RM return code
RC RM_Record::GetRid(RID &rid) const
{
    // If the record is not viable
    if (!viable)
    {
        // Return warning
        return RM_RECORD_NOT_VIABLE;
    }
    else
    {
        // Get data
        rid = this->rid;

        // Return OK
        return OK_RC;
    }
}

//
// releasePData
//
// Desc:    Release pData if viable
void RM_Record::releaseData()
{
    if (viable)
    {
        delete[] pData;
        viable = false;
    }
}

//
// reAllocatePData
//
// Desc:    Use rec to reallocate
// In:      A RM_Record to be copied
void RM_Record::copyData(const RM_Record &rec)
{
    if (rec.viable)
    {
        this->releaseData();
        this->pData = new char[rec.dataSize];
        memcpy(pData, rec.pData, sizeof(char) * rec.dataSize);
        viable = true;
    }
}