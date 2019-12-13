//
// File:        ix_manager.cc
// Description: IX_Manager class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "ix_internal.h"
#include "ix.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <stddef.h>
using namespace std;

// Constructor
IX_Manager::IX_Manager(PF_Manager &pfm) : pfm(pfm) {}

// Destructor
IX_Manager::~IX_Manager()
{
    // Nothing to free
}

RC IX_Manager::CreateIndex(const char *fileName, int indexNo,
                           AttrType attrType, int attrLength)
{
    try
    {
        // Check legality
        if (strchr(fileName, '.') != nullptr)
            throw RC{IX_ILLEGAL_FILENAME};

        // Step 1: Create index file
        // 20 is a hard-code number,
        // it works as long as it could contain the string version of [indexNo].
        char indexFileName[strlen(fileName) + 20] = "";
        strcat(indexFileName, fileName);
        sprintf(indexFileName + strlen(fileName), ".%d", indexNo);
        IX_Try(pfm.CreateFile(indexFileName), IX_MANAGER_CREATE_FAIL);
        // Open index file
        PF_FileHandle indexFileHandle;
        IX_Try(pfm.OpenFile(indexFileName, indexFileHandle), IX_MANAGER_CREATE_OPEN_FILE_FAIL);

        // Step 2: Allocate and write head page
        // If everything runs right, the pagenum of head page should be 0.
        PF_PageHandle headerPageHandle;
        char *headerData;
        IX_Try(indexFileHandle.AllocatePage(headerPageHandle), IX_MANAGER_CREATE_HEAD_FAIL);
        IX_TryElseUnpin(headerPageHandle.GetData(headerData), IX_MANAGER_CREATE_HEAD_FAIL_UNPIN_FAIL, IX_MANAGER_CREATE_HEAD_FAIL, indexFileHandle, 0ll);
        IX_TryElseUnpin(indexFileHandle.MarkDirty(0ll), IX_MANAGER_CREATE_HEAD_FAIL_UNPIN_FAIL, IX_MANAGER_CREATE_HEAD_FAIL, indexFileHandle, 0ll);
        // Store header information into the header page
        *(AttrType *)(headerData + offsetof(IX_IndexHeader, attrType)) = attrType;
        *(int *)(headerData + offsetof(IX_IndexHeader, attrLength)) = attrLength;
        *(PageNum *)(headerData + offsetof(IX_IndexHeader, rootPage)) = 1ll;
        *(PageNum *)(headerData + offsetof(IX_IndexHeader, pageTot)) = 2ll;
        IX_Try(indexFileHandle.UnpinPage(0ll), IX_MANAGER_CREATE_HEAD_BUT_UNPIN_FAIL);

        // Step 3: Allocate and write root page
        PF_PageHandle rootPageHandle;
        char *rootData;
        IX_Try(indexFileHandle.AllocatePage(rootPageHandle), IX_MANAGER_CREATE_ROOT_FAIL);
        IX_TryElseUnpin(rootPageHandle.GetData(rootData), IX_MANAGER_CREATE_ROOT_FAIL_UNPIN_FAIL, IX_MANAGER_CREATE_ROOT_FAIL, indexFileHandle, 1ll);
        IX_TryElseUnpin(indexFileHandle.MarkDirty(1ll), IX_MANAGER_CREATE_ROOT_FAIL_UNPIN_FAIL, IX_MANAGER_CREATE_ROOT_FAIL, indexFileHandle, 1ll);
        *(bool *)(rootData + 0) = true;
        *(int *)(rootData + 1) = 0;
        IX_Try(indexFileHandle.UnpinPage(1ll), IX_MANAGER_CREATE_ROOT_BUT_UNPIN_FAIL);

        // Close the file
        IX_Try(pfm.CloseFile(indexFileHandle), IX_MANAGER_CREATE_BUT_CLOSE_FILE_FAIL);

        // Everything is done.
        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

RC IX_Manager::DestroyIndex(const char *fileName, int indexNo)
{
    try
    {
        // Check legality
        if (strchr(fileName, '.') != nullptr)
            throw RC{IX_ILLEGAL_FILENAME};

        // Calculate [indexFileName]
        char indexFileName[strlen(fileName) + 20] = "";
        strcat(indexFileName, fileName);
        sprintf(indexFileName + strlen(fileName), ".%d", indexNo);
        IX_Try(pfm.DestroyFile(indexFileName), IX_MANAGER_DESTROY_FAIL);

        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle)
{
    try
    {
        // Check legality
        if (strchr(fileName, '.') != nullptr)
            throw RC{IX_ILLEGAL_FILENAME};

        // Calculate [indexFileName]
        char indexFileName[strlen(fileName) + 20] = "";
        strcat(indexFileName, fileName);
        sprintf(indexFileName + strlen(fileName), ".%d", indexNo);

        // Open index file
        IX_Try(pfm.OpenFile(indexFileName, indexHandle.pFFileHandle), IX_MANAGER_OPEN_FAIL);

        // Read header page
        PF_PageHandle headerPageHandle;
        char *headerData;
        IX_Try(indexHandle.pFFileHandle.GetThisPage(0ll, headerPageHandle), IX_MANAGER_OPEN_FAIL);
        IX_TryElseUnpin(headerPageHandle.GetData(headerData), IX_MANAGER_OPEN_FAIL_UNPIN_FAIL, IX_MANAGER_OPEN_FAIL, indexHandle.pFFileHandle, 0ll);
        indexHandle.header.attrType = *(AttrType *)(headerData + offsetof(IX_IndexHeader, attrType));
        indexHandle.header.attrLength = *(int *)(headerData + offsetof(IX_IndexHeader, attrLength));
        indexHandle.header.rootPage = *(PageNum *)(headerData + offsetof(IX_IndexHeader, rootPage));
        indexHandle.header.pageTot = *(PageNum *)(headerData + offsetof(IX_IndexHeader, pageTot));
        indexHandle.header.innerEntryLength = indexHandle.header.attrLength + sizeof(PageNum);
        indexHandle.header.leafEntryLength = indexHandle.header.attrLength + sizeof(RID);
        indexHandle.header.innerDeg = (PF_PAGE_SIZE - sizeof(bool) - sizeof(int)) / indexHandle.header.innerEntryLength;
        indexHandle.header.leafDeg = (PF_PAGE_SIZE - sizeof(bool) - sizeof(int)) / indexHandle.header.leafEntryLength;
        IX_Try(indexHandle.pFFileHandle.UnpinPage(0ll), IX_MANAGER_OPEN_BUT_UNPIN_FAIL);

        printf("Open an Index Manager.\n");
        printf("innerDeg = %d, leafDeg = %d\n", indexHandle.header.innerDeg, indexHandle.header.leafDeg);

        indexHandle.open = true;

        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle)
{
    try
    {
        // Check legality
        if (!indexHandle.open)
            throw RC{IX_MANAGER_CLOSE_CLOSED_FILE_HANDLE};

        // Write back header
        if (indexHandle.header.modified)
        {
            PF_PageHandle headerPage;
            char *headerData;
            IX_Try(indexHandle.pFFileHandle.GetThisPage(0ll, headerPage), IX_MANAGER_CLOSE_FAIL);
            IX_TryElseUnpin(headerPage.GetData(headerData), IX_MANAGER_CLOSE_FAIL_UNPIN_FAIL, IX_MANAGER_CLOSE_FAIL, indexHandle.pFFileHandle, 0ll);
            IX_TryElseUnpin(indexHandle.pFFileHandle.MarkDirty(0ll), IX_MANAGER_CLOSE_FAIL_UNPIN_FAIL, IX_MANAGER_CLOSE_FAIL, indexHandle.pFFileHandle, 0ll);

            *(AttrType *)(headerData + offsetof(IX_IndexHeader, attrType)) = indexHandle.header.attrType;
            *(int *)(headerData + offsetof(IX_IndexHeader, attrLength)) = indexHandle.header.attrLength;
            *(PageNum *)(headerData + offsetof(IX_IndexHeader, rootPage)) = indexHandle.header.rootPage;
            *(PageNum *)(headerData + offsetof(IX_IndexHeader, pageTot)) = indexHandle.header.pageTot;

            IX_Try(indexHandle.pFFileHandle.UnpinPage(0ll), IX_MANAGER_CLOSE_HEAD_BUT_UNPIN_FAIL);

            indexHandle.header.modified = false;
        }

        // Close
        IX_Try(pfm.CloseFile(indexHandle.pFFileHandle), IX_MANAGER_CLOSE_FAIL);
        indexHandle.open = false;

        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}