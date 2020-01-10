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

#ifdef IX_LOG
        printf("==== Insert: ");
        Attr_Print(pData);
        printf(" =====\n");
#endif

        if (BPlus_Exists(header.rootPage, pData, rid))
            throw RC{IX_HANDLE_INSERT_EXISTS};
        else
        {
            BPlus_Insert(header.rootPage, pData, rid);
        }
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid)
{
    try
    {
        if (!open)
            throw RC{IX_HANDLE_CLOSED};

#ifdef IX_LOG
        printf("==== Delete: ");
        Attr_Print(pData);
        printf(" =====\n");
#endif

        // It's not necessary that check if [BPlus_Exists],
        // since the [BPlus_Delete] is almost the same as [BPlus_Exists].
        if (!BPlus_Delete(header.rootPage, pData, rid))
            throw RC{IX_HANDLE_DELETE_NOT_EXIST};
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
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
    return OK_RC;
}

bool IX_IndexHandle::BPlus_Exists(PageNum nodePageNum, const void *pData, const RID &rid)
{
#ifdef IX_LOG
    // printf("IX_IndexHandle:: BPlus_Exists(nodePageNum = %lld)\n", nodePageNum);
#endif

    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_EXISTS_FAIL);
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_EXISTS_FAIL_UNPIN_FAIL, IX_HANDLE_EXISTS_FAIL, pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));

#ifdef IX_LOG
    // printf("IX_IndexHandle::BPlus_Exsits(nodePageNum = %d) isLeaf = %d, childTot = %d\n", nodePageNum, isLeaf, childTot);
#endif

    if (!isLeaf)
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
#ifdef IX_LOG
            // printf("IX_IndexHandle::BPlus_Exsits(nodePageNum = %d) isLeaf = %d, the page number of child %d is %lld\n", nodePageNum, isLeaf, i, *(nodePageData + j + header.attrLength));
#endif

            if ((cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0)) || (i != childTot - 1 && cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerEntryLength) == 0))
            { // Regular condition: [, )
                if (BPlus_Exists(*(PageNum *)(nodePageData + j + header.attrLength), pData, rid))
                {
                    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INNER_EXISTS_BUT_UNPIN_FAIL);
                    return true;
                }
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
                // if (header.attrType == STRING)
                // {
                //     printf("Find existing: ");
                //     for (int i = 0; i < header.attrLength; ++i)
                //         putchar(*((char *)pData + i));
                //     printf(": STRING[%d]\n", header.attrLength);
                // }

                IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_LEAF_EXISTS_BUT_UNPIN_FAIL);
                return true;
            }
        }
    }

#ifdef IX_LOG
    // printf("IX_IndexHandle::BPlus_Exists: does not exist!\n");
#endif

    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_NOT_EXISTS_BUT_UNPIN_FAIL);
    return false;
}

