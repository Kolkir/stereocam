#ifndef _DEVICE_H_
#define _DEVICE_H_

namespace cuda {

class Device final
{
public:
    Device();
    ~Device();
    void printInfo();
private:
    int device;
};

}

#endif

