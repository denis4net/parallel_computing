#ifndef CLMANAGER_H
#define CLMANAGER_H

#include <CL/cl.hpp>
#include <memory>

class CLManager
{
public:
    static CLManager *getInstance();
    std::shared_ptr<cl::Device> getDevice();
    int type() const;
    void setType(int type);

private:
    int m_type;
    CLManager();
    static CLManager *instance;
    std::shared_ptr<cl::Device> devicePtr;
};

#endif // CLMANAGER_H
