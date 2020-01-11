//
// File:        ql_error.cc
// Description: QL_PrintError implementation
// Authors:     Aditya Bhandari (adityasb@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "ql.h"

using namespace std;

static char *QL_WarnMsg[] = {
    (char *)"QL_DATABASE_CLOSED",                                                 // QL_DATABASE_CLOSED (START_QL_WARN + 0)
    (char *)"QL_REL_NOT_EXIST",                                                   // QL_REL_NOT_EXIST (START_QL_WARN + 1)
    (char *)"QL_SELECT_FAIL",                                                     // QL_SELECT_FAIL (START_QL_WARN + 2)
    (char *)"QL_ATTR_OF_MULT_REL",                                                // QL_ATTR_OF_MULT_REL (START_QL_WARN + 3)
    (char *)"QL_ATTR_OF_NO_REL",                                                  // QL_ATTR_OF_NO_REL (START_QL_WARN + 4)
    (char *)"QL_SAME_REL_APPEAR_AGAIN",                                           // QL_SAME_REL_APPEAR_AGAIN (START_QL_WARN + 5)
    (char *)"QL_TYPES_INCOMPATIBLE",                                              // QL_TYPES_INCOMPATIBLE (START_QL_WARN + 6)
    (char *)"QL_NULLPTR_RELATION",                                                // QL_NULLPTR_RELATION (START_QL_WARN + 7)
    (char *)"QL_SYS_CAT",                                                         // QL_SYS_CAT (START_QL_WARN + 8)
    (char *)"QL_INCORRECT_ATTRCOUNT",                                             // QL_INCORRECT_ATTRCOUNT (START_QL_WARN + 9)
    (char *)"QL_INCORRECT_ATTRTYPE",                                              // QL_INCORRECT_ATTRTYPE (START_QL_WARN + 10)
    (char *)"QL_ATTR_INCORRECT_REL",                                              // QL_ATTR_INCORRECT_REL (START_QL_WARN + 11)
    (char *)"QL_ATTR_INCORRECT_ATTR",                                             // QL_ATTR_INCORRECT_ATTR (START_QL_WARN + 12)
    (char *)"QL_INSERT_TOO_LONG_STRING",                                          // QL_INSERT_TOO_LONG_STRING (START_QL_WARN + 13)
    (char *)"QL_UPDATE_TOO_LONG_STRING",                                          // QL_UPDATE_TOO_LONG_STRING (START_QL_WARN + 14)
    (char *)"Something's wrong when inserting, perhaps the table doesn't exist?", // QL_INSERT_NOT_EXIST (START_QL_WARN + 15)
};

static char *QL_ErrorMsg[] = {
    (char *)"QL_RELS_SCAN_FAIL", // QL_RELS_SCAN_FAIL (START_QL_ERR - 0)
    (char *)"QL_INSERT_FAIL",    // QL_INSERT_FAIL (START_QL_ERR - 1)
    (char *)"QL_DELETE_FAIL",    // QL_DELETE_FAIL (START_QL_ERR - 2)
    (char *)"QL_UPDATE_FAIL",    // QL_UPDATE_FAIL(START_QL_ERR - 3)
};

//
// QL_PrintError
//
// Desc: Send a message corresponding to a QL return code to cerr
// In:   rc - return code for which a message is desired
//
void QL_PrintError(RC rc)
{
    // Check the return code is within proper limits
    if (rc >= START_QL_WARN && rc <= QL_LASTWARN)
        // Print warning
        cerr << "QL warning: " << QL_WarnMsg[rc - START_QL_WARN] << "\n";
    // Error codes are negative, so invert everything
    else if (-rc >= -START_QL_ERR && -rc < -QL_LASTERROR)
        // Print error
        cerr << "QL error: " << QL_ErrorMsg[-rc + START_QL_ERR] << "\n";
    else if (rc == QL_UNIX)
#ifdef PC
        cerr << "OS error\n";
#else
        cerr << strerror(errno) << "\n";
#endif
    else if (rc == 0)
        cerr << "QL_PrintError called with return code of 0\n";
    else
        cerr << "QL error: " << rc << " is out of bounds\n";
}
