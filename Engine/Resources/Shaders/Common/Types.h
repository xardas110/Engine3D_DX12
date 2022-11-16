#ifndef TYPES_H
#define TYPES_H

#ifdef HLSL
#include "HlslCompat.h"
#endif

struct ObjectCB
{
    XMMATRIX model; // Updates pr. object
    XMMATRIX mvp; // Updates pr. object
    XMMATRIX invTransposeMvp; // Updates pr. object

    XMMATRIX view; // Updates pr. frame
    XMMATRIX proj; // Updates pr. frame

    XMMATRIX invView; // Updates pr. frame
    XMMATRIX invProj; // Updates pr. frame

    UINT entId;
    UINT meshId;
    int pad1;
    int pad2;

    XMMATRIX transposeInverseModel;
};

#endif