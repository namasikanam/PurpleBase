//
// File:        ql_internal.h
// Description: Classes for all the operators in the QL component
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#ifndef QL_INTERNAL_H
#define QL_INTERNAL_H

#include <string>
#include <stdlib.h>
#include <cstdio>
#include <memory>
#include "redbase.h"
#include "parser.h"
#include "printer.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "ql.h"

bool QL_PrintRC(RC rc)
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
    else
    {
        return false;
    }
    return true;
}

void QL_Try(RC rc, RC ql_rc)
{
    if (rc)
    {
        QL_PrintRC(rc);
        throw ql_rc;
    }
}

inline int strcompare(const char *x, const char *y, int lx, int ly)
{
    for (int i = 0; i < lx && i < ly; ++i)
    {
        if (x[i] != y[i])
        {
            return x[i] < y[i] ? -1 : 1;
        }
    }
    if (lx < ly)
    {
        return -1;
    }
    else if (lx == ly)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

inline bool compare(AttrType type, CompOp op, const void *x, const void *y)
{
    switch (type)
    {
    case INT:
        // printf("compare(%d, %d)\n", *(int *)x, *(int *)y);

        switch (op)
        {
        case NO_OP:
            return true;
        case EQ_OP:
            return *(int *)x == *(int *)y;
        case NE_OP:
            return *(int *)x != *(int *)y;
        case LT_OP:
            return *(int *)x < *(int *)y;
        case GT_OP:
            return *(int *)x > *(int *)y;
        case LE_OP:
            return *(int *)x <= *(int *)y;
        case GE_OP:
            return *(int *)x >= *(int *)y;
        }
        break;
    case FLOAT:
        // printf("compare(%f, %f)\n", *(float *)x, *(float *)y);

        switch (op)
        {
        case NO_OP:
            return true;
        case EQ_OP:
            return *(float *)x == *(float *)y;
        case NE_OP:
            return *(float *)x != *(float *)y;
        case LT_OP:
            return *(float *)x < *(float *)y;
        case GT_OP:
            return *(float *)x > *(float *)y;
        case LE_OP:
            return *(float *)x <= *(float *)y;
        case GE_OP:
            return *(float *)x >= *(float *)y;
        }
        break;
    case STRING:
        // printf("compare(%s, %s)\n", (const char *)x, (const char *)y);

        switch (op)
        {
        case NO_OP:
            return true;
        case EQ_OP:
            return strcmp((const char *)x, (const char *)y) == 0;
        case NE_OP:
            return strcmp((const char *)x, (const char *)y) != 0;
        case LT_OP:
            return strcmp((const char *)x, (const char *)y) < 0;
        case GT_OP:
            return strcmp((const char *)x, (const char *)y) > 0;
        case LE_OP:
            return strcmp((const char *)x, (const char *)y) <= 0;
        case GE_OP:
            return strcmp((const char *)x, (const char *)y) >= 0;
        }
        break;
    }
}

#endif
