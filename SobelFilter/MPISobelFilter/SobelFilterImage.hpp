#ifndef SOBEL_FILTER_IMAGE_H_
#define SOBEL_FILTER_IMAGE_H_

#include <CL/cl.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "pngimage.h"

#define GROUP_SIZE 256

#define CHECK_OPENCL_ERROR(x, message) if (x) { std::cerr << message << std::endl; exit(x); }
#define OPENCL_EXPECTED_ERROR(message) { std::err << message << std::endl; exit(-2); }
#define CHECK_ERROR(x, y, message) if (x!=y) { { std::cerr << message << std::endl; exit(-3); }

class SobelFilterImage
{
        cl_uint* inputImageData;          /**< Input bitmap data to device */
        cl_uint* outputImageData;         /**< Output from device */
        cl::Context context;                            /**< CL context */
        std::vector<cl::Device> devices;                /**< CL device list */
        std::vector<cl::Device> device;                 /**< CL device to be used */
        std::vector<cl::Platform> platforms;            /**< list of platforms */
        cl::Image2D inputImage2D;                       /**< CL Input image2d */
        cl::Image2D outputImage2D;                      /**< CL Output image2d */
        cl::CommandQueue commandQueue;                  /**< CL command queue */
        cl::Program program;                            /**< CL program  */
        cl::Kernel kernel;                              /**< CL kernel */

        std::shared_ptr<cl_uint> pixelDataPtr;       /**< Pointer to image data */
        cl_uint pixelSize;                  /**< Size of a pixel in BMP format> */
        cl_uint width;                      /**< Width of image */
        cl_uint height;                     /**< Height of image */
        cl_bool byteRWSupport;
        size_t kernelWorkGroupSize;         /**< Group Size returned by kernel */
        size_t blockSizeX;                  /**< Work-group size in x-direction */
        size_t blockSizeY;                  /**< Work-group size in y-direction */
        int iterations;                     /**< Number of iterations for kernel execution */
        PngImage m_image;

    public:

        // this structure store options passed via cmdline interface to the programm
        struct {
            std::string deviceType = "cpu";
            std::string clKernelPath = "SobelFilterImage_Kernels.cl";
            bool isThereGPU = false;


            int parseCmdLine(int argc, char** argv)
            {
                for (int i=1; i<argc; i++)
                {
                    switch (i) {
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

        } sampleArgs;


        SobelFilterImage()
            : inputImageData(nullptr),
              outputImageData(nullptr),
              byteRWSupport(true)
        {
            pixelSize = sizeof(cl_char4);
            blockSizeX = GROUP_SIZE;
            blockSizeY = 1;
            iterations = 1;
        }

        ~SobelFilterImage()
        {
            if (inputImageData != nullptr)
                delete inputImageData;

            if (outputImageData != nullptr)
                delete outputImageData;
        }


        cl_uint* getOutputBuffer();

        int setInputBuffer(cl_uint* buf, int width, int height);

        int setupSobelFilterImage();

        int genBinaryImage();

        int setupCL();

        int runCLKernels();

        int setup();

        int run();

        std::string readFile(const char *fileName);
};

#endif // SOBEL_FILTER_IMAGE_H_
