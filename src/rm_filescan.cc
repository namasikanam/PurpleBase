//
// File:        rm_filescan.cc
// Description: RM_FileScan class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "rm_internal.h"
#include "rm.h"
#include <bits/stdc++.h>
using namespace std;

// Constructor
RM_FileScan::RM_FileScan() : open(false) {}

// Destructor
RM_FileScan::~RM_FileScan() {}

// Open a scan
RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp, const void *value, ClientHint pinHint)
{
    if (!fileHandle.open)
        return RM_SCAN_OPEN_CLOSED_FILE;

#ifdef RM_LOG
    printf("=== OpenScan ===\n");
#endif

    this->rMFileHandle = fileHandle;
    this->attrType = attrType;
    this->attrLength = attrLength;
    this->attrOffset = attrOffset;
    this->compOp = compOp;

    if (attrType == STRING && value != nullptr)
    {
#ifdef RM_LOG
        printf("Open scan with %s\n", (const char *)value);
#endif

        char *s = new char[attrLength];
        const char *_s = (const char *)value;
        memset(s, 0, attrLength);
        for (int i = 0; i < attrLength && _s[i]; ++i)
            s[i] = _s[i];
        this->value = s;
    }
    else
        this->value = value;

    open = true;

    curPageNum = 0;
    curSlotNum = 0;

#ifdef RM_LOG
    printf("scanvalue = ");
    if (this->value != nullptr)
    {
        switch (attrType)
        {
        case INT:
            printf("%d", *(int *)(this->value));
            break;
        case FLOAT:
            printf("%f", *(float *)(this->value));
            break;
        case STRING:
            for (int i = 0; i < attrLength; ++i)
                putchar(((char *)(this->value))[i]);
            break;
        }
    }
    else
        printf("nullptr");
    putchar('\n');
#endif

    return OK_RC;
}

// This is the core of the File Scan
RC RM_FileScan::GetNextRec(RM_Record &rec)
{
    try
    {
        if (!open)
            throw RC{RM_SCAN_CLOSED};
        for (RC tmp_rc;;)
            switch ((tmp_rc = rMFileHandle.GetRec(RID(curPageNum, curSlotNum), rec)))
            {
            case OK_RC:
#ifdef RM_LOG
                printf("Scan find something at (%lld, %d)\n", curPageNum, curSlotNum);
#endif

                nextSlot(curPageNum, curSlotNum, rMFileHandle.slotNumPerPage);

#ifdef RM_LOG
                // puts("Gone to next slot!");
#endif

                // Check if this record is satisfied.
                if ([this, &rec]() -> bool {
                        char *pData;
                        RM_ChangeRC(rec.GetData(pData), RM_SCAN_NEXT_FAIL);

#ifdef RM_LOG
                        printf("recData = ");
#endif
                        switch (attrType)
                        {
                        case INT:
                        {
                            int recData = *(int *)(pData + attrOffset), scanData = *(int *)value;

#ifdef RM_LOG
                            printf("%d\n", recData);
#endif

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
                            float recData = *(float *)(pData + attrOffset), scanData = *(float *)value;

#ifdef RM_LOG
                            printf("%f\n", recData);
#endif

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
                            char *recData = pData + attrOffset, *scanData = (char *)value;

#ifdef RM_LOG
                            for (int i = 0; i < attrLength; ++i)
                                putchar(recData[i]);
                            puts("");
#endif

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
                {
                    throw RC{OK_RC};
                }
                break;
            case RM_FILE_GET_NOT_FOUND:
                // There's no record at this rid,
                // because the record at this slot is deleted.
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
    {
        return rc;
    }
}

// I think that deleting [value] is not my responsibility.
RC RM_FileScan::CloseScan()
{
    open = false;

    if (attrType == STRING && value != nullptr)
    {
        delete[] value;
    }

    return OK_RC;
}