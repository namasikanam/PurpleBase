//
// File:        ql_internal.h
// Description: Classes for all the operators in the QL component
// Authors:     Aditya Bhandari (adityasb@stanford.edu)
//

#ifndef QL_INTERNAL_H
#define QL_INTERNAL_H

#include <string>
#include <stdlib.h>
#include <memory>
#include "redbase.h"
#include "parser.h"
#include "printer.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "ql.h"

void QL_Try(RC rc, RC ql_rc)
{
    if (rc)
    {
        if (rc >= START_PF_WARN && rc <= END_PF_WARN || rc >= START_PF_ERR && rc <= END_PF_ERR)
        {
            PF_PrintError(rc);
        }
        else if (rc >= START_RM_WARN && rc <= END_RM_WARN || rc >= START_RM_ERR && rc <= END_RM_ERR)
        {
            RM_PrintError(rc);
        }
        else if (rc >= START_IX_WARN && rc <= END_IX_WARN || rc >= START_IX_ERR && rc <= END_IX_ERR)
        {
            IX_PrintError(rc);
        }
        else if (rc >= START_SM_WARN && rc <= END_SM_WARN || rc >= START_SM_ERR && rc <= END_SM_ERR)
        {
            SM_PrintError(rc);
        }
        throw ql_rc;
    }
}

#endif
