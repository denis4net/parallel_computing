#include "SobelFilterImage.hpp"

#include <cmath>
#include <fstream>
#include <mpi.h>
#include <vector>
#include <sys/time.h>

extern "C" {
#include <sys/types.h>
#include <dirent.h>
}

namespace OS {
std::vector<std::string> getDirListing(const char* path) {
    std::vector<std::string> listing;
    DIR* h = opendir(path);
    struct dirent* de;

    while((de = readdir(h)) != NULL) {
        std::string name = de->d_name;
        if (!name.compare("..") || !name.compare("."))
            continue;
        listing.push_back(name);
    }

    return listing;
}

void printTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    std::cout << tv.tv_sec << ' ' << tv.tv_usec << std::endl;
}

}


std::string SobelFilterImage::readFile(const char *fileName)
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


cl_uint *SobelFilterImage::getOutputBuffer() {
    return outputImageData;
}


int SobelFilterImage::setInputBuffer(cl_uint *buf, int w, int h)
{
    inputImageData = buf;
    width = w;
    height = h;
    outputImageData = new cl_uint[width * height];
}


int SobelFilterImage::setupCL()
{
    cl_int err = CL_SUCCESS;
    cl_device_type dType;

    if(sampleArgs.deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    else //deviceType = "gpu"
    {
        dType = CL_DEVICE_TYPE_GPU;
        if(sampleArgs.isThereGPU)
        {
            std::cout << "GPU not found. Falling back to CPU device" << std::endl;
            dType = CL_DEVICE_TYPE_CPU;
        }
    }

    err = cl::Platform::get(&platforms);
    CHECK_OPENCL_ERROR(err, "Platform::get() failed.");

    std::vector<cl::Platform>::iterator i;

    int deviceId = -1;

    for (i=platforms.begin(); i!=platforms.end(); i++)
    {
        //std::cout << "Platform :" << (*i).getInfo<CL_PLATFORM_VENDOR>().c_str() << "\n";


        devices.clear();
        i->getDevices(dType, &devices);

        if (devices.size() < 0)
            break;

        deviceId = 0;
#if 0
        for (std::vector<cl::Device>::iterator i = devices.begin(); i != devices.end(); ++i)
        {
            std::cout << "Device " << " : ";
            std::string deviceName = (*i).getInfo<CL_DEVICE_NAME>();
            std::cout << deviceName.c_str() << "\n";
        }
#endif
        std::cout << "\n";

    }

    if (deviceId == -1) {
        std::cerr << "Cant find CL device" << std::endl;
        exit(-5);
    }

    device.push_back(devices[deviceId]);

    context = cl::Context( device );
    commandQueue = cl::CommandQueue(context, devices[deviceId], 0,
                                    &err);
    CHECK_OPENCL_ERROR(err, "CommandQueue::CommandQueue() failed.");

    cl::ImageFormat imageFormat(CL_RGBA, CL_UNSIGNED_INT8);
    /*
    * Create and initialize memory objects
    */
    inputImage2D = cl::Image2D(context,
                               CL_MEM_READ_ONLY,
                               imageFormat,
                               width,
                               height,
                               0,
                               NULL,
                               &err);
    CHECK_OPENCL_ERROR(err, "Image2D::Image2D() failed. (inputImage2D)");


    // Create memory objects for output Image
    outputImage2D = cl::Image2D(context,
                                CL_MEM_WRITE_ONLY,
                                imageFormat,
                                width,
                                height,
                                0,
                                0,
                                &err);
    CHECK_OPENCL_ERROR(err, "Image2D::Image2D() failed. (outputImage2D)");

    // create a CL program using the kernel source


    std::string clSourceData = readFile(sampleArgs.clKernelPath.c_str());

    // create program source
    cl::Program::Sources programSource(1, std::make_pair(clSourceData.c_str(), clSourceData.length()));

    // Create program object
    program = cl::Program(context, programSource, &err);
    CHECK_OPENCL_ERROR(err, "Program::Program() failed.");



    std::string flagsStr = std::string("");

    if(flagsStr.size() != 0)
    {
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;
    }

    err = program.build( { device }, flagsStr.c_str());

    if(err != CL_SUCCESS)
    {
        if(err == CL_BUILD_PROGRAM_FAILURE)
        {
            std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[deviceId]);

            std::cout << " \n\t\t\tBUILD LOG\n";
            std::cout << " ************************************************\n";
            std::cout << str << std::endl;
            std::cout << " ************************************************\n";
        }
    }
    CHECK_OPENCL_ERROR(err, "Program::build() failed.");

    // Create kernel
    kernel = cl::Kernel(program, "sobel_filter",  &err);
    CHECK_OPENCL_ERROR(err, "Kernel::Kernel() failed.");

    // Check group size against group size returned by kernel
    kernelWorkGroupSize = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>
            (devices[deviceId], &err);
    CHECK_OPENCL_ERROR(err, "Kernel::getWorkGroupInfo()  failed.");

    if((blockSizeX * blockSizeY) > kernelWorkGroupSize)
    {
        std::cout << "Out of Resources!" << std::endl;
        std::cout << "Group Size specified : "
                  << blockSizeX * blockSizeY << std::endl;
        std::cout << "Max Group Size supported on the kernel : "
                  << kernelWorkGroupSize << std::endl;
        std::cout << "Falling back to " << kernelWorkGroupSize << std::endl;

        if(blockSizeX > kernelWorkGroupSize)
        {
            blockSizeX = kernelWorkGroupSize;
            blockSizeY = 1;
        }
    }

    return 0;
}


