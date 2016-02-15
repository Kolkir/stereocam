#include "depthmapbuilder.h"

DepthMapBuilder::DepthMapBuilder()
    : sgbm(0, 16, 3)
    , leftSource(nullptr)
    , rightSource(nullptr)
{
   /*sgbm.SADWindowSize = 5;
    sgbm.numberOfDisparities = 192;
    sgbm.preFilterCap = 4;
    sgbm.minDisparity = -64;
    sgbm.uniquenessRatio = 1;
    sgbm.speckleWindowSize = 150;
    sgbm.speckleRange = 2;
    sgbm.disp12MaxDiff = 10;
    sgbm.fullDP = false;
    sgbm.P1 = 600;
    sgbm.P2 = 2400;*/


    sgbm.minDisparity = 0;
    sgbm.uniquenessRatio = 10;
    sgbm.speckleWindowSize = 100;
    sgbm.speckleRange = 32;
    sgbm.disp12MaxDiff = 1;
    sgbm.preFilterCap = 63;
    sgbm.fullDP = true;
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

void DepthMapBuilder::processing()
{
    cv::Mat leftImg;
    cv::Mat rightImg;
    cv::Mat tmpMap;
    bool done =false;
    while(!done)
    {
        {
            std::unique_lock<std::mutex> lock(processGuard);
            if (rightSource != nullptr && leftSource!= nullptr)
            {
                leftSource->getFrame(leftImg);
                rightSource->getFrame((rightImg));
            }
        }

        {
            std::unique_lock<std::mutex> lock(outGuard);
            done = stop;
        }

        if (!leftImg.empty() && !rightImg.empty())
        {
            if (leftImg.channels() == 3)
            {
                cv::cvtColor(leftImg, leftImg, CV_BGR2GRAY);
            }
            if (rightImg.channels() == 3)
            {
                cv::cvtColor(rightImg, rightImg, CV_BGR2GRAY);
            }

            int numberOfDisparities = ((leftImg.cols/8) + 15) & -16;
            sgbm.numberOfDisparities = numberOfDisparities;

            int cn = leftImg.channels();

            int sgbmWinSize = 3;

            sgbm.P1 = (8*cn*sgbmWinSize*sgbmWinSize);
            sgbm.P2 = (32*cn*sgbmWinSize*sgbmWinSize);

            sgbm(leftImg, rightImg, tmpMap);
            normalize(tmpMap, tmpMap, 0, 255, CV_MINMAX, CV_8U);

            std::unique_lock<std::mutex> lock(outGuard);
            tmpMap.copyTo(depthMap);
        }
    }
}
