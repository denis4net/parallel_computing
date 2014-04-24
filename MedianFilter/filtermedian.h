#ifndef FILTERMEDIAN_H
#define FILTERMEDIAN_H

#include <exception>
#include "filterbase.h"
#include "pngimage.h"


class FilterMedian : public FilterBase
{
public:
    FilterMedian(const char* clKernelPath);
    virtual PngImage apply(PngImage& sourceImage);
    virtual int getLatency() const;

    PngImage openCLLocalImplementation(PngImage &sourceImage);
    bool useAsyncCopy() const;
    void setUseAsyncCopy(bool useAsyncCopy);

    int streamsCount() const;
    void setStreamsCount(int streamsCount);

private:
    int latency;
    bool m_useAsyncCopy;
    const char* m_kernelPath;
    int m_streamsCount;

    PngImage openCLGlobalMemImplementation(PngImage& sourceImage);
    std::string readKernel(const char* fileName) throw(std::exception);
};

#endif // FILTERMEDIAN_H
