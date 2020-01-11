//
// File:        ql_manager.cc
// Description: QL_Manager class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <bits/stdc++.h>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include <sstream>
#include <memory>
#include "redbase.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include "printer.h"
#include "parser.h"
#include "ql_internal.h"

using namespace std;

// Method: QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
// Constructor for the QL Manager
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : smManager(smm), ixManager(ixm), rmManager(rmm) {}

// Method: ~QL_Manager()
// Destructor for the QL Manager
QL_Manager::~QL_Manager()
{
    // Nothing to free
}

/************ SELECT ************/

// Method:
// Handle the select clause
/* Steps:
    1) Check whether the database is open
    2) Obtain attribute information for the relations and check
    3) Validate the selection expressions
    4) Validate the conditions
    5) Form the physical operator tree
    6) Get the tuples from the root node
    7) Print the physical query plan
*/
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char *const relations[],
                      int nConditions, const Condition conditions[])
{
    try
    {
        // Check whether database is open
        if (!smManager.open)
        {
            return QL_DATABASE_CLOSED;
        }

        // Check if the relations exist
        DataAttrInfo *attributes[nRelations];
        SM_RelcatRecord rcRecord[nRelations];
        for (int i = 0; i < nRelations; ++i)
        {
            rcRecord[i] = smManager.GetRelInfo(relations[i]);
            attributes[i] = new DataAttrInfo[rcRecord[i].attrCount];
            smManager.GetAttrInfo(relations[i], rcRecord[i].attrCount, (char *)attributes[i]);
        }

        // Check every relation is distinguishable
        for (int i = 0; i < nRelations; ++i)
            for (int j = i + 1; j < nRelations; ++j)
                if (strcmp(relations[i], relations[j]) == 0)
                    throw QL_SAME_REL_APPEAR_AGAIN;

        // Validate the select attributes
        RelAttr *changedSelAttrs;
        if (nSelAttrs == 1 && strcmp(selAttrs[0].attrName, "*") == 0)
        {
            // In case of *, select all attributes
            nSelAttrs = 0;
            for (int i = 0; i < nRelations; i++)
            {
                nSelAttrs += rcRecord[i].attrCount;
            }
        }

        // The informations of attributes
        int indexRelOfPrintAttr[nSelAttrs];
        int offsetOfPrintAttr[nSelAttrs]; // offset in the original record
        DataAttrInfo printAttrs[nSelAttrs];

        if (nSelAttrs >= 1 && strcmp(selAttrs[0].attrName, "*") == 0)
        {
            changedSelAttrs = new RelAttr[nSelAttrs];
            for (int i = 0, k = 0; i < nRelations; ++i)
            {
                for (int j = 0; j < rcRecord[i].attrCount; ++j, ++k)
                {
                    changedSelAttrs[k] = RelAttr{
                        .relName = (char *)relations[i],
                        .attrName = attributes[i][j].attrName};

                    indexRelOfPrintAttr[k] = i;
                    offsetOfPrintAttr[k] = attributes[i][j].offset;

                    printAttrs[k] = attributes[i][j];
                    printAttrs[k].offset = k == 0 ? 0 : printAttrs[k - 1].offset + (printAttrs[k - 1].attrType == STRING ? printAttrs[k - 1].attrLength + 1 : 4);
                }
            }
        }
        else
        {
            changedSelAttrs = (RelAttr *)selAttrs;
            for (int i = 0; i < nSelAttrs; ++i)
            {
                SM_AttrcatRecord acRecord = checkAttr(changedSelAttrs[i], nRelations, relations);

                indexRelOfPrintAttr[i] = indexOfRel(acRecord.relName, nRelations, relations);
                offsetOfPrintAttr[i] = acRecord.offset;

                strcmp(printAttrs[i].relName, acRecord.relName);
                strcmp(printAttrs[i].attrName, acRecord.attrName);
                printAttrs[i].attrType = acRecord.attrType;
                printAttrs[i].attrLength = acRecord.attrLength;
                printAttrs[i].indexNo = acRecord.indexNo;
                printAttrs[i].offset = i == 0 ? 0 : printAttrs[i - 1].offset + (printAttrs[i - 1].attrType == STRING ? printAttrs[i - 1].attrLength + 1 : 4);
            }
        }

        if (smManager.debug)
        {
            printf("nConditions = %d\n", nConditions);
        }

        int indexRelOfCondLHS[nConditions];
        int indexRelOfCondRHS[nConditions];
        int offsetOfCondLHS[nConditions];
        int offsetOfCondRHS[nConditions];

        // Validate the conditions
        Condition changedConditions[nConditions];
        for (int i = 0; i < nConditions; ++i)
        {
            changedConditions[i] = conditions[i];

            // Check whether LHS is a valid attribute
            SM_AttrcatRecord lrec = checkAttr(changedConditions[i].lhsAttr, nRelations, relations);
            indexRelOfCondLHS[i] = indexOfRel(lrec.relName, nRelations, relations);
            offsetOfCondLHS[i] = lrec.offset;
            AttrType lhsType = lrec.attrType;

            // If RHS is a attribute, check it
            AttrType rhsType;
            if (changedConditions[i].bRhsIsAttr)
            {
                SM_AttrcatRecord rrec = checkAttr(changedConditions[i].rhsAttr, nRelations, relations);
                indexRelOfCondRHS[i] = indexOfRel(rrec.relName, nRelations, relations);
                offsetOfCondRHS[i] = rrec.offset;
                rhsType = rrec.attrType;
            }
            else
            {
                rhsType = changedConditions[i].rhsValue.type;
            }

            // Check if the types of LHS and RHS are compatible (same)
            if (lhsType != rhsType)
            {
                throw QL_TYPES_INCOMPATIBLE;
            }
            changedConditions[i].rhsValue.type = lhsType;
        }

        if (smManager.debug)
        {
            // printf("Before building printer, indexRelOfPrintAttr[0] = %d\n", indexRelOfPrintAttr[0]);
        }

        Printer p(printAttrs, nSelAttrs);
        p.PrintHeader(cout);

        char buf[printAttrs[nSelAttrs - 1].offset + printAttrs[nSelAttrs - 1].attrLength];
        char *records[nRelations];
        scanRelations(0, nRelations, relations, records, nConditions, changedConditions, indexRelOfCondLHS, offsetOfCondLHS, indexRelOfCondRHS, offsetOfCondRHS, nSelAttrs, printAttrs, indexRelOfPrintAttr, offsetOfPrintAttr, p, buf);

        p.PrintFooter(cout);
    }
    catch (RC rc)
    {
        if (QL_PrintRC(rc))
        {
            return QL_SELECT_FAIL;
        }
        else
        {
            return rc;
        }
    }
    return OK_RC;
}

