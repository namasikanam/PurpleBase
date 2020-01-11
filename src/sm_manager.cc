//
// File:        sm_manager.cc
// Description: SM_Manager class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <bits/stdc++.h>
#include <unistd.h>
#include "redbase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include "printer.h"
#include "parser.h"
#include "sm_internal.h"
using namespace std;

SM_RelcatRecord::SM_RelcatRecord(char *_relName, int _tupleLength, int _attrCount, int _indexCount) : tupleLength(_tupleLength), attrCount(_attrCount), indexCount(_indexCount)
{
    if (_relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (strlen(_relName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_RELNAME};
    }
    memset(relName, 0, sizeof(relName));
    strcpy(relName, _relName);
}

SM_RelcatRecord::SM_RelcatRecord(const char *_relName, int _tupleLength, int _attrCount, int _indexCount) : tupleLength(_tupleLength), attrCount(_attrCount), indexCount(_indexCount)
{
    if (_relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (strlen(_relName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_RELNAME};
    }
    memset(relName, 0, sizeof(relName));
    strcpy(relName, _relName);
}

SM_AttrcatRecord::SM_AttrcatRecord(char *_relName, char *_attrName, int _offset, AttrType _attrType, int _attrLength, int _indexNo) : offset(_offset), attrType(_attrType), attrLength(_attrLength), indexNo(_indexNo)
{
    if (_relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (_attrName == nullptr)
    {
        throw RC{SM_NULLPTR_ATTR_NAME};
    }
    if (strlen(_relName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_RELNAME};
    }
    memset(relName, 0, sizeof(relName));
    strcpy(relName, _relName);

    if (strlen(_attrName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_ATTRNAME};
    }
    memset(attrName, 0, sizeof(attrName));
    strcpy(attrName, _attrName);

    // printf("When constructing, attrName = %s\n", attrName);
}

SM_AttrcatRecord::SM_AttrcatRecord(const char *_relName, char *_attrName, int _offset, AttrType _attrType, int _attrLength, int _indexNo) : offset(_offset), attrType(_attrType), attrLength(_attrLength), indexNo(_indexNo)
{
    if (_relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (_attrName == nullptr)
    {
        throw RC{SM_NULLPTR_ATTR_NAME};
    }
    if (strlen(_relName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_RELNAME};
    }
    memset(relName, 0, sizeof(relName));
    strcpy(relName, _relName);

    if (strlen(_attrName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_ATTRNAME};
    }
    memset(attrName, 0, sizeof(attrName));
    strcpy(attrName, _attrName);

    // printf("When constructing, attrName = %s\n", attrName);
}

SM_AttrcatRecord::SM_AttrcatRecord(const char *_relName, const char *_attrName, int _offset, AttrType _attrType, int _attrLength, int _indexNo) : offset(_offset), attrType(_attrType), attrLength(_attrLength), indexNo(_indexNo)
{
    if (_relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (_attrName == nullptr)
    {
        throw RC{SM_NULLPTR_ATTR_NAME};
    }
    if (strlen(_relName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_RELNAME};
    }
    memset(relName, 0, sizeof(relName));
    strcpy(relName, _relName);

    if (strlen(_attrName) > MAXNAME)
    {
        throw RC{SM_TOO_LONG_ATTRNAME};
    }
    memset(attrName, 0, sizeof(attrName));
    strcpy(attrName, _attrName);

    // printf("When constructing, attrName = %s\n", attrName);
}

// Constructor
SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) : iXManager(ixm), rMManager(rmm), open(false)
{
}

// Destructor
SM_Manager::~SM_Manager()
{
    // Nothing to free
}

// Method: OpenDb(const char *dbName)
// Open the database
/* Steps:
    1) Check if the database is already open
    2) Change to the database directory
    3) Open the system catalogs
    4) Update flag
*/
RC SM_Manager::OpenDb(const char *dbName)
{
    if (dbName == nullptr)
    {
        return SM_NULLPTR_DB_NAME;
    }
    // Check if open
    if (open)
    {
        return SM_OPEN_OPEND;
    }

    // Change to the database directory
    if (chdir(dbName) == -1)
    {
        return SM_DATABASE_DOES_NOT_EXIST;
    }

    try
    {
        // Open the system catalogs
        SM_Try_RM(rMManager.OpenFile("relcat", relcatRMFH), SM_OPEN_RELCAT_FAIL);
        SM_Try_RM(rMManager.OpenFile("attrcat", attrcatRMFH), SM_OPEN_ATTRCAT_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }

    // Update flag
    open = true;

    // Return OK
    return OK_RC;
}

// Method: CloseDb()
// Close the database
/* Steps:
    1) Check that the database is not closed
    2) Close the system catalogs
    3) Update flag
*/
RC SM_Manager::CloseDb()
{
    // Check if closed
    if (!open)
    {
        return SM_CLOSE_CLOSED;
    }

    try
    {
        // Close the system catalogs
        SM_Try_RM(rMManager.CloseFile(relcatRMFH), SM_CLOSE_RELCAT_FAIL);
        SM_Try_RM(rMManager.CloseFile(attrcatRMFH), SM_CLOSE_ATTRCAT_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }

    // Change to the up directory
    if (chdir("../") == -1)
    {
        return SM_INVALID_DATABASE_CLOSE;
    }

    // Update flag
    open = FALSE;

    // Return OK
    return OK_RC;
}

// Method: CreateTable(const char *relName, int attrCount, AttrInfo *attributes)
// Create relation relName, given number of attributes and attribute data
/* Steps:
    1) Check that the database is open
    2) Check whether the table already exists
    3) Update the system catalogs
    4) Create a RM file for the relation
    5) Flush the system catalogs
*/
RC SM_Manager::CreateTable(const char *relName, int attrCount, AttrInfo *attributes)
{
    try
    {
        if (relName == nullptr)
        {
            throw RC{SM_NULLPTR_REL_NAME};
        }
        // 1) Check that the database is open
        if (!open)
        {
            throw RC{SM_CREATE_TABLE_CLOSED};
        }

        { // 2) Check whether the table already exists
            RM_FileScan rmfs;
            SM_Try_RM(rmfs.OpenScan(relcatRMFH, STRING, MAXNAME, offsetof(SM_RelcatRecord, relName), EQ_OP, relName), SM_CREATE_TABLE_SCAN_FAIL);
            {
                RM_Record rec;
                SM_Try_RM_Or_Close_Scan(rmfs.GetNextRec(rec), rmfs, SM_CREATE_TABLE_SCAN_FAIL, SM_CREATE_TABLE_SCAN_FAIL_CLOSE_SCAN_FAIL, RM_EOF);
            }
            SM_Try_RM(rmfs.CloseScan(), SM_CREATE_TABLE_SCAN_FAIL);
        }

        // 3) Update the system catalogs
        int recordSize = 0;
        {
            RID rid;
            int offset = 0;

            // 3.1) A tuple for each attribute should be added to attrcat
            for (int i = 0; i < attrCount; offset += attributes[i++].attrLength)
            {
                SM_AttrcatRecord acRecord = SM_AttrcatRecord(
                    relName,
                    attributes[i].attrName,
                    offset,
                    attributes[i].attrType,
                    attributes[i].attrLength,
                    -1);
                SM_Try_RM(attrcatRMFH.InsertRec((char *)&acRecord, rid), SM_CREATE_TABLE_INSERT_ATTR_CAT_FAIL);
            }

            // 3.2) The new relation should be added to relcat
            recordSize = offset;
            SM_RelcatRecord rcRecord = SM_RelcatRecord(relName, recordSize, attrCount, 0);
            SM_Try_RM(relcatRMFH.InsertRec((char *)&rcRecord, rid), SM_CREATE_TABLE_INSERT_ATTR_CAT_FAIL);
        }

        // 4) Create a RM file for the relation
        SM_Try_RM(rMManager.CreateFile(relName, recordSize), SM_CREATE_TABLE_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: DropTable(const char *relName)
// Destroy a relation
/* Steps:
    1) Check that the database is open
    2) Delete the entry from relcat
    3) Scan through attrcat
        - Destroy the indexes and delete the entries
    4) Destroy the RM file for the relation
*/
RC SM_Manager::DropTable(const char *relName)
{
    try
    {
        if (relName == nullptr)
        {
            throw RC{SM_NULLPTR_REL_NAME};
        }
        // 1) Check that the database is open
        if (!open)
        {
            throw RC{SM_DROP_TABLE_CLOSED};
        }
        // 2) Delete the entry from relcat
        RM_FileScan relcatFS;
        {
            SM_Try_RM(relcatFS.OpenScan(relcatRMFH, STRING, MAXNAME, offsetof(SM_RelcatRecord, relName), EQ_OP, relName), SM_DROP_TABLE_REL_CAT_SCAN_FAIL);

            // Get the record
            RM_Record rec;
            SM_Try_RM_Or_Close_Scan(relcatFS.GetNextRec(rec), relcatFS, SM_DROP_TABLE_REL_CAT_SCAN_FAIL, SM_DROP_TABLE_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);

            // Get the RID of the record
            RID rid;
            SM_Try_RM(rec.GetRid(rid), SM_DROP_TABLE_REL_CAT_SCAN_FAIL);

            // Delete the record
            SM_Try_RM(relcatRMFH.DeleteRec(rid), SM_DROP_TABLE_REL_CAT_SCAN_FAIL);

            SM_Try_RM(relcatFS.CloseScan(), SM_DROP_TABLE_REL_CAT_SCAN_FAIL);
        }
        // 3) Scan through attrcat to destroy the indexes and delete the entries
        RM_FileScan attrcatFS;
        SM_Try_RM(attrcatFS.OpenScan(attrcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL);
        {
            RM_Record rec;
            RID rid;
            char *recordData;
            for (RC rc; (rc = attrcatFS.GetNextRec(rec)) != RM_EOF;)
            {
                if (rc)
                {
                    RM_PrintError(rc);
                    SM_Try_RM(attrcatFS.CloseScan(), SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                    throw RC{SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL};
                }

                // Get the RID of the record
                SM_Try_RM_Or_Close_Scan(rec.GetRid(rid), attrcatFS, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);

                // Check if index exists
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), attrcatFS, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                SM_AttrcatRecord *acRecord = (SM_AttrcatRecord *)recordData;
                if (acRecord->indexNo != -1)
                {
                    // Destroy the index
                    SM_Try_RM_Or_Close_Scan(iXManager.DestroyIndex(relName, acRecord->indexNo), attrcatFS, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                }

                // Delete the record
                SM_Try_RM_Or_Close_Scan(attrcatRMFH.DeleteRec(rid), attrcatFS, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL, SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
            }
        }

        // 4) Destroy the RM file for the relation
        SM_Try_RM(rMManager.DestroyFile(relName), SM_DROP_TABLE_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: CreateIndex(const char *relName, const char *attrName)
// Create an index for relName.attrName
/* Steps:
    1) Check that the database is open
    2) Check whether the index exists
    3) Create and open the index file
    4) Scan all the tuples and insert in the index
    5) Close the index file
*/
RC SM_Manager::CreateIndex(const char *relName, const char *attrName)
{
    try
    {
        if (relName == nullptr)
        {
            throw RC{SM_NULLPTR_REL_NAME};
        }
        if (attrName == nullptr)
        {
            throw RC{SM_NULLPTR_ATTR_NAME};
        }
        // Check that the database is open
        if (!open)
        {
            throw RC{SM_CREATE_INDEX_CLOSED};
        }

        // Check whether the index exists
        SM_AttrcatRecord attrRecord = GetAttrInfo(relName, attrName);
        if (attrRecord.indexNo != -1)
        {
            throw RC{SM_CREATE_INDEX_EXISTS};
        }

        int offset = attrRecord.offset;
        AttrType attrType = attrRecord.attrType;
        int attrLength = attrRecord.attrLength;

        // Update relcat
        RM_FileScan relcatFS;
        RM_Record rec;
        SM_Try_RM(relcatFS.OpenScan(relcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_CREATE_INDEX_REL_CAT_SCAN_FAIL);
        SM_Try_RM_Or_Close_Scan(relcatFS.GetNextRec(rec), relcatFS, SM_CREATE_INDEX_REL_CAT_SCAN_FAIL, SM_CREATE_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
        char *recordData;
        SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), relcatFS, SM_CREATE_INDEX_REL_CAT_SCAN_FAIL, SM_CREATE_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
        ++(((SM_RelcatRecord *)recordData)->indexCount);
        SM_Try_RM_Or_Close_Scan(relcatRMFH.UpdateRec(rec), relcatFS, SM_CREATE_INDEX_REL_CAT_SCAN_FAIL, SM_CREATE_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
        SM_Try_RM(relcatFS.CloseScan(), SM_CREATE_INDEX_REL_CAT_SCAN_FAIL);

        // Update attrcat
        RM_FileScan attrcatFS;
        SM_Try_RM(attrcatFS.OpenScan(attrcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL);
        int position = 0;
        for (RC rc = OK_RC; rc != RM_EOF; ++position)
        {
            rc = attrcatFS.GetNextRec(rec);
            if (rc != 0 && rc != RM_EOF)
            {
                RM_PrintError(rc);
                SM_Try_RM(attrcatFS.CloseScan(), SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                throw RC{SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL};
            }

            if (rc != RM_EOF)
            {
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), attrcatFS, SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL, SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                SM_AttrcatRecord &acRecord = *(SM_AttrcatRecord *)recordData;

                // Check for the required attribute
                if (strcmp(acRecord.attrName, attrName) == 0)
                {
                    acRecord.indexNo = position;
                    SM_Try_RM_Or_Close_Scan(attrcatRMFH.UpdateRec(rec), attrcatFS, SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL, SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                    break;
                }
            }
        }
        SM_Try_RM(attrcatFS.CloseScan(), SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL);

        // Create and open the index file
        SM_Try_IX(iXManager.CreateIndex(relName, position, attrType, attrLength), SM_CREATE_INDEX_FAIL);
        IX_IndexHandle ixIH;
        SM_Try_IX(iXManager.OpenIndex(relName, position, ixIH), SM_CREATE_INDEX_FAIL);

        // Scan all the tuples in the relation
        RM_FileHandle rmFH;
        RM_FileScan rmFS;
        RID rid;
        SM_Try_RM(rMManager.OpenFile(relName, rmFH), SM_CREATE_INDEX_FAIL);
        SM_Try_RM(rmFS.OpenScan(rmFH, INT, 4, 0, NO_OP, NULL), SM_CREATE_INDEX_RM_SCAN_FAIL);
        for (RC rc = OK_RC; rc != RM_EOF;)
        {
            rc = rmFS.GetNextRec(rec);
            if (rc != 0 && rc != RM_EOF)
            {
                RM_PrintError(rc);
                SM_Try_RM(rmFS.CloseScan(), SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL);
                throw RC{SM_CREATE_INDEX_RM_SCAN_FAIL};
            }

            // Get the record data and rid
            if (rc != RM_EOF)
            {
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), rmFS, SM_CREATE_INDEX_RM_SCAN_FAIL, SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL);
                SM_Try_RM_Or_Close_Scan(rec.GetRid(rid), rmFS, SM_CREATE_INDEX_RM_SCAN_FAIL, SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL);

                // Insert the attribute value in the index
                SM_Try_IX_Or_Close_Scan(ixIH.InsertEntry(recordData + offset, rid), rmFS, SM_CREATE_INDEX_RM_SCAN_FAIL, SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL);
            }
        }
        SM_Try_RM(rmFS.CloseScan(), SM_CREATE_INDEX_RM_SCAN_FAIL);

        // Close the files
        SM_Try_RM(rMManager.CloseFile(rmFH), SM_CREATE_INDEX_RM_SCAN_FAIL);
        SM_Try_IX(iXManager.CloseIndex(ixIH), SM_CREATE_INDEX_RM_SCAN_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: DropIndex(const char *relName, const char *attrName)
// Destroy index on relName.attrName
/* Steps:
    1) Check that the database is open
    2) Check whether the index exists
    3) Update and flush the system catalogs
    4) Destroy the index file
*/
RC SM_Manager::DropIndex(const char *relName, const char *attrName)
{
    try
    {
        if (relName == nullptr)
        {
            throw RC{SM_NULLPTR_REL_NAME};
        }
        if (attrName == nullptr)
        {
            throw RC{SM_NULLPTR_ATTR_NAME};
        }
        if (!open)
        {
            throw RC{SM_DROP_INDEX_CLOSED};
        }

        // Check whether the index exists
        SM_AttrcatRecord attrRecord = GetAttrInfo(relName, attrName);
        if (attrRecord.indexNo == -1)
        {
            throw RC{SM_INDEX_DOES_NOT_EXIST};
        }

        // Update relcat
        RM_FileScan relcatFS;
        RM_Record rec;
        SM_Try_RM(relcatFS.OpenScan(relcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_DROP_INDEX_REL_CAT_SCAN_FAIL);
        SM_Try_RM_Or_Close_Scan(relcatFS.GetNextRec(rec), relcatFS, SM_DROP_INDEX_REL_CAT_SCAN_FAIL, SM_DROP_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
        char *recordData;
        SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), relcatFS, SM_DROP_INDEX_REL_CAT_SCAN_FAIL, SM_DROP_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
        --((SM_RelcatRecord *)recordData)->indexCount;
        SM_Try_RM_Or_Close_Scan(relcatRMFH.UpdateRec(rec), relcatFS, SM_DROP_INDEX_REL_CAT_SCAN_FAIL, SM_DROP_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
        SM_Try_RM(relcatFS.CloseScan(), SM_DROP_INDEX_REL_CAT_SCAN_FAIL);

        // Update attrcat
        RM_FileScan attrcatFS;
        int position = -1;
        SM_Try_RM(attrcatFS.OpenScan(attrcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL);
        RC rc;
        while (rc != RM_EOF)
        {
            rc = attrcatFS.GetNextRec(rec);
            if (rc != OK_RC && rc != RM_EOF)
            {
                RM_PrintError(rc);
                SM_Try_RM(attrcatFS.CloseScan(), SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                throw RC{SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL};
            }

            if (rc != RM_EOF)
            {
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), attrcatFS, SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL, SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);

                SM_AttrcatRecord &acRecord = *(SM_AttrcatRecord *)recordData;

                // Check for the required attribute
                if (strcmp(acRecord.attrName, attrName) == 0)
                {
                    position = acRecord.indexNo;
                    acRecord.indexNo = -1;
                    SM_Try_RM_Or_Close_Scan(attrcatRMFH.UpdateRec(rec), attrcatFS, SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL, SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                    break;
                }
            }
        }
        SM_Try_RM(attrcatFS.CloseScan(), SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL);

        // Destroy the index file
        SM_Try_IX(iXManager.DestroyIndex(relName, position), SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: Load(const char *relName, const char *fileName)
// Load relName from fileName
/* Steps:
    1) Check the parameters
    2) Check whether the database is open
    2) Obtain attribute information for the relation
    3) Open the RM file and each index file
    4) Open the data file
    5) Read the tuples from the file
        - Insert the tuple in the relation
        - Insert the entries in the indexes
    6) Close the files
*/
RC SM_Manager::Load(const char *relName, const char *fileName)
{
    try
    {
        // Check the parameters
        if (relName == nullptr)
        {
            throw RC{SM_NULLPTR_REL_NAME};
        }
        if (fileName == nullptr)
        {
            throw RC{SM_NULLPTR_FILE_NAME};
        }
        if (strcmp(relName, "relcat") == 0 || strcmp(relName, "attrcat") == 0)
        {
            return SM_LOAD_SYSTEM_CAT;
        }

        // Check that the database is open
        if (!open)
        {
            throw RC{SM_LOAD_CLOSED};
        }

        // Get the relation and attributes information
        SM_RelcatRecord rcRecord = GetRelInfo(relName);
        int tupleLength = rcRecord.tupleLength;
        int attrCount = rcRecord.attrCount;

        DataAttrInfo attributes[attrCount];
        GetAttrInfo(relName, attrCount, (void *)attributes);

        // Open the data file
        ifstream dataFile(fileName);
        if (!dataFile.is_open())
        {
            throw RC{SM_LOAD_INVALID_DATA_FILE};
        }

        // Open the RM file
        RM_FileHandle rmFH;
        RID rid;
        SM_Try_RM(rMManager.OpenFile(relName, rmFH), SM_LOAD_FAIL);

        // Open the indexes
        IX_IndexHandle ixIH[attrCount];
        for (int i = 0; i < attrCount; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                SM_Try_IX(iXManager.OpenIndex(relName, attributes[i].indexNo, ixIH[i]), SM_LOAD_FAIL);
            }
        }

        // Read each line of the file
        string line;
        while (getline(dataFile, line))
        {
            RC rc;

            // for uniformity
            line += ",";

            char tupleData[tupleLength];
            memset(tupleData, 0, sizeof(tupleData));

            for (int i = 0, pos = 0; i < attrCount; ++i)
            {
                // Parse the line
                int next_pos = line.find(',', pos + 1);
                string dataValue = line.substr(pos + 1, next_pos - pos - 1);
                pos = next_pos;

                // Build the tuple of the relation
                switch (attributes[i].attrType)
                {
                case INT:
                    *(int *)(tupleData + attributes[i].offset) = stoi(dataValue);
                    break;
                case FLOAT:
                    *(float *)(tupleData + attributes[i].offset) = stof(dataValue);
                    break;
                case STRING:
                    strcpy(tupleData + attributes[i].offset, dataValue.c_str());
                    break;
                }

                // Insert the entries into the indexes
                if (attributes[i].indexNo != -1)
                {
                    void *value;
                    int valueINT;
                    float valueFLOAT;
                    switch (attributes[i].attrType)
                    {
                    case INT:
                        valueINT = stoi(dataValue);
                        value = &valueINT;
                        break;
                    case FLOAT:
                        valueFLOAT = stof(dataValue);
                        value = &valueFLOAT;
                        break;
                    case STRING:
                        value = malloc(attributes[i].attrLength + 1);
                        strcpy((char *)value, dataValue.c_str());
                        break;
                    }
                    if ((rc = ixIH[i].InsertEntry(value, rid)))
                    {
                        IX_PrintError(rc);

                        SM_Try_RM(rMManager.CloseFile(rmFH), SM_LOAD_FAIL_CLOSE_FAIL);
                        for (int i = 0; i < attrCount; ++i)
                        {
                            if (attributes[i].indexNo != -1)
                            {
                                SM_Try_IX(iXManager.CloseIndex(ixIH[i]), SM_LOAD_FAIL_CLOSE_FAIL);
                            }
                        }

                        throw RC{SM_LOAD_FAIL};
                    }
                }
            }

            // Insert the tuple into the relation
            if ((rc = rmFH.InsertRec(tupleData, rid)))
            {
                IX_PrintError(rc);

                SM_Try_RM(rMManager.CloseFile(rmFH), SM_LOAD_FAIL_CLOSE_FAIL);
                for (int i = 0; i < attrCount; ++i)
                {
                    if (attributes[i].indexNo != -1)
                    {
                        SM_Try_IX(iXManager.CloseIndex(ixIH[i]), SM_LOAD_FAIL_CLOSE_FAIL);
                    }
                }

                throw RC{SM_LOAD_FAIL};
            }
        }

        // Close the RM file
        SM_Try_RM(rMManager.CloseFile(rmFH), SM_LOAD_FAIL);

        // Close the indexes
        for (int i = 0; i < attrCount; ++i)
        {
            if (attributes[i].indexNo != -1)
            {
                SM_Try_IX(iXManager.CloseIndex(ixIH[i]), SM_LOAD_FAIL);
            }
        }
    }
    catch (RC rc)
    {
        return rc;
    }

    return OK_RC;
}

// Method: Help()
// Print relations in db
/* Steps:
    1) Create the attributes structure
    2) Create a Printer object
    3) Print the header
    4) Start relcat file scan and print each tuple
    5) Print the footer
    6) Close the scan and clean up
*/
RC SM_Manager::Help()
{
    try
    {
        // Check that the database is open
        if (!open)
        {
            throw RC{SM_HELP_CLOSED};
        }

        // Fill the attributes structure
        DataAttrInfo attributes[SM_RELCAT_ATTR_COUNT];
        GetAttrInfo("relcat", SM_RELCAT_ATTR_COUNT, (void *)attributes);

        if (debug)
        {
            for (int i = 0; i < SM_RELCAT_ATTR_COUNT; ++i)
            {
                cout << "attr(" << i << ") = (relName = " << attributes[i].relName << "), (attrName = " << attributes[i].attrName << ")\n";
            }
        }

        // Create a Printer object
        Printer p(attributes, SM_RELCAT_ATTR_COUNT);

        // Print the header
        p.PrintHeader(cout);

        // Start the relcat file scan
        RM_FileScan relcatFS;
        SM_Try_RM(relcatFS.OpenScan(relcatRMFH, STRING, MAXNAME, 0, NO_OP, nullptr), SM_HELP_REL_CAT_SCAN_FAIL);

        RM_Record rec;
        char *recordData;
        for (RC rc = OK_RC; rc != RM_EOF;)
        {
            rc = relcatFS.GetNextRec(rec);
            if (rc != 0 && rc != RM_EOF)
            {
                RM_PrintError(rc);
                SM_Try_RM(relcatFS.CloseScan(), SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                throw RC{SM_HELP_REL_CAT_SCAN_FAIL};
            }
            if (rc != RM_EOF)
            {
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), relcatFS, SM_HELP_REL_CAT_SCAN_FAIL, SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                p.Print(cout, recordData);
            }
        }

        // Print the footer
        p.PrintFooter(cout);

        // Close the scan and clean up
        SM_Try_IX(relcatFS.CloseScan(), SM_HELP_REL_CAT_SCAN_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: Help(const char* relName)
// Print schema of relName
/* Steps:
    1) Create the attributes structure
    2) Create a Printer object
    3) Print the header
    4) Start attrcat scan and print each tuple
    5) Print the footer
    6) Close the scan and clean up
*/
RC SM_Manager::Help(const char *relName)
{
    try
    {
        // Check that the database is open
        if (!open)
        {
            throw RC{SM_HELP_CLOSED};
        }

        // Check the parameter
        if (relName == nullptr)
        {
            throw RC{SM_NULLPTR_REL_NAME};
        }

        // Check if the relation exists
        GetRelInfo(relName);

        // Fill the attributes structure
        DataAttrInfo attributes[SM_ATTRCAT_ATTR_COUNT];
        GetAttrInfo("attrcat", SM_ATTRCAT_ATTR_COUNT, (void *)attributes);

        if (debug)
        {
            printf("========= Help %s =========\n", relName);
            printf("attributes = \n");
            for (int i = 0; i < SM_ATTRCAT_ATTR_COUNT; ++i)
            {
                printf("{\n");
                printf("\trelName = %s\n", attributes[i].relName);
                printf("\tattrName = %s\n", attributes[i].attrName);
                printf("\toffset = %d\n", attributes[i].offset);
                printf("\tattrType = %s\n", attributes[i].attrType == INT ? "INT" : attributes[i].attrType == FLOAT ? "FLOAT" : "STRING");
                printf("\tattrLength = %d\n", attributes[i].attrLength);
                printf("\tindexNo = %d\n", attributes[i].indexNo);
                printf("}\n");
            }
        }

        // Create a Printer object
        Printer p(attributes, SM_ATTRCAT_ATTR_COUNT);

        // Print the header
        p.PrintHeader(cout);

        // Start the attrcat file scan
        RM_FileScan attrcatFS;
        SM_Try_RM(attrcatFS.OpenScan(attrcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_HELP_REL_CAT_SCAN_FAIL);

        // Print each tuple
        RM_Record rec;
        char *recordData;
        for (RC rc = OK_RC; rc != RM_EOF;)
        {
            rc = attrcatFS.GetNextRec(rec);
            if (rc != 0 && rc != RM_EOF)
            {
                RM_PrintError(rc);
                SM_Try_RM(attrcatFS.CloseScan(), SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                throw RC{SM_HELP_REL_CAT_SCAN_FAIL};
            }
            if (rc != RM_EOF)
            {
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), attrcatFS, SM_HELP_REL_CAT_SCAN_FAIL, SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                p.Print(cout, recordData);
            }
        }

        // Print the footer
        p.PrintFooter(cout);

        // Close the scan and clean up
        SM_Try_IX(attrcatFS.CloseScan(), SM_HELP_REL_CAT_SCAN_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: Print(const char *relName)
// Print relName contents
/* Steps:
    1) Create the attributes structure
    2) Create a Printer object
    3) Print the header
    4) Open the RM file
    5) Start a RM file scan and print each tuple
        - If distributed, get data from all nodes and then print UNION
    6) Print the footer
    7) Close the scan and file and clean up
*/
RC SM_Manager::Print(const char *relName)
{
    try
    {
        // Check that the database is open
        if (!open)
        {
            return SM_PRINT_CLOSED;
        }

        // Check the parameters
        if (relName == nullptr)
        {
            return SM_NULLPTR_REL_NAME;
        }

        // Fill the attribute structure
        SM_RelcatRecord rcRecord = GetRelInfo(relName);
        int attrCount = rcRecord.attrCount;
        DataAttrInfo attributes[attrCount];
        GetAttrInfo(relName, attrCount, (void *)attributes);

        // Create a Printer object
        Printer p(attributes, attrCount);

        // Print the header
        p.PrintHeader(cout);

        // Open the relation RM file
        RM_FileHandle rmFH;
        SM_Try_RM(rMManager.OpenFile(relName, rmFH), SM_PRINT_FAIL);

        // Start the relcat file scan
        RM_FileScan rmFS;
        SM_Try_RM(rmFS.OpenScan(rmFH, INT, 4, 0, NO_OP, nullptr), SM_PRINT_SCAN_FAIL);

        // Print each tuple
        RM_Record rec;
        char *recordData;
        for (RC rc = OK_RC; rc != RM_EOF;)
        {
            rc = rmFS.GetNextRec(rec);
            if (rc != 0 && rc != RM_EOF)
            {
                RM_PrintError(rc);
                SM_Try_RM(rmFS.CloseScan(), SM_PRINT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                throw RC{SM_PRINT_SCAN_FAIL};
            }
            if (rc != RM_EOF)
            {
                SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), rmFS, SM_PRINT_SCAN_FAIL, SM_PRINT_SCAN_FAIL_CLOSE_SCAN_FAIL);
                p.Print(cout, recordData);
            }
        }

        // Print the footer
        p.PrintFooter(cout);

        // Close the scan and file and clean up
        SM_Try_RM(rmFS.CloseScan(), SM_PRINT_SCAN_FAIL);
        SM_Try_RM(rMManager.CloseFile(rmFH), SM_PRINT_SCAN_FAIL_CLOSE_SCAN_FAIL);
    }
    catch (RC rc)
    {
        return rc;
    }
    return OK_RC;
}

// Method: Set(const char *paramName, const char *value)
// Set parameter to value
/* System parameters:
*/
RC SM_Manager::Set(const char *paramName, const char *value)
{
    if (strcmp(paramName, "debug") == 0)
    {
        if (strcmp(value, "TRUE") == 0)
        {
            debug = true;
        }
        else if (strcmp(value, "FALSE") == 0)
        {
            debug = false;
        }
        else
        {
            return SM_SET_DEBUG_INVALID;
        }
    }
    return OK_RC; // Nothing to set yet
}

// Method: GetAttrInfo(const char* relName, int attrCount, DataAttrInfo* attributes)
// Get the attribute info about a relation from attrcat
/* Steps:
    1) Start file scan of attrcat for relName
    2) For each record, fill attributes array
*/
void SM_Manager::GetAttrInfo(const char *relName, int attrCount, void *_attributes)
{
    // Check the parameters
    if (relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (attrCount < 0)
    {
        throw RC{SM_INCORRECT_ATTRCOUNT};
    }

    RC rc;
    RM_FileScan attrcatFS;
    RM_Record rec;
    char *recordData;
    SM_AttrcatRecord *acRecord;
    DataAttrInfo *attributes = (DataAttrInfo *)_attributes;

    // Start file scan
    if (debug)
    {
        // printf("At GetAttrInfo, openscan with %s\n", relName);
    }

    SM_Try_RM(attrcatFS.OpenScan(attrcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_GET_ALL_ATTR_INFO_SCAN_FAIL);

    // Get all the attribute tuples
    if (debug)
    {
        // cout << "GetAttrInfo(" << relName << ", " << attrCount << ")\n";
    }
    for (int i = 0; rc != RM_EOF; ++i)
    {
        rc = attrcatFS.GetNextRec(rec);
        if (rc != 0 && rc != RM_EOF)
        {
            RM_PrintError(rc);
            SM_Try_RM(attrcatFS.CloseScan(), SM_GET_ALL_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL);
            throw RC{SM_PRINT_SCAN_FAIL};
        }

        if (rc != RM_EOF)
        {
            if (debug)
            {
                // cout << "GetAttrInfo(" << relName << ", " << attrCount << "): Now attributes[" << i << "]\n";
            }

            if (i == attrCount)
            {
                throw SM_INCORRECT_ATTRCOUNT;
            }

            SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), attrcatFS, SM_GET_ALL_ATTR_INFO_SCAN_FAIL, SM_GET_ALL_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL);
            acRecord = (SM_AttrcatRecord *)recordData;

            // Fill the attributes array
            strcpy(attributes[i].relName, acRecord->relName);
            strcpy(attributes[i].attrName, acRecord->attrName);
            attributes[i].offset = acRecord->offset;
            attributes[i].attrType = acRecord->attrType;
            attributes[i].attrLength = acRecord->attrLength;
            attributes[i].indexNo = acRecord->indexNo;
        }
    }

    // Close the scan
    SM_Try_RM(attrcatFS.CloseScan(), SM_GET_ALL_ATTR_INFO_SCAN_FAIL);
}

// Method: GetAttrInfo(const char* relName, const char* attrName, SM_AttrcatRecord* attributeData)
// Get an attribute info about an attribute of a relation from attrcat
/* Steps:
    1) Start file scan of attrcat for relName
    2) For each record, check whether the required attribute
    3) Fill the attribute structure
*/
SM_AttrcatRecord SM_Manager::GetAttrInfo(const char *relName, const char *attrName)
{
    // Check the parameters
    if (relName == nullptr)
    {
        throw RC{SM_NULLPTR_REL_NAME};
    }
    if (attrName == nullptr)
    {
        throw RC{SM_NULLPTR_ATTR_NAME};
    }

    RC rc;
    RM_FileScan attrcatFS;
    RM_Record rec;
    char *recordData;

    // Start file scan
    SM_Try_RM(attrcatFS.OpenScan(attrcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_GET_ATTR_INFO_SCAN_FAIL);

    // Get all the attribute tuples
    for (int i = 0; rc != RM_EOF; ++i)
    {
        rc = attrcatFS.GetNextRec(rec);
        if (rc != 0 && rc != RM_EOF)
        {
            RM_PrintError(rc);
            SM_Try_RM(attrcatFS.CloseScan(), SM_GET_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL);
            throw RC{SM_PRINT_SCAN_FAIL};
        }
        if (rc != RM_EOF)
        {
            SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), attrcatFS, SM_GET_ATTR_INFO_SCAN_FAIL, SM_GET_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL);
            SM_AttrcatRecord acRecord = *(SM_AttrcatRecord *)recordData;

            // Check for the required attribute
            if (strcmp(acRecord.attrName, attrName) == 0)
            {
                SM_Try_RM(attrcatFS.CloseScan(), SM_GET_ATTR_INFO_FAIL);

                return acRecord;
            }
        }
    }

    // Close the scan
    SM_Try_RM(attrcatFS.CloseScan(), SM_GET_ATTR_INFO_SCAN_FAIL);

    throw RC{SM_GET_ATTR_INFO_FAIL};
}

// Method: GetRelInfo(const char* relName, SM_RelcatRecord* relationData)
// Get the relation info from relcat
/* Steps:
    1) Start file scan of relcat for relName
    2) Fill relation data
*/
SM_RelcatRecord SM_Manager::GetRelInfo(const char *relName)
{
    // Check the parameters
    if (relName == nullptr)
    {
        throw SM_NULLPTR_REL_NAME;
    }

    RM_FileScan relcatFS;
    RM_Record rec;
    char *recordData;

    // Start file scan
    SM_Try_RM(relcatFS.OpenScan(relcatRMFH, STRING, MAXNAME, 0, EQ_OP, relName), SM_GET_REL_INFO_SCAN_FAIL);

    // Get the relation tuple
    SM_Try_RM_Or_Close_Scan(relcatFS.GetNextRec(rec), relcatFS, SM_GET_REL_INFO_SCAN_FAIL, SM_GET_REL_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL);
    SM_Try_RM_Or_Close_Scan(rec.GetData(recordData), relcatFS, SM_GET_REL_INFO_SCAN_FAIL, SM_GET_REL_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL);

    SM_RelcatRecord rcRecord = *(SM_RelcatRecord *)recordData;

    // Close the scan
    SM_Try_RM(relcatFS.CloseScan(), SM_GET_REL_INFO_FAIL);

    // Fill the relation data
    return rcRecord;
}