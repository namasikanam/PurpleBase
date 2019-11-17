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
// More than once opening is not prevented, but it's dangerous,
// I have no idea what will happen.
RM_FileHandle::RM_FileHandle(const RM_FileHandle &fileHandle) : open(fileHandle.open)
{
    if (fileHandle.open)
    {
        pFFileHandle = fileHandle.pFFileHandle;
        recordSize = fileHandle.recordSize;
        recordTot = fileHandle.recordTot;
        pageTot = fileHandle.pageTot;
        pageAvailable = fileHandle.pageAvailable;
        headerModified = fileHandle.headerModified;
        slotNumPerPage = fileHandle.slotNumPerPage;
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
    try
    {
        if (!open)
            throw RC{RM_FILE_HANDLE_CLOSED};

        // First, we need to extract information of pages and slots from rid.
        PageNum pageNum;
        SlotNum slotNum;
        RM_ChangeRC(rid.GetPageNum(pageNum), RM_FILE_GET_FAIL);
        RM_ChangeRC(rid.GetSlotNum(slotNum), RM_FILE_GET_FAIL);
        if (pageNum < 0 || pageNum >= pageTot || slotNum < 0 || slotNum >= slotNumPerPage)
            throw RC{RM_FILE_GET_ILLEGAL_RID};

        // Fetch the data of the destination page
        PF_PageHandle pFPageHandle;
        char *pageData;
        RM_ChangeRC(pFFileHandle.GetThisPage(pageNum + 1, pFPageHandle), RM_FILE_GET_FAIL);
        RM_TryElseUnpin(pFPageHandle.GetData(pageData), RM_FILE_GET_FAIL_UNPIN_FAIL, RM_FILE_GET_FAIL, pFFileHandle, pageNum + 1);
        if (~pageData[slotNum / 8] >> slotNum % 8 & 1)
            throw RC{RM_FILE_GET_NOT_FOUND};

        rec.releaseData();
        rec.pData = new char[recordSize];
        rec.rid = rid;
        rec.viable = true;
        rec.dataSize = recordSize;
        memcpy(rec.pData, pageData + (slotNumPerPage + 7) / 8 + slotNum * recordSize, recordSize);

        RM_ChangeRC(pFFileHandle.UnpinPage(pageNum + 1), RM_FILE_GET_BUT_UNPIN_FAIL);
        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

//
// InsertRec
//
// Desc:    Insert a record into the file.
//          It's assumed that the pData has the correct length.
//          The page will be unpinned after insert.
//          Update bitmap in the header page and the header of page.
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
        PF_PageHandle pFPageHandle;
        char *pageData;
        PageNum pageNum;
        vector<char>::iterator pageAvailableIterator;

        // The actual insertion step
        auto insertAt = [this, &pageNum, &pData, &rid, &pageData, &pageAvailableIterator](SlotNum slotNum) {
            // The exactly RID is found.
            rid = RID(pageNum, slotNum);

            // Actually insert the data into the available old page.
            pageData[slotNum / 8] |= 1 << slotNum % 8;
            memcpy(pageData + (slotNumPerPage + 7) / 8 + recordSize * slotNum, pData, recordSize);

            // If this insertion make a page unavailable, we need update the information of heade page
            if ([pageData, this]() {
                    for (SlotNum slotNumIt = 0; slotNumIt < slotNumPerPage; ++slotNumIt)
                        if (~pageData[slotNumIt / 8] >> slotNumIt % 8 & 1)
                            return 0;
                    return 1;
                }())
                *pageAvailableIterator = *pageAvailableIterator & ~(1 << pageNum);
            ++recordTot;
            headerModified = true;

            // Unpin the page after insertion
            RM_ChangeRC(pFFileHandle.UnpinPage(pageNum + 1), RM_ERROR_FILE_INSERT_BUT_UNPIN_FAIL);

            throw RC{OK_RC};
        };

        // Firstly, we try to find if there's an available slot in existing old pages.
        pageNum = 0;
        for (pageAvailableIterator = pageAvailable.begin(); pageNum < pageTot; pageNum += 8, ++pageAvailableIterator)
            if (*pageAvailableIterator)
            {
                while (~*pageAvailableIterator >> pageNum % 8 & 1)
                    ++pageNum;
                // I've managed to find an available page!

                // Get the available page.
                RM_ChangeRC(pFFileHandle.GetThisPage(pageNum + 1, pFPageHandle), RM_FILE_INSERT_OLD_FAIL);

                // Get data from the availabe page.
                RM_TryElseUnpin(pFPageHandle.GetData(pageData), RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL, RM_FILE_INSERT_OLD_FAIL, pFFileHandle, pageNum + 1);

                // Mark darty before any solid modification
                RM_TryElseUnpin(pFFileHandle.MarkDirty(pageNum + 1), RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL, RM_FILE_INSERT_OLD_FAIL, pFFileHandle, pageNum + 1);

                for (SlotNum slotNum = 0; slotNum < slotNumPerPage; ++slotNum)
                    if (pageData[slotNum / 8] >> slotNum % 8 & 1)
                        insertAt(slotNum);
                throw RC{RM_FILE_INSERT_NO_AVAILABLE_SLOT_IN_AVAILABLE_PAGES};
            }

        // If there's no available page.
        pageNum = pageTot;

        // Allocate a new page, and get data from it!
        RM_ChangeRC(pFFileHandle.AllocatePage(pFPageHandle), RM_FILE_INSERT_NEW_PAGE_FAIL);
        RM_TryElseUnpin(pFPageHandle.GetData(pageData), RM_FILE_INSERT_NEW_FAIL_UNPIN_FAIL, RM_FILE_INSERT_NEW_PAGE_FAIL, pFFileHandle, pageNum);
        RM_TryElseUnpin(pFFileHandle.MarkDirty(pageNum + 1), RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL, RM_FILE_INSERT_OLD_FAIL, pFFileHandle, pageNum + 1);

        // Update the information of header page
        ++pageTot;
        headerModified = true;
        if (pageTot % 8 == 0)
            pageAvailable.push_back(char(0));
        pageAvailableIterator = --pageAvailable.end();

        // Initialize the bitmap at the head of the page
        memset(pageData, 0, (slotNumPerPage + 7) / 8);

        // Insert! Now!
        insertAt(0);

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
//          "delete" just means that the corresponding position in the bitmap of the head of the page is set to 0;
//          Similar to [GetRec];
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
        SlotNum slotNum;
        RM_ChangeRC(rid.GetPageNum(pageNum), RM_FILE_DELETE_FAIL);
        RM_ChangeRC(rid.GetSlotNum(slotNum), RM_FILE_DELETE_FAIL);
        if (pageNum < 0 || pageNum >= pageTot || slotNum < 0 || slotNum >= slotNumPerPage)
            throw RC{RM_FILE_DELETE_ILLEGAL_RID};

        // Fetch the data of the destination page
        PF_PageHandle pFPageHandle;
        char *pageData;
        RM_ChangeRC(pFFileHandle.GetThisPage(pageNum + 1, pFPageHandle), RM_FILE_DELETE_FAIL);
        RM_TryElseUnpin(pFPageHandle.GetData(pageData), RM_FILE_DELETE_FAIL_UNPIN_FAIL, RM_FILE_DELETE_FAIL, pFFileHandle, pageNum + 1);
        RM_TryElseUnpin(pFFileHandle.MarkDirty(pageNum + 1), RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL, RM_FILE_INSERT_OLD_FAIL, pFFileHandle, pageNum + 1);
        if (~pageData[slotNum / 8] >> slotNum % 8 & 1)
            throw RC{RM_FILE_DELETE_NOT_FOUND};

        // Update the header of a page
        pageData[slotNum / 8] &= ~(1 << slotNum % 8);

        // Update the header page
        if (~pageAvailable[pageNum / 8] >> pageNum % 8 & 1)
            pageAvailable[pageNum / 8] |= 1 << pageNum % 8;
        --recordTot;
        headerModified = true;

        RM_ChangeRC(pFFileHandle.UnpinPage(pageNum + 1), RM_FILE_DELETE_BUT_UNPIN_FAIL);

        throw RC{OK_RC};
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
    try
    {
        if (!open)
            throw RC{RM_FILE_HANDLE_CLOSED};
        if (rec.dataSize != recordSize)
            throw RC{RM_FILE_UPDATE_SIZE_NEQ};

        // Parse rec
        char *recData;
        RID rid;
        PageNum pageNum;
        SlotNum slotNum;
        RM_ChangeRC(rec.GetRid(rid), RM_FILE_UPDATE_FAIL);
        RM_ChangeRC(rec.GetData(recData), RM_FILE_UPDATE_FAIL);
        RM_ChangeRC(rid.GetPageNum(pageNum), RM_FILE_UPDATE_FAIL);
        RM_ChangeRC(rid.GetSlotNum(slotNum), RM_FILE_UPDATE_FAIL);
        if (pageNum < 0 || pageNum >= pageTot || slotNum < 0 || slotNum >= slotNumPerPage)
            throw RC{RM_FILE_UPDATE_ILLEGAL_RID};

        // Get data from the page
        PF_PageHandle pFPageHandle;
        char *pageData;
        RM_ChangeRC(pFFileHandle.GetThisPage(pageNum + 1, pFPageHandle), RM_FILE_UPDATE_FAIL);
        RM_TryElseUnpin(pFPageHandle.GetData(pageData), RM_FILE_UPDATE_FAIL_UNPIN_FAIL, RM_FILE_UPDATE_FAIL, pFFileHandle, pageNum + 1);
        RM_TryElseUnpin(pFFileHandle.MarkDirty(pageNum + 1), RM_FILE_UPDATE_FAIL_UNPIN_FAIL, RM_FILE_UPDATE_FAIL, pFFileHandle, pageNum + 1);
        if (~pageData[slotNum / 8] >> slotNum % 8 & 1)
            throw RC{RM_FILE_UPDATE_NOT_FOUND};

        // Update
        memcpy(pageData + (slotNumPerPage + 7) / 8 + slotNum * recordSize, recData, recordSize);

        // Unpin and finish
        RM_ChangeRC(pFFileHandle.UnpinPage(pageNum + 1), RM_FILE_UPDATE_BUT_UNPIN_FAIL);
        throw RC{OK_RC};
    }
    catch (RC rc)
    {
        return rc;
    }
}

//
// ForcePages
//
// Desc:        Copy the contents of one or all dirty pages of the file from the buffer pool to disk.
//              Just call the corresponding method at the underlying layer!
// In:          The pagenum that needed to force.
// Ret:         RM return code
RC RM_FileHandle::ForcePages(PageNum pageNum)
{
    try{
        RM_ChangeRC(pFFileHandle.ForcePages(pageNum), RM_FILE_FORCE_FAIL);

        throw RC{OK_RC};
    }
    catch (RC rc) {return rc;}
}