SM_AttrcatRecord QL_Manager::checkAttr(RelAttr &attr, int nRelations, const char *const relations[])
{
    SM_AttrcatRecord ans;
    // If relName is not specified, check if unique
    if (attr.relName == nullptr)
    {
        // Check all the relations in the relations array
        int cnt = 0;
        for (int j = 0; j < nRelations; ++j)
        {
            try
            {
                SM_AttrcatRecord acRecord = smManager.GetAttrInfo(relations[j], attr.attrName);
                if (cnt == 0)
                {
                    ans = acRecord;
                    ++cnt;
                    attr.attrName = (char *)relations[j];
                }
                else if (cnt == 1)
                {
                    throw QL_ATTR_OF_MULT_REL;
                }
            }
            catch (RC rc)
            {
            }
        }
        if (cnt == 0)
        {
            throw QL_ATTR_OF_NO_REL;
        }
    }
    else
    {
        // check if the attribute exists in the given relName
        ans = smManager.GetAttrInfo(attr.relName, attr.attrName);

        // check if the relName exisis in relations
        int cnt = 0;
        for (int j = 0; j < nRelations; ++j)
            cnt += strcmp(attr.relName, relations[j]) == 0;
        if (cnt == 0)
        {
            throw QL_ATTR_OF_NO_REL;
        }
        else if (cnt > 1)
        {
            throw QL_ATTR_OF_MULT_REL;
        }
    }
    return ans;
}

