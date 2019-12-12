//
// File:        ix_error.cc
// Description: IX_PrintError implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "ix_internal.h"
#include "ix.h"

using namespace std;

// Writing messages is too hard for me,
// the messages are not written rigorously.
static char *IX_WarnMsg[] = {
    (char *)"The filename is illegal.",                      // IX_ILLEGAL_FILENAME (START_IX_WARN + 0)
    (char *)"Failed to create an IX manager.",               // IX_MANAGER_CREATE_FAIL (START_IX_WARN + 1)
    (char *)"Failed to destroy an IX manager.",              // IX_MANAGER_DESTROY_FAIL (START_IX_WARN + 2)
    (char *)"Failed to open an IX manager.",                 // IX_MANAGER_OPEN_FAIL (START_IX_WARN + 3)
    (char *)"Failed to get RID.",                            // IX_HANDLE_GETRID_FAIL (START_IX_WARN + 4)
    (char *)"Closing an closed file.",                       // IX_MANAGER_CLOSE_CLOSED_FILE_HANDLE (START_IX_WARN + 5)
    (char *)"Fail to close an IX manager.",                  // IX_MANAGER_CLOSE_FAIL (START_IX_WARN + 6)
    (char *)"Fail to force all pages in an IX manager.",     // IX_HANDLE_FORCE_FAIL (START_IX_WARN + 7)
    (char *)"Fail to insert in an IX handle.",               // IX_HANDLE_INSERT_FAIL (START_IX_WARN + 8)
    (char *)"Deleting a deleted entry.",                     // IX_HANDLE_DELETE_NOT_EXIST (START_IX_WARN + 9)
    (char *)"Inserting an existing entry.",                  // IX_HANDLE_INSERT_EXISTS (START_IX_WARN + 10)
    (char *)"Open a scan with comp operator NE.",            // IX_OPEN_SCAN_NE (START_IX_WARN + 11)
    (char *)"Fail to split a leaf.",                         // IX_HANDLE_LEAF_SPLIT_FAIL (START_IX_WARN + 12)
    (char *)"Fail to create a new root.",                    // IX_HANDLE_LEAF_NEW_ROOT_FAIL (START_IX_WARN + 13)
    (char *)"The scanning encounters EOF.",                  // IX_EOF (START_IX_WARN + 14)
    (char *)"Failed to check if an entry exists.",           // IX_HANDLE_EXISTS_FAIL (START_IX_WARN + 15)
    (char *)"Failed to split an inner node of the B+ tree.", // IX_HANDLE_INNER_SPLIT_FAIL (START_IX_WARN + 16)
    (char *)"Failed to create a new root.",                  // IX_HANDLE_INNER_NEW_ROOT_FAIL (START_IX_WARN + 17)
    (char *)"Failed to delete some entry.",                  // IX_HANDLE_DELETE_FAIL
};

