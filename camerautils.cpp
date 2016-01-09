#include "camerautils.h"

#include <stdexcept>

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>

namespace camera {
namespace utils {


ScopedVideoDevice::ScopedVideoDevice(int id)
{
    auto devName = "/dev/video" + std::to_string(id);
    int fd = open(devName.c_str(), O_RDWR);
    if((fd) < 0)
    {
        throw std::runtime_error("Unable open video device : " + devName);
    }
    devFileName = devName;
    fileDesc = fd;
}

ScopedVideoDevice::~ScopedVideoDevice()
{
    close(fileDesc);
}

const std::string& ScopedVideoDevice::fileName() const
{
    return devFileName;
}

int ScopedVideoDevice::fd() const
{
    return fileDesc;
}


std::string getDeviceName(int id)
{
    std::string devName;
    try
    {
        ScopedVideoDevice dev(id);
        struct v4l2_capability capabilities;
        if(ioctl(dev.fd(), VIDIOC_QUERYCAP, &capabilities) >= 0)
        {
            devName = reinterpret_cast<char*>(capabilities.card);
        }
    }
    catch(...)
    {
        //ignore, return empty string
    }

    return devName;
}

std::vector<VideoDevId> getDeviceList()
{
    std::vector<VideoDevId> list;
    struct dirent* epdf = nullptr;
    DIR* dpdf = opendir("/dev/");
    if (dpdf != nullptr)
    {
        VideoDevId dev;
        while ((epdf = readdir(dpdf)) != nullptr)
        {
            std::string fileName(epdf->d_name);
            if (!fileName.empty())
            {
                auto pos = fileName.find("video");
                if (pos == 0)
                {
                    dev.id = std::atoi(fileName.substr(5).c_str());
                    dev.name = getDeviceName(dev.id);
                    list.push_back(dev);
                }
            }
        }
        closedir(dpdf);
    }
    return list;
}

std::vector<VideoDevFormat> getDeviceFormats(int id)
{
    std::vector<VideoDevFormat> formats;
    try
    {
        ScopedVideoDevice dev(id);
        VideoDevFormat format;
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        for(;;)
        {
            auto err = ioctl(dev.fd(), VIDIOC_ENUM_FMT, &fmtdesc);
            if  (err >= 0)
            {
                format.description = reinterpret_cast<char*>(fmtdesc.description);
                format.pixelformat = fmtdesc.pixelformat;
                //enumerate frame sizes
                v4l2_frmsizeenum frmsize;
                frmsize.pixel_format = fmtdesc.pixelformat;
                frmsize.index = 0;
                for(;;)
                {
                    err = ioctl(dev.fd(), VIDIOC_ENUM_FRAMESIZES, &frmsize);
                    if (err >= 0)
                    {
                        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                        {
                            format.width = frmsize.discrete.width;
                            format.height = frmsize.discrete.height;
                            format.description += " " + std::to_string(format.width) + "x" + std::to_string(format.height);
                            formats.push_back(format);
                        }
                    }
                    else
                    {
                        break;
                    }
                    frmsize.index += 1;
                }
            }
            else
            {
                break;
            }
            fmtdesc.index += 1;
        }
    }
    catch(...)
    {
        //ignore, return empty list
    }

    return formats;
}



}}
