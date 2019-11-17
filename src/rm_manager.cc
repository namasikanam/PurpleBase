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
        RM_ChangeRC(pFManager.CreateFile(fileName), RM_CREATE_FAIL);

        // Allocate header page
        PF_FileHandle pFFileHandle;
        pFManager.OpenFile(fileName, pFFileHandle);
        PF_PageHandle headerPage;
        RM_ChangeRC(pFFileHandle.AllocatePage(headerPage), RM_CREATE_FAIL);

        // Write header page
        char *headerPageData;
        RM_TryElseUnpin(headerPage.GetData(headerPageData), RM_CREATE_FAIL_UNPIN_FAIL, RM_CREATE_FAIL, pFFileHandle, 0ll);

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
        else
            sprintf(headerPageDataPtr + 8, "%lld", 0ll);
        
    }
    catch (RC rc) {
        return rc;
    }
}