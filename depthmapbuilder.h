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
    int getMinDisparity() const;
    void setMinDisparity(int minDisparities);

    int getNumDisparities() const;
    void setNumDisparities(int numDisparities);

    int getBlockSize() const;
    void setBlockSize(int blockSize);

    int  getP1() const;
    void setP1(int p1);

    int getP2() const;
    void setP2(int p2);

    int getDisp12MaxDiff() const;
    void setDisp12MaxDiff(int disp12MaxDiff);

    int getPreFilterCap() const;
    void setPreFilterCap(int preFilterCap);

    int getUniquenessRatio() const;
    void setUniquenessRatio(int uniquenessRatio);

    int getSpeckleWindowSize() const;
    void setSpeckleWindowSize(int speckleWindowSize);

    int getSpeckleRange() const;\
    void setSpeckleRange(int speckleRange);

    int getMode() const;
    void setMode(int mode);

    //calibration

    void getLeftMapping(const cv::Size& imgSize, cv::Mat& mapx, cv::Mat& mapy, cv::Rect& roi);
    void getRightMapping(const cv::Size& imgSize, cv::Mat& mapx, cv::Mat& mapy, cv::Rect& roi);

    void saveDepthMap(const std::string& fileName);

private:

    void processing();

    void initCalibration(const cv::Size& imgSize);

private:
    cv::Ptr<cv::StereoSGBM> stereoMatcher;

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
