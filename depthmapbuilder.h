#ifndef DEPTHMAPBUILDER_H
#define DEPTHMAPBUILDER_H

#include "framesource.h"

#include <opencv2/opencv.hpp>
#include <opencv2/cudastereo.hpp>

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

    bool loadCalibrationParams(const std::string& fileName);

    //configuration
    int getNumDisparities() const;
    void setNumDisparities(int numDisparities);

    int getBlockSize() const;
    void setBlockSize(int blockSize);

    int getTextureThreshold() const;
    void setTextureThreshold(int textureThreshold);

    //calibration

    void getLeftMapping(const cv::Size& imgSize, cv::Mat& mapx, cv::Mat& mapy, cv::Rect& roi);
    void getRightMapping(const cv::Size& imgSize, cv::Mat& mapx, cv::Mat& mapy, cv::Rect& roi);

private:

    void processing();

    void initCalibration(const cv::Size& imgSize);

private:
    cv::Ptr<cv::cuda::StereoBM> sbm;

    FrameSource* leftSource;
    FrameSource* rightSource;
    cv::Mat depthMap;

    mutable std::mutex outGuard;
    std::mutex processGuard;

    std::thread thread;
    bool stop;

    cv::Mat cameraMatrixLeft;
    cv::Mat cameraMatrixRight;
    cv::Mat distCoeffsLeft;
    cv::Mat distCoeffsRight;
    cv::Mat R, T, E, F;
    cv::Mat mapLeftx, mapLefty;
    cv::Mat mapRightx, mapRighty;
    cv::Rect leftRoi;
    cv::Rect rightRoi;
    cv::Rect commonRoi;
    cv::Mat Q;
    bool hasUndistort;
};

#endif // DEPTHMAPBUILDER_H
