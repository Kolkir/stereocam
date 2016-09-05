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

    void setApplyNoiseFilter(bool filter);

    void setDrawLines(bool drawLines);

    bool isUndistortApplied() const;

    bool isNoiseFilterApplied() const;

    bool isDrawLines() const;

    bool loadCalibrationParams(const std::string& fileName);

    void setUndistortMappings(const cv::Mat& mapx, const cv::Mat& mapy, const cv::Rect& roi);

private:

    void processing();

private:

    double outScaleFactor;
    int outChannel;
    bool outGray;
    bool outUndistort;
    bool outDrawLines;
    bool outNoiseFilter;
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    cv::Mat mapx;
    cv::Mat mapy;
    cv::Rect remapRoi;

    cv::Mat frame;
    cv::Mat outFrame;

    std::mutex outGuard;
    std::mutex processGuard;

    std::thread thread;
    bool stop;

};

#endif // FRAMEPROCESSOR_H
