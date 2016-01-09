#include "v4lcamera.h"

#include <stdexcept>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <linux/videodev2.h>

namespace camera {
namespace utils {

const unsigned vide_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

V4LCamera::ScopedMMapBuffer::ScopedMMapBuffer()
    : buffer(nullptr)
    , length(0)
{

}

V4LCamera::ScopedMMapBuffer::~ScopedMMapBuffer()
{
    if(buffer != nullptr && length != 0)
    {
        munmap(buffer, length);
    }
}

void V4LCamera::ScopedMMapBuffer::init(void* buf, size_t length)
{
    this->buffer = buf;
    this->length  = length;
}

V4LCamera::V4LCamera(int devId, const VideoDevFormat &format)
    : device(devId)
    , capturing(false)
{
    //setup format
    struct v4l2_format v4lformat;
    v4lformat.type = vide_type;
    v4lformat.fmt.pix.pixelformat = format.pixelformat;
    v4lformat.fmt.pix.width = format.width;
    v4lformat.fmt.pix.height = format.height;

    if(ioctl(device.fd(), VIDIOC_S_FMT, &v4lformat) < 0)
    {
        throw std::runtime_error("Unable to set device format : " + format.description);
    }

    //memory buffers setup
    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = vide_type;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 1;

    if(ioctl(device.fd(), VIDIOC_REQBUFS, &bufrequest) < 0)
    {
        throw std::runtime_error("Unable request memory buffers for device : " + device.fileName());
    }

    //get size of the reqired buffer
    struct v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = vide_type;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    if(ioctl(device.fd(), VIDIOC_QUERYBUF, &bufferinfo) < 0)
    {
        throw std::runtime_error("Unable request size of memory buffers for device : " + device.fileName());
    }

    //map memory
    char* buffer_start = static_cast<char*>(mmap(
        NULL,
        bufferinfo.length,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        device.fd(),
        bufferinfo.m.offset
    ));

    if(buffer_start == MAP_FAILED)
    {
        throw std::runtime_error("Unable map memory buffers for device : " + device.fileName());
    }
    else
    {
        //wrap to RAII
        buffer.init(buffer_start, bufferinfo.length);
    }

    //clear emory
    memset(buffer_start, 0, bufferinfo.length);
}

V4LCamera::~V4LCamera()
{
    stopCapture();
}

void V4LCamera::startCapture()
{
    auto type = vide_type;
    if(ioctl(device.fd(), VIDIOC_STREAMON, &type) < 0)
    {
        throw std::runtime_error("Unable to strat streaming device : " + device.fileName());
    }
    capturing = true;
}

void V4LCamera::stopCapture()
{
    if (capturing)
    {
        auto type = vide_type;
        if(ioctl(device.fd(), VIDIOC_STREAMOFF, &type) < 0)
        {
            throw std::runtime_error("Unable to stop streaming device : " + device.fileName());
        }
        capturing = false;
    }
}

void V4LCamera::getFrame(std::vector<char> &buffer)
{
    struct v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = vide_type;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;    

    // Put the buffer in the incoming queue.
    if(ioctl(device.fd(), VIDIOC_QBUF, &bufferinfo) < 0)
    {
       throw std::runtime_error("Unable to put buffer in device queue : " + device.fileName());
    }

    // The buffer's waiting in the outgoing queue.
    if(ioctl(device.fd(), VIDIOC_DQBUF, &bufferinfo) < 0)
    {
        throw std::runtime_error("Unable to query buffer from device : " + device.fileName());
    }

    buffer.resize(this->buffer.length);
    memcpy(buffer.data(), this->buffer.buffer, this->buffer.length);    
}



}}
