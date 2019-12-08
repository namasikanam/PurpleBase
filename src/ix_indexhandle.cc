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

        if (BPlus_Exists(header.rootPage, pData, rid))
            throw RC{IX_HANDLE_INSERT_EXISTS};
        else
            BPlus_Insert(header.rootPage, pData, rid);
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

        if (!BPlus_Exists(header.rootPage, pData, rid))
            throw RC{IX_HANDLE_DELETE_NOT_EXIST};
        else
            BPlus_Delete(header.rootPage, pData, rid);
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

bool IX_IndexHandle::BPlus_Exists(const PageNum &nodePageNum, const void *pData, const RID &rid)
{
    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_INSERT_FAIL);
    IX_TryElseUnpin(pFFileHandle.MarkDirty(nodePageNum), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));
    if (!isLeaf)
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerKeyLength)
        {
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerKeyLength) < 0) || cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerKeyLength) == 0)
            { // Regular condition: [, )
                if (BPlus_Exists(*(PageNum *)(nodePageData + j + header.attrLength), pData, rid))
                    return true;
            }
        }
    }
    else
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.leafKeyLength)
        {
            // For leaf, there's no segment more.
            // Keys stored in the node are the exact keys.
            if (cmp(pData, nodePageData + j) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

//
// Desc: Insert a (pData, rid) into the B+ tree.
// Note: If there have existed some keys equal to the inserted key,
// this one will inserted to the right of them.
const void *IX_IndexHandle::BPlus_Insert(const PageNum &nodePageNum, const void *pData, const RID &rid)
{
    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_INSERT_FAIL);
    IX_TryElseUnpin(pFFileHandle.MarkDirty(nodePageNum), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));
    if (isLeaf)
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerKeyLength)
        {
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1) || cmp(pData, nodePageData + j + header.innerKeyLength))
            {
                ++i, j += header.innerKeyLength;
                // Now, the correct position has been found!
                if (childTot + 1 < header.leafDeg)
                { // There's some empty room remaining, just insert it!
                    // Move the right ones one step
                    memmove(nodePageData + j + header.innerKeyLength, nodePageData + j, header.innerKeyLength * (childTot - i));
                    // Copy the inserting information
                    memcpy(nodePageData + j, pData, header.attrLength);
                    *(RID *)(nodePageData + j + header.attrLength) = rid;
                }
                else
                { // No more room! A split is waiting!
                    // Create a new node as the right one: [0, (leafDeg + 1) / 2)
                    // The original node will be the left one: [(leafDeg + 1) / 2, leafDeg + 1)

                    // Allocate a new page
                    PF_PageHandle rightPageHandle;
                    char *rightPageData;
                    PageNum rightPageNum = header.pageTot + 1;
                    // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                    IX_Try(pFFileHandle.AllocatePage(rightPageHandle), IX_HANDLE_SPLIT_FAIL);
                    IX_TryElseUnpin(rightPageHandle.GetData(rightPageData), IX_HANDLE_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_SPLIT_FAIL, pFFileHandle, rightPageNum);
                    ++header.pageTot;
                    header.modified = true;

                    // Write the right data to the new page
                    *(bool *)rightPageData = true;
                    *(int *)(rightPageData + sizeof(bool)) = (header.leafDeg + 1) - (header.leafDeg + 1) / 2;
                    if (i >= (header.leafDeg + 1) / 2)
                    { // The inserted entry located in the right page
                        memcpy(rightPageData + sizeof(bool))
                    }
                    else
                    { // The inserted entry located in the left page
                    }

                    // Adjust the data of the original node

                    if (header.rootPage == nodePageNum)
                    { // If this is the root
                        // Create a new root
                    }
                    else
                    {
                    }
                }
                break;
            }
        }
    }
    else
    {
        // Find the correct child to insert
        if ()
        { // If the inserted child splits
            if (childTot < header.innerDeg)
            {
            }
            else
            {
                if ()
                { // If this is the root
                }
                else
                {
                }
            }
        }
    }
}

bool IX_IndexHandle::BPlus_Delete(const PageNum &nodePageNum, const void *pData, const RID &rid)
{
    // TODO: BPlus_Delete
}

// Even though an update can be decomposed to an insert and a deletion intuitionally,
// but things are not like that because of the applied lazy deletion.
void BPlus_Update(const PageNum &nodePageNum, const void *pData, const RID &rid)
{
    // TODO: BPlus_Update
}

int IX_IndexHandle::cmp(const void *data1, const void *data2)
{
    switch (header.attrType)
    {
    case FLOAT:
        if (*(float *)data1 < *(float *)data2)
            return -1;
        else if (*(float *)data1 == *(float *)data2)
            return 0;
        else
            return 1;
    case INT:
        if (*(int *)data1 < *(int *)data2)
            return -1;
        else if (*(int *)data1 == *(int *)data2)
            return 0;
        else
            return 1;
    case STRING:
        for (int i = 0; i < header.attrLength; ++i)
        {
            char char1 = ((const char *)data1)[i], char2 = ((const char *)data2)[i];
            if (char1 != char2)
                return char1 < char2 ? -1 : 1;
        }
        return 0;
    }
}