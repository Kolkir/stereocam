#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include "framesource.h"

#include <opencv2/opencv.hpp>

#include <memory>
#include <mutex>
#include <thread>
#include <future>

class FrameProcessor : public FrameSource
{
public:
    FrameProcessor();

    ~FrameProcessor();

    FrameProcessor(const FrameProcessor&) = delete;

    FrameProcessor& operator=(const FrameProcessor&) = delete;

    void setFrame(const cv::Mat frame);

    void getFrame(cv::Mat& frame) override;

    void startProcessing();

    void stopProcessing();

    void setOutScaleFactor(double factor);

    void setOutChannel(int index);

    void setOutGray(bool gray);

    void setApplyUndistort(bool undistort);

    bool isUndistortApplied() const;

    bool loadCalibrationParams(const std::string& fileName);

private:

    void processing();

private:

    double outScaleFactor;
    int outChannel;
    bool outGray;
    bool outUndistort;
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;

    cv::Mat frame;
    cv::Mat outFrame;

    std::mutex outGuard;
    std::mutex processGuard;

    std::thread thread;
    bool stop;

};

#endif // FRAMEPROCESSOR_H
