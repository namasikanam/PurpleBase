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
IX_Manager::~IX_Manager() {
    // Nothing to free
}

RC IX_Manager::CreateIndex(const char *fileName, int indexNo,
                           AttrType attrType, int attrLength) {
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
        IX_Try(pfm.CreateFile(indexFileName), IX_CREATE_FAIL);
        // Open index file
        PF_FileHandle indexFileHandle;
        IX_Try(pfm.OpenFile(indexFileName, indexFileHandle), IX_CREATE_OPEN_FILE_FAIL);

        // Step 2: Allocate and write head page
        // If everything runs right, the pagenum of head page should be 0.
        PF_PageHandle headerPageHandle;
        char *headerData;
        IX_Try(indexFileHandle.AllocatePage(headerPageHandle), IX_CREATE_HEAD_FAIL);
        IX_TryElseUnpin(headerPageHandle.GetData(headerData), IX_CREATE_HEAD_FAIL_UNPIN_FAIL, IX_CREATE_HEAD_FAIL, indexFileHandle, 0ll);
        IX_TryElseUnpin(indexFileHandle.MarkDirty(0ll), IX_CREATE_HEAD_FAIL_UNPIN_FAIL, IX_CREATE_HEAD_FAIL, indexFileHandle, 0ll);
        // Store header information into the header page
        *(AttrType *)(headerData + offsetof(IX_IndexHeader, attrType)) = attrType;
        *(int *)(headerData + offsetof(IX_IndexHeader, attrLength)) = attrType;
        *(PageNum *)(headerData + offsetof(IX_IndexHeader, rootPage)) = 2ll;
        *(PageNum *)(headerData + offsetof(IX_IndexHeader, bucketPage)) = 1ll;
        *(int *)(headerData + offsetof(IX_IndexHeader, degree)) = IX_CalDegree(attrType, attrLength);
        *(int *)(headerData + offsetof(IX_IndexHeader, bucketTot)) = 0ll;
        IX_Try(indexFileHandle.UnpinPage(0ll), IX_CREATE_HEAD_BUT_UNPIN_FAIL);

        // Step 4: Allocate and write bucket page
        // Bucket page is only one, so its [pageNum] won't ever change.
        PF_PageHandle bucketPageHandle;
        IX_Try(indexFileHandle.AllocatePage(bucketPageHandle), IX_CREATE_BUCKET_FAIL);
        // Surprise! Bucket page is empty!
        IX_Try(indexFileHandle.UnpinPage(1ll), IX_CREATE_BUCKET_BUT_UNPIN_FAIL);

        // Step 3: Allocate and write root page
        PF_PageHandle rootPageHandle;
        char *rootData;
        IX_Try(indexFileHandle.AllocatePage(rootPageHandle), IX_CREATE_ROOT_FAIL);
        IX_TryElseUnpin(rootPageHandle.GetData(rootData), IX_CREATE_ROOT_FAIL_UNPIN_FAIL, IX_CREATE_ROOT_FAIL, indexFileHandle, 2ll);
        IX_TryElseUnpin(indexFileHandle.MarkDirty(2ll), IX_CREATE_ROOT_FAIL_UNPIN_FAIL, IX_CREATE_ROOT_FAIL, indexFileHandle, 2ll);
        *(int *)rootData = 0;

        // Everything is done.
        throw RC{OK_RC};
    }
    catch (RC rc) {
        return rc;
    }
}

RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
}

RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
}