void QL_Manager::scanRelations(int id, int nRelations, const char *const relations[], char *records[], int nConditions, Condition conditions[], int indexRelOfCondLHS[], int offsetOfCondLHS[], int indexRelOfCondRHS[], int offsetOfCondRHS[], int nSelAttrs, DataAttrInfo printAttrs[], int indexRelOfPrintAttr[], int offsetOfPrintAttr[], Printer &p, char *buf)
{
    if (smManager.debug)
    {
        // printf("Entering scanRelations, indexRelOfPrintAttr[0] = %d\n", indexRelOfPrintAttr[0]);
        printf("scanRelations(%d)\n", id);
    }

    // Open the relation RM file
    RM_FileHandle rmFH;
    QL_Try(rmManager.OpenFile(relations[id], rmFH), QL_RELS_SCAN_FAIL);

    // Start the relcat file scan
    RM_FileScan rmFS;
    QL_Try(rmFS.OpenScan(rmFH, INT, 4, 0, NO_OP, NULL), QL_RELS_SCAN_FAIL);

    RM_Record rec;
    char *record;
    for (int rc = OK_RC; rc != RM_EOF;)
    {
        rc = rmFS.GetNextRec(rec);

        if (rc != 0 && rc != RM_EOF)
        {
            QL_PrintRC(rc);
            throw QL_RELS_SCAN_FAIL;
        }

        if (rc != RM_EOF)
        {
            QL_Try(rec.GetData(record), QL_RELS_SCAN_FAIL);
            records[id] = record;

            if (id + 1 == nRelations)
            {
                // Check condition
                for (int i = 0; i < nConditions; ++i)
                {
                    if (conditions[i].bRhsIsAttr)
                    {
                        if (!compare(conditions[i].rhsValue.type, conditions[i].op, records[indexRelOfCondLHS[i]] + offsetOfCondLHS[i], records[indexRelOfCondRHS[i]] + offsetOfCondRHS[i]))
                        {
                            return;
                        }
                    }
                    else
                    {
                        if (!compare(conditions[i].rhsValue.type, conditions[i].op, records[indexRelOfCondLHS[i]] + offsetOfCondLHS[i], records[indexRelOfCondRHS[i]] + offsetOfCondRHS[i]))
                        {
                            return;
                        }
                    }
                }
                // Print
                for (int i = 0; i < nSelAttrs; ++i)
                {
                    if (smManager.debug)
                    {
                        printf("Select an attr (indexRel = %d, offset = %d): ", indexRelOfPrintAttr[i], offsetOfPrintAttr[i]);
                        switch (printAttrs[i].attrType)
                        {
                        case INT:
                            printf("(INT)%d,", *(int *)(records[indexRelOfPrintAttr[i]] + offsetOfPrintAttr[i]));
                            break;
                        case FLOAT:
                            printf("(FLOAT)%f,", *(float *)(records[indexRelOfPrintAttr[i]] + offsetOfPrintAttr[i]));
                            break;
                        case STRING:
                            printf("(STRING)%s,", records[indexRelOfPrintAttr[i]] + offsetOfPrintAttr[i]);
                            break;
                        }
                        puts("");
                    }

                    memcpy(buf + printAttrs[i].offset, records[indexRelOfPrintAttr[i]] + offsetOfPrintAttr[i], printAttrs[i].attrLength);
                }
                p.Print(cout, buf);
            }
            else
            {
                scanRelations(id + 1, nRelations, relations, records, nConditions, conditions, indexRelOfCondLHS, offsetOfCondLHS, indexRelOfCondRHS, offsetOfCondRHS, nSelAttrs, printAttrs, indexRelOfPrintAttr, offsetOfPrintAttr, p, buf);
            }
        }
    }
}

int QL_Manager::indexOfRel(const char *relName, int nRelations, const char *const relations[])
{
    int i = 0;
    for (; i < nRelations; ++i)
        if (strcmp(relName, relations[i]))
            return i;
    return i;
}

/************ INSERT ************/

