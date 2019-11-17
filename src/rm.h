//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
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
#include <bits/stdc++.h>
using namespace std;

//
// RM_Record: RM Record interface
//
class RM_Record
{
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
    friend class PF_Manager; // The situation is similar as PF.
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
    int recordSize;      // Size of record
    int recordTot;       // Total number of records
    int pageTot;         // Total number of pages
    char *pageAvailable; // A bitmap of pages, which shows that if some page is available.
    // This is suggested to be implemented as a linked list in the documentation, which I disagree.
    // The length of this bitmap is [pageNum], equivalent to [(pageNum + 7) / 8].

    bool headerModified; // Modified flag for the file header

    // Information that needs calculation
    int slotNumPerPage;
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan
{
public:
    RM_FileScan();
    ~RM_FileScan();

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
    PageNum pageNum;          // Current page number
    SlotNum slotNumber;       // Current slot number
    RM_FileHandle fileHandle; // File handle for the file
    AttrType attrType;        // Attribute type
    int attrLength;           // Attribute length
    int attrOffset;           // Attribute offset
    CompOp compOp;            // Comparison operator
    void *value;              // Value to be compared
    bool viable;              // Viablity flag

    int getIntegerValue(char *recordData);         // Get integer attribute valuez
    float getFloatValue(char *recordData);         // Get float attribute value
    std::string getStringValue(char *recordData);  // Get string attribute value
    bool isBitFilled(int bitNumber, char *bitmap); // Check whether a slot is filled

    template <typename T>
    bool matchRecord(T recordValue, T givenValue); // Match the record value with
                                                   // the given valuez
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

// Warnings
#define RM_RECORD_NOT_VIABLE (START_RM_WARN + 0)               // Record is not viable
#define RM_FILE_HANDLE_CLOSED (START_RM_WARN + 1)              // File is not open when trying to read or write by a file handle.
#define RM_FILE_INSERT_OLD_FAIL (START_RM_WARN + 2)            // Fail to insert record in old pages
#define RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL (START_RM_WARN + 3) // Fail when inserting record in old pages and also when unpinning
#define RM_FILE_INSERT_OLD_SUCC_UNPIN_FAIL (START_RM_WARN + 4) // Succeed when inserting record in old pages but fail when unpinning
#define RM_FILE_INSERT_NEW_PAGE_FAIL (START_RM_WARN + 5)       // Fail when trying to insert a new page into file
#define RM_FILE_INSERT_NEW_FAIL_UNPIN_FAIL (START_RM_WARN + 6) // Fail when inserting a new record and fail when
#define RM_FILE_INSERT_NEW_FAIL (START_RM_WARN + 7)            // Fail when inserting the record into a new-allocated page
#define RM_FILE_INSERT_NEW_SUCC_UNPIN_FAIL (START_RM_WARN + 8) // Fail when inserting the record into a new-allocated page and also fail when unpinning
#define RM_FILE_DELETE_FAIL (START_RM_WARN + 9)                // Fail when deleting the record from a file
#define RM_FILE_DELETE_ILLEGAL_RID (START_RM_WARN + 10)        // It's detected that an illegal rid is passed in to delete
#define RM_FILE_DELETE_NOT_FOUND (START_RM_WARN + 11)          // The record at rid is not found, maybe it has been deleted.

// Errors

// Example:
// #define RM_INVALIDNAME          (START_RM_ERR - 0) // Invalid PC file name

// // Warnings
// #define RM_LARGE_RECORD (START_RM_WARN + 0)             // Record size is too large
// #define RM_SMALL_RECORD (START_RM_WARN + 1)             // Record size is too small
// #define RM_FILE_OPEN (START_RM_WARN + 2)                // File is already open
// #define RM_FILE_CLOSED (START_RM_WARN + 3)              // File is closed
// #define RM_RECORD_NOT_VALID (START_RM_WARN + 4)         // Record is not valid
// #define RM_INVALID_SLOT_NUMBER (START_RM_WARN + 5)      // Slot number is not valid
// #define RM_INVALID_PAGE_NUMBER (START_RM_WARN + 6)      // Page number is not valid
// #define RM_ATTRIBUTE_NOT_CONSISTENT (START_RM_WARN + 7) // Attribute is not consistent
// #define RM_SCAN_CLOSED (START_RM_WARN + 8)              // Scan is not open
// #define RM_INVALID_FILENAME (START_RM_WARN + 9)         // Invalid filename
// #define RM_INVALID_ATTRIBUTE (START_RM_WARN + 10)       // Invalid attribute
// #define RM_INVALID_OFFSET (START_RM_WARN + 11)          // Invalid offset
// #define RM_INVALID_OPERATOR (START_RM_WARN + 12)        // Invalid operator
// #define RM_NULL_RECORD (START_RM_WARN + 13)             // Null record
// #define RM_EOF (START_RM_WARN + 14)                     // End of file
// #define RM_LASTWARN RM_EOF

// // Errors
// #define RM_INVALIDNAME (START_RM_ERR - 0)         // Invalid PC file name
// #define RM_INCONSISTENT_BITMAP (START_RM_ERR - 1) // Inconsistent bitmap in page

// Error in UNIX system call or library routine
#define RM_UNIX (START_RM_ERR - 2) // Unix error
#define RM_LASTERROR RM_UNIX

#endif
