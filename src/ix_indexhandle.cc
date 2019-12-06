//
// File:        ix_indexhandle.cc
// Description: IX_IndexHandle class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "ix_internal.h"
#include "ix.h"
#include <cstring>
#include <iostream>
using namespace std;

// Constructor
IX_IndexHandle::IX_IndexHandle() : open(false) {}

// Destructor
IX_IndexHandle::~IX_IndexHandle() {}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid)
{
    try
    {
        if (!open)
            throw RC{IX_HANDLE_CLOSED};

        InsertBPlus(header.rootPage, pData, rid);
    }
    catch (RC rc)
    {
        return rc;
    }
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid)
{
    try
    {
        if (!open)
            throw RC{IX_HANDLE_CLOSED};

        DeleteBPlus(header.rootPage, pData, rid);
    }
    catch (RC rc)
    {
        return rc;
    }
}

RC IX_IndexHandle::ForcePages()
{
    try
    {
        if (!open)
            throw RC{IX_HANDLE_CLOSED};

        IX_Try(pFFileHandle.ForcePages(), IX_HANDLE_FORCE_FAIL);

        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

// TODO: Throw an error if the rid to insert has been inserted.
void IX_IndexHandle::InsertBPlus(const PageNum &nodePageNum, const void *pData, const RID &rid)
{
    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_INSERT_FAIL);
    IX_TryElseUnpin(pFFileHandle.MarkDirty(nodePageNum));
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)(nodePageData + 0);
    int childTot = *(int *)(nodePageData + 1);
    if (isLeaf)
    {
    }
    else
    {
    }
}

// TODO: Throw an error if the rid to delete has been inserted.
void IX_IndexHandle::DeleteBPlus(const PageNum &nodePageNum, const void *pData, const RID &rid)
{
}