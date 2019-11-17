//
// File:        rm_filescan.cc
// Description: RM_FileScan class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "rm_internal.h"
#include "rm.h"

// Constructor
RM_FileScan::RM_FileScan() : open(false) {}

// Destructor
RM_FileScan::~RM_FileScan() {}

// Open a scan
RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp, void *value, ClientHint pinHint)
{
    if (!fileHandle.open)
        return RM_SCAN_OPEN_CLOSED_FILE;

    this->rMFileHandle = fileHandle;
    this->attrType = attrType;
    this->attrLength = attrLength;
    this->compOp = compOp;
    this->value = value;

    open = true;
    
    curPageNum = 0;
    curSlotNum = 0;

    return OK_RC;
}

RC RM_FileScan::GetNextRec(RM_Record &rec) {
    try{
        if (!open)
            throw RC{RM_SCAN_CLOSED};
        for (RC tmp_rc; (tmp_rc = rMFileHandle.GetRec(RID(curPageNum, curSlotNum), rec));)
            switch(tmp_rc) {
                case OK_RC:
                    throw RC{OK_RC};
                case RM_FILE_GET_ILLEGAL_RID:
                    throw RC{RM_EOF};
                case RM_FILE_GET_NOT_FOUND:
                    if (curSlotNum + 1 == rMFileHandle.slotNumPerPage)
                    {
                        ++curPageNum;
                        curSlotNum = 0;
                    }
                    else
                        ++curSlotNum;
                    break;
                default:
                    RM_PrintError(tmp_rc);
                    throw RC{RM_SCAN_NEXT_FAIL};
                }
        // Even though the following statement won't run
        // if everything is right. I add it to prevent unnecessary warning.
        throw RC{OK_RC};
    }
    catch (RC rc)
    { return rc; }
}

RC RM_FileScan::CloseScan() {
    open = false;
    return OK_RC;
}