//
// Desc: Insert a (pData, rid) into the B+ tree.
// Note: If there have existed some keys equal to the inserted key,
// this one will inserted in some arbitrary place of them.
const pair<const void *, PageNum> IX_IndexHandle::BPlus_Insert(PageNum nodePageNum, const void *pData, const RID &rid)
{
    // printf("BPlus_Insert(nodePageNum = %lld)\n", nodePageNum);

    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_INSERT_FAIL);
    IX_TryElseUnpin(pFFileHandle.MarkDirty(nodePageNum), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_INSERT_FAIL_UNPIN_FAIL, IX_HANDLE_INSERT_FAIL, pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));
    if (isLeaf)
    {
        // printf("Insert: Leaf node.\n");

        for (int i = -1, j = sizeof(bool) + sizeof(int) - header.leafEntryLength; i < childTot; ++i, j += header.leafEntryLength)
        {
            // if (header.attrType == STRING && i >= 0 && i < childTot)
            // {
            //     printf("cmp(pData, %d) = %d\n", i, cmp(pData, nodePageData + j));
            //     printf("The condition = %d, [i == childTot - 1] = %d\n", (int)((cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0))), (int)(i == childTot - 1));
            // }

            if (childTot == 0 || (i == -1 && cmp(pData, nodePageData + j + header.leafEntryLength) < 0) || (i >= 0 && cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.leafEntryLength) < 0)))
            {
                // printf("Find a place to insert.\n");

                ++i, j += header.leafEntryLength;
                // Now, the correct position has been found!
                if (childTot + 1 <= header.leafDeg)
                { // There's some empty room remaining, just insert it!
                    // ++childTot
                    ++*(int *)(nodePageData + sizeof(bool));

                    // Move the right ones by one unit
                    memmove(nodePageData + j + header.leafEntryLength, nodePageData + j, header.leafEntryLength * (childTot - i));

                    // Copy the inserting information
                    memcpy(nodePageData + j, pData, header.attrLength);
                    *(RID *)(nodePageData + j + header.attrLength) = rid;

                    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_LEAF_JUST_INSERT_BUT_UNPIN_FAIL);
                    return make_pair(nullptr, -1);
                }
                else
                {   // No more room! A split is going on!
                    // Create a new node as the right one: [0, (leafDeg + 1) / 2)
                    // The original node will be the left one: [(leafDeg + 1) / 2, leafDeg + 1)

#ifdef IX_LOG
                    printf("A split of a CHILD starts!\n");
                    printf("The data will be inserted at %d\n", i);
#endif

                    // Allocate a new page
                    PF_PageHandle rightPageHandle;
                    char *rightPageData;
                    PageNum rightPageNum = header.pageTot;
                    // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                    IX_Try(pFFileHandle.AllocatePage(rightPageHandle), IX_HANDLE_LEAF_SPLIT_FAIL);
                    ++header.pageTot;
                    IX_TryElseUnpin(rightPageHandle.GetData(rightPageData), IX_HANDLE_LEAF_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_LEAF_SPLIT_FAIL, pFFileHandle, rightPageNum);
                    header.modified = true;

                    PageNum correctRightPageNum;
                    rightPageHandle.GetPageNum(correctRightPageNum);

                    // printf("Guessed rightPageNum = %lld, correct rightPageNum = %lld\n", rightPageNum, correctRightPageNum);

                    // printf("Write data to the right page.\n");

                    // Write the right data to the new page
                    *(bool *)rightPageData = true;
                    *(int *)(rightPageData + sizeof(bool)) = (header.leafDeg + 1) - (header.leafDeg + 1) / 2;
                    if (i >= (header.leafDeg + 1) / 2)
                    { // The inserted entry located in the right page
                        // printf("Right page is inserted.\n");

                        // Copy entries before the inserted entry

                        // printf("Copy the entry before.\n");

                        memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + (header.leafDeg + 1) / 2 * header.leafEntryLength, (i - (header.leafDeg + 1) / 2) * header.leafEntryLength);

                        // printf("Copy the inserted one.\n");

                        int j1 = sizeof(bool) + sizeof(int) + (i - (header.leafDeg + 1) / 2) * header.leafEntryLength; // The [j] in the right page

                        // printf("Insert key.\n");

                        memcpy(rightPageData + j1, pData, header.attrLength);

                        // printf("Insert RID at (rightPageData + %d).\n", j1 + header.attrLength);

                        *(RID *)(rightPageData + j1 + header.attrLength) = rid;

                        // printf("Copy the entry after.\n");

                        // Copy entries after the inserted entry
                        memcpy(rightPageData + j1 + header.leafEntryLength, nodePageData + j, (childTot - i) * header.leafEntryLength);
                    }
                    else
                    { // The inserted entry located in the left page
                        // printf("Right page is not inserted.\n");

                        memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + ((header.leafDeg + 1) / 2 - 1) * header.leafEntryLength, (header.leafDeg + 1 - (header.leafDeg + 1) / 2) * header.leafEntryLength);
                    }

                    // printf("Bye~ My right page~\n");

                    // Adjust the data of the original node
                    *(int *)(nodePageData + sizeof(bool)) = (header.leafDeg + 1) / 2;
                    if (i < (header.leafDeg + 1) / 2)
                    { // The inserted entry needs inserting in the left page
                        // Move the right ones by one unit
                        memmove(nodePageData + j + header.leafEntryLength, nodePageData + j, header.leafEntryLength * ((header.leafDeg + 1) / 2 - 1 - i));
                        // Copy the inserting information
                        memcpy(nodePageData + j, pData, header.attrLength);
                        *(RID *)(nodePageData + j + header.attrLength) = rid;
                    }
                    else
                    { // The inserted entry needs inserting in the right page
                        // Nothing to do
                    }

                    // printf("The original node is adjusted.\n");

                    // printf("Now, header.rootPage = %lld\n", header.rootPage);
                    // printf("nodePageNum = %lld\n ", nodePageNum);

                    if (header.rootPage == nodePageNum)
                    { // If this is the root
                        // printf("Create a new root.\n");

                        // Create a new root
                        PF_PageHandle rootPageHandle;
                        char *rootPageData;
                        // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                        IX_Try(pFFileHandle.AllocatePage(rootPageHandle), IX_HANDLE_LEAF_NEW_ROOT_FAIL);
                        header.rootPage = header.pageTot;
                        ++header.pageTot;
                        IX_TryElseUnpin(rootPageHandle.GetData(rootPageData), IX_HANDLE_LEAF_NEW_ROOT_FAIL_UNPIN_FAIL, IX_HANDLE_LEAF_NEW_ROOT_FAIL, pFFileHandle, header.rootPage);
                        // [header.modified] is assumed to be set to [true] in the executions above.

                        *(bool *)rootPageData = false;
                        *(int *)(rootPageData + sizeof(bool)) = 2;

                        memcpy(rootPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int), header.attrLength);
                        *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.attrLength) = nodePageNum;
                        memcpy(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength, rightPageData + sizeof(bool) + sizeof(int), header.attrLength);
                        *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength + header.attrLength) = rightPageNum;

                        // printf("Unpin the right page %lld.\n", rightPageNum);

                        IX_Try(pFFileHandle.UnpinPage(rightPageNum), IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_RIGHT_FAIL);

                        // printf("Unpin the root page %lld.\n", header.rootPage);

                        IX_Try(pFFileHandle.UnpinPage(header.rootPage), IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_ROOT_FAIL);

                        // printf("Unpin the node page %lld.\n", nodePageNum);

                        IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_FAIL);
                        return make_pair(nullptr, -1ll);
                    }
                    else
                    {
                        // printf("No new root is needed.\n");

                        void *key;
                        switch (header.attrType)
                        {
                        case INT:
                            key = new int(*(int *)(rightPageData + sizeof(bool) + sizeof(int)));
                        case FLOAT:
                            key = new double(*(double *)(rightPageData + sizeof(bool) + sizeof(int)));
                        case STRING:
                            key = new char[header.attrLength];
                            memcpy(key, rightPageData + sizeof(bool) + sizeof(int), header.attrLength);
                            break;
                        }

                        IX_Try(pFFileHandle.UnpinPage(rightPageNum), IX_HANDLE_INSERT_LEAF_SPLIT_BUT_UNPIN_RIGHT_FAIL);
                        IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_LEAF_SPLIT_BUT_UNPIN_FAIL);
                        return make_pair(key, rightPageNum);
                    }
                }
                break;
            }
        }
    }
    else
    {
        // printf("Insert: Inner node.\n");

        // Things here are similar to things above
        for (int i = -1, j = sizeof(bool) + sizeof(int) - header.innerEntryLength; i < childTot; ++i, j += header.innerEntryLength)
        {
            // Find the correct child to insert
            if ((i == -1 && cmp(pData, nodePageData + j + header.innerEntryLength) < 0) || (i >= 0 && cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0)))
            {
#ifdef IX_LOG
                printf("At page %d, insert in child %d (page %d).\n", nodePageNum, i, *(PageNum *)(nodePageData + j + (i == -1 ? header.innerEntryLength : 0) + header.attrLength));
#endif
                if (i == -1)
                {
                    ++i;
                    j += header.innerEntryLength;
                    memcpy(nodePageData + j, pData, header.attrLength);
                }

                pair<const void *, PageNum> insertedChild = BPlus_Insert(*(PageNum *)(nodePageData + j + header.attrLength), pData, rid);
                if (insertedChild.first != nullptr)
                { // If the inserted child splits
                    ++i, j += header.innerEntryLength;

                    const void *pData = insertedChild.first;
                    PageNum pageNum = insertedChild.second;
                    if (childTot + 1 <= header.innerDeg)
                    { // There's some emtpy room
                        // ++childTot
                        ++*(int *)(nodePageData + sizeof(bool));
                        // Move the right ones by one unit
                        memmove(nodePageData + j + header.innerEntryLength, nodePageData + j, header.innerEntryLength * (childTot - i));
                        // Copy the inserting information
                        memcpy(nodePageData + j, pData, header.attrLength);
                        *(PageNum *)(nodePageData + j + header.attrLength) = pageNum;

                        IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_INNER_JUST_INSERT_BUT_UNPIN_FAIL);
                        return make_pair(nullptr, -1);
                    }
                    else
                    { // No more room! A split is going on!
                        // Create a new node as the right one: [0, (innerDeg + 1) / 2)
                        // The original node will be left one: [(innerDeg + 1) / 2, innerDeg + 1)

                        // printf("A split starts!\n");

                        // Allocate a new page
                        PF_PageHandle rightPageHandle;
                        char *rightPageData;
                        PageNum rightPageNum = header.pageTot;
                        // Since we never deallocate a page, the pagenum will be allocated sequentially here.
                        IX_Try(pFFileHandle.AllocatePage(rightPageHandle), IX_HANDLE_INNER_SPLIT_FAIL);
                        ++header.pageTot;
                        IX_TryElseUnpin(rightPageHandle.GetData(rightPageData), IX_HANDLE_INNER_SPLIT_FAIL_UNPIN_FAIL, IX_HANDLE_INNER_SPLIT_FAIL, pFFileHandle, rightPageNum);
                        header.modified = true;

#ifdef IX_LOG
                        printf("Trying to split an inner node %lld.\n", nodePageNum);

                        printf("Before split, the children of %lld are ", nodePageNum);
                        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
                        {
                            printf("%d ", *(nodePageData + j + header.attrLength));
                        }
                        puts("");
#endif

                        // Write the right data to the new page
                        *(bool *)rightPageData = false;
                        *(int *)(rightPageData + sizeof(bool)) = (header.innerDeg + 1) - (header.innerDeg + 1) / 2;
                        if (i >= (header.innerDeg + 1) / 2)
                        { // The inserted entry located in the right page
#ifdef IX_LOG
                            printf("IX_IndexHandle::BPlus_Insert(nodePageNum = %lld) The inserted entry located in the right page.\n", nodePageNum);
#endif

                            // Copy entries before the inserted entry
                            memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + (header.innerDeg + 1) / 2 * header.innerEntryLength, (i - (header.innerDeg + 1) / 2) * header.innerEntryLength);

                            int j1 = sizeof(bool) + sizeof(int) + (i - (header.innerDeg + 1) / 2) * header.innerEntryLength; // The [j] in the right page
                            memcpy(rightPageData + j1, pData, header.attrLength);
                            *(PageNum *)(rightPageData + j1 + header.attrLength) = pageNum;

                            // Copy entries after the inserted entry
                            memcpy(rightPageData + j1 + header.innerEntryLength, nodePageData + j, (childTot - i) * header.innerEntryLength);

#ifdef IX_LOG
                            for (int i = 0, j = sizeof(bool) + sizeof(int); i < (header.innerDeg + 1) - (header.innerDeg + 1) / 2; ++i, j += header.innerEntryLength)
                            {
                                printf("IX_IndexHandle::BPlus_Insert(nodePageNum = %d) rightPageNum = %lld, after split, the child %d of page %lld is %d\n", nodePageNum, rightPageNum, i, rightPageNum, *(rightPageData + j + header.attrLength));
                            }
#endif
                        }
                        else
                        { // The inserted entry located in the left page
                            memcpy(rightPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int) + ((header.innerDeg + 1) / 2 - 1) * header.innerEntryLength, (header.innerDeg + 1 - (header.innerDeg + 1) / 2) * header.innerEntryLength);
                        }

                        // printf("Split an inner node.\n");

                        // Adjust the data of the original node
                        *(int *)(nodePageData + sizeof(bool)) = (header.innerDeg + 1) / 2;
                        if (i < (header.innerDeg + 1) / 2)
                        { // The inserted entry needs inserting in the left page
                            // Move the right ones by one unit
                            memmove(nodePageData + j + header.innerEntryLength, nodePageData + j, header.innerEntryLength * ((header.innerDeg + 1) / 2 - 1 - i));
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
                            IX_Try(pFFileHandle.AllocatePage(rootPageHandle), IX_HANDLE_INNER_NEW_ROOT_FAIL);
                            header.rootPage = header.pageTot;
                            ++header.pageTot;
                            IX_TryElseUnpin(rootPageHandle.GetData(rootPageData), IX_HANDLE_INNER_NEW_ROOT_FAIL_UNPIN_FAIL, IX_HANDLE_INNER_NEW_ROOT_FAIL, pFFileHandle, header.rootPage);
                            // [header.modified] is assumed to be set to [true] in the executions above.

                            *(bool *)rootPageData = false;
                            *(int *)(rootPageData + sizeof(bool)) = 2;

                            memcpy(rootPageData + sizeof(bool) + sizeof(int), nodePageData + sizeof(bool) + sizeof(int), header.attrLength);
                            *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.attrLength) = nodePageNum;
                            memcpy(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength, rightPageData + sizeof(bool) + sizeof(int), header.attrLength);
                            *(PageNum *)(rootPageData + sizeof(bool) + sizeof(int) + header.innerEntryLength + header.attrLength) = rightPageNum;

                            IX_Try(pFFileHandle.UnpinPage(header.rootPage), IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_ROOT_FAIL);

                            IX_Try(pFFileHandle.UnpinPage(rightPageNum), IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_RIGHT_FAIL);
                            IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_FAIL);
                            return make_pair(nullptr, -1ll);
                        }
                        else
                        {
                            void *key;
                            switch (header.attrType)
                            {
                            case INT:
                                key = new int(*(int *)(rightPageData + sizeof(bool) + sizeof(int)));
                                break;
                            case FLOAT:
                                key = new double(*(double *)(rightPageData + sizeof(bool) + sizeof(int)));
                                break;
                            case STRING:
                                key = new char[header.attrLength];
                                memcpy(key, rightPageData + sizeof(bool) + sizeof(int), header.attrLength);
                                break;
                            }

                            IX_Try(pFFileHandle.UnpinPage(rightPageNum), IX_HANDLE_INSERT_INNER_SPLIT_BUT_UNPIN_RIGHT_FAIL);
                            IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_INNER_SPLIT_BUT_UNPIN_FAIL);
                            return make_pair(key, rightPageNum);
                        }
                    }
                }
                else
                {
                    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_INSERT_BUT_UNPIN_FAIL);
                    return make_pair(nullptr, -1);
                }
                break;
            }
        }
    }
    // Nothing happens,
    // which won't happen
    // if everything works normally
    // printf("Nothing to insert here!\n");
    return make_pair(nullptr, 0ll);
}

