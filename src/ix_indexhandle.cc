//
// File:        ix_indexhandle.cc
// Description: IX_IndexHandle class implementation
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#include "ix_internal.h"
#include "ix.h"
#include <cstring>
#include <iostream>
using namespace std;

// Constructor
IX_IndexHandle::IX_IndexHandle() {
}

// Destructor
IX_IndexHandle::~IX_IndexHandle() {
    // Nothing to free
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
}

RC IX_IndexHandle::ForcePages() {
}