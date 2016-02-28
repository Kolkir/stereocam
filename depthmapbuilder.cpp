#include "depthmapbuilder.h"

#include <opencv2/photo/cuda.hpp>

DepthMapBuilder::DepthMapBuilder()
    : stereoMatcher(cv::StereoSGBM::create(0, 16, 3))
    , leftSource(nullptr)
    , rightSource(nullptr)
    , hasUndistort(false)
{
    stereoMatcher->setPreFilterCap(63);
    int sgbmWinSize = 3;
    int cn = 1;
    stereoMatcher->setBlockSize(sgbmWinSize);
    stereoMatcher->setP1(8*cn*sgbmWinSize*sgbmWinSize);
    stereoMatcher->setP2(32*cn*sgbmWinSize*sgbmWinSize);
    stereoMatcher->setUniquenessRatio(10);
    stereoMatcher->setSpeckleWindowSize(100);
    stereoMatcher->setSpeckleRange(32);
    stereoMatcher->setDisp12MaxDiff(1);
    stereoMatcher->setMode(cv::StereoSGBM::MODE_HH);
}

DepthMapBuilder::~DepthMapBuilder()
{
    stopProcessing();
}

void DepthMapBuilder::setLeftSource(FrameSource &source)
{
    std::unique_lock<std::mutex> lock(processGuard);
    leftSource = &source;
}

void DepthMapBuilder::setRightSource(FrameSource &source)
{
    std::unique_lock<std::mutex> lock(processGuard);
    rightSource = &source;
}

void DepthMapBuilder::getFrame(cv::Mat& map)
{
    std::unique_lock<std::mutex> lock(outGuard);
    depthMap.copyTo(map);
}

void DepthMapBuilder::startProcessing()
{
    stop = false;
    thread = std::move(std::thread(std::bind(&DepthMapBuilder::processing,this)));
}

void DepthMapBuilder::stopProcessing()
{
    {
        std::unique_lock<std::mutex> lock(outGuard);
        stop = true;
    }

    if (thread.joinable())
    {
        thread.join();
    }
}

bool DepthMapBuilder::loadCalibrationParams(const std::__cxx11::string &fileName)
{
    std::unique_lock<std::mutex> lock(outGuard);
    try
    {
        cv::FileStorage storage(fileName, cv::FileStorage::READ);
        storage["CMLeft"] >> cameraMatrixLeft;
        storage["DLeft"] >> distCoeffsLeft;
        storage["CMRight"] >> cameraMatrixRight;
        storage["DRight"] >> distCoeffsRight;
        storage["R"] >> R;
        storage["T"] >> T;
        storage["E"] >> E;
        storage["F"] >> F;
        hasUndistort = true;
        return true;
    }
    catch(...)
    {
        return false;
    }
}

int DepthMapBuilder::getMinDisparity() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getMinDisparity();
}

void DepthMapBuilder::setMinDisparity(int minDisparity)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setMinDisparity(minDisparity);
}

int DepthMapBuilder::getNumDisparities() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getNumDisparities();
}

void DepthMapBuilder::setNumDisparities(int numDisparities)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setNumDisparities(numDisparities);
}

int DepthMapBuilder::getBlockSize() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getBlockSize();
}

void DepthMapBuilder::setBlockSize(int blockSize)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setBlockSize(blockSize);
}

int  DepthMapBuilder::getP1() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getP1();
}

void DepthMapBuilder::setP1(int p1)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setP1(p1);
}

int DepthMapBuilder::getP2() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getP2();
}

void DepthMapBuilder::setP2(int p2)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setP2(p2);
}

int DepthMapBuilder::getDisp12MaxDiff() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getDisp12MaxDiff();
}

void DepthMapBuilder::setDisp12MaxDiff(int disp12MaxDiff)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setDisp12MaxDiff(disp12MaxDiff);
}

int DepthMapBuilder::getPreFilterCap() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getPreFilterCap();
}

void DepthMapBuilder::setPreFilterCap(int preFilterCap)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setPreFilterCap(preFilterCap);
}

int DepthMapBuilder::getUniquenessRatio() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getUniquenessRatio();
}

void DepthMapBuilder::setUniquenessRatio(int uniquenessRatio)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setUniquenessRatio(uniquenessRatio);
}

