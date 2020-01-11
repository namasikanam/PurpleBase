//
// sm.h
//   The System Management Component Interfaces
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include <stdlib.h>
#include <string.h>
#include "redbase.h" // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"

// Data structures

// SM_RelcatRecord - Records stored in the relcat relation
/* Stores the following:
    1) relName - name of the relation - char*
    2) tupleLength - length of the tuples - integer
    3) attrCount - number of attributes - integer
    4) indexCount - number of indexes - integer
*/
struct SM_RelcatRecord
{
    char relName[MAXNAME + 1]; // + 1 for coding convenience
    int tupleLength;
    int attrCount;
    int indexCount;

    SM_RelcatRecord(char *_relName, int _tupleLength, int _attrCount, int _indexCount);
    SM_RelcatRecord(const char *_relName, int _tupleLength, int _attrCount, int _indexCount);
    SM_RelcatRecord() {}
};

// SM_AttrcatRecord - Records stored in the attrcat relation
/* Stores the following:
    1) relName - name of the relation - char*
    2) attrName - name of the attribute - char*
    3) offset - offset of the attribute - integer
    4) attrType - type of the attribute - AttrType
    5) attrLength - length of the attribute - integer
    6) indexNo - number of the index - integer
*/
struct SM_AttrcatRecord
{
    char relName[MAXNAME + 1];  // + 1 for coding convenience
    char attrName[MAXNAME + 1]; // + 1 for coding convenience
    int offset;
    AttrType attrType;
    int attrLength;
    int indexNo;

    SM_AttrcatRecord(char *_relName, char *_attrName, int _offset, AttrType _attrType, int _attrLength, int _indexNo);
    SM_AttrcatRecord(const char *_relName, char *_attrName, int _offset, AttrType _attrType, int _attrLength, int _indexNo);
    SM_AttrcatRecord(const char *_relName, const char *_attrName, int _offset, AttrType _attrType, int _attrLength, int _indexNo);
    SM_AttrcatRecord() {}
};

// Constants
#define SM_RELCAT_ATTR_COUNT 4
#define SM_ATTRCAT_ATTR_COUNT 6

//
// SM_Manager: provides data management
//
class SM_Manager
{
    friend class QL_Manager;

public:
    SM_Manager(IX_Manager &ixm, RM_Manager &rmm);
    ~SM_Manager(); // Destructor

    RC OpenDb(const char *dbName); // Open the database
    RC CloseDb();                  // close the database

    RC CreateTable(const char *relName,   // create relation relName
                   int attrCount,         //   number of attributes
                   AttrInfo *attributes); //   attribute data
    RC CreateIndex(const char *relName,   // create an index for
                   const char *attrName); //   relName.attrName
    RC DropTable(const char *relName);    // destroy a relation

    RC DropIndex(const char *relName,   // destroy index on
                 const char *attrName); //   relName.attrName
    RC Load(const char *relName,        // load relName from
            const char *fileName);      //   fileName
    RC Help();                          // Print relations in db
    RC Help(const char *relName);       // print schema of relName

    RC Print(const char *relName); // print relName contents

    RC Set(const char *paramName, // set parameter to
           const char *value);    //   value

    IX_Manager &iXManager; // IX_Manager object
    RM_Manager &rMManager; // RM_Manager object

    RM_FileHandle relcatRMFH;  // RM file handle for relcat
    RM_FileHandle attrcatRMFH; // RM file handle for attrcat
    bool open;                 // Flag whether the database is open

    // Utilities
    void GetAttrInfo(const char *relName, int attrCount, void *_attributes);
    SM_AttrcatRecord GetAttrInfo(const char *relName, const char *attrName);
    SM_RelcatRecord GetRelInfo(const char *relName);

    bool debug = true;
};

//
// Print-error function
//
void SM_PrintError(RC rc);

