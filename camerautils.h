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
    VideoDevFormat() : pixelformat(0), width(0), height(0) {}
    std::string description;
    unsigned int pixelformat;
    unsigned int width;
    unsigned int height;
};

std::vector<VideoDevFormat> getDeviceFormats(int id);

class ScopedVideoDevice
{
public:
    ScopedVideoDevice(int id);
    ~ScopedVideoDevice();
    ScopedVideoDevice(const ScopedVideoDevice&) = delete;
    ScopedVideoDevice& operator=(const ScopedVideoDevice&) = delete;

    int fd() const;
    const std::string& fileName() const;
private:
    std::string devFileName;
    int fileDesc;
};

bool calibrate(int squareSize,
               int wcount,
               int hcount,
               const std::vector<std::string>& files,
               const std::string& fileName);

bool stereoCalibrate(int squareSize,
                     int wcount,
                     int hcount,
                     const std::vector<std::string>& filesLeft,
                     const std::vector<std::string>& filesRight,
                     const std::string& fileName);

struct CameraParameter
{
    int id;
    std::string name;
    int minimum;
    int maximum;
    int value;
    CameraParameter();
    CameraParameter(int id, const std::string& name, int minimum, int maximum, int value);
};

void getCameraParameters(int fd, std::vector<CameraParameter>& parameters);

void setCameraParameter(int fd, const CameraParameter& param);

}}

#endif
