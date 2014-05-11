#ifndef SOBEL_FILTER_IMAGE_H_
#define SOBEL_FILTER_IMAGE_H_

#include <CL/cl.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "pngimage.h"

#define GROUP_SIZE 256

#define CHECK_OPENCL_ERROR(x, message) if (x) { std::cerr << message << '(' << x << ')' << std::endl; exit(x); }
#define OPENCL_EXPECTED_ERROR(message) { std::err << message << std::endl; exit(-2); }
#define CHECK_ERROR(x, y, message) if (x!=y) { { std::cerr << message << std::endl; exit(-3); }

class OpenCLEngine
{
public:

    struct {
        std::string deviceType = "gpu";
    } sampleArgs;


    OpenCLEngine()
        : byteRWSupport(true)
    {
        pixelSize = sizeof(cl_uint);
        blockSizeX = GROUP_SIZE;
        blockSizeY = 1;
        counter = 0;
    }

    ~OpenCLEngine()
    {
    }

    int setup() {
        cl_int err;

        findSuitableDevice();
        initContext();

        copyKernel = initKernel("Copy_Kernels.cl");
        noiseKernel = initKernel("Noise_Kernels.cl");
        filterKernel = initKernel("Median_Kernels.cl");

        kernelWorkGroupSize = filterKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device[0], &err);
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
    }

    int runCLFilter()
    {
        return runKernel(textures[2], textures[3], filterKernel, "Filtering");
    }

    int runCLCopy() {
        return runKernel(textures[1], textures[2], copyKernel, "Texture coping");
    }

    int runCLNoise(cl_uint seed, float f=0.5)
    {
        noiseKernel.setArg(2, seed);
        noiseKernel.setArg(3, f);
        return runKernel(textures[0], textures[1], noiseKernel,"Noise genearating");
    }

    int run()
    {
        return runCLFilter();
    }

    int addTextures(GLuint textureID[], int w, int h)
    {
        int errcode, i;
        textures.clear();

        for (i=0; textureID[i] != ~0; i++) {
            cl::ImageGL tmp = cl::ImageGL(context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, textureID[i], &errcode);
            CHECK_OPENCL_ERROR(errcode, "Can't create cl::ImageGL");
            textures.push_back(tmp);
        }

        width = w;
        height = h;

        if (blockSizeX > width)
            blockSizeX = width;

        return i;
    }

