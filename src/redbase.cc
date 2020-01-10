//
// redbase.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.
//
// Improved by: Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include <bits/stdc++.h>
#include <unistd.h>
#include "redbase.h"
#include "rm.h"
#include "sm.h"
#include "ql.h"
#include "util_internal.h"
using namespace std;

//
// main
//
/* Steps:
    1) Initialize redbase components
    2) Open the database
    3) Call the parser
    4) Close the database
*/
int main(int argc, char *argv[])
{
    // Look for 2 arguments.  The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    char *dbname = argv[1];

    // Initialize RedBase components
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);
    QL_Manager qlm(smm, ixm, rmm);

    // Open the database
    Try_SM(smm.OpenDb(dbname));

    // Call the parser
    RBparse(pfm, smm, qlm);

    // Close the database
    Try_SM(smm.CloseDb());

    cout << "Valar Morghulis.\n";
    return 0;
}