// Warnings
#define SM_DATABASE_DOES_NOT_EXIST (START_SM_WARN + 0) // Database does not exist
#define SM_CREATE_TABLE_CLOSED (START_SM_WARN + 1)     // Database is closed
#define SM_OPEN_OPEND (START_SM_WARN + 2)              // Database is open
#define SM_CLOSE_CLOSED (START_SM_WARN + 3)            // Database is closed
#define SM_INCORRECT_ATTRCOUNT (START_SM_WARN + 4)
#define SM_DROP_TABLE_CLOSED (START_SM_WARN + 5)
#define SM_CREATE_INDEX_EXISTS (START_SM_WARN + 6)
#define SM_CREATE_INDEX_CLOSED (START_SM_WARN + 7)
#define SM_DROP_INDEX_CLOSED (START_SM_WARN + 8)
#define SM_INDEX_DOES_NOT_EXIST (START_SM_WARN + 9)
#define SM_NULLPTR_REL_NAME (START_SM_WARN + 10)
#define SM_NULLPTR_ATTR_NAME (START_SM_WARN + 11)
#define SM_NULLPTR_DB_NAME (START_SM_WARN + 12)
#define SM_NULLPTR_FILE_NAME (START_SM_WARN + 13)
#define SM_LOAD_CLOSED (START_SM_WARN + 14)
#define SM_LOAD_INVALID_DATA_FILE (START_SM_WARN + 15)
#define SM_HELP_CLOSED (START_SM_WARN + 16)
#define SM_PRINT_CLOSED (START_SM_WARN + 17)
#define SM_LOAD_SYSTEM_CAT (START_SM_WARN + 18)
#define SM_GET_ALL_ATTR_INFO_FAIL (START_SM_WARN + 19)
#define SM_GET_ALL_ATTR_INFO_SCAN_FAIL (START_SM_WARN + 20)
#define SM_GET_ALL_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_WARN + 21)
#define SM_GET_ATTR_INFO_FAIL (START_SM_WARN + 22)
#define SM_GET_ATTR_INFO_SCAN_FAIL (START_SM_WARN + 23)
#define SM_GET_ATTR_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_WARN + 24)
#define SM_GET_REL_INFO_FAIL (START_SM_WARN + 25)
#define SM_GET_REL_INFO_SCAN_FAIL (START_SM_WARN + 26)
#define SM_GET_REL_INFO_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_WARN + 27)
#define SM_SET_DEBUG_INVALID (START_SM_WARN + 28)
#define SM_CREATE_TABLE_EXIST (START_SM_WARN + 29)
#define SM_LOAD_STRING_TOO_LONG (START_SM_WARN + 30)
#define SM_LOAD_BAD_INT (START_SM_WARN + 31)
#define SM_LOAD_BAD_FLOAT (START_SM_WARN + 32)
#define SM_LASTWARN SM_LOAD_BAD_FLOAT

// Errors
#define SM_INVALID_DATABASE_NAME (START_SM_ERR - 0) // Invalid database file name
#define SM_OPEN_RELCAT_FAIL (START_SM_ERR - 1)      // Invalid attribute
#define SM_OPEN_ATTRCAT_FAIL (START_SM_ERR - 2)     // Invalid attribute
#define SM_CLOSE_RELCAT_FAIL (START_SM_ERR - 3)
#define SM_CLOSE_ATTRCAT_FAIL (START_SM_ERR - 4)
#define SM_INVALID_DATABASE_CLOSE (START_SM_ERR - 5) // Database cannot be closed
#define SM_UNDEFINED (START_SM_ERR - 6)
#define SM_TOO_LONG_RELNAME (START_SM_ERR - 7)
#define SM_TOO_LONG_ATTRNAME (START_SM_ERR - 8)
#define SM_CREATE_TABLE_FAIL (START_SM_ERR - 9)
#define SM_PRINT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 10)
#define SM_CREATE_TABLE_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 11)
#define SM_CREATE_TABLE_INSERT_ATTR_CAT_FAIL (START_SM_ERR - 12)
#define SM_CREATE_TABLE_INSERT_REL_CAT_FAIL (START_SM_ERR - 13)
#define SM_DROP_TABLE_FAIL (START_SM_ERR - 14)
#define SM_DROP_TABLE_REL_CAT_SCAN_FAIL (START_SM_ERR - 15)
#define SM_DROP_TABLE_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 16)
#define SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL (START_SM_ERR - 17)
#define SM_DROP_TABLE_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 18)
#define SM_CREATE_INDEX_REL_CAT_SCAN_FAIL (START_SM_ERR - 19)
#define SM_CREATE_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 20)
#define SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL (START_SM_ERR - 21)
#define SM_CREATE_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 22)
#define SM_CREATE_INDEX_FAIL (START_SM_ERR - 23)
#define SM_CREATE_INDEX_RM_SCAN_FAIL (START_SM_ERR - 24)
#define SM_CREATE_INDEX_RM_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 25)
#define SM_DROP_INDEX_REL_CAT_SCAN_FAIL (START_SM_ERR - 26)
#define SM_DROP_INDEX_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 27)
#define SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL (START_SM_ERR - 28)
#define SM_DROP_INDEX_ATTR_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 29)
#define SM_DROP_INDEX_FAIL (START_SM_ERR - 30)
#define SM_LOAD_FAIL (START_SM_ERR - 31)
#define SM_LOAD_FAIL_CLOSE_FAIL (START_SM_ERR - 32)
#define SM_HELP_REL_CAT_SCAN_FAIL (START_SM_ERR - 33)
#define SM_HELP_REL_CAT_SCAN_FAIL_CLOSE_SCAN_FAIL (START_SM_ERR - 34)
#define SM_PRINT_FAIL (START_SM_ERR - 35)
#define SM_PRINT_SCAN_FAIL (START_SM_ERR - 36)

// Error in UNIX system call or library routine
#define SM_UNIX (START_SM_ERR - 37) // Unix error
#define SM_LASTERROR SM_UNIX

#endif
