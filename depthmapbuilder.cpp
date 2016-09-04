#include "depthmapbuilder.h"

#include <opencv2/photo/cuda.hpp>



DepthMapBuilder::DepthMapBuilder()
    : leftSource(nullptr)
    , rightSource(nullptr)
    , hasUndistort(false)
{
    int sgbmWinSize = 3;
    int cn = 1;
    int min_disp = 0;
    int max_disp = 96;
    int p1 = 8*cn*sgbmWinSize*sgbmWinSize;
    int p2 = 32*cn*sgbmWinSize*sgbmWinSize;



    leftStereoMatcher = cv::StereoSGBM::create(min_disp, max_disp, sgbmWinSize,p1,p2,1,63,10,100,32, cv::StereoSGBM::MODE_HH);
    //rightStereoMatcher = cv::StereoSGBM::create(min_disp, max_disp, sgbmWinSize,p1,p2,1,63,10,100,32, cv::StereoSGBM::MODE_HH);
/*
    leftStereoMatcher->setPreFilterCap(63);

    leftStereoMatcher->setBlockSize(sgbmWinSize);
    leftStereoMatcher->setP1(8*cn*sgbmWinSize*sgbmWinSize);
    leftStereoMatcher->setP2(32*cn*sgbmWinSize*sgbmWinSize);
    leftStereoMatcher->setUniquenessRatio(10);
    leftStereoMatcher->setSpeckleWindowSize(100);
    leftStereoMatcher->setSpeckleRange(32);
    leftStereoMatcher->setDisp12MaxDiff(1);
    leftStereoMatcher->setMode(cv::StereoSGBM::MODE_HH);
*/
    wls_filter = cv::ximgproc::createDisparityWLSFilter(leftStereoMatcher);
    rightStereoMatcher = cv::ximgproc::createRightMatcher(leftStereoMatcher);
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

void DepthMapBuilder::getPoints(std::vector<cv::Vec3f> &pts, std::vector<cv::Vec3b>& colors) const
{
    std::unique_lock<std::mutex> lock(outGuard);
    pts = points3d;
    colors = pointsColor;
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
    return leftStereoMatcher->getMinDisparity();
}

void DepthMapBuilder::setMinDisparity(int minDisparity)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setMinDisparity(minDisparity);
    rightStereoMatcher->setMinDisparity(minDisparity);
}

int DepthMapBuilder::getNumDisparities() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getNumDisparities();
}

void DepthMapBuilder::setNumDisparities(int numDisparities)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setNumDisparities(numDisparities);
    rightStereoMatcher->setNumDisparities(numDisparities);
}

int DepthMapBuilder::getBlockSize() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getBlockSize();
}

void DepthMapBuilder::setBlockSize(int blockSize)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setBlockSize(blockSize);
    rightStereoMatcher->setBlockSize(blockSize);
}

int  DepthMapBuilder::getP1() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getP1();
}

void DepthMapBuilder::setP1(int p1)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setP1(p1);
    //rightStereoMatcher->setP1(p1);
}

int DepthMapBuilder::getP2() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getP2();
}

void DepthMapBuilder::setP2(int p2)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setP2(p2);
    //rightStereoMatcher->setP2(p2);
}

int DepthMapBuilder::getDisp12MaxDiff() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getDisp12MaxDiff();
}

void DepthMapBuilder::setDisp12MaxDiff(int disp12MaxDiff)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setDisp12MaxDiff(disp12MaxDiff);
    rightStereoMatcher->setDisp12MaxDiff(disp12MaxDiff);
}

int DepthMapBuilder::getPreFilterCap() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getPreFilterCap();
}

void DepthMapBuilder::setPreFilterCap(int preFilterCap)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setPreFilterCap(preFilterCap);
    //rightStereoMatcher->setPreFilterCap(preFilterCap);
}

int DepthMapBuilder::getUniquenessRatio() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getUniquenessRatio();
}

void DepthMapBuilder::setUniquenessRatio(int uniquenessRatio)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setUniquenessRatio(uniquenessRatio);
    //rightStereoMatcher->setUniquenessRatio(uniquenessRatio);
}

int DepthMapBuilder::getSpeckleWindowSize() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getSpeckleWindowSize();
}

void DepthMapBuilder::setSpeckleWindowSize(int speckleWindowSize)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setSpeckleWindowSize(speckleWindowSize);
    rightStereoMatcher->setSpeckleWindowSize(speckleWindowSize);
}

int DepthMapBuilder::getSpeckleRange() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getSpeckleRange();
}

void DepthMapBuilder::setSpeckleRange(int speckleRange)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setSpeckleRange(speckleRange);
    rightStereoMatcher->setSpeckleRange(speckleRange);
}

int DepthMapBuilder::getMode() const
{
    std::unique_lock<std::mutex> lock(outGuard);
    return leftStereoMatcher->getMode();
}

