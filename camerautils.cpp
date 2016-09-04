#include "camerautils.h"

#include <stdexcept>

#include <opencv2/opencv.hpp>

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>

namespace camera {
namespace utils {


ScopedVideoDevice::ScopedVideoDevice(int id)
{
    auto devName = "/dev/video" + std::to_string(id);
    int fd = open(devName.c_str(), O_RDWR);
    if((fd) < 0)
    {
        throw std::runtime_error("Unable open video device : " + devName);
    }
    devFileName = devName;
    fileDesc = fd;
}

ScopedVideoDevice::~ScopedVideoDevice()
{
    close(fileDesc);
}

const std::string& ScopedVideoDevice::fileName() const
{
    return devFileName;
}

int ScopedVideoDevice::fd() const
{
    return fileDesc;
}


std::string getDeviceName(int id)
{
    std::string devName;
    try
    {
        ScopedVideoDevice dev(id);
        struct v4l2_capability capabilities;
        if(ioctl(dev.fd(), VIDIOC_QUERYCAP, &capabilities) >= 0)
        {
            devName = reinterpret_cast<char*>(capabilities.card);
        }
    }
    catch(...)
    {
        //ignore, return empty string
    }

    return devName;
}

std::vector<VideoDevId> getDeviceList()
{
    std::vector<VideoDevId> list;
    struct dirent* epdf = nullptr;
    DIR* dpdf = opendir("/dev/");
    if (dpdf != nullptr)
    {
        VideoDevId dev;
        while ((epdf = readdir(dpdf)) != nullptr)
        {
            std::string fileName(epdf->d_name);
            if (!fileName.empty())
            {
                auto pos = fileName.find("video");
                if (pos == 0)
                {
                    dev.id = std::atoi(fileName.substr(5).c_str());
                    dev.name = getDeviceName(dev.id);
                    list.push_back(dev);
                }
            }
        }
        closedir(dpdf);
    }
    return list;
}

std::vector<VideoDevFormat> getDeviceFormats(int id)
{
    std::vector<VideoDevFormat> formats;
    try
    {
        ScopedVideoDevice dev(id);
        VideoDevFormat format;
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        for(;;)
        {
            auto err = ioctl(dev.fd(), VIDIOC_ENUM_FMT, &fmtdesc);
            if  (err >= 0)
            {                                
                format.pixelformat = fmtdesc.pixelformat;
                //enumerate frame sizes
                v4l2_frmsizeenum frmsize;
                frmsize.pixel_format = fmtdesc.pixelformat;
                frmsize.index = 0;
                for(;;)
                {
                    err = ioctl(dev.fd(), VIDIOC_ENUM_FRAMESIZES, &frmsize);
                    if (err >= 0)
                    {
                        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                        {
                            format.width = frmsize.discrete.width;
                            format.height = frmsize.discrete.height;
                            format.description = reinterpret_cast<char*>(fmtdesc.description);
                            format.description += " " + std::to_string(format.width) + "x" + std::to_string(format.height);

                            if (fmtdesc.pixelformat == V4L2_PIX_FMT_MJPEG)
                            {
                                formats.push_back(format);
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                    frmsize.index += 1;
                }
            }
            else
            {
                break;
            }
            fmtdesc.index += 1;
        }
    }
    catch(...)
    {
        //ignore, return empty list
    }

    return formats;
}

namespace
{
    void getImagePoints(const cv::Size& boardSize,
                        const std::vector<std::string> &files,
                        std::vector<std::vector<cv::Point2f>>& imagePoints,
                        cv::Size& imageSize)
    {
        imageSize = cv::Size(-1,-1);
        imagePoints.clear();
        std::vector<cv::Point2f> pointbuf;
        for (auto& ifn : files)
        {
            cv::Mat img = cv::imread(ifn, CV_LOAD_IMAGE_GRAYSCALE);

            if (imageSize.width == -1)
            {
                imageSize = img.size();
            }

            pointbuf.clear();
            bool found = cv::findChessboardCorners(img, boardSize, pointbuf, CV_CALIB_CB_ADAPTIVE_THRESH |
                                                                             CV_CALIB_CB_FILTER_QUADS);
            if(found)
            {
                cv::cornerSubPix(img, pointbuf, cv::Size(11,11),
                            cv::Size(-1,-1), cv::TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));

                imagePoints.push_back(pointbuf);
            }
        }
    }
}


bool calibrate(int squareSize,
               int wcount,
               int hcount,
               const std::vector<std::string> &files,
               const std::string &fileName)
{
    //generate Chessbard pattern
    cv::Size boardSize(wcount, hcount);
    std::vector<std::vector<cv::Point3f> > objectPoints(1);
    for( int i = 0; i < boardSize.height; i++ )
    {
        for( int j = 0; j < boardSize.width; j++ )
        {
            objectPoints[0].push_back(cv::Point3f(float(j*squareSize), float(i*squareSize), 0));
        }
    }

    //extract image points
    cv::Size imageSize;
    std::vector<std::vector<cv::Point2f>> imagePoints;
    getImagePoints(boardSize, files, imagePoints, imageSize);

    //set correspondance between image points and object
    objectPoints.resize(imagePoints.size(),objectPoints[0]);

    //calibrate
    cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);

    std::vector<cv::Mat> rvecs, tvecs;

    cv::calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
                        distCoeffs, rvecs, tvecs, CV_CALIB_FIX_K4|CV_CALIB_FIX_K5);

    bool ok = cv::checkRange(cameraMatrix) && cv::checkRange(distCoeffs);

    if (ok)
    {
        cv::FileStorage storage(fileName, cv::FileStorage::WRITE);
        storage << "cameraMatrix" << cameraMatrix;
        storage << "distCoeffs" << distCoeffs;
    }

    return ok;
}

bool stereoCalibrate(int squareSize,
                     int wcount,
                     int hcount,
                     const std::vector<std::string> &filesLeft,
                     const std::vector<std::string> &filesRight,
                     const std::string &fileName)
{
    //generate Chessbard pattern
    cv::Size boardSize(wcount, hcount);
    std::vector<std::vector<cv::Point3f> > objectPoints(1);
    for( int i = 0; i < boardSize.height; i++ )
    {
        for( int j = 0; j < boardSize.width; j++ )
        {
            objectPoints[0].push_back(cv::Point3f(float(j*squareSize), float(i*squareSize), 0));
        }
    }

    //extract image points
    cv::Size imageSize;

    std::vector<std::vector<cv::Point2f>> imagePointsLeft;
    getImagePoints(boardSize, filesLeft, imagePointsLeft, imageSize);

    std::vector<std::vector<cv::Point2f>> imagePointsRight;
    getImagePoints(boardSize, filesRight, imagePointsRight, imageSize);

    //set correspondance between image points and object
    objectPoints.resize(imagePointsLeft.size(),objectPoints[0]);

    //calibrate
    cv::Mat cameraMatrixLeft = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat cameraMatrixRight = cv::Mat::eye(3, 3, CV_64F);
    cameraMatrixLeft = cv::initCameraMatrix2D(objectPoints,imagePointsLeft,imageSize,0);
    cameraMatrixRight = cv::initCameraMatrix2D(objectPoints,imagePointsRight,imageSize,0);


    cv::Mat distCoeffsLeft, distCoeffsRight;
    cv::Mat R, T, E, F;

    cv::stereoCalibrate(objectPoints, imagePointsLeft, imagePointsRight,
                        cameraMatrixLeft, distCoeffsLeft, cameraMatrixRight, distCoeffsRight, imageSize, R, T, E, F,
                        cv::CALIB_FIX_ASPECT_RATIO +
                        cv::CALIB_ZERO_TANGENT_DIST +
                        cv::CALIB_USE_INTRINSIC_GUESS +
                        cv::CALIB_SAME_FOCAL_LENGTH +
                        cv::CALIB_RATIONAL_MODEL +
                        cv::CALIB_FIX_K3 + cv::CALIB_FIX_K4 + cv::CALIB_FIX_K5,
                        cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 100, 1e-5));