int SobelFilterImage::runCLKernels()
{
    cl_int status;

    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    cl::size_t<3> region;
    region[0] = width;
    region[1] = height;
    region[2] = 1;

    cl::Event writeEvt;
    status = commandQueue.enqueueWriteImage(
                inputImage2D,
                CL_TRUE,
                origin,
                region,
                0,
                0,
                inputImageData,
                NULL,
                &writeEvt);
    CHECK_OPENCL_ERROR(status,
                       "CommandQueue::enqueueWriteImage failed. (inputImage2D)");

    status = commandQueue.flush();
    CHECK_OPENCL_ERROR(status, "cl::CommandQueue.flush failed.");

    cl_int eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = writeEvt.getInfo<cl_int>(
                    CL_EVENT_COMMAND_EXECUTION_STATUS,
                    &eventStatus);
        CHECK_OPENCL_ERROR(status,
                           "cl:Event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS) failed.");

    }

    // Set appropriate arguments to the kernel
    // input buffer image
    status = kernel.setArg(0, inputImage2D);
    CHECK_OPENCL_ERROR(status, "Kernel::setArg() failed. (inputImageBuffer)");

    // outBuffer imager
    status = kernel.setArg(1, outputImage2D);
    CHECK_OPENCL_ERROR(status, "Kernel::setArg() failed. (outputImageBuffer)");

    /*
    * Enqueue a kernel run call.
    */
    cl::NDRange globalThreads(width, height);

    if (blockSizeX > width)
        blockSizeX = width;

    cl::NDRange localThreads(blockSizeX, blockSizeY);

    cl::Event ndrEvt;
    status = commandQueue.enqueueNDRangeKernel(
                kernel,
                cl::NullRange,
                globalThreads,
                localThreads,
                0,
                &ndrEvt);
    CHECK_OPENCL_ERROR(status, "CommandQueue::enqueueNDRangeKernel() failed.");

    status = commandQueue.flush();
    CHECK_OPENCL_ERROR(status, "cl::CommandQueue.flush failed.");

    eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = ndrEvt.getInfo<cl_int>(
                    CL_EVENT_COMMAND_EXECUTION_STATUS,
                    &eventStatus);
        CHECK_OPENCL_ERROR(status,
                           "cl:Event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS) failed.");
    }

    // Enqueue Read Image
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    region[0] = width;
    region[1] = height;
    region[2] = 1;

    // Enqueue readBuffer
    cl::Event readEvt;
    status = commandQueue.enqueueReadImage(
                outputImage2D,
                CL_FALSE,
                origin,
                region,
                0,
                0,
                outputImageData,
                NULL,
                &readEvt);
    CHECK_OPENCL_ERROR(status, "CommandQueue::enqueueReadImage failed.");

    status = commandQueue.flush();
    CHECK_OPENCL_ERROR(status, "cl::CommandQueue.flush failed.");

    eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = readEvt.getInfo<cl_int>(
                    CL_EVENT_COMMAND_EXECUTION_STATUS,
                    &eventStatus);
        CHECK_OPENCL_ERROR(status,
                           "cl:Event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS) failed.");

    }

    return 0;
}


int SobelFilterImage::setup()
{
    int status = setupCL();
    if (status != 0)
    {
        return status;
    }

    return 0;

}


int SobelFilterImage::run()
{

    for(int i = 0; i < iterations; i++)
    {
        // Set kernel arguments and run kernel
        if(runCLKernels() != 0)
        {
            return -1;
        }

    }

    return 0;
}


#define TAG_IMAGE_SIZE 1
#define TAG_IMAGE_DATA 2
#define TAG_IMAGE_RESULT 3

class MPIComputing {
private:
    std::vector<PngImage> images;
    int currentRank;
    int currentSize;
    int peerCount;
    MPI::Intercomm comm;

    struct {
        std::string deviceType = "cpu";
        std::string inputImagePath = "input";
        std::string outputImagePath = "output";
        std::string clKernelPath = "SobelFilterImage_Kernels.cl";
        bool isThereGPU = false;

