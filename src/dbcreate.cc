//
// dbcreate.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.
//
// Improved by: Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <bits/stdc++.h>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "redbase.h"
#include "util_internal.h"
using namespace std;

//
// main
//
/* Steps:
    1) Create a subdirectory for the database
    4) Create the system catalogs
        - Create RM files for relcat and attrcat
        - Open the files
        - Insert the relcat and attrcat as relations into relcat
        - Insert the attributions of relcat and attrcat into attrcat
        - Close the files
*/
int main(int argc, char *argv[])
{
    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the database.
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " dbname\n";
        exit(1);
    }

    // The database name is the second argument
    char *dbname = argv[1];

    // Create a subdirectory for the database
    char command[255] = "mkdir ";
    if (system(strcat(command, dbname)) != 0)
    {
        cerr << argv[0] << " cannot create directory: " << dbname << "\n";
        exit(1);
    }

    // Change to the database subdirectory
    if (chdir(dbname) < 0)
    {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    PF_Manager pfManager;
    RM_Manager rmManager(pfManager);
    char relcatName[] = "relcat";
    char attrcatName[] = "attrcat";

    // Create the system catalogs
    // Create RM files for relcat and attrcat
    Try_RM(rmManager.CreateFile(relcatName, sizeof(SM_RelcatRecord)));
    Try_RM(rmManager.CreateFile(attrcatName, sizeof(SM_AttrcatRecord)));

    // Open the files
    RM_FileHandle relcatFH;
    RM_FileHandle attrcatFH;
    Try_RM(rmManager.OpenFile(relcatName, relcatFH));
    Try_RM(rmManager.OpenFile(attrcatName, attrcatFH));

    // Insert relcat record in relcat
    RID rid;
    SM_RelcatRecord rcRecord = SM_RelcatRecord{
        "relcat",
        sizeof(SM_RelcatRecord),
        SM_RELCAT_ATTR_COUNT,
        0};
    Try_RM(relcatFH.InsertRec((char *)&rcRecord, rid));

    // Insert attrcat record in relcat
    rcRecord = SM_RelcatRecord{
        "attrcat",
        sizeof(SM_AttrcatRecord),
        SM_ATTRCAT_ATTR_COUNT,
        0};
    Try_RM(relcatFH.InsertRec((char *)&rcRecord, rid));

    // Insert relcat attributes in attrcat
    SM_AttrcatRecord acRecord = SM_AttrcatRecord{
        "relcat",
        "relName",
        offsetof(SM_RelcatRecord, relName),
        STRING,
        MAXNAME + 1,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "relcat",
        "tupleLength",
        offsetof(SM_RelcatRecord, tupleLength),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "relcat",
        "attrCount",
        offsetof(SM_RelcatRecord, attrCount),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "relcat",
        "indexCount",
        offsetof(SM_RelcatRecord, indexCount),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));

    // Insert attrcat attributes in attrcat
    acRecord = SM_AttrcatRecord{
        "attrcat",
        "relName",
        offsetof(SM_AttrcatRecord, relName),
        STRING,
        MAXNAME + 1,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "attrcat",
        "attrName",
        offsetof(SM_AttrcatRecord, attrName),
        STRING,
        MAXNAME + 1,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "attrcat",
        "offset",
        offsetof(SM_AttrcatRecord, offset),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "attrcat",
        "attrType",
        offsetof(SM_AttrcatRecord, attrName),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "attrcat",
        "attrLength",
        offsetof(SM_AttrcatRecord, attrLength),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));
    acRecord = SM_AttrcatRecord{
        "attrcat",
        "indexNo",
        offsetof(SM_AttrcatRecord, indexNo),
        INT,
        4,
        -1};
    Try_RM(attrcatFH.InsertRec((char *)&acRecord, rid));

    // Close the files
    Try_RM(rmManager.CloseFile(relcatFH));
    Try_RM(rmManager.CloseFile(attrcatFH));
}
