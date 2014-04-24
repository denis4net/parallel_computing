#include <iostream>
#include <cstdlib>
#include <cstring>
#include <CL/cl.hpp>

using namespace std;

#include "pngimage.h"
#include "filtermedian.h"
#include "clmanager.h"

int main(int argc, char** argv)
{

    cout << "Initializing PngImage object" << endl;

    const char* clKernelPath = argv[1];
    const char* imagePath = argv[2];
    const int streamsCount = std::atoi(argv[3]);
    int type = (!std::strcmp(argv[4], "gpu")) ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU;

    cout << "Kernel path: " << clKernelPath << "; image path: " << imagePath
         << "; queues count: " << streamsCount
         << " device type: " << argv[4] << ' ' << type << std::endl;

    FilterMedian f(clKernelPath);
    f.setStreamsCount(streamsCount);
    CLManager::getInstance()->setType(type);

    PngImage img(imagePath);
    img.write("/tmp/readed_image.png");

    PngImage rImage;

    try {
        rImage = f.apply(img);
        rImage.write("/tmp/processed_image.png");
    } catch(std::string e)
    {
        std::cerr << e << std::endl;
    }

    return 0;
}