static char *IX_ErrorMsg[] = {
    (char *)"Failed to create file when opening a file. Invalid PC file name.",                         // IX_MANAGER_CREATE_OPEN_FILE_FAIL(START_IX_ERR - 0) // Invalid PC file name
    (char *)"Failed to create header page.",                                                            // IX_MANAGER_CREATE_HEAD_FAIL (START_IX_ERR - 1)
    (char *)"Failed when creating the header page, and then failed to unpin the header page.",          // IX_MANAGER_CREATE_HEAD_FAIL_UNPIN_FAIL (START_IX_ERR - 2)
    (char *)"Succeed to create the header page, but failed to unpin after creating.",                   // IX_MANAGER_CREATE_HEAD_BUT_UNPIN_FAIL (START_IX_ERR - 3)
    (char *)"There's no deletion happened here, but failed when trying to unpin.",                      // IX_HANDLE_NOT_DELETE_BUT_UNPIN_FAIL (START_IX_ERR - 4)
    (char *)"Succeed to insert but fail to unpin.",                                                     // IX_HANDLE_INSERT_BUT_UNPIN_FAIL (START_IX_ERR - 5)
    (char *)"The answer of asking if existing is no, failed when unpinning after asking if existing.",  // IX_HANDLE_NOT_EXISTS_BUT_UNPIN_FAIL (START_IX_ERR - 6)
    (char *)"Failed to create root.",                                                                   // IX_MANAGER_CREATE_ROOT_FAIL (START_IX_ERR - 7)
    (char *)"Failed to create root and fail to unpin.",                                                 // IX_MANAGER_CREATE_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 8)
    (char *)"Succeeded to create root but failed to unpin.",                                            // IX_MANAGER_CREATE_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 9)
    (char *)"Open a file, but failed when unpinning.",                                                  // IX_MANAGER_OPEN_BUT_UNPIN_FAIL (START_IX_ERR - 10)
    (char *)"Close a file, but failed when unpinning.",                                                 // IX_MANAGER_CLOSE_FAIL_UNPIN_FAIL (START_IX_ERR - 11)
    (char *)"Close a head, but failed when unpinning.",                                                 // IX_MANAGER_CLOSE_HEAD_BUT_UNPIN_FAIL (START_IX_ERR - 12)
    (char *)"Trying to operate on a closed IX handle.",                                                 // IX_HANDLE_CLOSED (START_IX_ERR - 13)
    (char *)"Fail to open and also fail to unpin.",                                                     // IX_MANAGER_OPEN_FAIL_UNPIN_FAIL (START_IX_ERR - 14)
    (char *)"Fail to insert and also fail to unpin.",                                                   // IX_HANDLE_INSERT_FAIL_UNPIN_FAIL (START_IX_ERR - 15)
    (char *)"Fail to get rid, and also fail to unpin.",                                                 // IX_HANDLE_GETRID_FAIL_UNPIN_FAIL (START_IX_ERR - 16)
    (char *)"Rid is got, but fail to unpin.",                                                           // IX_HANDLE_GETRID_BUT_UNPIN_FAIL (START_IX_ERR - 17)
    (char *)"Fail to split a leaf, and also fail to unpin.",                                            // IX_HANDLE_LEAF_SPLIT_FAIL_UNPIN_FAIL (START_IX_ERR - 18)
    (char *)"Fail to check existence, and also fail to unpin.",                                         // IX_HANDLE_EXISTS_FAIL_UNPIN_FAIL (START_IX_ERR - 19)
    (char *)"Fail to split an inner node, and also fail to unpin.",                                     // IX_HANDLE_INNER_SPLIT_FAIL_UNPIN_FAIL (START_IX_ERR - 20)
    (char *)"Fail to create a new root when splitting a leaf, and even also fail to unpin.",            // IX_HANDLE_LEAF_NEW_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 21)
    (char *)"Fail to create a new root when splitting an inner node, and even also fail to unpin.",     // IX_HANDLE_INNER_NEW_ROOT_FAIL_UNPIN_FAIL (START_IX_ERR - 22)
    (char *)"Fail to delete, and also fail to unpin.",                                                  // IX_HANDLE_DELETE_FAIL_UNPIN_FAIL (START_IX_ERR - 23)
    (char *)"The inner node exists, but also fail to unpin.",                                           // IX_HANDLE_INNER_EXISTS_BUT_UNPIN_FAIL (START_IX_ERR - 24)
    (char *)"Finding some entry in some leaf, but fail to unpin.",                                      // IX_HANDLE_LEAF_EXISTS_BUT_UNPIN_FAIL (START_IX_ERR - 25)
    (char *)"Just insert an entry in a leaf (no split), but fail to unpin, which is a little strange.", // IX_HANDLE_INSERT_LEAF_JUST_INSERT_BUT_UNPIN_FAIL (START_IX_ERR - 25)
    (char *)"A new root is created when inserting an entry to a leaf, but fail to unpin.",              // IX_HANDLE_INSERT_LEAF_NEW_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 26)
    (char *)"A leaf is split when inserting an entry to it, but fail to unpin after the success.",      // IX_HANDLE_INSERT_LEAF_SPLIT_BUT_UNPIN_FAIL (START_IX_ERR - 27)
    (char *)"An inner node is inserted a new son without splitting, but fail to unpin.",                // IX_HANDLE_INSERT_INNER_JUST_INSERT_BUT_UNPIN_FAIL (START_IX_ERR - 28)
    (char *)"A new root is created when inserting, but fail to unpin after success.",                   // IX_HANDLE_INSERT_INNER_NEW_ROOT_BUT_UNPIN_FAIL (START_IX_ERR - 29)
    (char *)"An inner node is split when inserting, but fail to unpin.",                                // IX_HANDLE_INSERT_INNER_SPLIT_BUT_UNPIN_FAIL (START_IX_ERR - 30)
    (char *)"A son of an inner node is deleted, but fail to unpin.",                                    // IX_HANDLE_DELETE_INNER_BUT_UNPIN_FAIL (START_IX_ERR - 31)
    (char *)"A leaf is deleted, but fail to unpin.",                                                    // IX_HANDLE_DELETE_LEAF_BUT_UNPIN_FAIL (START_IX_ERR - 32)
    (char *)"An inner node is updated, but fail to unpin.",                                             // IX_HANDLE_UPDATE_INNER_BUT_UNPIN_FAIL (START_IX_ERR - 33)
    (char *)"A leaf node is updated, but fail to unpin.",                                               // IX_HANDLE_UPDATE_LEAF_BUT_UNPIN_FAIL (START_IX_ERR - 34)
    (char *)"Nothing to update here, but unpinning fails magically.",                                   // IX_HANDLE_NOT_UPDATE_BUT_UNPIN_FAIL (START_IX_ERR - 35)
    (char *)"Error in Unix system call or library routine.",                                            // IX_UNIX (START_IX_ERR - 36)
};

//
// IX_PrintError
//
// Desc: Send a message corresponding to an IX return code to cerr
// In:   rc - return code for which a message is desired
//
void IX_PrintError(RC rc)
{
    // Check the return code is within proper limits
    if (rc >= START_IX_WARN && rc <= IX_LASTWARN)
        // Print warning
        cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";
    // Error codes are negative, so invert everything
    else if (-rc >= -START_IX_ERR && -rc < -IX_LASTERROR)
        // Print error
        cerr << "IX error: " << IX_ErrorMsg[-rc + START_IX_ERR] << "\n";
    else if (rc == IX_UNIX)
#ifdef PC
        cerr << "OS error\n";
#else
        cerr << strerror(errno) << "\n";
#endif
    else if (rc == 0)
        cerr << "IX_PrintError called with return code of 0\n";
    else
        cerr << "IX error: " << rc << " is out of bounds\n";
}
