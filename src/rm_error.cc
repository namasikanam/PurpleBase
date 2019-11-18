//
// File:        rm_error.cc
// Description: RM_PrintError implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include "rm_internal.h"
#include "rm.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
    (char *)"Record is not viable.",                                                                     // + 0: RM_RECORD_NOT_VIABLE
    (char *)"File is closed.",                                                                           // + 1: RM_FILE_HANDLE_CLOSED
    (char *)"Fail to insert record into old (existing) pages of some file.",                             // + 2: RM_FILE_INSERT_OLD_FAIL
    (char *)"Fail to insert record into old (existing) pages of some file and fail to unpin some page.", // + 3: RM_FILE_INSERT_OLD_FAIL_UNPIN_FAIL
    (char *)"Fail to get next record of some scan.",                                                     // + 4: RM_SCAN_NEXT_FAIL
    (char *)"Fail to allocate a new page of some file when inserting a record.",                         // + 5: RM_FILE_INSERT_NEW_PAGE_FAIL
    (char *)"Fail to insert record into a new-allocated page of some file and fail to unpin some page.", // + 6: RM_FILE_INSERT_NEW_FAIL_UNPIN_FAIL
    (char *)"Fail to insert record into a new-allocated page of some file.",                             // + 7: RM_FILE_INSERT_NEW_FAIL
    (char *)"Have progressed to EOF.",                                                                   // + 8: RM_FILE_GET_SUCC_UNPIN_FAIL
    (char *)"Fail to delete a record from a file.",                                                      // + 9: RM_FILE_DELETE_FAIL
    (char *)"The rid to delete is illegal.",                                                             // + 10: RM_FILE_DELETE_ILLEGAL_RID
    (char *)"The record to delete is not found, perhaps it has been deleted before.",                    // + 11: RM_FILE_DELETE_NOT_FOUND
    (char *)"Try to get record with illegal rid.",                                                       // + 12: RM_FILE_GET_ILLEGAL_RID
    (char *)"Fail to get a record from a file.",                                                         // + 13: RM_FILE_GET_FAIL
    (char *)"The record to get is not found.",                                                           // + 14: RM_FILE_GET_NOT_FOUND
    (char *)"Fail to get a record from a file, and also fail to unpin some page.",                       // + 15: RM_FILE_GET_FAIL_UNPIN_FAIL
    (char *)"Fail to delete a record from a file, and also fail to unpin some page.",                    // + 16: RM_FILE_DELETE_FAIL_UNPIN_FAIL
    (char *)"Fail to update a record of a file.",                                                        // + 17: RM_FILE_UPDATE_FAIL
    (char *)"The rid to update is illegal.",                                                             // + 18: RM_FILE_UPDATE_ILLEGAL_RID
    (char *)"Fail to update some record of some file, and also fail to unpin some page.",                // + 19: RM_FILE_UPDATE_FAIL_UNPIN_FAIL
    (char *)"The record to update is not found.",                                                        // + 20: RM_FILE_UPDATE_NOT_FOUND
    (char *)"The size of record to update doesn't fit the size of the file.",                            // + 21: RM_FILE_UPDATE_NOT_FOUND
    (char *)"The scan is closed.",                                                                       // + 22: RM_SCAN_CLOSED
    (char *)"The file to scan is closed.",                                                               // + 23: RM_SCAN_OPEN_CLOSED_FILE
    (char *)"Recordsize is too large for a file.",                                                       // + 24: RM_MANAGER_RECORDSIZE_TOO_LARGE
    (char *)"Fail to create file.",                                                                      // + 25: RM_MANAGER_CREATE_FAIL
    (char *)"Fail to create file and unpin some (header) page.",                                         // + 26: RM_MANAGER_CREATE_FAIL_UNPIN_FAIL
    (char *)"Fail to destroy file.",                                                                     // + 27: RM_MANAGER_DESTROY_FAIL
    (char *)"Fail to open file.",                                                                        // + 28: RM_MANAGER_OPEN_FAIL
    (char *)"Succeed to get a record but failed to unpin.",                                              // + 29: RM_FILE_GET_BUT_UNPIN_FAIL
    (char *)"Fail to force pages of a file.",                                                            // + 30: RM_FILE_FORCE_FAIL
    (char *)"Try to close a closed file.",                                                               // + 31: RM_MANAGER_CLOSE_CLOSED_FILE
    (char *)"Fail to close some file.",                                                                  // + 32: RM_MANAGER_CLOSE_FAIL
    (char *)"Fail to close some file and fail to unpin again.",                                          // + 33: RM_MANAGER_CLOSE_FAIL_UNPIN_FAIL
    (char *)"The record to get is not found in the file and also fail to unpin.",                        // + 34: RM_FILE_GET_NOT_FOUND_UNPIN_FAIL
    (char *)"The record to delete is not found in the file and also fail to unpin.",                        // + 35: RM_FILE_DELETE_NOT_FOUND_UNPIN_FAIL
    (char *)"The record to update is not found in the file and also fail to unpin.",                        // + 36: RM_FILE_UPDATE_NOT_FOUND_UNPIN_FAIL
};

static char *RM_ErrorMsg[] = {
    (char *)"Here's an available page without available slot, which is found during insertion.", // - 0: RM_FILE_INSERT_NO_AVAILABLE_SLOT_IN_AVAILABLE_PAGES
    (char *)"Having succeeded to insert, failed to unpin unexpectedly.",                         // - 1: RM_ERROR_FILE_INSERT_BUT_UNPIN_FAIL
    (char *)"Having succeeded to deletion, failed to unpin unexpectedly.",                       // - 2: RM_FILE_DELETE_BUT_UNPIN_FAIL
    (char *)"Having succeeded to update, failed to unpin unexpectedly.",                         // - 3: RM_FILE_UPDATE_BUT_UNPIN_FAIL
    (char *)"Succeed to create file but fail to unpin.",                                         // -4: RM_MANAGER_CREATE_BUT_UNPIN_FAIL
    (char *)"Succeed to close file but fail to unpin.",                                          // -5: RM_MANAGER_CLOSE_BUT_UNPIN_FAIL
    (char *)"Succeed to open file but fail to unpin.",                                           // -6: RM_MANAGER_OPEN_BUT_UNPIN_FAIL
    (char *)"Succeed to create file but fail to close.",                                         // -7: RM_MANAGER_CREATE_BUT_CLOSE_FAIL
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to an RM return code to cerr
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
    // Check the return code is within proper limits
    if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
        // Print warning
        cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
    // Error codes are negative, so invert everything
    else if (-rc >= -START_RM_ERR && -rc <= -RM_LASTERROR)
        // Print error
        cerr << "RM error: " << RM_ErrorMsg[-rc + START_RM_ERR] << "\n";
    else if (rc == OK_RC)
        cerr << "RM_PrintError called with return code of OK_RC\n";
    else
        cerr << "RM error: " << rc << " is out of bounds\n";
}