// Method: Insert(const char *relName, int nValues, const Value values[])
// Insert the values into relName
/* Steps:
    1) Check the parameters
    2) Check whether the database is open
    3) Obtain attribute information for the relation and check
    4) Open the RM file and each index file
    5) Insert the tuple in the relation
    6) Insert the entry in the indexes
    7) Print the inserted tuple
    8) Close the files
*/
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
    try
    {
        // Check the parameters
        if (relName == nullptr)
        {
            return QL_NULLPTR_RELATION;
        }

        // Check whether database is open
        if (!smManager.open)
        {
            return QL_DATABASE_CLOSED;
        }

        if (strcmp(relName, "relcat") == 0 || strcmp(relName, "attrcat") == 0)
        {
            return QL_SYS_CAT;
        }

        // Get the relation and attributes information
        SM_RelcatRecord rcRecord = smManager.GetRelInfo(relName);
        int attrCount = rcRecord.attrCount;

        if (nValues != attrCount)
        {
            return QL_INCORRECT_ATTRCOUNT;
        }

        DataAttrInfo attributes[nValues];
        smManager.GetAttrInfo(relName, nValues, (char *)attributes);

        // Check the values passed
        for (int i = 0; i < nValues; ++i)
        {
            if (values[i].type != attributes[i].attrType)
            {
                return QL_INCORRECT_ATTRTYPE;
            }
            if (values[i].type == STRING && strlen((char *)(values[i].data)) > attributes[i].attrLength)
            {
                return QL_INSERT_TOO_LONG_STRING;
            }
        }

        // Open the RM file
        RM_FileHandle rmFH;
        RID rid;
        QL_Try(rmManager.OpenFile(relName, rmFH), QL_INSERT_FAIL);

        // Insert the tuple into the relation
        char tupleData[rcRecord.tupleLength];
        memset(tupleData, 0, sizeof(tupleData));
        for (int i = 0; i < nValues; ++i)
            memcpy(tupleData + attributes[i].offset, values[i].data, attributes[i].attrLength);
        QL_Try(rmFH.InsertRec(tupleData, rid), QL_INSERT_FAIL);

        // Close the RM file
        QL_Try(rmManager.CloseFile(rmFH), QL_INSERT_FAIL);

        // Insert the entries into the indexes
        for (int i = 0; i < nValues; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                IX_IndexHandle ixIH;
                QL_Try(ixManager.OpenIndex(relName, attributes[i].indexNo, ixIH), QL_INSERT_FAIL);
                QL_Try(ixIH.InsertEntry(values[i].data, rid), QL_INSERT_FAIL);
                QL_Try(ixManager.CloseIndex(ixIH), QL_INSERT_FAIL);
            }
        }

        // Print the inserted tuple
        cout << "Inserted tuple:" << endl;
        Printer p(attributes, attrCount);
        p.PrintHeader(cout);
        p.Print(cout, tupleData);
        p.PrintFooter(cout);
    }
    catch (RC rc)
    {
        if (QL_PrintRC(rc))
        {
            return QL_INSERT_FAIL;
        }
        else
        {
            return rc;
        }
    }
    return OK_RC;
}

/************ DELETE ************/

