#ifndef CAMERAUTILS_H
#define CAMERAUTILS_H

#include <vector>
#include <string>

namespace camera { 
namespace utils {

struct VideoDevId
{
    VideoDevId() : id(-1) {}
    VideoDevId(int id, const std::string& name) : id(id), name(name) {}

    int id;
    std::string name;
};

std::vector<VideoDevId> getDeviceList();

struct VideoDevFormat
{
    VideoDevFormat() : format(0), width(0), height(0) {}
    std::string description;
    unsigned int format;
    unsigned int width;
    unsigned int height;
};

std::vector<VideoDevFormat> getDeviceFormats(int id);

}}

#endif
