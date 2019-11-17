//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "rm_internal.h"
#include "rm.h"
using namespace std;

RM_Manager::RM_Manager(PF_Manager &pfm): pFManager(pfm) {}

RM_Manager::~RM_Manager() {}

RC RM_Manager::CreateFile(const char *fileName, int recordSize) {
    try {
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
        char *headerPageDataPtr = headerPageData + 4;
        sprintf(headerPageData, "%d", recordSize);
        // recordTot is equal to 1
        if (sizeof(SlotNum) == 4) {
            sprintf(headerPageDataPtr + 4, "%d", 1);
            headerPageDataPtr += 4;
        }
        else if (sizeof(SlotNum) == 8) {
            sprintf(headerPageDataPtr + 8, "%lld", 1ll);
            headerPageDataPtr += 8;
        }
        else
            throw RC{RM_UNKNOWN_SLOTNUM};
        // pageTot is equal to 0
        if (sizeof(PageNum) == 4)
            sprintf(headerPageDataPtr + 4, "%d", 0);
        else if(sizeof(PageNum) == 8)
            sprintf(headerPageDataPtr + 8, "%lld", 0ll);
        else
            throw RC{RM_UNKNOWN_PAGENUM};
        // pageAvailable is just empty.

        // Header page is written and being unpinned
        RM_ChangeRC(pFFileHandle.UnpinPage(0ll), RM_MANAGER_CREATE_BUT_UNPIN_FAIL);
        throw RC{OK_RC};
    }
    catch (RC rc) {
        return rc;
    }
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
    try {
        // Open PF File
        PF_FileHandle pFFileHandle;
        RM_ChangeRC(pFManager.OpenFile(fileName, pFFileHandle), RM_MANAGER_OPEN_FAIL);
        
        // Read header file
        PF_PageHandle headerPage;
        char *headerPageData;
        RM_ChangeRC(pFFileHandle.GetFirstPage(headerPage), RM_MANAGER_OPEN_FAIL);
        RM_ChangeRC(headerPage.GetData(headerPageData), RM_MANAGER_OPEN_FAIL);
        
        // Read from data
        // Similar to the situation of output
        char *headerPageDataPtr = headerPageData + 4;
        sscanf(headerPageData, "%d", &fileHandle.recordSize);
        if (sizeof(SlotNum) == 4)
        {
            sscanf(headerPageDataPtr + 4, "%d", &fileHandle.recordTot);
            headerPageDataPtr += 4;
        }
        else if (sizeof(SlotNum) == 8)
        {
            sscanf(headerPageDataPtr + 8, "%lld", &fileHandle.recordTot);
            headerPageDataPtr += 8;
        }
        else
            throw RC{RM_UNKNOWN_SLOTNUM};
        if (sizeof(PageNum) == 4) {
            sscanf(headerPageDataPtr + 4, "%d", &fileHandle.pageTot);
            headerPageDataPtr += 4;
        }
        else if (sizeof(PageNum) == 8) {
            sprintf(headerPageDataPtr + 8, "%lld", &fileHandle.pageTot);
            headerPageDataPtr += 8;
        }
        else
            throw RC{RM_UNKNOWN_PAGENUM};
        fileHandle.pageAvailable.clear();
        for (PageNum pageID = (fileHandle.pageTot + 7) / 8; pageID--; ++headerPageDataPtr)
            fileHandle.pageAvailable.push_back(*headerPageDataPtr);

        // After read, you need to unpin
        RM_ChangeRC(pFFileHandle.UnpinPage(0ll), RM_MANAGER_OPEN_BUT_UNPIN_FAIL);

        // Some information needed to be set or calculated
        fileHandle.open = true;
        fileHandle.headerModified = false;
        fileHandle.slotNumPerPage = 1;
        while((fileHandle.slotNumPerPage + 1) * fileHandle.recordSize + (fileHandle.slotNumPerPage + 1 + 7) / 8 <= PF_PAGE_SIZE)
            ++fileHandle.slotNumPerPage;

        throw RC{OK_RC};
    }
    catch (RC rc) {
        return rc;
    }
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {
    try{
        // You can't close closed file.
        if (!fileHandle.open)
            throw RC{RM_MANAGER_CLOSE_CLOSED_FILE};
        
        // Close
        RM_ChangeRC(pFManager.CloseFile(fileHandle.pFFileHandle), RM_MANAGER_CLOSE_FAIL);

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
            char *headerPageDataPtr = headerPageData + 4;
            sprintf(headerPageData, "%d", fileHandle.recordSize);
            if (sizeof(SlotNum) == 4)
            {
                sprintf(headerPageDataPtr + 4, "%d", fileHandle.recordTot);
                headerPageDataPtr += 4;
            }
            else if (sizeof(SlotNum) == 8)
            {
                sprintf(headerPageDataPtr + 8, "%lld", fileHandle.recordTot);
                headerPageDataPtr += 8;
            }
            else
                throw RC{RM_UNKNOWN_SLOTNUM};
            if (sizeof(PageNum) == 4)
                sprintf(headerPageDataPtr + 4, "%d", fileHandle.pageTot);
            else if (sizeof(PageNum) == 8)
                sprintf(headerPageDataPtr + 8, "%lld", fileHandle.pageTot);
            else
                throw RC{RM_UNKNOWN_PAGENUM};
            for (char pageID: fileHandle.pageAvailable)
                *(headerPageDataPtr++) = pageID;
            
            // Unpin after write
            RM_ChangeRC(fileHandle.pFFileHandle.UnpinPage(0ll), RM_MANAGER_CLOSE_BUT_UNPIN_FAIL);
        }

        // Do some destruction
        // For safety this is necessary
        fileHandle.open = false;
        fileHandle.headerModified = false;
        fileHandle.pageAvailable.clear();

        throw RC{OK_RC};
    }
    catch (RC rc) {
        return rc;
    }
}