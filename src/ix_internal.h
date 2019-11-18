//
// File:        ix_internal.h
// Description: Declarations internal to the IX component
// Authors:     Xingyu Xie (xiexy17@mails.tsinghua.edu.cn)
//

#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H

#include "ix.h"

// A wrapper to execute the API of PF.
void IX_Try(RC pf_rc, RC ix_rc) {
    if (pf_rc) {
        PF_PrintError(pf_rc);
        throw RC{ix_rc};
    }
}

#endif