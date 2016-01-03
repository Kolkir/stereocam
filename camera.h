#ifndef CAMERA_H
#define CAMERA_H

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

    static int getDeviceCount();

    static bool calibrate(int squareSize,
                          int wcount,
                          int hcount,
                          const std::vector<std::string>& files,
                          const std::string& fileName);

    void setFrameCallback(std::function<void (cv::Mat&)> func);

    bool startCapture(int cameraId);

    void stopCapture();

    cv::Size getResolution() const;

    void setResolution(const cv::Size& resolution);

    void takeSnapshoot(const std::string& fileName);

    bool canTakeSnapshoot() const;

private:

    void capturing(int cameraId);

private:

    std::function<void (cv::Mat&)> frameCallback;
    mutable std::mutex guard;
    std::thread thread;
    bool stop;
    std::promise<bool> captureStarted;

    cv::Size resolution;
    cv::Size actualResolution;

    mutable std::mutex snapGuard;
    bool freeForSnap;
    std::string snapFileName;
};

#endif // CAMERA_H
