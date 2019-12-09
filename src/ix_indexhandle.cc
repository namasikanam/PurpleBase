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

        // It's not necessary that check if [BPlus_Exists],
        // since the [BPlus_Delete] is almost the same as [BPlus_Exists].
        if (!BPlus_Delete(header.rootPage, pData, rid))
            throw RC{IX_HANDLE_DELETE_NOT_EXIST};
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
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0) || cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerEntryLength) == 0)
            { // Regular condition: [, )
                if (BPlus_Exists(*(PageNum *)(nodePageData + j + header.attrLength), pData, rid))
                    return true;
            }
        }
    }
    else
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.leafEntryLength)
        {
            // For leaf, there's no segment more.
            // Keys stored in the node are the exact keys.
            if (cmp(pData, nodePageData + j) == 0 && ((RID *)(nodePageData + j + header.attrLength))->viable)
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
const pair<void *, PageNum> IX_IndexHandle::BPlus_Insert(const PageNum &nodePageNum, const void *pData, const RID &rid)
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
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.leafEntryLength)
        {
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1) || cmp(pData, nodePageData + j + header.leafEntryLength))
            {
                ++i, j += header.leafEntryLength;
                // Now, the correct position has been found!
                if (childTot + 1 < header.leafDeg)
                { // There's some empty room remaining, just insert it!
                    // ++childTot
                    ++*(int *)(nodePageData + sizeof(bool));
                    // Move the right ones by one unit
                    memmove(nodePageData + j + header.leafEntryLength, nodePageData + j, header.leafEntryLength * (childTot - i));
                    // Copy the inserting information
                    memcpy(nodePageData + j, pData, header.attrLength);
                    *(RID *)(nodePageData + j + header.attrLength) = rid;

                    return make_pair(nullptr, -1);
                }
                else
                { // No more room! A split is going on!
                    // Create a new node as the right one: [0, (leafDeg + 1) / 2)
                    // The original node will be the left one: [(leafDeg + 1) / 2, leafDeg + 1)

                    // Allocate a new page
                    PF_PageHandle rightPageHandle;
                    char *rightPageData;
                    PageNum rightPageNum = header.pageTot;
                    // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                    IX_Try(pFFileHandle.AllocatePage(rightPageHandle), IX_HANDLE_SPLIT_FAIL);
                    ++header.pageTot;
                    IX_TryElseUnpin(rightPageHandle.GetData(rightPageData), IX_HANDLE_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_SPLIT_FAIL, pFFileHandle, rightPageNum);
                    header.modified = true;

                    // Write the right data to the new page
                    *(bool *)rightPageData = true;
                    *(int *)(rightPageData + sizeof(bool)) = (header.leafDeg + 1) - (header.leafDeg + 1) / 2;
                    if (i >= (header.leafDeg + 1) / 2)
                    { // The inserted entry located in the right page
                        // Copy entries before the inserted entry
                        memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + (header.leafDeg + 1) / 2 * header.leafEntryLength, (i - (header.leafDeg + 1) / 2) * header.leafEntryLength);

                        int j1 = sizeof(bool) + sizeof(int) + (i - (header.leafDeg + 1) / 2) * header.leafEntryLength; // The [j] in the right page
                        memcpy(rightPageData + j1, pData, header.attrLength);
                        *(RID *)(rightPageData + j1 + header.attrLength) = rid;

                        // Copy entries after the inserted entry
                        memcpy(rightPageData + j1 + header.leafEntryLength, nodePageData + j, (childTot - j) * header.leafEntryLength);
                    }
                    else
                    { // The inserted entry located in the left page
                        memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + ((header.leafDeg + 1) / 2 - 1) * header.leafEntryLength, (header.leafDeg + 1 - (header.leafDeg + 1) / 2) * header.leafEntryLength);
                    }

                    // Adjust the data of the original node
                    *(int *)(nodePageData + sizeof(bool)) = (header.leafDeg + 1) / 2;
                    if (i < (header.leafDeg + 1) / 2)
                    { // The inserted entry needs inserting in the left page
                        // Move the right ones by one unit
                        memmove(nodePageData + j + header.innerEntryLength, nodePageData + j, header.innerEntryLength * ((header.leafDeg + 1) / 2 - i));
                        // Copy the inserting information
                        memcpy(nodePageData + j, pData, header.attrLength);
                        *(RID *)(nodePageData + j + header.attrLength) = rid;
                    }
                    else
                    { // The inserted entry needs inserting in the right page
                        // Nothing to do
                    }

                    if (header.rootPage == nodePageNum)
                    { // If this is the root
                        // Create a new root
                        PF_PageHandle rootPageHandle;
                        char *rootPageData;
                        // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                        IX_Try(pFFileHandle.AllocatePage(rootPageHandle), IX_HANDLE_SPLIT_FAIL);
                        header.rootPage = header.pageTot;
                        ++header.pageTot;
                        IX_TryElseUnpin(rootPageHandle.GetData(rightPageData), IX_HANDLE_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_SPLIT_FAIL, pFFileHandle, header.rootPage);
                        // [header.modified] is assumed to be set to [true] in the executions above.

                        *(bool *)rootPageData = false;
                        *(int *)(rootPageData + sizeof(bool)) = 2;

                        memcpy(rootPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int), header.attrLength);
                        *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.attrLength) = nodePageNum;
                        memcpy(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength, rightPageData + sizeof(bool) + sizeof(int), header.attrLength);
                        *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength + header.attrLength) = rightPageNum;

                        // What to return is meaningless
                        return make_pair(nullptr, -1ll);
                    }
                    else
                        return make_pair(rightPageData + sizeof(bool) + sizeof(int), rightPageNum);
                }
                break;
            }
        }
    }
    else
    {
        // Things here are similar to things above
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            // Find the correct child to insert
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1) || cmp(pData, nodePageData + j + header.innerEntryLength))
            {
                pair<void *, PageNum> insertedChild = BPlus_Insert(*(PageNum *)(nodePageData + j), pData, rid);
                if (insertedChild.first != nullptr)
                { // If the inserted child splits
                    void *pData = insertedChild.first;
                    PageNum pageNum = insertedChild.second;
                    if (childTot + 1 < header.innerDeg)
                    { // There's some emtpy room
                        // ++childTot
                        ++*(int *)(nodePageData + sizeof(bool));
                        // Move the right ones by one unit
                        memmove(nodePageData + j + header.innerEntryLength, nodePageData + j, header.innerEntryLength * (childTot - i));
                        // Copy the inserting information
                        memcpy(nodePageData + j, pData, header.attrLength);
                        *(PageNum *)(nodePageData + j + header.attrLength) = pageNum;

                        return make_pair(nullptr, -1);
                    }
                    else
                    { // No more room! A split is going on!
                        // Create a new node as the right one: [0, (innerDeg + 1) / 2)
                        // The original node will be left one: [(innerDeg + 1) / 2, innerDeg + 1)

                        // Allocate a new page
                        PF_PageHandle rightPageHandle;
                        char *rightPageData;
                        PageNum rightPageNum = header.pageTot;
                        // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                        IX_Try(pFFileHandle.AllocatePage(rightPageHandle), IX_HANDLE_SPLIT_FAIL);
                        ++header.pageTot;
                        IX_TryElseUnpin(rightPageHandle.GetData(rightPageData), IX_HANDLE_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_SPLIT_FAIL, pFFileHandle, rightPageNum);
                        header.modified = true;

                        // Write the right data to the new page
                        *(bool *)rightPageData = true;
                        *(int *)(rightPageData + sizeof(bool)) = (header.innerDeg + 1) - (header.innerDeg + 1) / 2;
                        if (i >= (header.innerDeg + 1) / 2)
                        { // The inserted entry located in the right page
                            // Copy entries before the inserted entry
                            memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + (header.innerDeg + 1) / 2 * header.innerEntryLength, (i - (header.innerDeg + 1) / 2) * header.innerEntryLength);

                            int j1 = sizeof(bool) + sizeof(int) + (i - (header.innerDeg + 1) / 2) * header.innerEntryLength; // The [j] in the right page
                            memcpy(rightPageData + j1, pData, header.attrLength);
                            *(PageNum *)(rightPageData + j1 + header.attrLength) = pageNum;

                            // Copy entries after the inserted entry
                            memcpy(rightPageData + j1 + header.innerEntryLength, nodePageData + j, (childTot - j) * header.innerEntryLength);
                        }
                        else
                        { // The inserted entry located in the left page
                            memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + ((header.innerDeg + 1) / 2 - 1) * header.innerEntryLength, (header.innerDeg + 1 - (header.innerDeg + 1) / 2) * header.innerEntryLength);
                        }

                        // Adjust the data of the original node
                        *(int *)(nodePageData + sizeof(bool)) = (header.innerDeg + 1) / 2;
                        if (i < (header.innerDeg + 1) / 2)
                        { // The inserted entry needs inserting in the left page
                            // Move the right ones by one unit
                            memmove(nodePageData + j + header.innerEntryLength, nodePageData + j, header.innerEntryLength * ((header.innerDeg + 1) / 2 - i));
                            // Copy the inserting information
                            memcpy(nodePageData + j, pData, header.attrLength);
                            *(PageNum *)(nodePageData + j + header.attrLength) = pageNum;
                        }
                        else
                        { // The inserted entry needs inserting in the right page
                            // Nothing to do
                        }

                        if (header.rootPage == nodePageNum)
                        { // If this is the root
                            // Create a new root
                            PF_PageHandle rootPageHandle;
                            char *rootPageData;
                            // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                            IX_Try(pFFileHandle.AllocatePage(rootPageHandle), IX_HANDLE_SPLIT_FAIL);
                            header.rootPage = header.pageTot;
                            ++header.pageTot;
                            IX_TryElseUnpin(rootPageHandle.GetData(rightPageData), IX_HANDLE_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_SPLIT_FAIL, pFFileHandle, header.rootPage);
                            // [header.modified] is assumed to be set to [true] in the executions above.

                            *(bool *)rootPageData = false;
                            *(int *)(rootPageData + sizeof(bool)) = 2;

                            memcpy(rootPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int), header.attrLength);
                            *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.attrLength) = nodePageNum;
                            memcpy(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength, rightPageData + sizeof(bool) + sizeof(int), header.attrLength);
                            *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength + header.attrLength) = rightPageNum;

                            // What to return is meaningless
                            return make_pair(nullptr, -1ll);
                        }
                        else
                            return make_pair(rightPageData + sizeof(bool) + sizeof(int), rightPageNum);
                    }
                }
                else
                    return make_pair(nullptr, -1);
                break;
            }
        }
    }
}

