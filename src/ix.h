//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"

// IX_IndexHeader: Struct for the index file header
/* Stores the following:
    1) attrType - Attribute type for the index - AttrType
    2) attrLength - Attribute length - integer
    3) rootPage - Page number of the B+ Tree root - PageNum
    4) bucketPage - Page number of the bucket - PageNum
    5) degree - Degree of a node in the B+ Tree - integer
    6) modified - Has the header modified since last read from the header page? - This shouldn't be stored in the header page. - Bool
*/
struct IX_IndexHeader {
    AttrType attrType;
    int attrLength;
    PageNum rootPage;
    PageNum bucketPage;
    int degree;
    bool modified = false;
};

//
// IX_IndexHandle: IX Index File interface
//
// 1) The key of B+ tree is (someAttribute, pageNum, slotNum).
// 2) Different with traditional B+ tree,
//    here we adopt a structure of left-inclusive right-exclusive intervals.
//    The number of keys stored in the node is the same as children.
// 3) Rather than RM component, here we directly store information in the
//    header page, which is stored by transformation to string in RM.
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();

private:
    PF_FileHandle pFFileHandle; // Underlying file handle

    bool open;

    // As in the RM component,
    // a header page of the file is needed.
    IX_IndexHeader header;
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();

private:
};

//
// IX_Manager: provides IX index file management
//
// We have to assume that [fileName] doesn't contain '.'.
class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy an Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);

private:
    PF_Manager& pfm;      // PF_Manager object
};

//
// Print-error function
//
void IX_PrintError(RC rc);

// Principles to design warnings and errors:
//  1) Easily to detect where exactly the error happens at every layer.
//  2) The order of the numbers are meaningless, the numbers are assigned by the order of the implementation.
//  3) Warnings can be expected and will happen normal, but errors happen unexpectedly and confuse the programmer.
//  4) When an error occurs, perhaps something've got unconsistent.
//  5) The name needs to identify where the error or warning happen uniquely.
//  6) The first string after `IX` is the name of what class the warning or error happens.

// Warnings
#define IX_ILLEGAL_FILENAME             (START_IX_WARN + 0) // Index number is negative
#define IX_CREATE_FAIL                   (START_IX_WARN + 1)
#define IX_LASTWARN IX_CREAT_FAIL

// Errors
#define IX_CREATE_OPEN_FILE_FAIL         (START_IX_ERR - 0) // Invalid PC file name
#define IX_CREATE_HEAD_FAIL (START_IX_ERR - 1)

// The exact definition needs to be modified.
// Error in UNIX system call or library routine
#define IX_UNIX            (START_IX_ERR - 10) // Unix error
#define IX_LASTERROR       IX_UNIX

#endif
