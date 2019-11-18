//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "rm_internal.h"
#include "rm.h"
#include <stddef.h>
using namespace std;

RM_Manager::RM_Manager(PF_Manager &pfm): pFManager(pfm) {}

RM_Manager::~RM_Manager() {}

RC RM_Manager::CreateFile(const char *fileName, int recordSize) {
    try
    {
        // Is my size too large?
        if (recordSize >= PF_PAGE_SIZE)
            throw RC{RM_MANAGER_RECORDSIZE_TOO_LARGE};
        
        // Create file
        RM_ChangeRC(pFManager.CreateFile(fileName), RM_MANAGER_CREATE_FAIL);

        // Allocate header page
        PF_FileHandle pFFileHandle;
        pFManager.OpenFile(fileName, pFFileHandle);
        PF_PageHandle headerPage;
        RM_ChangeRC(pFFileHandle.AllocatePage(headerPage), RM_MANAGER_CREATE_FAIL);

        // Write header page
        char *headerPageData;
        RM_TryElseUnpin(headerPage.GetData(headerPageData), RM_MANAGER_CREATE_FAIL_UNPIN_FAIL, RM_MANAGER_CREATE_FAIL, pFFileHandle, 0ll);
        RM_TryElseUnpin(pFFileHandle.MarkDirty(0ll), RM_MANAGER_CREATE_FAIL_UNPIN_FAIL, RM_MANAGER_CREATE_FAIL, pFFileHandle, 0ll);

        // There're four variables to write into the header page.
        // recordSize is an input argument
        char *headerPageDataPtr = headerPageData;
        *(int *)(headerPageDataPtr) = recordSize;
        // recordTot is equal to 1
        headerPageDataPtr += sizeof(int);
        *(SlotNum *)headerPageDataPtr = 1;
        // pageTot is equal to 0
        headerPageDataPtr += sizeof(SlotNum);
        *(PageNum *)headerPageDataPtr = 0ll;
        // pageAvailable is just empty.

        // Header page is written and being unpinned
        RM_ChangeRC(pFFileHandle.UnpinPage(0ll), RM_MANAGER_CREATE_BUT_UNPIN_FAIL);
        RM_ChangeRC(pFManager.CloseFile(pFFileHandle), RM_MANAGER_CREATE_BUT_CLOSE_FAIL);
        throw RC{OK_RC};
    }
    catch (RC rc) { return rc; }
}

RC RM_Manager::DestroyFile(const char *fileName) {
    try {
        RM_ChangeRC(pFManager.DestroyFile(fileName), RM_MANAGER_DESTROY_FAIL);
        throw RC{OK_RC};
    }
    catch (RC rc) {
        return rc;
    }
}

RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle) {
    try
    {
        // Open PF File
        RM_ChangeRC(pFManager.OpenFile(fileName, fileHandle.pFFileHandle), RM_MANAGER_OPEN_FAIL);
        
        // Read header file
        PF_PageHandle headerPage;
        char *headerPageData;
        RM_ChangeRC(fileHandle.pFFileHandle.GetFirstPage(headerPage), RM_MANAGER_OPEN_FAIL);
        RM_ChangeRC(headerPage.GetData(headerPageData), RM_MANAGER_OPEN_FAIL);
        
        // Read from data
        // Similar to the situation of output
        char *headerPageDataPtr = headerPageData;
        fileHandle.recordSize = *(int *)headerPageDataPtr;
        headerPageDataPtr += sizeof(int);
        fileHandle.recordTot = *(SlotNum *)headerPageDataPtr;
        headerPageDataPtr += sizeof(SlotNum);
        fileHandle.pageTot = *(PageNum *)headerPageDataPtr;
        headerPageDataPtr += sizeof(PageNum);
        fileHandle.pageAvailable.clear();
        for (PageNum pageID = (fileHandle.pageTot + 7) / 8; pageID--; ++headerPageDataPtr)
            fileHandle.pageAvailable.push_back(*headerPageDataPtr);

        // After read, you need to unpin
        RM_ChangeRC(fileHandle.pFFileHandle.UnpinPage(0ll), RM_MANAGER_OPEN_BUT_UNPIN_FAIL);

        // Some information needed to be set or calculated
        fileHandle.open = true;
        fileHandle.headerModified = false;
        fileHandle.slotNumPerPage = 1;
        while((fileHandle.slotNumPerPage + 1) * fileHandle.recordSize + (fileHandle.slotNumPerPage + 1 + 7) / 8 <= PF_PAGE_SIZE)
            ++fileHandle.slotNumPerPage;

        throw RC{OK_RC};
    }
    catch (RC rc) { return rc; }
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {
    try
    {
        // You can't close closed file.
        if (!fileHandle.open)
            throw RC{RM_MANAGER_CLOSE_CLOSED_FILE};

        // If need, write back header page
        if (fileHandle.headerModified) {
            // Get Data
            PF_PageHandle headerPage;
            char *headerPageData;
            RM_ChangeRC(fileHandle.pFFileHandle.GetFirstPage(headerPage), RM_MANAGER_CLOSE_FAIL);
            RM_TryElseUnpin(headerPage.GetData(headerPageData), RM_MANAGER_CLOSE_FAIL_UNPIN_FAIL, RM_MANAGER_CLOSE_FAIL, fileHandle.pFFileHandle, 0ll);
            // Differently, dirty mark is needed here
            RM_TryElseUnpin(fileHandle.pFFileHandle.MarkDirty(0ll), RM_MANAGER_CLOSE_FAIL_UNPIN_FAIL, RM_MANAGER_CLOSE_FAIL, fileHandle.pFFileHandle, 0ll);

            // Write the information into data
            char *headerPageDataPtr = headerPageData;
            *(int *)(headerPageDataPtr) = fileHandle.recordSize;
            headerPageDataPtr += sizeof(int);
            *(SlotNum *)headerPageDataPtr = fileHandle.recordTot;
            headerPageDataPtr += sizeof(SlotNum);
            *(PageNum *)headerPageDataPtr = fileHandle.pageTot;
            headerPageDataPtr += sizeof(PageNum);
            for (char pageID : fileHandle.pageAvailable)
                *(headerPageDataPtr++) = pageID;
            
            // Unpin after write
            RM_ChangeRC(fileHandle.pFFileHandle.UnpinPage(0ll), RM_MANAGER_CLOSE_BUT_UNPIN_FAIL);
        }

        // Close
        RM_ChangeRC(pFManager.CloseFile(fileHandle.pFFileHandle), RM_MANAGER_CLOSE_FAIL);

        // Do some destruction
        // For safety this is necessary
        fileHandle.open = false;
        fileHandle.headerModified = false;
        fileHandle.pageAvailable.clear();

        throw RC{OK_RC};
    }
    catch (RC rc) { return rc; }
}