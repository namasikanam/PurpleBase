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
        for (RC tmp_rc;;)
            switch ((tmp_rc = rMFileHandle.GetRec(RID(curPageNum, curSlotNum), rec)))
            {
            case OK_RC:
                printf("Scan find something at (%lld, %d)\n", curPageNum, curSlotNum);
                nextSlot(curPageNum, curSlotNum, rMFileHandle.slotNumPerPage);
                // Check if this record is satisfied.
                if ([this, rec]() -> bool {
                        char *pData;
                        RM_ChangeRC(rec.GetData(pData), RM_SCAN_NEXT_FAIL);
                        switch (attrType)
                        {
                        case INT:
                        {
                            int recData, scanData;
                            sscanf(pData + attrOffset, "%d", &recData);
                            sscanf((char *)value, "%d", &scanData);
                            switch (compOp)
                            {
                            case NO_OP:
                                return true;
                            case EQ_OP:
                                return recData == scanData;
                            case NE_OP:
                                return recData != scanData;
                            case LT_OP:
                                return recData < scanData;
                            case GT_OP:
                                return recData > scanData;
                            case LE_OP:
                                return recData <= scanData;
                            case GE_OP:
                                return recData >= scanData;
                            }
                            break;
                        }
                        case FLOAT:
                        {
                            float recData, scanData;
                            sscanf(pData + attrOffset, "%f", &recData);
                            sscanf((char *)value, "%f", &scanData);
                            switch (compOp)
                            {
                            case NO_OP:
                                return true;
                            case EQ_OP:
                                return recData == scanData;
                            case NE_OP:
                                return recData != scanData;
                            case LT_OP:
                                return recData < scanData;
                            case GT_OP:
                                return recData > scanData;
                            case LE_OP:
                                return recData <= scanData;
                            case GE_OP:
                                return recData >= scanData;
                            }
                            break;
                        }
                        case STRING:
                        {
                            char recData[attrLength], *scanData = (char *)value;
                            sscanf(pData + attrOffset, "%s", recData);
                            switch (compOp)
                            {
                            case NO_OP:
                                return true;
                            case EQ_OP:
                                return strcmp(attrLength, recData, scanData) == 0;
                            case NE_OP:
                                return strcmp(attrLength, recData, scanData) != 0;
                            case LT_OP:
                                return strcmp(attrLength, recData, scanData) < 0;
                            case GT_OP:
                                return strcmp(attrLength, recData, scanData) > 0;
                            case LE_OP:
                                return strcmp(attrLength, recData, scanData) <= 0;
                            case GE_OP:
                                return strcmp(attrLength, recData, scanData) >= 0;
                            }
                            break;
                        }
                        }
                        // The following statement is added to avoid warning,
                        // which is supposed to not reach
                        return false;
                    }())
                    throw RC{OK_RC};
                break;
            case RM_FILE_GET_NOT_FOUND:
                nextSlot(curPageNum, curSlotNum, rMFileHandle.slotNumPerPage);
                break;
            case RM_FILE_GET_ILLEGAL_RID:
                throw RC{RM_EOF};
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