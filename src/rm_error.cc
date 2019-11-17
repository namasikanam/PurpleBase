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
    (char *)"record is not viable", // 0: RM_RECORD_NOT_VIABLE
};

static char *RM_ErrorMsg[] = {};

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