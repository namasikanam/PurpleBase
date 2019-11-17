//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <cstring>
#include "rm_internal.h"
#include "rm.h"
using namespace std;

// Default constructor
// De facto, RM_FileHandle can't be constructed by default, which is meaningless.
RM_FileHandle::RM_FileHandle() : open(false) {}

// Destructor
// Nothing needs doing, because the actual destruction is executed when the file is closed.
RM_FileHandle::~RM_FileHandle() {}

// Copy constructor
RM_FileHandle::RM_FileHandle(const RM_FileHandle &fileHandle) : open(fileHandle.open)
{
    if (fileHandle.open)
    {
        pFFileHandle = fileHandle.pFFileHandle;
        recordSize = fileHandle.recordSize;
        recordTot = fileHandle.recordTot;
        pageTot = fileHandle.pageTot;

        pageAvailable = new char[(pageTot + 7) / 8];
        memcpy(pageAvailable, fileHandle.pageAvailable, (pageTot + 7) / 8);

        headerModified = fileHandle.headerModified;
    }
}

//
// GetRec
//
// Desc:    Get a record.
// In:      A RID
// Out:     A record
// Ret:     RM return code
RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const
{
    if (!open)
        return RM_FILE_HANDLE_CLOSED;
    else
    {
    }
}

//
// InsertRec
//
// Desc:    Insert a record into the file;
//          It's assumed that the pData has the correct length.
//          The page will be unpinned after insert.
// In:      A fragment of data which needs recording
// Out:     A RID
// Ret:     RM return code
RC RM_FileHandle::InsertRec(const char *pData, RID &rid)
{
    try
    {
        // Check if open, which is a general enter condition of all functions
        if (!open)
            throw RC{RM_FILE_HANDLE_CLOSED};
        // Declare three variables that will be used often later.
        int tmp_rc;
        PF_PageHandle pFPageHandle;
        char *pageData;
        PageNum pageNum;
        // Wrap unpin as a
        // Firstly, we try to find if there's an available slot in existing old pages.
        for (pageNum = 0; pageNum < pageTot; pageNum += 8)
            if (pageAvailable[pageNum / 8])
            {
                while (~pageAvailable[pageNum / 8] >> pageNum % 8 & 1)
                    ++pageNum;
                // I've managed to find an available page!

                // Get the available page.
                RM_ChangeRC(pFFileHandle.GetThisPage(pageNum, pFPageHandle), RM_FILE_INSERT_OLD_FAIL);

                // Get data from the availabe page.
                if (tmp_rc = pFPageHandle.GetData(pageData))
                {
                    PF_PrintError(tmp_rc);

                    // Unpin the page if having failed to get data.
                    RM_ChangeRC(pFFileHandle.UnpinPage(pageNum), RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL);

                    throw RC{RM_FILE_INSERT_OLD_FAIL};
                };
                if (tmp_rc = pFFileHandle.MarkDirty(pageNum))
                {
                    PF_PrintError(tmp_rc);
                    RM_ChangeRC(pFFileHandle.UnpinPage(pageNum), RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL);
                    throw RC{RM_FILE_INSERT_OLD_FAIL};
                }
                for (SlotNum slotNum = 0; slotNum < slotNumPerPage; ++slotNum)
                    if (pageData[slotNum / 8] >> slotNum % 8 & 1)
                    {
                        // Actually insert the data into the available old page.
                        pageData[slotNum / 8] |= 1 << slotNum % 8;
                        memcpy(pageData + (slotNumPerPage + 7) / 8 + recordSize * slotNum, pData, recordSize);

                        // The exactly RID is found.
                        rid = RID(pageNum, slotNum);

                        RM_ChangeRC(pFFileHandle.UnpinPage(pageNum), RM_FILE_INSERT_OLD_SUCC_UNPIN_FAIL);

                        return OK_RC;
                    }
            }

        // If there's no available page.
        pageNum = pageTot;
        RM_ChangeRC(pFFileHandle.AllocatePage(pFPageHandle), RM_FILE_INSERT_NEW_PAGE_FAIL);
        ++pageTot;
        RM_ChangeRC(pFPageHandle.GetData(pageData), RM_FILE_INSERT_NEW_PAGE_FAIL);
        if (tmp_rc = pFFileHandle.MarkDirty(pageNum))
        {
            PF_PrintError(tmp_rc);
            RM_ChangeRC(pFFileHandle.UnpinPage(pageNum), RM_FILE_INSERT_NEW_FAIL_UNPIN_FAIL);
            return RM_FILE_INSERT_NEW_FAIL;
        }
        // Initialize the bitmap at the head of the page
        memset(pageData, 0, (slotNumPerPage + 7) / 8);
        pageData[0] |= 1;
        // Copy the record to the newly built page
        memcpy(pageData + (slotNumPerPage + 7) / 8, pData, recordSize);
        // Output the rid.
        rid = RID(pageNum, 0);
        // Record is inserted successfully in the new page
        RM_ChangeRC(pFFileHandle.UnpinPage(pageNum), RM_FILE_INSERT_NEW_SUCC_UNPIN_FAIL);
        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

//
// DeleteRec
//
// Desc:    Delete a record from the file;
//          "delete" just means that the corresponding position in the bitmap of the head of the page is set to 0.
// In:      The rid that needs to delete
// Ret:     RM return code
RC RM_FileHandle::DeleteRec(const RID &rid)
{
    try
    {
        if (!open)
            throw RC{RM_FILE_HANDLE_CLOSED};
        // First, we need to extract information of pages and slots from rid.
        PageNum pageNum;
        RM_ChangeRC(rid.GetPageNum(pageNum), RM_FILE_DELETE_FAIL);
        SlotNum slotNum;
        RM_ChangeRC(rid.GetSlotNum(slotNum), RM_FILE_DELETE_FAIL);
        if (pageNum < 0 || pageNum >= pageTot || slotNum < 0 || slotNum >= slotNumPerPage)
            throw RC{RM_FILE_DELETE_ILLEGAL_RID};

        // Fetch the data of the destination page
        PF_PageHandle pFPageHandle;
        char *pageData;
        RM_ChangeRC(pFFileHandle.GetThisPage(pageNum, pFPageHandle), RM_FILE_DELETE_FAIL);
        RM_ChangeRC(pFPageHandle.GetData(pageData), RM_FILE_DELETE_FAIL);

        if (~pageData[slotNum / 8] >> slotNum % 8 & 1)
            throw RC{RM_FILE_DELETE_NOT_FOUND};
        pageData[slotNum / 8] &= ~(1 << slotNum % 8);
    }
    catch (RC rc)
    {
        return rc;
    }
}

//
// UpdateRec
//
// Desc:    update the contents of the record in the file that is associated with [rec].
// In:      The record that needed to updated to
// Ret:     RM return code
RC RM_FileHandle::UpdateRec(const RM_Record &rec)
{
    // Parse the record

    //
}

RC RM_FileHandle::ForcePages(PageNum pageNum = ALL_PAGES) {}