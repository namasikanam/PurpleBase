//
// File:        sm_error.cc
// Description: SM_PrintError implementation
// Authors:     Aditya Bhandari (adityasb@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "sm.h"

using namespace std;

static char *SM_WarnMsg[] = {
    (char *)"The database does not exist.",
    (char *)"Trying to create a table in a closed database.",                                                                                                              // SM_CREATE_TABLE_CLOSED (START_SM_WARN + 1)
    (char *)"Trying to open an opened database.",                                                                                                                          // SM_DATABASE_OPEN (START_SM_WARN + 2)
    (char *)"Trying to close a closed database.",                                                                                                                          // SM_DATABASE_CLOSED (START_SM_WARN + 3)
    (char *)"SM_INCORRECT_ATTRCOUNT",                                                                                                                                      // SM_INCORRECT_ATTRCOUNT (START_SM_WARN + 4)
    (char *)"SM_DROP_TABLE_CLOSED",                                                                                                                                        // SM_DROP_TABLE_CLOSED (START_SM_WARN + 5)
    (char *)"SM_CREATE_INDEX_EXISTS",                                                                                                                                      // SM_CREATE_INDEX_EXISTS (START_SM_WARN + 6)
    (char *)"SM_CREATE_INDEX_CLOSED",                                                                                                                                      // SM_CREATE_INDEX_CLOSED (START_SM_WARN + 7)
    (char *)"SM_DROP_INDEX_CLOSED",                                                                                                                                        // SM_DROP_INDEX_CLOSED (START_SM_WARN + 8)
    (char *)"SM_INDEX_DOES_NOT_EXIST",                                                                                                                                     // SM_INDEX_DOES_NOT_EXIST (START_SM_WARN + 9)
    (char *)"SM_NULLPTR_REL_NAME",                                                                                                                                         // SM_NULLPTR_REL_NAME (START_SM_WARN + 10)
    (char *)"SM_NULLPTR_ATTR_NAME",                                                                                                                                        // SM_NULLPTR_ATTR_NAME (START_SM_WARN + 11)
    (char *)"SM_NULLPTR_DB_NAME",                                                                                                                                          // SM_NULLPTR_DB_NAME (START_SM_WARN + 12)
    (char *)"SM_NULLPTR_FILE_NAME",                                                                                                                                        // SM_NULLPTR_FILE_NAME (START_SM_WARN + 13)
    (char *)"SM_LOAD_CLOSED",                                                                                                                                              // SM_LOAD_CLOSED (START_SM_WARN + 14)
    (char *)"SM_LOAD_INVALID_DATA_FILE",                                                                                                                                   // SM_LOAD_INVALID_DATA_FILE (START_SM_WARN + 15)
    (char *)"SM_HELP_CLOSED",                                                                                                                                              // SM_HELP_CLOSED (START_SM_WARN + 16)
    (char *)"SM_PRINT_CLOSED",                                                                                                                                             // SM_PRINT_CLOSED (START_SM_WARN + 17)
    (char *)"SM_LOAD_SYSTEM_CAT",                                                                                                                                          // SM_LOAD_SYSTEM_CAT (START_SM_WARN + 18)
    (char *)"SM_GET_ALL_ATTR_INFO_FAIL",                                                                                                                                   // SM_GET_ALL_ATTR_INFO_FAIL (START_SM_WARN + 19)
    (char *)"SM_GET_ALL_ATTR_INFO_SCAN_FAIL",                                                                                                                              // SM_GET_ALL_ATTR_INFO_SCAN_FAIL (START_SM_WARN + 20)
    (char *)"SM_GET_ALL_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL",                                                                                                              // SM_GET_ALL_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_WARN + 21)
    (char *)"SM_GET_ATTR_INFO_FAIL",                                                                                                                                       // SM_GET_ATTR_INFO_FAIL (START_SM_ERR + 22)
    (char *)"SM_GET_ATTR_INFO_SCAN_FAIL",                                                                                                                                  // SM_GET_ATTR_INFO_SCAN_FAIL (START_SM_ERR + 23)
    (char *)"SM_GET_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL",                                                                                                                  // SM_GET_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR + 24)
    (char *)"SM_GET_REL_INFO_FAIL",                                                                                                                                        // SM_GET_REL_INFO_FAIL (START_SM_ERR + 25)
    (char *)"Something is wrong when scanning in the system relation catalog to seek the information the requested relation, perhaps the queried relation doesn't exist.", // SM_GET_REL_INFO_SCAN_FAIL (START_SM_ERR + 26)
    (char *)"SM_GET_REL_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL",                                                                                                                   // SM_GET_REL_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR + 27)
    (char *)"Usage: set debug [TRUE | FALSE]",                                                                                                                             // SM_SET_DEBUG_INVALID (START_SM_ERR + 28)
    (char *)"The tablename to create has existed.",                                                                                                                        // SM_CREATE_TABLE_EXIST (START_SM_WARN + 29)
    (char *)"A value (string) is too long to load.",                                                                                                                       // SM_LOAD_STRING_TOO_LONG (START_SM_WARN + 30)
    (char *)"A value (int) is not in the correct format to load.",                                                                                                         // SM_LOAD_BAD_INT (START_SM_WARN + 31)
    (char *)"A value (float) is not in the correct format to load.",                                                                                                       // SM_LOAD_BAD_INT (START_SM_WARN + 31)
};