//
// Desc: Delete some entry fromm B+ tree
//
// Note: Similar to [BPlus_Exists]
bool IX_IndexHandle::BPlus_Delete(PageNum nodePageNum, const void *pData, const RID &rid)
{
    PF_PageHandle nodePageHandle;
    char *nodePageData;
    IX_Try(pFFileHandle.GetThisPage(nodePageNum, nodePageHandle), IX_HANDLE_DELETE_FAIL);
    IX_TryElseUnpin(pFFileHandle.MarkDirty(nodePageNum), IX_HANDLE_DELETE_FAIL_UNPIN_FAIL, IX_HANDLE_DELETE_FAIL, pFFileHandle, nodePageNum);
    IX_TryElseUnpin(nodePageHandle.GetData(nodePageData), IX_HANDLE_DELETE_FAIL_UNPIN_FAIL, IX_HANDLE_DELETE_FAIL, pFFileHandle, nodePageNum);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));
    if (!isLeaf)
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            if ((cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0)) || (cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerEntryLength) == 0))
            { // Regular condition: [, )
                if (BPlus_Delete(*(PageNum *)(nodePageData + j + header.attrLength), pData, rid))
                {
                    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_DELETE_INNER_BUT_UNPIN_FAIL);
                    return true;
                }
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
                IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_DELETE_LEAF_BUT_UNPIN_FAIL);
                return true;
            }
        }
    }
    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_NOT_DELETE_BUT_UNPIN_FAIL);
    return false;
}