private:
        cl::Context context;                            /**< CL context */
        std::vector<cl::Device> device;                 /**< CL device to be used */
        cl::Platform platform;
        std::vector<cl::ImageGL> textures;
        cl::Kernel filterKernel, noiseKernel, copyKernel;   /**< CL kernel */

        cl_uint pixelSize;                  /**< Size of a pixel in BMP format> */
        cl_uint width;                      /**< Width of image */
        cl_uint height;                     /**< Height of image */
        cl_bool byteRWSupport;
        size_t kernelWorkGroupSize;         /**< Group Size returned by kernel */
        size_t blockSizeX;                  /**< Work-group size in x-direction */
        size_t blockSizeY;                  /**< Work-group size in y-direction */

        int counter;


        cl::Device findSuitableDevice() {
            std::vector<cl::Platform> platforms;
            std::vector<cl::Device> devices;

            cl_int err = CL_SUCCESS;
            cl_device_type dType;

            if(sampleArgs.deviceType.compare("cpu") == 0)
                dType = CL_DEVICE_TYPE_CPU;
            else //deviceType = "gpu"
                dType = CL_DEVICE_TYPE_GPU;


            err = cl::Platform::get(&platforms);
            CHECK_OPENCL_ERROR(err, "Platform::get() failed.");

            std::vector<cl::Platform>::iterator platformIter;

            bool found = false;

            for (platformIter=platforms.begin(); platformIter!=platforms.end() && !found; platformIter++)
            {
                std::cout << "Platform: " << platformIter->getInfo<CL_PLATFORM_NAME>().c_str() << std::endl;

                devices.clear();
                platformIter->getDevices(dType, &devices);

                if (devices.size() < 1)
                    continue;

                std::vector<cl::Device>::iterator devItr;
                int devNum;
                for (devItr = devices.begin(), devNum = 0; devItr != devices.end() && !found; ++devItr, ++devNum) {
                    int devSupportImage = devItr->getInfo<CL_DEVICE_IMAGE_SUPPORT>();

                    if (devSupportImage) {
                        std::cout << "Suitable device was found" << std::endl;
                        std::cout << "Device " << " : ";
                        std::string deviceName = (*devItr).getInfo<CL_DEVICE_NAME>();
                        std::cout << deviceName.c_str() << "\n";
                        std::cout << "\tCL_DEVICE_SUPPORT_IMAGE: " <<devSupportImage << std::endl;

                        device.push_back(*devItr);
                        platform = *platformIter;
                        found = true;
                    }
                }
            }

            CHECK_OPENCL_ERROR(!found, "Can't find suitable device");
            return device[0];
        }

        int initContext()
        {
            cl_int err;
            //preparing context
            Display * display = XOpenDisplay(NULL);
            cl_context_properties properties[] = {
                CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()),
                CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(glXGetCurrentContext()),
                CL_GLX_DISPLAY_KHR, reinterpret_cast<cl_context_properties>(display),
                0
            };

            context = cl::Context(device, properties, NULL, NULL, &err);
            CHECK_OPENCL_ERROR(err, "can't create cl::Context");

            return 0;
        }

        cl::Kernel initKernel(const char* path) {
            cl_int err;

            // create a CL program using the kernel source
            std::string clSourceData = readFile(path);

            // create program source
            cl::Program::Sources programSource(1, std::make_pair(clSourceData.c_str(), clSourceData.length()));

            // Create program object
            cl::Program program = cl::Program(context, programSource, &err);
            CHECK_OPENCL_ERROR(err, "Program::Program() failed.");

            std::string flagsStr = std::string("");
            err = program.build( { device }, flagsStr.c_str());

            if(err != CL_SUCCESS)
            {
                if(err == CL_BUILD_PROGRAM_FAILURE)
                {
                    std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device[0]);

                    std::cout << " \n\t\t\tBUILD LOG\n";
                    std::cout << " ************************************************\n";
                    std::cout << str << std::endl;
                    std::cout << " ************************************************\n";
                }
            }
            CHECK_OPENCL_ERROR(err, "Program::build() failed.");

            // Create kernel
            cl::Kernel kernel = cl::Kernel(program, "main",  &err);
            CHECK_OPENCL_ERROR(err, "Kernel::Kernel() failed.");

            return kernel;
        }

        int runKernel(cl::ImageGL src, cl::ImageGL dst, cl::Kernel &kernel, const char* description="OpenCL kernel running")
        {
            std::cout << description << ": ";
            cl_int status, err;

            cl::size_t<3> origin;
            origin[0] = 0;
            origin[1] = 0;
            origin[2] = 0;

            cl::size_t<3> region;
            region[0] = width;
            region[1] = height;
            region[2] = 1;

            std::vector<cl::Memory> memObjects;
            memObjects.push_back(src);
            memObjects.push_back(dst);

            cl::Event event;
            cl::CommandQueue commandQueue = cl::CommandQueue(context, device[0], 0, &err);
            CHECK_OPENCL_ERROR(err, "CommandQueue::CommandQueue() failed.");

            glFinish();
            commandQueue.enqueueAcquireGLObjects(&memObjects, NULL, &event);
            event.wait();

            status = kernel.setArg(0, src); CHECK_OPENCL_ERROR(status, "Kernel::setArg() failed. (inputImageBuffer)");
            status = kernel.setArg(1, dst); CHECK_OPENCL_ERROR(status, "Kernel::setArg() failed. (inputImageBuffer)");

            cl::NDRange globalThreads(width, height);
            cl::NDRange localThreads(blockSizeX, blockSizeY);

            cl::Event ndrEvt;
            status = commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, globalThreads, localThreads, NULL, &ndrEvt);
            CHECK_OPENCL_ERROR(status, "CommandQueue::enqueueNDRangeKernel() failed.");

            commandQueue.finish();

            commandQueue.enqueueReleaseGLObjects(&memObjects, NULL, &event);
            event.wait();
            std::cout << " done" << std::endl;
            return 0;
        }

        std::string readFile(const char *fileName)
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

};

#endif // SOBEL_FILTER_IMAGE_H_
