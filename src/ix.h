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
#include <utility>
#include <list>

// To make the volume of the bucket larger, we use [short] as [BucketNum].
typedef short BucketNum;

// IX_IndexHeader: Struct for the index file header
/* Stores the following:
    1) attrType - Attribute type for the index - AttrType
    2) attrLength - Attribute length - integer
    3) rootPage - Page number of the B+ Tree root - PageNum
    4) pageTot - The number of pages now - PageNum

    5) innerEntryLength - The length of the entry in the inner node - int
    6) leafEntryLength - The length of the entry in the leaf node - int
    7) innerDeg - The degree of the inner node of the B+ tree - int
    8) leafDeg - The degree of the leaf node of the B+ tree - int
    9) modified - Has the header modified since last read from the header page? - This shouldn't be stored in the header page. - bool
*/
struct IX_IndexHeader
{
    AttrType attrType;
    int attrLength;
    PageNum rootPage;
    PageNum pageTot;

    // The followings don't need storing in the header page,
    // can be inferred when reading from the header page.
    int innerEntryLength;
    int leafEntryLength;
    // Deg means the maximum plus one.
    int innerDeg;
    int leafDeg;
    bool modified = false;
};

//
// IX_IndexHandle: IX Index File interface
//
// 1) Differences with traditional B+ tree:
//    1.1) here we adopt a structure of left-inclusive right-exclusive intervals: [l, r)
//    The number of keys stored in the node is the same as children.
//    1.2) keys of some nodes are possibly non-unique
// 2) In one page, i.e. one node of B+ tree, we store following things:
//    2.1) A bool that indicates if this node is a leaf
//    2.2) The number of its children w.
//    If this node is not a leaf:
//      2.3_1) (key, pageNum) * w
//    If this node is a leaf:
//      2.3_2) (key, RID) * w, where [RID.viable == false] means that this index is deleted.
// 3) Here we reuse [RID.viable] as tombstone.
// 4) The private functions starting with [BPlus_] are recursive functions on the B+ tree.
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
    bool BPlus_Exists(PageNum nodePageNum, const void *pData, const RID &rid);
    const std::pair<const void *, PageNum> BPlus_Insert(PageNum nodePageNum, const void *pData, const RID &rid);
    bool BPlus_Delete(PageNum nodePageNum, const void *pData, const RID &rid);
    bool BPlus_Update(PageNum nodePageNum, const void *pData, const RID &origin_rid, const RID &updated_rid);

    // Utilities

    // Compare two key of the current index.
    // Return:
    //        -1, if [data1] < [data2]
    //        0,  if [data1] == [data2]
    //        1,  if [data1] > [data2]
    int cmp(const void *data1, const void *data2) const;
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
    bool open;

    std::list<RID> scan;

    void BPlus_Find(const IX_IndexHandle &indexHandle, PageNum nodePageNum, CompOp compOp, void *value);
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
#define IX_HANDLE_GETRID_FAIL (START_IX_WARN + 4)
#define IX_MANAGER_CLOSE_CLOSED_FILE_HANDLE (START_IX_WARN + 5)
#define IX_MANAGER_CLOSE_FAIL (START_IX_WARN + 6)
#define IX_HANDLE_FORCE_FAIL (START_IX_WARN + 7)
#define IX_HANDLE_INSERT_FAIL (START_IX_WARN + 8)
#define IX_HANDLE_DELETE_NOT_EXIST (START_IX_WARN + 9)
#define IX_HANDLE_INSERT_EXISTS (START_IX_WARN + 10)
#define IX_OPEN_SCAN_NE (START_IX_WARN + 11)
#define IX_HANDLE_LEAF_SPLIT_FAIL (START_IX_WARN + 12)
#define IX_HANDLE_LEAF_NEW_ROOT_FAIL (START_IX_WARN + 13)
#define IX_EOF (START_IX_WARN + 14)
#define IX_HANDLE_EXISTS_FAIL (START_IX_WARN + 15)
#define IX_HANDLE_INNER_SPLIT_FAIL (START_IX_WARN + 16)
#define IX_HANDLE_INNER_NEW_ROOT_FAIL (START_IX_WARN + 17)
#define IX_HANDLE_DELETE_FAIL (START_IX_WARN + 18)
#define IX_LASTWARN IX_HANDLE_DELETE_FAIL