        int parseCmdLine(int argc, char** argv)
        {
            for (int i=1; i<argc; i++)
            {
                switch (i) {
                //TODO: parse options passed as arguments
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    std::cerr << "Invalid cmdline arguments" << std::endl;
                    exit(-4);
                }
            }
            return 0;
        }

    } options;


    struct Size{ int width; int height; };

    int peerComputing() {
        Size size;
        comm.Recv(&size, 2, MPI_INTEGER, 0, TAG_IMAGE_SIZE);
        int pixCount = size.height * size.width;
        std::cout << "Peer " << currentRank << " image size received: " << size.width << "x" << size.height << std::endl;

        if (pixCount != 0) {
            cl_uint *buffer = new cl_uint[pixCount];
            comm.Recv(buffer, pixCount, MPI_INTEGER, 0, TAG_IMAGE_DATA);
            std::cout << "Peer " << currentRank << " image data recieved" << std::endl;

            SobelFilterImage clSobelFilterImage;
            clSobelFilterImage.setInputBuffer(buffer, size.width, size.height);

            if( clSobelFilterImage.setup() != 0)
                return -1;

            std::cout << "Peer " << currentRank  << " computation start time:";
            OS::printTime();
            if(clSobelFilterImage.run() != 0)
                return -2;

            std::cout << "Peer " << currentRank  << " computation stop time:";
            OS::printTime();

            comm.Send(clSobelFilterImage.getOutputBuffer(), pixCount, MPI_INTEGER, 0, TAG_IMAGE_RESULT);


        }

        return 0;
    }

    int rootComputing() {
        auto listing = OS::getDirListing(options.inputImagePath.c_str());

        std::vector<PngImage> images;
        typedef std::shared_ptr<cl_uint> ImageDataPtr;
        std::vector<ImageDataPtr> imageData;

        for (int i=0; i<listing.size() && i<currentSize-1; i++) {
            std::string filePath = options.inputImagePath + "/" + listing[i];
            std::cout << "Opening image file " << filePath.c_str() << std::endl;

            PngImage image(filePath.c_str());
            images.push_back(image);
            ImageDataPtr dataPtr = image.getData();
            imageData.push_back(dataPtr);
        }

        std::cout << "Opened " << images.size() << " files" << std::endl;

        std::vector<MPI::Request> requests;

        std::cout << "Sendign data to peers\n";
        for (int i=1; i<currentSize; i++) {

            ImageDataPtr dataPtr = imageData[i-1];

            Size size;
            if (i > images.size())
                size = { 0, 0 };
            else {
                size.width = images[i-1].getWidth();
                size.height = images[i-1].getHeight();
            }

            std::cout << "Sending image size for peer " << i << std::endl;
            comm.Isend(&size, 2, MPI_INTEGER, i, TAG_IMAGE_SIZE);

            if(size.width != 0 && size.height != 0) {
                comm.Isend( (void*) dataPtr.get(), size.width*size.height, MPI_INTEGER, i, TAG_IMAGE_DATA);
                std::cout << "Sended initial data to peer " << i << std::endl;
            }
        }

        std::cout << "Receiving data from peers\n";
        std::vector<ImageDataPtr> buffers;
        for (int i=1; i<currentSize; i++) {
            size_t pixCount = images[i-1].getWidth() * images[i-1].getHeight();
            ImageDataPtr bufferPtr(new cl_uint[pixCount]);

            if (pixCount != 0) {
                MPI::Request r3 = comm.Irecv(bufferPtr.get(), pixCount, MPI_INTEGER, i, TAG_IMAGE_RESULT);
                requests.push_back(r3);
                buffers.push_back(bufferPtr);
            }
            else
                break;
        }

        for (int i=0; i<requests.size(); i++) {
            requests[i].Wait();
        }

        std::cout << "All communication operations done" << std::endl;

        for (int i=0; i<currentSize-1; i++) {
            //TODO: exit if no tasks for this peer was planed
            size_t size = images[i].getWidth() * images[i].getHeight();
            images[i].setData(buffers[i].get());
            std::string name = options.outputImagePath + "/" + std::to_string(i) +".png";
            std::cout << "Writing file to " << name << std::endl;
            images[i].write(name.c_str());
        }

        std::cout << "Done\n";
    }
public:

    int start(int argc, char** argv) {
        MPI::Init(argc, argv);

        comm = MPI::COMM_WORLD;

        currentRank = comm.Get_rank();
        currentSize = comm.Get_size();

        peerCount = currentSize - 1;

        if (currentSize<2) {
            std::cerr << ("At least 2 peers may instantiated and work properly!\n") << std::endl;
            return -2;
        }

        options.parseCmdLine(argc, argv);

        if (currentRank == 0)
            rootComputing();
        else
            peerComputing();

        MPI::Finalize();
        return 0;
    }
};


int main(int argc, char * argv[])
{
    MPIComputing c;
    return c.start(argc, argv);
}