static char *SM_ErrorMsg[] = {
    (char *)"Invalid database name",
    (char *)"Fail to open relcat, when opening a database.",      // SM_OPEN_RELCAT_FAIL (START_SM_WARN - 1)
    (char *)"Fail to open attrcat, when opening a database.",     // SM_OPEN_ATTRCAT_FAIL (START_SM_WARN - 2)
    (char *)"SM_CLOSE_RELCAT_FAIL",                               // SM_CLOSE_RELCAT_FAIL (START_SM_ERR - 3)
    (char *)"SM_CLOSE_RELCAT_FAIL",                               // SM_CLOSE_ATTRCAT_FAIL (START_SM_ERR - 4)
    (char *)"database cannot be closed",                          // SM_INVALID_DATABASE_CLOSE(START_SM_ERR - 5)
    (char *)"Undefined function.",                                // SM_UNDEFINED (START_SM_WARN - 6)
    (char *)"SM_TOO_LONG_RELNAME",                                // SM_TOO_LONG_RELNAME (START_SM_ERR - 7)
    (char *)"SM_TOO_LONG_ATTRNAME",                               // SM_TOO_LONG_ATTRNAME (START_SM_ERR - 8)
    (char *)"SM_CREATE_TABLE_FAIL",                               // SM_CREATE_TABLE_FAIL (START_SM_ERR - 9)
    (char *)"SM_PRINT_SCAN_FAIL_CLOSE_SCAN_FAIL",                 // SM_PRINT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 37)
    (char *)"SM_CREATE_TABLE_SCAN_FAIL_CLOSE_SCAN_FAIL",          // SM_CREATE_TABLE_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 11)
    (char *)"SM_CREATE_TABLE_INSERT_ATTR_CAT_FAIL",               // SM_CREATE_TABLE_INSERT_ATTR_CAT_FAIL (START_SM_ERR - 12)
    (char *)"SM_CREATE_TABLE_INSERT_REL_CAT_FAIL",                // SM_CREATE_TABLE_INSERT_REL_CAT_FAIL (START_SM_ERR - 13)
    (char *)"SM_DROP_TABLE_FAIL",                                 // SM_DROP_TABLE_FAIL (START_SM_ERR - 14)
    (char *)"SM_DROP_TABLE_REL_CAT_SCAN_FAIL",                    // SM_DROP_TABLE_REL_CAT_SCAN_FAIL (START_SM_ERR - 15)
    (char *)"SM_DROP_TABLE_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL",    // SM_DROP_TABLE_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 16)
    (char *)"SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL",                   // SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL (START_SM_ERR - 17)
    (char *)"SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL",   // SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 18)
    (char *)"SM_CREATE_INDEX_REL_CAT_SCAN_FAIL",                  // SM_CREATE_INDEX_REL_CAT_SCAN_FAIL (START_SM_ERR - 19)
    (char *)"SM_CREATE_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL",  // SM_CREATE_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 20)
    (char *)"SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL",                 // SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL (START_SM_ERR - 21)
    (char *)"SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL", // SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 22)
    (char *)"SM_CREATE_INDEX_FAIL",                               // SM_CREATE_INDEX_FAIL (START_SM_ERR - 23)
    (char *)"SM_CREATE_INDEX_RM_SCAN_FAIL",                       // SM_CREATE_INDEX_RM_SCAN_FAIL (START_SM_ERR - 24)
    (char *)"SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL",       // SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 25)
    (char *)"SM_DROP_INDEX_REL_CAT_SCAN_FAIL",                    //SM_DROP_INDEX_REL_CAT_SCAN_FAIL (START_SM_ERR - 26)
    (char *)"SM_DROP_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL",    // SM_DROP_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 27)
    (char *)"SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL",                   //SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL (START_SM_ERR - 28)
    (char *)"SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL",   // SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL(START_SM_ERR - 29)
    (char *)"SM_DROP_INDEX_FAIL",                                 // SM_DROP_INDEX_FAIL (START_SM_ERR - 30)
    (char *)"SM_LOAD_FAIL",                                       // SM_LOAD_FAIL (START_SM_ERR - 31)
    (char *)"SM_LOAD_FAIL_CLOSE_FAIL",                            // SM_LOAD_FAIL_CLOSE_FAIL (START_SM_ERR - 32)
    (char *)"SM_HELP_REL_CAT_SCAN_FAIL",                          // SM_HELP_REL_CAT_SCAN_FAIL (START_SM_ERR - 33)
    (char *)"SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL",          // SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 34)
    (char *)"SM_PRINT_FAIL",                                      // SM_PRINT_FAIL (START_SM_ERR - 35)
    (char *)"SM_PRINT_SCAN_FAIL",                                 // SM_PRINT_SCAN_FAIL (START_SM_ERR - 36)
};

//
// SM_PrintError
//
// Desc: Send a message corresponding to an SM return code to cerr
// In:   rc - return code for which a message is desired
//
void SM_PrintError(RC rc)
{
    // Check the return code is within proper limits
    if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
        // Print warning
        cerr
            << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
    // Error codes are negative, so invert everything
    else if (-rc >= -START_SM_ERR && -rc < -SM_LASTERROR)
        // Print error
        cerr
            << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";
    else if (rc == SM_UNIX)
#ifdef PC
        cerr
            << "OS error\n";
#else
        cerr
            << strerror(errno) << "\n";
#endif
    else if (rc == 0)
        cerr
            << "SM_PrintError called with return code of 0\n";
    else
        cerr << "SM error: " << rc << " is out of bounds\n";
}