void DepthMapBuilder::setMode(int mode)
{
    std::unique_lock<std::mutex> lock(outGuard);
    leftStereoMatcher->setMode(mode);
    //rightStereoMatcher->setMode(mode);
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

cv::Rect computeROI(cv::Size2i src_sz, cv::Ptr<cv::StereoMatcher> matcher_instance)
{
    int min_disparity = matcher_instance->getMinDisparity();
    int num_disparities = matcher_instance->getNumDisparities();
    int block_size = matcher_instance->getBlockSize();

    int bs2 = block_size/2;
    int minD = min_disparity, maxD = min_disparity + num_disparities - 1;

    int xmin = maxD + bs2;
    int xmax = src_sz.width + minD - bs2;
    int ymin = bs2;
    int ymax = src_sz.height - bs2;

    cv::Rect r(xmin, ymin, xmax - xmin, ymax - ymin);
    return r;
}

void DepthMapBuilder::processing()
{
    bool undistortInitialized = false;

    cv::Mat leftImgColor;
    cv::Mat leftImg;
    cv::Mat rightImg;
    cv::Mat leftDisp;
    cv::Mat rightDisp;

    cv::Mat image3d;

    unsigned long long numErrors = std::numeric_limits<unsigned long long>::max();

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

            leftImg.copyTo(leftImgColor);

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

                cv::remap(leftImgColor, leftImgColor, mapLeftx, mapLefty, cv::INTER_LINEAR);
                leftImgColor = leftImgColor(commonRoi);

                cv::remap(rightImg, rightImg, mapRightx, mapRighty, cv::INTER_LINEAR);
                rightImg = rightImg(commonRoi);
            }

            leftStereoMatcher->compute(leftImg, rightImg, leftDisp);
/*
            cv::Rect rcCrop = cv::getValidDisparityROI(leftRoi,
                                     rightRoi,
                                     leftStereoMatcher->getMinDisparity(),
                                     leftStereoMatcher->getNumDisparities(),
                                     leftStereoMatcher->getBlockSize());                       
            cv::Mat visDisp;
            //cv::ximgproc::getDisparityVis(leftDisp(rcCrop), visDisp);
            leftDisp.convertTo(visDisp, CV_8U, 255/(leftStereoMatcher->getNumDisparities()*16.));

            //cv::equalizeHist(visDisp,visDisp);

            std::unique_lock<std::mutex> lock(outGuard);
            visDisp.copyTo(depthMap);

*/

           // rightStereoMatcher->compute(rightImg, leftImg, rightDisp);

            const double lambda = 8000.0;
            const double sigma = 1.5;            
            wls_filter->setLambda(lambda);
            wls_filter->setSigmaColor(sigma);

            cv::Mat filteredDisp = leftDisp;
            //wls_filter->filter(leftDisp,leftImg,filteredDisp,rightDisp);

            //cv::reprojectImageTo3D(filteredDisp, image3d, Q, true, CV_32F);

            /*cv::Rect rcCrop = cv::getValidDisparityROI(leftRoi,
                                     rightRoi,
                                     leftStereoMatcher->getMinDisparity(),
                                     leftStereoMatcher->getNumDisparities(),
                                     leftStereoMatcher->getBlockSize());*/

            int shift = leftStereoMatcher->getNumDisparities() + leftStereoMatcher->getMinDisparity();
            cv::Rect rcCrop(shift,
                            0,
                            leftImg.cols - shift,
                            leftImg.rows);

            fillPoints(filteredDisp, Q, leftImgColor, rcCrop);

            filteredDisp = filteredDisp(rcCrop);

            cv::Mat visDisp;
            //cv::ximgproc::getDisparityVis(filteredDisp, visDisp);
            filteredDisp.convertTo(visDisp, CV_8U, 255/(leftStereoMatcher->getNumDisparities()*16.));
            //cv::equalizeHist(visDisp,visDisp);

            std::unique_lock<std::mutex> lock(outGuard);
            visDisp.copyTo(depthMap);

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

void DepthMapBuilder::fillPoints(const cv::Mat &disp, const cv::Mat &Q, const cv::Mat &color, const cv::Rect& rc )
{
    cv::Mat disp32F;
    disp.convertTo(disp32F, CV_32F, 1./16);

    cv::Mat Q_32F;
    Q.convertTo(Q_32F,CV_32F);
    cv::Mat_<float> vec(4,1);

    std::unique_lock<std::mutex> lock(outGuard);
    points3d.clear();
    pointsColor.clear();
    cv::Vec3f point;
    for (int y = 0; y < disp32F.rows; ++y)
    {
        for (int x = 0; x < disp32F.cols; ++x)
        {
            if (rc.contains(cv::Point(x,y)))
            {
                vec(0)=x;
                vec(1)=y;
                vec(2)=disp32F.at<float>(y,x);
                vec(3)=1;

                if(vec(2) != 0)
                {
                    vec = Q_32F*vec;
                    vec /= vec(3);

                    if(fabs(vec(0))>10 || fabs(vec(1))>10 || fabs(vec(2))>10)
                    {
                        point[0] = vec(0);
                        point[1] = vec(1);
                        point[2] = vec(2);

                        points3d.push_back(point);
                        pointsColor.push_back(color.at<cv::Vec3b>(y,x));
                    }
                }
            }
            /*cv::Vec3f p = image3d.at<cv::Vec3f>(y,x);
            if (p[2] != 10000 && rc.contains(cv::Point(x,y)))
            {
                points3d.push_back(p);
                pointsColor.push_back(color.at<cv::Vec3b>(y,x));
            }*/
        }
    }
}
