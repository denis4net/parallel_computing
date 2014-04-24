#include <cstdlib>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <chrono>
#include <string>

#include <CL/cl.hpp>

#include "clmanager.h"
#include "pngimage.h"
#include "filtermedian.h"


using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::steady_clock;

FilterMedian::FilterMedian(const char *clKernelPath) :
    m_kernelPath(clKernelPath), m_useAsyncCopy(false)
{}

PngImage FilterMedian::apply(PngImage& sourceImage)
{
    return openCLLocalImplementation(sourceImage);
}

int FilterMedian::getLatency() const
{
    return latency;
}

std::string FilterMedian::readKernel(const char *fileName) throw(std::exception)
{
    std::ifstream fs;
    fs.open(fileName);

    if (!fs.is_open())
        throw "Can't open kernel file";

    std::string buf;
    std::string result;

    while (!fs.eof()) {
        std::getline(fs, buf);
        result += buf +"\n";
    }

    return result;
}


PngImage FilterMedian::openCLLocalImplementation(PngImage& sourceImage)
{
    PngImage rImage = sourceImage;
    const int pixelsCount = rImage.getWidth() * rImage.getHeight();
    const int nStreams = streamsCount();

    if (rImage.getHeight() % nStreams)
        throw "Incorrect number of streams";

    const int linesPerStream = rImage.getHeight() / nStreams;
    //count of pixel passed as input argument to kernel main fuction. Each pixel represented in
    // ARGB format and stored in cl_ulong value type
    const int cQueueImageBuffer = (linesPerStream + 2) * (2 + rImage.getWidth()); //measured in pixels count
    const int queueImageBufferSize = cQueueImageBuffer*sizeof(cl_ulong); //measured in bytes
    std::chrono::time_point<std::chrono::system_clock> start, end;

    //get default device from first platformopen
    std::shared_ptr<cl::Device>  clDevice = CLManager::getInstance()->getDevice();

    if (!clDevice) {
        throw std::string("Can't find OpenCL device/platform on this computer. Try to install driver.");
    }

    const auto deviceMaxComputeUnits = clDevice->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    const auto deviceName = clDevice->getInfo<CL_DEVICE_NAME>();

    std::cout << deviceName << " have " << deviceMaxComputeUnits << " compute units" << std::endl;

    cl::Context clContext = cl::Context( { *clDevice } );
    //fill source container
    cl::Program::Sources clSources;
    std::string kText = readKernel(m_kernelPath);
    const char* kernelSourceCode = kText.c_str();
    clSources.push_back( { kernelSourceCode, std::strlen(kernelSourceCode) } );
    //create program
    cl::Program program( clContext, clSources);

    //try to build program
    if ( program.build( { *clDevice }, "-g" ) != CL_SUCCESS ) {
        std::cerr << "Can't build kernel program: " << std::endl;
        std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*clDevice) << std::endl << std::endl;
        throw std::string("Can't build OpenCL image");
    }

    cl_ulong *srcPx = new cl_ulong[pixelsCount];

    //copy image pixels data to array
    for (int y=0; y<sourceImage.getHeight(); y++)
        for (int x=0; x<sourceImage.getWidth(); x++)
            srcPx[sourceImage.getWidth()*y+x] = static_cast<cl_ulong>(sourceImage.pixel(x,y));

    cl::Buffer clDstBuffer = cl::Buffer( clContext, CL_MEM_WRITE_ONLY, pixelsCount*sizeof(cl_ulong) );

    std::vector<cl::Kernel> clKernels(nStreams);
    std::vector<cl::Event> clEvents(nStreams);
    std::vector<cl::CommandQueue> clQueues(nStreams);
    std::vector<cl::Buffer> clSrcBuffers(nStreams);
    std::vector<cl_ulong> offsets(nStreams);
    std::vector<cl_ulong*> buffers(nStreams);

    start = std::chrono::system_clock::now();

    std::vector<cl_ulong> maxItemDimensions = clDevice->getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    const int localRangeWidth = sourceImage.getWidth() > maxItemDimensions[0] ? maxItemDimensions[0] :  sourceImage.getWidth();
    const int localRangeHeight = 1;

    const int globalRangeHeight = linesPerStream;

    cl::NDRange localRange = cl::NDRange(localRangeWidth, localRangeHeight);
    const int globalRangeWidth = sourceImage.getWidth();
    cl::NDRange globalRange = cl::NDRange(globalRangeWidth, globalRangeHeight);

    std::cout << "Local range: " << localRangeWidth << 'x' << localRangeHeight << std::endl;
    std::cout << "Global range: " << globalRangeWidth << 'x' << globalRangeHeight << std::endl;

    for (int i=0; i<nStreams; i++) {
        clSrcBuffers[i] = cl::Buffer(clContext, CL_MEM_READ_ONLY, queueImageBufferSize);
        offsets[i] = linesPerStream * i;
        clKernels[i] = cl::Kernel(program, "medianFilter");
        clQueues[i] = cl::CommandQueue(clContext, *clDevice);

        cl_ulong* buf = new cl_ulong[cQueueImageBuffer];
        std::memset(buf, 0x0, queueImageBufferSize);

        //preparing input image buffer for queue with black pixels on edges
        const int xOffset = 1;
        const int yOffset = 1;

        std::cout << "Preparing Image buffer for queue: " << i << ", buffer size " << queueImageBufferSize/1024.0 << "kB"
                  << ", image part " << sourceImage.getWidth()+2 << "x" << linesPerStream+2 << std::endl;

        for(int y=0; y<linesPerStream; y++) {
            for (int x=0; x<sourceImage.getWidth(); x++) {
                buf[(y+yOffset)*(sourceImage.getWidth()+2*xOffset)+xOffset+x] = sourceImage.pixel(x, linesPerStream*i+y);
            }
        }

        buffers[i] = buf;
    }
    for (int i=0; i<nStreams; i++)
        clQueues[i].enqueueWriteBuffer(clSrcBuffers[i], CL_FALSE, 0,
                                       queueImageBufferSize, buffers[i]);

    for (int i=0; i<nStreams; i++)  {
        clKernels[i].setArg(0, clSrcBuffers[i]);
        clKernels[i].setArg(1, clDstBuffer);
        //clKernels[i].setArg(2, sizeof(offsets[i]), (void*) (&offsets[i]))
        cl_int err = clQueues[i].enqueueNDRangeKernel(clKernels[i], cl::NullRange, globalRange, localRange, NULL, &clEvents[i]);

        if (err != CL_SUCCESS)
            throw std::string("Can't enqueue buffer writing to target device");

    }

    cl::WaitForEvents(clEvents);
    //copying processed pixels from CLDevice global  memory
    //TODO: carryfully calculate buffer size
    clQueues[0].enqueueReadBuffer(clDstBuffer, CL_TRUE, 0, sourceImage.getHeight()*sourceImage.getWidth()*sizeof(cl_ulong), srcPx);

    //compose result image
    for (int y=0; y<rImage.getHeight(); y++)
        for (int x = 0; x<rImage.getWidth(); x++) {
            pixel_t pixel = static_cast<pixel_t>(srcPx[y*rImage.getWidth()+x]);
            rImage.setPixel(x, y, pixel);
        }

    end = std::chrono::system_clock::now();
    latency = (end - start).count();
    std::cout << "Median filter [OpenCL with local memory using] latency " << latency << "ns" << std::endl;
    delete srcPx;
    for (int i=0; i<nStreams; i++)
        delete buffers[i];

    return rImage;
}
bool FilterMedian::useAsyncCopy() const
{
    return m_useAsyncCopy;
}

void FilterMedian::setUseAsyncCopy(bool useAsyncCopy)
{
    m_useAsyncCopy = useAsyncCopy;
}
int FilterMedian::streamsCount() const
{
    return m_streamsCount;
}

void FilterMedian::setStreamsCount(int streamsCount)
{
    m_streamsCount = streamsCount;
}



