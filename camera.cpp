#include "camera.h"

#include "v4lcamera.h"

#include <iostream>

Camera::Camera()
    : stop(false)
    , freeForSnap(true)
{
}

Camera::~Camera()
{
    stopCapture();
}

void Camera::setFrameCallback(std::function<void (cv::Mat&)> func)
{
    std::unique_lock<std::mutex> lock(guard);
    frameCallback = func;
}

void Camera::capturing(int cameraId, camera::utils::VideoDevFormat format)
{
    try
    {
        camera::utils::V4LCamera camera(cameraId, format);

        camera.startCapture();

        captureStarted.set_value(true);
        std::vector<char> frameBuffer;
        cv::Mat frame;
        bool done = false;
        while(!done)
        {
            try
            {
                camera.getFrame(frameBuffer);
                frame = cv::imdecode(frameBuffer, CV_LOAD_IMAGE_UNCHANGED);

                //take snapshoot
                {
                    std::unique_lock<std::mutex> lock(snapGuard);
                    if (!freeForSnap && !snapFileName.empty())
                    {
                        cv::imwrite(snapFileName, frame);
                        snapFileName.clear();
                        freeForSnap = true;
                    }
                }                

                if (frameCallback)
                {
                    frameCallback(frame);
                }
                done = stop;
            }
            catch(std::exception& err)
            {
                std::cerr << err.what() << std::endl;
                lastError = err.what();
                break;
            }
        }

        camera.stopCapture();
    }
    catch(std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        lastError = err.what();
        captureStarted.set_value(false);
    }
}

int Camera::getId() const
{
    return cameraId;
}

bool Camera::startCapture(int cameraId, const camera::utils::VideoDevFormat& format)
{
    this->cameraId = cameraId;

    stopCapture();

    stop = false;
    captureStarted = std::move(std::promise<bool>());

    auto wait = captureStarted.get_future();

    thread = std::move(std::thread(std::bind(&Camera::capturing, this, cameraId, format)));

    try
    {
        bool res = wait.get();
        return res;
    }
    catch(...)
    {
        return false;
    }
}

void Camera::stopCapture()
{
    {
        std::unique_lock<std::mutex> lock(guard);
        stop = true;
    }

    if (thread.joinable())
    {
        thread.join();
    }
}


void Camera::takeSnapshoot(const std::string &fileName)
{
    std::unique_lock<std::mutex> lock(snapGuard);
    if (freeForSnap)
    {
        snapFileName = fileName;
        freeForSnap = false;
    }

}

bool Camera::canTakeSnapshoot() const
{
   std::unique_lock<std::mutex> lock(snapGuard);
   return freeForSnap;
}
