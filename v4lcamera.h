#ifndef V4LCAMERA_H
#define V4LCAMERA_H

#include "camerautils.h"

#include <memory>
#include <functional>

namespace camera {
namespace utils {


class V4LCamera
{
public:
    V4LCamera(int devId, const VideoDevFormat& format);
    V4LCamera(const V4LCamera&) = delete;
    V4LCamera& operator=(const V4LCamera&) = delete;
    ~V4LCamera();

    void startCapture();
    void stopCapture();

    void getFrame(std::vector<char>& buffer);

    void setParameter(const CameraParameter& param);

private:
    ScopedVideoDevice device;

    class ScopedMMapBuffer
    {
    public:
        ScopedMMapBuffer();
        ~ScopedMMapBuffer();
        void init(void* buf, size_t length);

        void* buffer;
        size_t length;
    };

    ScopedMMapBuffer buffer;
    bool capturing;
};

}}
#endif // V4LCAMERA_H
