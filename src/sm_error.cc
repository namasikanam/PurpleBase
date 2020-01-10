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
    (char *)"The database name to open is null.", // SM_NULL_DATABASE_NAME (START_SM_WARN + 1)
    (char *)"Trying to open an opened database.", // SM_DATABASE_OPEN (START_SM_WARN + 2)
    (char *)"Trying to close a closed database.", // SM_DATABASE_CLOSED (START_SM_WARN + 3)
};

static char *SM_ErrorMsg[] = {
    (char *)"Invalid database name",
    (char *)"Fail to open relcat, when opening a database.",  // SM_OPEN_RELCAT_FAIL (START_SM_WARN - 1)
    (char *)"Fail to open attrcat, when opening a database.", // SM_OPEN_ATTRCAT_FAIL (START_SM_WARN - 2)
    (char *)"database cannot be closed",                      // SM_INVALID_DATABASE_CLOSE(START_SM_ERR - 5)
    (char *)"Undefined function.",                            // SM_UNDEFINED (START_SM_WARN - 6)
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
        cerr << "SM_PrintError called with return code of 0\n";
    else
        cerr << "SM error: " << rc << " is out of bounds\n";
}
