#include "depthmapbuilder.h"

DepthMapBuilder::DepthMapBuilder()
    : sbm(cv::cuda::createStereoBM())
    , leftSource(nullptr)
    , rightSource(nullptr)
    , hasUndistort(false)
{
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

int DepthMapBuilder::getNumDisparities() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return sbm->getNumDisparities();
}

void DepthMapBuilder::setNumDisparities(int numDisparities)
{
    std::unique_lock<std::mutex> lock(outGuard);
    sbm->setNumDisparities(numDisparities);
}


int DepthMapBuilder::getBlockSize() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return sbm->getBlockSize();
}

void DepthMapBuilder::setBlockSize(int blockSize)
{
    std::unique_lock<std::mutex> lock(outGuard);
    sbm->setBlockSize(blockSize);
}

int DepthMapBuilder::getTextureThreshold() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return sbm->getTextureThreshold();
}

void DepthMapBuilder::setTextureThreshold(int textureThreshold)
{
    std::unique_lock<std::mutex> lock(outGuard);
    sbm->setTextureThreshold(textureThreshold);
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

void DepthMapBuilder::processing()
{
    bool undistortInitialized = false;
    bool initGpu = false;

    cv::cuda::GpuMat gpuLeft;
    cv::cuda::GpuMat gpuRight;
    cv::cuda::GpuMat gpuMap;   
    cv::cuda::GpuMat gpuQ;
    cv::cuda::GpuMat gpuXYZ;

    cv::Mat leftImg;
    cv::Mat rightImg;
    cv::Mat tmpMap;
    cv::Mat xyz;
    std::vector<cv::Mat> channels;
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

            if (!initGpu)
            {
                gpuLeft.create(leftImg.size(), leftImg.type());
                gpuRight.create(rightImg.size(), rightImg.type());
                gpuMap.create(leftImg.size(), CV_16S);
                tmpMap = cv::Mat(leftImg.size(), CV_16S);
                gpuQ.create(Q.size(), Q.type());
                gpuQ.upload(Q);
                gpuXYZ.create(leftImg.size(), CV_32FC3);
                xyz.create(leftImg.size(), CV_32FC3);
                initGpu = true;
            }

            gpuLeft.upload(leftImg);
            gpuRight.upload(rightImg);

            sbm->compute(gpuLeft, gpuRight, gpuMap);
            //cv::cuda::reprojectImageTo3D(gpuMap, gpuXYZ, gpuQ, 3);
            gpuMap.download(tmpMap);

            cv::normalize(tmpMap, tmpMap, 0, 255, CV_MINMAX, CV_8U);

            //cv::reprojectImageTo3D(tmpMap, xyz, Q, true,  CV_32F);

            //cv::split(xyz, channels);

            //cv::normalize(channels[2], tmpMap, 0, 255, CV_MINMAX, CV_8U);

            //cv::normalize(xyz, tmpMap, 0, 255, CV_MINMAX, CV_32FC3);
            //tmpMap.convertTo(tmpMap, CV_8UC3);

            std::unique_lock<std::mutex> lock(outGuard);
            tmpMap.copyTo(depthMap);
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