// Method: Delete (const char *relName, int nConditions, const Condition conditions[])
// Delete from the relName all tuples that satisfy conditions
/* Steps:
    1) Check the parameters
    2) Check whether the database is open
    3) Obtain attribute information for the relation and check
    4) Check the conditions
    5) Find index on some condition
    6) If index exists
        - Open index scan
        - Find tuples and delete
        - Close index scan
    7) If no index
        - Open file scan
        - Find tuples and delete
        - Close file scan
    8) Print the deleted tuples
*/
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
    try
    {
        // Check whether the database is open
        if (!smManager.open)
        {
            return QL_DATABASE_CLOSED;
        }

        // Check the parameters
        if (relName == nullptr)
        {
            return QL_NULLPTR_RELATION;
        }
        if (strcmp(relName, "relcat") == 0 || strcmp(relName, "attrcat") == 0)
        {
            return QL_SYS_CAT;
        }

        // Get the relation and attributes information
        SM_RelcatRecord rcRecord = smManager.GetRelInfo(relName);
        int attrCount = rcRecord.attrCount;
        DataAttrInfo attributes[attrCount];
        smManager.GetAttrInfo(relName, attrCount, (char *)attributes);

        // Validate the conditions
        Condition changedConditions[nConditions];
        int offsetsLHS[nConditions];
        int offsetsRHS[nConditions];
        for (int i = 0; i < nConditions; ++i)
        {
            changedConditions[i] = conditions[i];

            // Check whether LHS is a valid attribute
            DataAttrInfo attrInfo = checkAttr(changedConditions[i].lhsAttr, relName, rcRecord.attrCount, attributes);
            AttrType lhsType = attrInfo.attrType;
            offsetsLHS[i] = attrInfo.offset;

            // If RHS is a attribute, check it
            AttrType rhsType;
            if (changedConditions[i].bRhsIsAttr)
            {
                attrInfo = checkAttr(changedConditions[i].rhsAttr, relName, rcRecord.attrCount, attributes);
                offsetsRHS[i] = attrInfo.offset;
                rhsType = attrInfo.attrType;
            }
            else
            {
                rhsType = changedConditions[i].rhsValue.type;
            }

            // Check if the types of LHS and RHS are compatible (same)
            if (lhsType != rhsType)
            {
                throw QL_TYPES_INCOMPATIBLE;
            }
            changedConditions[i].rhsValue.type = lhsType;
        }

        // Prepare the printer class
        cout << "Deleted tuples:" << endl;
        Printer p(attributes, attrCount);
        p.PrintHeader(cout);

        // Open the RM file
        RM_FileHandle rmFH;
        QL_Try(rmManager.OpenFile(relName, rmFH), QL_DELETE_FAIL);

        // Open a file scan
        RM_FileScan rmFS;
        QL_Try(rmFS.OpenScan(rmFH, INT, 4, 0, NO_OP, nullptr), QL_DELETE_FAIL);

        // Open all the indexes
        IX_IndexHandle ixIHs[attrCount];
        for (int i = 0; i < attrCount; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                QL_Try(ixManager.OpenIndex(relName, attributes[i].indexNo, ixIHs[i]), QL_DELETE_FAIL);
            }
        }

        // Get the next record to delete
        RM_Record rec;
        RID rid;
        char *recordData;
        for (RC rc = OK_RC; rc != RM_EOF;)
        {
            rc = rmFS.GetNextRec(rec);
            if (rc != OK_RC && rc != RM_EOF)
            {
                RM_PrintError(rc);

                throw QL_DELETE_FAIL;
            }
            if (rc != RM_EOF)
            {
                QL_Try(rec.GetData(recordData), QL_DELETE_FAIL);
                // Check if satisfies all conditions
                bool satisfied = true;
                for (int i = 0; i < nConditions; ++i)
                {
                    if (changedConditions[i].bRhsIsAttr)
                    {
                        if (!compare(changedConditions[i].rhsValue.type, changedConditions[i].op, recordData + offsetsLHS[i], recordData + offsetsRHS[i]))
                        {
                            satisfied = false;
                            break;
                        }
                    }
                    else
                    {
                        if (!compare(changedConditions[i].rhsValue.type, changedConditions[i].op, recordData + offsetsLHS[i], changedConditions[i].rhsValue.data))
                        {
                            satisfied = false;
                            break;
                        }
                    }
                }
                // If all conditions are satisfied
                if (satisfied)
                {
                    QL_Try(rec.GetRid(rid), QL_DELETE_FAIL);
                    // Delete the tuple
                    QL_Try(rmFH.DeleteRec(rid), QL_DELETE_FAIL);

                    // Delete entries from all indexes
                    for (int i = 0; i < attrCount; ++i)
                    {
                        if (attributes[i].indexNo != -1)
                        {
                            QL_Try(ixIHs[i].DeleteEntry(recordData + attributes[i].offset, rid), QL_DELETE_FAIL);
                        }
                    }

                    // Print the deleted tuple
                    p.Print(cout, recordData);
                }
            }
        }

        // Close all the indexes
        for (int i = 0; i < attrCount; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                QL_Try(ixManager.CloseIndex(ixIHs[i]), QL_DELETE_FAIL);
            }
        }

        // Close the file scan and RM files
        QL_Try(rmFS.CloseScan(), QL_DELETE_FAIL);
        QL_Try(rmManager.CloseFile(rmFH), QL_DELETE_FAIL);

        // Print the footer
        p.PrintFooter(cout);
    }
    catch (RC rc)
    {
        if (QL_PrintRC(rc))
        {
            return QL_DELETE_FAIL;
        }
        else
        {
            return rc;
        }
    }

    return OK_RC;
}

/************ UPDATE ************/

