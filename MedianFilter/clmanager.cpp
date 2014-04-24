#include "clmanager.h"
#include <iostream>

CLManager* CLManager::instance = nullptr;

CLManager::CLManager(): m_type(CL_DEVICE_TYPE_GPU)
{

}


CLManager *CLManager::getInstance()
{
    if (!instance) {
        instance = new CLManager();
    }
    return instance;
}

std::shared_ptr<cl::Device> CLManager::getDevice()
{
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);

    if(all_platforms.size()==0){
        std::cerr << "No platforms found. Check OpenCL installation!\n";
        exit(1);
    }

    for (auto i=all_platforms.begin(); i != all_platforms.end(); i++)
    {
        std::vector<cl::Device> devices;
        i->getDevices(m_type, &devices);
        if (!devices.empty())
        {
            cl::Device *device = new cl::Device();
            *device = devices[0];
            devicePtr.reset(device);
            std::cout << "Found device " << device->getInfo<CL_DEVICE_NAME>() << std::endl;
            break;
        }
    }


    return devicePtr;
}
int CLManager::type() const
{
    return m_type;
}

void CLManager::setType(int type)
{
    m_type = type;
}

