//
// File:        ix_indexscan.cc
// Description: IX_IndexScan class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "ix_internal.h"
#include "ix.h"
#include <iostream>
#include <cstring>
using namespace std;

// Constructor
IX_IndexScan::IX_IndexScan() {
    // Set open scan flag to false
    scanOpen = FALSE;
}

// Destructor
IX_IndexScan::~IX_IndexScan() {
    // Nothing to free
}

// Rather than similar things to OpenScan in RM component,
// here we get all satisfied rids when opening.
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp,
                          void *value, ClientHint  pinHint) {
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
}

RC IX_IndexScan::CloseScan() {
}