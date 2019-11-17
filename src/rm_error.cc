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
static char *RM_WarnMsg[] = {};

static char *RM_ErrorMsg[] = {};

//
// RM_PrintError
//
// Desc: Send a message corresponding to an RM return code to cerr
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
}