// Errors
#define IX_MANAGER_CREATE_OPEN_FILE_FAIL (START_IX_ERR - 0) // Invalid PC file name
#define IX_MANAGER_CREATE_HEAD_FAIL (START_IX_ERR - 1)
#define IX_MANAGER_CREATE_HEAD_FAIL_UNPIN_FAIL (START_IX_ERR - 2)
#define IX_MANAGER_CREATE_HEAD_BUT_UNPIN_FAIL (START_IX_ERR - 3)
#define IX_HANDLE_NOT_DELETE_BUT_UNPIN_FAIL (START_IX_ERR - 4)
#define IX_HANDLE_INSERT_BUT_UNPIN_FAIL (START_IX_ERR - 5)
#define IX_HANDLE_NOT_EXISTS_BUT_UNPIN_FAIL (START_IX_ERR - 6)
#define IX_MANAGER_CREATE_ROOT_FAIL (START_IX_ERR - 7)
#define IX_MANAGER_CREATE_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 8)
#define IX_MANAGER_CREATE_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 9)
#define IX_MANAGER_OPEN_BUT_UNPIN_FAIL (START_IX_ERR - 10)
#define IX_MANAGER_CLOSE_FAIL_UNPIN_FAIL (START_IX_ERR - 11)
#define IX_MANAGER_CLOSE_HEAD_BUT_UNPIN_FAIL (START_IX_ERR - 12)
#define IX_HANDLE_CLOSED (START_IX_ERR - 13)
#define IX_MANAGER_OPEN_FAIL_UNPIN_FAIL (START_IX_ERR - 14)
#define IX_HANDLE_INSERT_FAIL_UNPIN_FAIL (START_IX_ERR - 15)
#define IX_HANDLE_GETRID_FAIL_UNPIN_FAIL (START_IX_ERR - 16)
#define IX_HANDLE_GETRID_BUT_UNPIN_FAIL (START_IX_ERR - 17)
#define IX_HANDLE_LEAF_SPLIT_FAIL_UNPIN_FAIL (START_IX_ERR - 18)
#define IX_HANDLE_EXISTS_FAIL_UNPIN_FAIL (START_IX_ERR - 19)
#define IX_HANDLE_INNER_SPLIT_FAIL_UNPIN_FAIL (START_IX_ERR - 20)
#define IX_HANDLE_LEAF_NEW_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 21)
#define IX_HANDLE_INNER_NEW_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 22)
#define IX_HANDLE_DELETE_FAIL_UNPIN_FAIL (START_IX_ERR - 23)
#define IX_HANDLE_INNER_EXISTS_BUT_UNPIN_FAIL (START_IX_ERR - 24)
#define IX_HANDLE_LEAF_EXISTS_BUT_UNPIN_FAIL (START_IX_ERR - 25)
#define IX_HANDLE_INSERT_LEAF_JUST_INSERT_BUT_UNPIN_FAIL (START_IX_ERR - 25)
#define IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 26)
#define IX_HANDLE_INSERT_LEAF_SPLIT_BUT_UNPIN_FAIL (START_IX_ERR - 27)
#define IX_HANDLE_INSERT_INNER_JUST_INSERT_BUT_UNPIN_FAIL (START_IX_ERR - 28)
#define IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 29)
#define IX_HANDLE_INSERT_INNER_SPLIT_BUT_UNPIN_FAIL (START_IX_ERR - 30)
#define IX_HANDLE_DELETE_INNER_BUT_UNPIN_FAIL (START_IX_ERR - 31)
#define IX_HANDLE_DELETE_LEAF_BUT_UNPIN_FAIL (START_IX_ERR - 32)
#define IX_HANDLE_UPDATE_INNER_BUT_UNPIN_FAIL (START_IX_ERR - 33)
#define IX_HANDLE_UPDATE_LEAF_BUT_UNPIN_FAIL (START_IX_ERR - 34)
#define IX_HANDLE_NOT_UPDATE_BUT_UNPIN_FAIL (START_IX_ERR - 35)
#define IX_MANAGER_CREATE_BUT_CLOSE_FILE_FAIL (START_IX_ERR - 36)
#define IX_HANDLE_INSERT_LEAF_SPLIT_BUT_UNPIN_RIGHT_FAIL (START_IX_ERR - 37)
#define IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_ROOT_FAIL (START_IX_ERR - 38)
#define IX_HANDLE_INSERT_INNER_SPLIT_BUT_UNPIN_RIGHT_FAIL (START_IX_ERR - 39)
#define IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_ROOT_FAIL (START_IX_ERR - 40)
#define IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_RIGHT_FAIL (START_IX_ERR - 41)
#define IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_RIGHT_FAIL (START_IX_ERR - 42)

// The exact definition needs to be modified.
// Error in UNIX system call or library routine
#define IX_UNIX (START_IX_ERR - 43) // Unix error
#define IX_LASTERROR IX_UNIX

#endif
