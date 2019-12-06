//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h" // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"

// IX_IndexHeader: Struct for the index file header
/* Stores the following:
    1) attrType - Attribute type for the index - AttrType
    2) attrLength - Attribute length - integer
    3) rootPage - Page number of the B+ Tree root - PageNum
    4) bucketPage - Page number of the bucket - PageNum
    5) degree - Degree of a node in the B+ Tree - integer
    6) bucketTot - How many rids are in the bucket now? - integer
    7) modified - Has the header modified since last read from the header page? - This shouldn't be stored in the header page. - bool
*/
struct IX_IndexHeader
{
    AttrType attrType;
    int attrLength;
    PageNum rootPage;
    PageNum bucketPage;
    int degree;
    int bucketTot;
    bool modified = false;
};

// IX_Entry: Keys stored in B+ tree
/* Stores the following:
    1) key - void*
    2) num - (positive) Pointer to some child or (negative) some value(RID) - PageNum / BucketNum
    3) tombstone - if this child is empty
*/
struct IX_Key
{
    void *key;
    PageNum num;
};

//
// IX_IndexHandle: IX Index File interface
//
// 1) Differences with traditional B+ tree:
//    1.1) here we adopt a structure of left-inclusive right-exclusive intervals.
//    The number of keys stored in the node is the same as children.
//    1.2) keys of some nodes are possibly non-unique
// 2) In one page, i.e. one node of B+ tree, we store following things:
//    2.1) A bool that indicates if this node is a leaf
//    If this node is not a leaf:
//      2.1_2) The number of its children w.
//      2.1_3) (key, pageNnum) * w
//    If this node is a leaf:
//      2.2_2) The number of its children w.
//      2.2_3) (key, bucketNum) * w
// 3) We just simply assume that the rids won't reach out the capacity of one bucket page.
// 4) Root page is possibily empty, which is a legal special case.
// 5) The bucket page is filled with things as (pageNum, slotNum, tombstone).
class IX_IndexHandle
{
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

    // Fundamental operations of B+ tree
    void InsertBPlus(const PageNum &nodePageNum, const void *pData, const RID &rid);
    void DeleteBPlus(const PageNum &nodePageNum, const void *pData, const RID &rid);
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan
{
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint pinHint = NO_HINT);

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
class IX_Manager
{
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
    PF_Manager &pfm; // PF_Manager object
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
#define IX_ILLEGAL_FILENAME (START_IX_WARN + 0) // Index number is negative
#define IX_MANAGER_CREATE_FAIL (START_IX_WARN + 1)
#define IX_MANAGER_DESTROY_FAIL (START_IX_WARN + 2)
#define IX_MANAGER_OPEN_FAIL (START_IX_WARN + 3)
#define IX_MANAGER_OPEN_FAIL_UNPIN_FAIL (START_IX_WARN + 4)
#define IX_MANAGER_CLOSE_CLOSED_FILE_HANDLE (START_IX_WARN + 5)
#define IX_MANAGER_CLOSE_FAIL (START_IX_WARN + 6)
#define IX_HANDLE_FORCE_FAIL (START_IX_WARN + 7)
#define IX_HANDLE_INSERT_FAIL (START_IX_WARN + 8)
#define IX_HANDLE_INSERT_FAIL_UNPIN_FAIL (START_IX_WARN + 9)
#define IX_LASTWARN IX_CREAT_FAIL

// Errors
#define IX_MANAGER_CREATE_OPEN_FILE_FAIL (START_IX_ERR - 0) // Invalid PC file name
#define IX_MANAGER_CREATE_HEAD_FAIL (START_IX_ERR - 1)
#define IX_MANAGER_CREATE_HEAD_FAIL_UNPIN_FAIL (START_IX_ERR - 2)
#define IX_MANAGER_CREATE_HEAD_BUT_UNPIN_FAIL (START_IX_ERR - 3)
#define IX_MANAGER_CREATE_BUCKET_FAIL (START_IX_ERR - 4)
#define IX_MANAGER_CREATE_BUCKET_FAIL_UNPIN_FAIL (START_IX_ERR - 5)
#define IX_MANAGER_CREATE_BUCKET_BUT_UNPIN_FAIL (START_IX_ERR - 6)
#define IX_MANAGER_CREATE_ROOT_FAIL (START_IX_ERR - 7)
#define IX_MANAGER_CREATE_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 8)
#define IX_MANAGER_CREATE_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 9)
#define IX_MANAGER_OPEN_BUT_UNPIN_FAIL (START_IX_ERR - 10)
#define IX_MANAGER_CLOSE_FAIL_UNPIN_FAIL (START_IX_ERR - 11)
#define IX_MANAGER_CLOSE_HEAD_BUT_UNPIN_FAIL (START_IX_ERR - 12)
#define IX_HANDLE_CLOSED (START_IX_ERR - 13)

// The exact definition needs to be modified.
// Error in UNIX system call or library routine
#define IX_UNIX (START_IX_ERR - 10) // Unix error
#define IX_LASTERROR IX_UNIX

#endif
