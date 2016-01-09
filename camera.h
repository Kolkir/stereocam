#ifndef CAMERA_H
#define CAMERA_H

#include "camerautils.h"

#include <opencv2/opencv.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <future>

class Camera
{
public:
    Camera();

    ~Camera();

    Camera(const Camera&) = delete;

    Camera& operator=(const Camera&) = delete;

    void setFrameCallback(std::function<void (cv::Mat&)> func);

    bool startCapture(int cameraId, const camera::utils::VideoDevFormat& format);

    void stopCapture();

    void takeSnapshoot(const std::string& fileName);

    bool canTakeSnapshoot() const;

private:

    void capturing(int cameraId, camera::utils::VideoDevFormat format);

private:

    std::function<void (cv::Mat&)> frameCallback;
    mutable std::mutex guard;
    std::thread thread;
    bool stop;
    std::promise<bool> captureStarted;

    mutable std::mutex snapGuard;
    bool freeForSnap;
    std::string snapFileName;

    std::string lastError;
};

#endif // CAMERA_H
