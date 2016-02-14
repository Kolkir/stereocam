#ifndef DEPTHMAPBUILDER_H
#define DEPTHMAPBUILDER_H

#include "framesource.h"

#include <opencv2/opencv.hpp>

#include <mutex>
#include <thread>

class DepthMapBuilder : public FrameSource
{
public:
    DepthMapBuilder();

     ~DepthMapBuilder();

    DepthMapBuilder(const DepthMapBuilder&) = delete;

    DepthMapBuilder& operator=(const DepthMapBuilder&) = delete;


    void setLeftSource(FrameSource& source);
    void setRightSource(FrameSource& source);

    void getFrame(cv::Mat& map) override;

    void startProcessing();

    void stopProcessing();

private:

    void processing();

private:
    cv::StereoSGBM sgbm;
    FrameSource* leftSource;
    FrameSource* rightSource;
    cv::Mat depthMap;

    std::mutex outGuard;
    std::mutex processGuard;

    std::thread thread;
    bool stop;
};

#endif // DEPTHMAPBUILDER_H
