//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h.
// All public interfaces are described in the requirement documentation.
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"
#include <list>
#include <algorithm>
#include <string>
#include <vector>

//
// RM_Record: RM Record interface
//
// When you set the viable true, you have to give data.
class RM_Record
{
    friend class RM_FileHandle;

public:
    RM_Record();
    ~RM_Record();
    RM_Record(const RM_Record &rec);
    RM_Record &operator=(const RM_Record &rec);

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid(RID &rid) const;

private:
    char *pData;  // Data in the record
    RID rid;      // RID of the record
    bool viable;  // Viability flag
    int dataSize; // Data size

    // Release pData if viable
    void releaseData();
    void copyData(const RM_Record &rec);
};

//
// RM_FileHandle: RM File interface
// Here a page of a file could be empty.
// To remain the pages sequential, we choose not to dispose any pages.
class RM_FileHandle
{
    friend class RM_Manager; // The situation is similar as PF.
    friend class RM_FileScan;

public:
    RM_FileHandle();
    ~RM_FileHandle();
    RM_FileHandle(const RM_FileHandle &fileHandle);
    // Here I give up to hold an overloaded =, because it's unconvenient to implement

    // Given a RID, return the record
    RC GetRec(const RID &rid, RM_Record &rec) const;

    RC InsertRec(const char *pData, RID &rid); // Insert a new record

    RC DeleteRec(const RID &rid);       // Delete a record
    RC UpdateRec(const RM_Record &rec); // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages(PageNum pageNum = ALL_PAGES);

private:
    PF_FileHandle pFFileHandle; // PF file handle
    bool open;                  // File handle open flag

    // Information in header page
    int recordSize;                  // Size of record
    SlotNum recordTot;               // Total number of records
    PageNum pageTot;                 // Total number of pages
    std::vector<char> pageAvailable; // The list of available pages

    bool headerModified; // Modified flag for the file header

    // Information that needs calculation
    SlotNum slotNumPerPage;
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan
{
public:
    RM_FileScan();
    ~RM_FileScan();
    RM_FileScan(const RM_FileScan &rec);
    RM_FileScan &operator=(const RM_FileScan &rec);

    RC OpenScan(const RM_FileHandle &fileHandle,
                AttrType attrType,
                int attrLength,
                int attrOffset,
                CompOp compOp,
                void *value,
                ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);             // Get next matching record
    RC CloseScan();                            // Close the scan

private:
    RM_FileHandle rMFileHandle;
    AttrType attrType;
    int attrLength;
    int attrOffset;
    CompOp compOp;
    void *value;

    bool open;

    PageNum curPageNum;
    SlotNum curSlotNum;
};

//
// RM_Manager: provides RM file management
//
class RM_Manager
{
public:
    RM_Manager(PF_Manager &pfm);
    ~RM_Manager();

    RC CreateFile(const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile(const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile(RM_FileHandle &fileHandle);

private:
    PF_Manager *pfManager; // PF_Manager object
    int findNumberRecords(int recordSize);
};

//
// Print-error function
//
void RM_PrintError(RC rc);

// Principles to design warnings and errors:
//  1) Easily to detect where exactly the error happens at every layer.
//  2) The order of the numbers are meaningless, the numbers are assigned by the order of the implementation.
//  3) Warnings can be expected and will happen normal, but errors happen unexpectedly and confuse the programmer.
//  4) When an error occurs, perhaps something've got unconsistent.

// Warnings
#define RM_RECORD_NOT_VIABLE (START_RM_WARN + 0)               // Record is not viable
#define RM_FILE_HANDLE_CLOSED (START_RM_WARN + 1)              // File is not open when trying to read or write by a file handle.
#define RM_FILE_INSERT_OLD_FAIL (START_RM_WARN + 2)            // Fail to insert record in old pages
#define RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL (START_RM_WARN + 3) // Fail when inserting record in old pages and also when unpinning
#define RM_SCAN_NEXT_FAIL (START_RM_WARN + 4)
#define RM_FILE_INSERT_NEW_PAGE_FAIL (START_RM_WARN + 5)       // Fail when trying to insert a new page into file
#define RM_FILE_INSERT_NEW_FAIL_UNPIN_FAIL (START_RM_WARN + 6) // Fail when inserting a new record and fail when
#define RM_FILE_INSERT_NEW_FAIL (START_RM_WARN + 7)            // Fail when inserting the record into a new-allocated page
#define RM_EOF (START_RM_WARN + 8)
#define RM_FILE_DELETE_FAIL (START_RM_WARN + 9)         // Fail when deleting the record from a file
#define RM_FILE_DELETE_ILLEGAL_RID (START_RM_WARN + 10) // It's detected that an illegal rid is passed in to delete
#define RM_FILE_DELETE_NOT_FOUND (START_RM_WARN + 11)   // The record at rid is not found, maybe it has been deleted.
#define RM_FILE_GET_ILLEGAL_RID (START_RM_WARN + 12)    // Try to get some record by an illegal rid
#define RM_FILE_GET_FAIL (START_RM_WARN + 13)           // Fail to get some record from the file
#define RM_FILE_GET_NOT_FOUND (START_RM_WARN + 14)      // The record at rid is not found, maybe it has been deleted.
#define RM_FILE_GET_FAIL_UNPIN_FAIL (START_RM_WARN + 15)
#define RM_FILE_DELETE_FAIL_UNPIN_FAIL (START_RM_WARN + 16)
#define RM_FILE_UPDATE_FAIL (START_RM_WARN + 17)
#define RM_FILE_UPDATE_ILLEGAL_RID (START_RM_WARN + 18)
#define RM_FILE_UPDATE_FAIL_UNPIN_FAIL (START_RM_WARN + 19)
#define RM_FILE_UPDATE_NOT_FOUND (START_RM_WARN + 20)
#define RM_FILE_UPDATE_SIZE_NEQ (START_RM_WARN + 21)
#define RM_SCAN_CLOSED (START_RM_WARN + 22)
#define RM_SCAN_OPEN_CLOSED_FILE (START_RM_WARN + 23)
#define RM_LASTWARN RM_EOF // Mark the last warn, to be updated

// Errors
#define RM_FILE_INSERT_NO_AVAILABLE_SLOT_IN_AVAILABLE_PAGES (START_RM_ERR - 0) // When inserting some record to some file, find a available page without available slot, here makes a contradiction.
#define RM_ERROR_FILE_INSERT_BUT_UNPIN_FAIL (START_RM_ERR - 1)
#define RM_FILE_DELETE_BUT_UNPIN_FAIL (START_RM_ERR - 2)
#define RM_FILE_UPDATE_BUT_UNPIN_FAIL (START_RM_ERR - 3)
#define RM_FILE_GET_BUT_UNPIN_FAIL (START_RM_ERR - 4)
#define RM_LASTERROR RM_EOF // Mark the last erro to be update.

// Example:
// #define RM_INVALIDNAME          (START_RM_ERR - 0) // Invalid PC file name

// Error in UNIX system call or library routine
#define RM_UNIX (START_RM_ERR - 2) // Unix error
#define RM_LASTERROR RM_UNIX

#endif