//
// Desc: Update some entry
//
// Even though an update can be decomposed to an insert and a deletion intuitionally,
// but things are not like that because of the applied lazy deletion.
bool IX_IndexHandle::BPlus_Update(PageNum nodePageNum, const void *pData, const RID &origin_rid, const RID &updated_rid)
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
            if ((cmp(pData, nodePageData + j) >= 0 && (i == childTot - 1 || cmp(pData, nodePageData + j + header.innerEntryLength) < 0)) || (cmp(pData, nodePageData + j) == 0 && cmp(pData, nodePageData + j + header.innerEntryLength) == 0))
            { // Regular condition: [, )
                if (BPlus_Update(*(PageNum *)(nodePageData + j + header.attrLength), pData, origin_rid, updated_rid))
                {
                    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_UPDATE_INNER_BUT_UNPIN_FAIL);
                    return true;
                }
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
                IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_UPDATE_LEAF_BUT_UNPIN_FAIL);
                return true;
            }
        }
    }
    IX_Try(pFFileHandle.UnpinPage(nodePageNum), IX_HANDLE_NOT_UPDATE_BUT_UNPIN_FAIL);
    return false;
}

void IX_IndexHandle::BPlus_Print(PageNum nodePageNum) const
{
    PF_PageHandle nodePageHandle;
    char *nodePageData;
    pFFileHandle.GetThisPage(nodePageNum, nodePageHandle);
    nodePageHandle.GetData(nodePageData);

    bool isLeaf = *(bool *)nodePageData;
    int childTot = *(int *)(nodePageData + sizeof(bool));

    printf("Page %lld is %s, the children are: ", nodePageNum, isLeaf ? "a child" : "an inner node");

    if (!isLeaf)
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            InnerEntry_Print(nodePageData + j);
            putchar(' ');
        }
        puts("");

        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.innerEntryLength)
        {
            BPlus_Print(*(PageNum *)(nodePageData + j + header.attrLength));
        }
    }
    else
    {
        for (int i = 0, j = sizeof(bool) + sizeof(int); i < childTot; ++i, j += header.leafEntryLength)
        {
            LeafEntry_Print(nodePageData + j);
            putchar(' ');
        }
        puts("");
    }
    pFFileHandle.UnpinPage(nodePageNum);
}

int IX_IndexHandle::cmp(const void *data1, const void *data2) const
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
            char char1 = ((const char *)data1)[i];
            char char2 = ((const char *)data2)[i];
            if (char1 != char2)
                return char1 < char2 ? -1 : 1;
        }
        return 0;
    }
    return 0; // This command won't run if everything works normally
}

void IX_IndexHandle::InnerEntry_Print(void *data) const
{
    printf("(");
    Attr_Print(data);
    printf(", %lld)", *(PageNum *)(data + header.attrLength));
}
void IX_IndexHandle::LeafEntry_Print(void *data) const
{
    printf("(");
    Attr_Print(data);
    RID rid = *(RID *)(data + header.attrLength);
    printf(", {pageNum = %lld, slotNum = %d, viable = %d}) ", rid.pageNum, rid.slotNum, rid.viable);
}

void IX_IndexHandle::Attr_Print(void *data) const
{
    switch (header.attrType)
    {
    case INT:
        printf("%d", *(int *)data);
        break;
    case FLOAT:
        printf("%f", *(float *)data);
        break;
    case STRING:
        for (int k = 0; k < header.attrLength; ++k)
        {
            char c = *(char *)(data + k);
            if (c != ' ')
                putchar(c);
        }
    };
}