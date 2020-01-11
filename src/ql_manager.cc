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
    // Check whether database is open
    if (!smManager.open)
    {
        return QL_DATABASE_CLOSED;
    }

    // Check if the relations exist
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
    return QL_UNDEFINED;
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
    return QL_UNDEFINED;
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
    return QL_UNDEFINED;
}