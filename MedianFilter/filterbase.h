#ifndef FILTERBASE_H
#define FILTERBASE_H

#include <inttypes.h>
#include "pngimage.h"

class FilterBase
{
public:
    virtual PngImage apply(PngImage& image) = 0;
    virtual int getLatency() const = 0;
};

#endif // FILTERBASE_H
