//
// File:        ix_indexscan.cc
// Description: IX_IndexScan class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "ix_internal.h"
#include "ix.h"
#include <iostream>
#include <cstring>
#include <list>
using namespace std;

// Constructor
IX_IndexScan::IX_IndexScan() : open(false)
{ // Set open scan flag to false
}

// Destructor
IX_IndexScan::~IX_IndexScan()
{
    // Nothing to free
}

//
// Desc: Create a list containing rids of all satisfied entries
//
// Note: Rather than similar things to OpenScan in RM component,
//       here we get all satisfied rids when opening.
//       Simply, we just ignore [pinHint].
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp,
                          void *value, ClientHint pinHint)
{
    // [scan] is assumed to be emtpy
    try
    {
        if (compOp == NE_OP)
            throw RC{IX_OPEN_SCAN_NE};

        printf("==== OpenScan: ");
        switch (compOp)
        {
        case EQ_OP:
            printf("[EQ] ");
            break;
        }
        if (indexHandle.header.attrType == STRING)
        {
            for (int i = 0; i < indexHandle.header.attrLength; ++i)
                putchar(*((char *)value + i));
        }
        printf(" =====\n");

        BPlus_Find(indexHandle, indexHandle.header.rootPage, compOp, value);
    }
    catch (RC rc)
    {
        return rc;
    }

    open = true;
    return OK_RC;
}

// Similar to [BPlus_Exists]
void IX_IndexScan::BPlus_Find(const IX_IndexHandle &indexHandle, PageNum nodePageNum, CompOp compOp, void *value)
{
    // printf("BPlus_Find(..., nodePageNum = %lld, ...)\n", nodePageNum);

    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(indexHandle.pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_EXISTS_FAIL);
    IX_TryElseUnpin(indexHandle.pFFileHandle.MarkDirty(nodePageNum), IX_HANDLE_EXISTS_FAIL_UNPIN_FAIL, IX_HANDLE_EXISTS_FAIL, indexHandle.pFFileHandle, nodePageNum);
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_EXISTS_FAIL_UNPIN_FAIL, IX_HANDLE_EXISTS_FAIL, indexHandle.pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));
    if (!isLeaf)
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += indexHandle.header.innerEntryLength)
        {
            // De facto, no condition is also acceptable.
            // We check here, just for speeding up.
            if ([&compOp, &indexHandle, &i, &j, &childTot, &nodePageData, &value]() -> bool {
                    switch (compOp)
                    {
                    case NO_OP:
                        return true;
                    case EQ_OP:
                        return ((indexHandle.cmp(nodePageData + j, value) <= 0) && (i + 1 == childTot || indexHandle.cmp(nodePageData + j + indexHandle.header.innerEntryLength, value) > 0)) || (indexHandle.cmp(nodePageData + j, value) == 0 && indexHandle.cmp(nodePageData + j + indexHandle.header.innerEntryLength, value) == 0);
                    case LT_OP:
                        return indexHandle.cmp(nodePageData + j, value) < 0;
                    case LE_OP:
                        return indexHandle.cmp(nodePageData + j, value) <= 0;
                    case GT_OP:
                        return i + 1 == childTot || indexHandle.cmp(nodePageData + j + indexHandle.header.innerEntryLength, value) > 0;
                    case GE_OP:
                        return i + 1 == childTot || indexHandle.cmp(nodePageData + j + indexHandle.header.innerEntryLength, value) > 0;
                    case NE_OP:
                        throw RC{IX_OPEN_SCAN_NE};
                    }
                    return false; // meaningless return to avoid warning
                }())
            {
                BPlus_Find(indexHandle, *(PageNum *)(nodePageData + j + indexHandle.header.attrLength), compOp, value);
            }
        }
    }
    else
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += indexHandle.header.leafEntryLength)
        {
            if ([&compOp, &indexHandle, &value, &nodePageData, &j]() -> bool {
                    switch (compOp)
                    {
                    case NO_OP:
                        return true;
                    case EQ_OP:
                        return indexHandle.cmp(nodePageData + j, value) == 0;
                    case LT_OP:
                        return indexHandle.cmp(nodePageData + j, value) < 0;
                    case GT_OP:
                        return indexHandle.cmp(nodePageData + j, value) > 0;
                    case LE_OP:
                        return indexHandle.cmp(nodePageData + j, value) <= 0;
                    case GE_OP:
                        return indexHandle.cmp(nodePageData + j, value) >= 0;
                    case NE_OP:
                        throw RC{IX_OPEN_SCAN_NE};
                    }
                    return false; // meaningless return to avoid warning
                }())
            {
                scan.push_back(*(RID *)(nodePageData + j + indexHandle.header.attrLength));
            }
        }
    }
    IX_Try(indexHandle.pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_NOT_EXISTS_BUT_UNPIN_FAIL);
}

RC IX_IndexScan::GetNextEntry(RID &rid)
{
    try
    {
        if (scan.empty())
            throw RC{IX_EOF};
        rid = scan.front();
        scan.pop_front();
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

RC IX_IndexScan::CloseScan()
{
    open = false;
    scan.clear();
    return OK_RC;
}