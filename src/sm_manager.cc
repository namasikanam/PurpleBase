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

// Constructor
SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) : rMManager(rmm), iXManager(ixm), isOpen(false)
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
    // Check if open
    if (isOpen)
    {
        return SM_DATABASE_OPEN;
    }

    // Check the parameter
    if (dbName == NULL)
    {
        return SM_NULL_DATABASE_NAME;
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
    isOpen = true;

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
    if (!isOpen)
    {
        return SM_DATABASE_CLOSED;
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
    isOpen = FALSE;

    // Return OK
    return OK_RC;
}

// Method: CreateTable(const char *relName, int attrCount, AttrInfo *attributes,
//                     int isDistributed, const char* partitionAttrName, int nValues, const Value values[])
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
    return SM_UNDEFINED;
}

// Method: DropTable(const char *relName)
// Destroy a relation
/* Steps:
    1) Check that the database is open
    2) Print the drop table command
    3) Delete the entry from relcat
    4) Scan through attrcat
        - Destroy the indexes and delete the entries
    5) Destroy the RM file for the relation
    6) Flush the system catalogs
*/
RC SM_Manager::DropTable(const char *relName)
{
    return SM_UNDEFINED;
}

// Method: CreateIndex(const char *relName, const char *attrName)
// Create an index for relName.attrName
/* Steps:
    1) Check the parameters
    2) Check that the database is open
    3) Check whether the index exists
    4) Update and flush the system catalogs
    5) Create and open the index file
    6) Scan all the tuples and insert in the index
    7) Close the index file
*/
RC SM_Manager::CreateIndex(const char *relName, const char *attrName)
{
    return SM_UNDEFINED;
}

// Method: DropIndex(const char *relName, const char *attrName)
// Destroy index on relName.attrName
/* Steps:
    1) Check the parameters
    2) Check that the database is open
    3) Check whether the index exists
    4) Update and flush the system catalogs
    5) Destroy the index file
*/
RC SM_Manager::DropIndex(const char *relName, const char *attrName)
{
    return SM_UNDEFINED;
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
    return SM_UNDEFINED;
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
    return SM_UNDEFINED;
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
    return SM_UNDEFINED;
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
    return SM_UNDEFINED;
}

// Method: Set(const char *paramName, const char *value)
// Set parameter to value
/* System parameters:
    1) printCommands - TRUE or FALSE
*/
RC SM_Manager::Set(const char *paramName, const char *value)
{
    return SM_UNDEFINED;
}