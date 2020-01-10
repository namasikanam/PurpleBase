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

private:
    // TODO: 这俩有什么用？
    IX_Manager &iXManager; // IX_Manager object
    RM_Manager &rMManager; // RM_Manager object

    RM_FileHandle relcatRMFH;  // RM file handle for relcat
    RM_FileHandle attrcatRMFH; // RM file handle for attrcat
    bool isOpen;               // Flag whether the database is open
};

//
// Print-error function
//
void SM_PrintError(RC rc);

// Warnings
#define SM_DATABASE_DOES_NOT_EXIST (START_SM_WARN + 0) // Database does not exist
#define SM_NULL_DATABASE_NAME (START_SM_WARN + 1)
#define SM_DATABASE_OPEN (START_SM_WARN + 2)   // Database is open
#define SM_DATABASE_CLOSED (START_SM_WARN + 3) // Database is closed
#define SM_LASTWARN SM_DATABASE_CLOSED

// Errors
#define SM_INVALID_DATABASE_NAME (START_SM_ERR - 0) // Invalid database file name
#define SM_OPEN_RELCAT_FAIL (START_SM_ERR - 1)      // Invalid attribute
#define SM_OPEN_ATTRCAT_FAIL (START_SM_ERR - 2)     // Invalid attribute
#define SM_CLOSE_RELCAT_FAIL (START_SM_ERR - 3)
#define SM_CLOSE_ATTRCAT_FAIL (START_SM_ERR - 4)
#define SM_INVALID_DATABASE_CLOSE (START_SM_ERR - 5) // Database cannot be closed
#define SM_UNDEFINED (START_SM_WARN - 6)

// Error in UNIX system call or library routine
#define SM_UNIX (START_SM_ERR - 7) // Unix error
#define SM_LASTERROR SM_UNIX

#endif