int DepthMapBuilder::getSpeckleWindowSize() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getSpeckleWindowSize();
}

void DepthMapBuilder::setSpeckleWindowSize(int speckleWindowSize)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setSpeckleWindowSize(speckleWindowSize);
}

int DepthMapBuilder::getSpeckleRange() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getSpeckleRange();
}

void DepthMapBuilder::setSpeckleRange(int speckleRange)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setSpeckleRange(speckleRange);
}

int DepthMapBuilder::getMode() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return stereoMatcher->getMode();
}

void DepthMapBuilder::setMode(int mode)
{
    std::unique_lock<std::mutex> lock(outGuard);
    stereoMatcher->setMode(mode);
}

void DepthMapBuilder::getLeftMapping(const cv::Size &imgSize, cv::Mat &mapx, cv::Mat &mapy, cv::Rect& roi)
{
    initCalibration(imgSize);
    mapLeftx.copyTo(mapx);
    mapLefty.copyTo(mapy);
    roi = commonRoi;
}

void DepthMapBuilder::getRightMapping(const cv::Size &imgSize, cv::Mat &mapx, cv::Mat &mapy, cv::Rect& roi)
{
    initCalibration(imgSize);
    mapRightx.copyTo(mapx);
    mapRighty.copyTo(mapy);
    roi = commonRoi;
}

void DepthMapBuilder::saveDepthMap(const std::string &fileName)
{
    std::unique_lock<std::mutex> lock(outGuard);
    cv::imwrite(fileName, depthMap);
}

void DepthMapBuilder::processing()
{
    bool undistortInitialized = false;

    cv::Mat leftImg;
    cv::Mat rightImg;
    cv::Mat disp;
    cv::Mat filterBuf;

    bool done = false;
    while(!done)
    {
        {
            std::unique_lock<std::mutex> lock(processGuard);
            if (rightSource != nullptr && leftSource!= nullptr)
            {
                leftSource->getFrame(leftImg);
                rightSource->getFrame(rightImg);
            }
        }

        {
            std::unique_lock<std::mutex> lock(outGuard);
            done = stop;
        }

        if (!leftImg.empty() && !rightImg.empty())
        {
            if (!undistortInitialized && hasUndistort)
            {
                initCalibration(leftImg.size());
                undistortInitialized = true;
            }

            if (leftImg.channels() == 3)
            {
                cv::cvtColor(leftImg, leftImg, CV_BGR2GRAY);
            }
            if (rightImg.channels() == 3)
            {
                cv::cvtColor(rightImg, rightImg, CV_BGR2GRAY);
            }            

            //undistort
            if (hasUndistort)
            {
                cv::remap(leftImg, leftImg, mapLeftx, mapLefty, cv::INTER_LINEAR);
                leftImg = leftImg(commonRoi);
                cv::remap(rightImg, rightImg, mapRightx, mapRighty, cv::INTER_LINEAR);
                rightImg = rightImg(commonRoi);
            }

            //cv::fastNlMeansDenoising(leftImg, leftImg, 15);
            //cv::fastNlMeansDenoising(rightImg, rightImg, 15);

            stereoMatcher->compute(leftImg, rightImg, disp);

            //cv::filterSpeckles(disp,0, 10, 1, filterBuf);

            disp.convertTo(disp, CV_8U, 255/(stereoMatcher->getNumDisparities()*16.));

            std::unique_lock<std::mutex> lock(outGuard);
            disp.copyTo(depthMap);
        }
    }
}

void DepthMapBuilder::initCalibration(const cv::Size &imgSize)
{
    cv::Mat R1, R2, P1, P2;
    cv::stereoRectify(cameraMatrixLeft, distCoeffsLeft,
                     cameraMatrixRight, distCoeffsRight,
                     imgSize,
                     R, T, R1, R2, P1, P2, Q,
                     cv::CALIB_ZERO_DISPARITY, 1,
                     imgSize, &leftRoi, &rightRoi);

    commonRoi = leftRoi & rightRoi;

    initUndistortRectifyMap(cameraMatrixLeft, distCoeffsLeft, R1, P1, imgSize, CV_32FC1, mapLeftx, mapLefty);
    initUndistortRectifyMap(cameraMatrixRight, distCoeffsRight, R2, P2, imgSize, CV_32FC1, mapRightx, mapRighty);
}