//
// Desc: Delete some entry fromm B+ tree
//
// Note: Similar to [BPlus_Exists]
bool IX_IndexHandle::BPlus_Delete(const PageNum &nodePageNum, const void *pData, const RID &rid)
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
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0) || cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerEntryLength) == 0)
            { // Regular condition: [, )
                if (BPlus_Delete(*(PageNum *)(nodePageData + j + header.attrLength), pData, rid))
                    return true;
            }
        }
    }
    else
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.leafEntryLength)
        {
            // For leaf, there's no segment more.
            // Keys stored in the node are the exact keys.
            if (cmp(pData, nodePageData + j) == 0 && ((RID *)(nodePageData + j + header.attrLength))->viable)
            {
                ((RID *)(nodePageData + j + header.attrLength))->viable = false;
                return true;
            }
        }
    }
    return false;
}

//
// Desc: Update some entry
//
// Even though an update can be decomposed to an insert and a deletion intuitionally,
// but things are not like that because of the applied lazy deletion.
bool IX_IndexHandle::BPlus_Update(const PageNum &nodePageNum, const void *pData, const RID &origin_rid, const RID &updated_rid)
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
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            if (cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0) || cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerEntryLength) == 0)
            { // Regular condition: [, )
                if (BPlus_Update(*(PageNum *)(nodePageData + j + header.attrLength), pData, origin_rid, updated_rid))
                    return true;
            }
        }
    }
    else
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.leafEntryLength)
        {
            // For leaf, there's no segment more.
            // Keys stored in the node are the exact keys.
            if (cmp(pData, nodePageData + j) == 0 && ((RID *)(nodePageData + j + header.attrLength))->viable)
            {
                *((RID *)(nodePageData + j + header.attrLength)) = updated_rid;
                return true;
            }
        }
    }
    return false;
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