    bool ok = cv::checkRange(cameraMatrixLeft) && cv::checkRange(distCoeffsLeft) &&
              cv::checkRange(cameraMatrixRight) && cv::checkRange(distCoeffsRight);

    if (ok)
    {
        cv::FileStorage storage(fileName, cv::FileStorage::WRITE);
        storage << "CMLeft" << cameraMatrixLeft;
        storage << "DLeft" << distCoeffsLeft;
        storage << "CMRight" << cameraMatrixRight;
        storage << "DRight" << distCoeffsRight;
        storage << "R" << R;
        storage << "T" << T;
        storage << "E" << E;
        storage << "F" << F;
    }

    return ok;
}

void GetCameraParameters(int fd, std::vector<CameraParameter> &parameters)
{
    parameters.clear();

    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));

    queryctrl.id = V4L2_CTRL_CLASS_USER | V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (V4L2_CTRL_ID2CLASS(queryctrl.id) != V4L2_CTRL_CLASS_USER)
            break;
        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            continue;

        struct v4l2_control ctrl;
        ctrl.id = queryctrl.id;
        ctrl.value = queryctrl.default_value;
        ioctl(fd, VIDIOC_S_CTRL, &ctrl);

        ioctl(fd, VIDIOC_G_CTRL, &ctrl);

        //std::cout << "Control : " << queryctrl.name << " " << queryctrl.minimum << " - " << queryctrl.maximum  << " = " << ctrl.value<< std::endl;

        parameters.emplace_back(ctrl.id, reinterpret_cast<char*>(queryctrl.name), queryctrl.minimum, queryctrl.maximum, ctrl.value);

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

}

CameraParameter::CameraParameter()
    : id(-1)
    , minimum(-1)
    , maximum(-1)
    , value(-1)
{}

CameraParameter::CameraParameter(int id, const std::string &name, int minimum, int maximum, int value)
    : id(id)
    , name(name)
    , minimum(minimum)
    , maximum(maximum)
    , value(value)
{

}


}}