// Method: Update(const char *relName, const RelAttr &updAttr, const int bIsValue,
//                const RelAttr &rhsRelAttr, const Value &rhsValue, int nConditions,
//                const Condition conditions[])
// Update from the relName all tuples that satisfy conditions
/* Steps:
    1) Check the parameters
    2) Check whether the database is open
    3) Obtain attribute information for the relation and check
    4) Check the update attribute
    5) Check the conditions
    6) Find index on some condition
    7) If index exists
        - Open index scan
        - Find tuples and update
        - Update index entries
        - Close index scan
    8) If no index
        - Open file scan
        - Find tuples and update
        - Update index entries
        - Close file scan
    9) Print the updated tuples
*/
RC QL_Manager::Update(const char *relName,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions[])
{
    try
    {
        // Check whether the database is open
        if (!smManager.open)
        {
            return QL_DATABASE_CLOSED;
        }

        // Check the parameters
        if (relName == nullptr)
        {
            return QL_NULLPTR_RELATION;
        }
        if (strcmp(relName, "relcat") == 0 || strcmp(relName, "attrcat") == 0)
        {
            return QL_SYS_CAT;
        }

        // Get the relation and attributes information
        SM_RelcatRecord rcRecord = smManager.GetRelInfo(relName);
        int attrCount = rcRecord.attrCount;
        DataAttrInfo attributes[attrCount];
        smManager.GetAttrInfo(relName, attrCount, (char *)attributes);

        // Check the update attribute
        Condition changedupdCond = Condition{
            updAttr,
            NO_OP,
            bIsValue,
            rhsRelAttr,
            rhsValue};
        int offsetLHS;
        int offsetRHS;
        int lengthLHS;
        int lengthRHS;
        {
            DataAttrInfo attrInfo = checkAttr(changedupdCond.lhsAttr, relName, rcRecord.attrCount, attributes);
            AttrType lhsType = attrInfo.attrType;
            offsetLHS = attrInfo.offset;
            lengthLHS = attrInfo.attrLength;

            // If RHS is a attribute, check it
            AttrType rhsType;
            if (!bIsValue)
            {
                attrInfo = checkAttr(changedupdCond.rhsAttr, relName, rcRecord.attrCount, attributes);
                offsetRHS = attrInfo.offset;
                rhsType = attrInfo.attrType;
                lengthRHS = attrInfo.attrLength;
            }
            else
            {
                rhsType = changedupdCond.rhsValue.type;
                lengthRHS = rhsType == STRING ? strlen((char *)changedupdCond.rhsValue.data) : 4;
            }

            // Check if the types of LHS and RHS are compatible (same)
            if (lhsType != rhsType)
            {
                throw QL_TYPES_INCOMPATIBLE;
            }
            changedupdCond.rhsValue.type = lhsType;

            if (lhsType == STRING && lengthLHS < lengthRHS)
            {
                throw QL_UPDATE_TOO_LONG_STRING;
            }
        }

        // Validate the conditions
        Condition changedConditions[nConditions];
        int offsetsLHS[nConditions];
        int offsetsRHS[nConditions];
        for (int i = 0; i < nConditions; ++i)
        {
            changedConditions[i] = conditions[i];

            DataAttrInfo attrInfo = checkAttr(changedConditions[i].lhsAttr, relName, rcRecord.attrCount, attributes);
            AttrType lhsType = attrInfo.attrType;
            offsetsLHS[i] = attrInfo.offset;

            // If RHS is a attribute, check it
            AttrType rhsType;
            if (changedConditions[i].bRhsIsAttr)
            {
                attrInfo = checkAttr(changedConditions[i].rhsAttr, relName, rcRecord.attrCount, attributes);
                offsetsRHS[i] = attrInfo.offset;
                rhsType = attrInfo.attrType;
            }
            else
            {
                rhsType = changedConditions[i].rhsValue.type;
            }

            // Check if the types of LHS and RHS are compatible (same)
            if (lhsType != rhsType)
            {
                throw QL_TYPES_INCOMPATIBLE;
            }
            changedConditions[i].rhsValue.type = lhsType;
        }

        // Prepare the printer class
        cout << "Updated tuples:" << endl;
        Printer p(attributes, attrCount);
        p.PrintHeader(cout);

        // Open the RM file
        RM_FileHandle rmFH;
        QL_Try(rmManager.OpenFile(relName, rmFH), QL_UPDATE_FAIL);

        // Open a file scan
        RM_FileScan rmFS;
        QL_Try(rmFS.OpenScan(rmFH, INT, 4, 0, NO_OP, nullptr), QL_UPDATE_FAIL);

        // Open all the indexes
        IX_IndexHandle ixIHs[attrCount];
        for (int i = 0; i < attrCount; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                QL_Try(ixManager.OpenIndex(relName, attributes[i].indexNo, ixIHs[i]), QL_UPDATE_FAIL);
            }
        }

        // Get the next record to update
        RM_Record rec;
        RID rid;
        char *recordData;
        for (RC rc = OK_RC; rc != RM_EOF;)
        {
            rc = rmFS.GetNextRec(rec);
            if (rc != OK_RC && rc != RM_EOF)
            {
                RM_PrintError(rc);

                throw QL_UPDATE_FAIL;
            }
            if (rc != RM_EOF)
            {
                QL_Try(rec.GetData(recordData), QL_UPDATE_FAIL);
                // Check if satisfies all conditions
                bool satisfied = true;
                for (int i = 0; i < nConditions; ++i)
                {
                    if (conditions[i].bRhsIsAttr)
                    {
                        if (!compare(conditions[i].rhsValue.type, conditions[i].op, recordData + offsetsLHS[i], recordData + offsetsRHS[i]))
                        {
                            satisfied = false;
                            break;
                        }
                    }
                    else
                    {
                        if (!compare(conditions[i].rhsValue.type, conditions[i].op, recordData + offsetsLHS[i], conditions[i].rhsValue.data))
                        {
                            satisfied = false;
                            break;
                        }
                    }
                }
                // If all conditions are satisfied
                if (satisfied)
                {
                    QL_Try(rec.GetRid(rid), QL_UPDATE_FAIL);

                    // Delete the entry of index
                    for (int i = 0; i < attrCount; ++i)
                    {
                        if (attributes[i].indexNo != -1)
                        {
                            QL_Try(ixIHs[i].DeleteEntry(recordData + attributes[i].offset, rid), QL_UPDATE_FAIL);
                        }
                    }

                    // Update the record
                    memset(recordData + offsetLHS, 0, lengthLHS);
                    if (bIsValue)
                    {
                        memcpy(recordData + offsetLHS, recordData + offsetRHS, lengthRHS);
                    }
                    else
                    {
                        memcpy(recordData + offsetLHS, rhsValue.data, lengthRHS);
                    }

                    // Update the tuple
                    QL_Try(rmFH.UpdateRec(rec), QL_UPDATE_FAIL);

                    // Insert the entry of index
                    for (int i = 0; i < attrCount; ++i)
                    {
                        if (attributes[i].indexNo != -1)
                        {
                            QL_Try(ixIHs[i].InsertEntry(recordData + attributes[i].offset, rid), QL_UPDATE_FAIL);
                        }
                    }

                    // Print the updated tuple
                    p.Print(cout, recordData);
                }
            }
        }

        // Close all the indexes
        for (int i = 0; i < attrCount; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                QL_Try(ixManager.CloseIndex(ixIHs[i]), QL_UPDATE_FAIL);
            }
        }

        // Close the file scan and RM files
        QL_Try(rmFS.CloseScan(), QL_UPDATE_FAIL);
        QL_Try(rmManager.CloseFile(rmFH), QL_UPDATE_FAIL);

        // Print the footer
        p.PrintFooter(cout);
    }
    catch (RC rc)
    {
        if (QL_PrintRC(rc))
        {
            return QL_UPDATE_FAIL;
        }
        else
        {
            return rc;
        }
    }

    return OK_RC;
}

DataAttrInfo QL_Manager::checkAttr(RelAttr &attr, const char *relName, int attrCount, DataAttrInfo attributes[])
{
    if (attr.relName != nullptr && strcmp(attr.relName, relName) != 0)
    {
        throw QL_ATTR_INCORRECT_REL;
    }
    else
    {
        strcpy(attr.relName, relName);
        for (int i = attrCount; i--;)
            if (strcmp(attributes[i].attrName, attr.attrName) == 0)
            {
                return attributes[i];
            }
        throw QL_ATTR_INCORRECT_ATTR;
    }
}