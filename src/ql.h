//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H

#include <stdlib.h>
#include <string.h>
#include "redbase.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "printer.h"

//
// QL_Manager: query language (DML)
//
class QL_Manager
{
public:
    QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~QL_Manager(); // Destructor

    RC Select(int nSelAttrs,                 // # attrs in select clause
              const RelAttr selAttrs[],      // attrs in select clause
              int nRelations,                // # relations in from clause
              const char *const relations[], // relations in from clause
              int nConditions,               // # conditions in where clause
              const Condition conditions[]); // conditions in where clause

    RC Insert(const char *relName,   // relation to insert into
              int nValues,           // # values
              const Value values[]); // values to insert

    RC Delete(const char *relName,           // relation to delete from
              int nConditions,               // # conditions in where clause
              const Condition conditions[]); // conditions in where clause

    RC Update(const char *relName,           // relation to update
              const RelAttr &updAttr,        // attribute to update
              const int bIsValue,            // 1 if RHS is a value, 0 if attribute
              const RelAttr &rhsRelAttr,     // attr on RHS to set LHS equal to
              const Value &rhsValue,         // or value to set attr equal to
              int nConditions,               // # conditions in where clause
              const Condition conditions[]); // conditions in where clause

    SM_Manager &smManager; // SM_Manager object
    IX_Manager &ixManager; // IX_Manager object
    RM_Manager &rmManager; // RM_Manager object

    // Utilities
    SM_AttrcatRecord checkAttr(RelAttr &attr, int nRelations, const char *const relations[]);
    DataAttrInfo checkAttr(RelAttr &attr, const char *relName, int attrCount, DataAttrInfo attributes[]);
    void scanRelations(int id, int nRelations, const char *const relations[], char *records[], int nConditions, Condition conditions[], int indexRelOfCondLHS[], int offsetOfCondLHS[], int indexRelOfCondRHS[], int offsetOfCondRHS[], int nSelAttrs, DataAttrInfo printAttrs[], int indexRelOfPrintAttr[], int offsetOfPrintAttr[], Printer &p, char *buf);
    int indexOfRel(const char *relName, int nRelations, const char *const relations[]);
};

//
// Print-error function
//
void QL_PrintError(RC rc);

// Warnings
#define QL_DATABASE_CLOSED (START_QL_WARN + 0)
#define QL_REL_NOT_EXIST (START_QL_WARN + 1)
#define QL_SELECT_FAIL (START_QL_WARN + 2)
#define QL_ATTR_OF_MULT_REL (START_QL_WARN + 3)
#define QL_ATTR_OF_NO_REL (START_QL_WARN + 4)
#define QL_SAME_REL_APPEAR_AGAIN (START_QL_WARN + 5)
#define QL_TYPES_INCOMPATIBLE (START_QL_WARN + 6)
#define QL_NULLPTR_RELATION (START_QL_WARN + 7)
#define QL_SYS_CAT (START_QL_WARN + 8)
#define QL_INCORRECT_ATTRCOUNT (START_QL_WARN + 9)
#define QL_INCORRECT_ATTRTYPE (START_QL_WARN + 10)
#define QL_ATTR_INCORRECT_REL (START_QL_WARN + 11)
#define QL_ATTR_INCORRECT_ATTR (START_QL_WARN + 12)
#define QL_INSERT_TOO_LONG_STRING (START_QL_WARN + 13)
#define QL_UPDATE_TOO_LONG_STRING (START_QL_WARN + 14)
#define QL_INSERT_NOT_EXIST (START_QL_WARN + 15)
#define QL_LASTWARN QL_INSERT_NOT_EXIST

// Errors
#define QL_RELS_SCAN_FAIL (START_QL_ERR - 0)
#define QL_INSERT_FAIL (START_QL_ERR - 1)
#define QL_DELETE_FAIL (START_QL_ERR - 2)
#define QL_UPDATE_FAIL (START_QL_ERR - 3)

// Error in UNIX system call or library routine
#define QL_UNIX (START_QL_ERR - 2) // Unix error
#define QL_LASTERROR QL_UNIX

#endif
