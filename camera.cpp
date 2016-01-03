#include "camera.h"

Camera::Camera()
    : stop(false)
    , resolution(-1,-1)
{
}

Camera::~Camera()
{
    stopCapture();
}

void Camera::setFrameCallback(std::function<void (cv::Mat&)> func)
{
    std::unique_lock<std::mutex> lock(guard);
    frameCallback = func;
}

void Camera::capturing(int cameraId)
{
    cv::VideoCapture vcap(cameraId);

    if (vcap.isOpened())
    {
        captureStarted.set_value(true);
        cv::Mat frame;
        bool done = false;
        while(!done)
        {
            if (vcap.read(frame))
            {
                //take snapshoot
                {
                    std::unique_lock<std::mutex> lock(snapGuard);
                    if (!freeForSnap && !snapFileName.empty())
                    {
                        cv::imwrite(snapFileName, frame);
                        snapFileName.clear();
                        freeForSnap = true;
                    }
                }                

                if (frameCallback)
                {
                    frameCallback(frame);
                }
                done = stop;
                actualResolution.width = static_cast<int>(vcap.get(CV_CAP_PROP_FRAME_WIDTH));
                actualResolution.height = static_cast<int>(vcap.get(CV_CAP_PROP_FRAME_HEIGHT));

                if (resolution.width > 0 && resolution.height > 0)
                {
                    vcap.set(CV_CAP_PROP_FRAME_WIDTH, resolution.width);
                    vcap.set(CV_CAP_PROP_FRAME_HEIGHT, resolution.height);
                    resolution = cv::Size(-1,-1);
                }
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        captureStarted.set_value(false);
    }
}

bool Camera::startCapture(int cameraId)
{
    stopCapture();

    stop = false;
    captureStarted = std::move(std::promise<bool>());

    auto wait = captureStarted.get_future();

    thread = std::move(std::thread(std::bind(&Camera::capturing,this, cameraId)));

    try
    {
        bool res = wait.get();
        return res;
    }
    catch(...)
    {
        return false;
    }
}

void Camera::stopCapture()
{
    {
        std::unique_lock<std::mutex> lock(guard);
        stop = true;
    }

    if (thread.joinable())
    {
        thread.join();
    }
}

cv::Size Camera::getResolution() const
{
    std::unique_lock<std::mutex> lock(guard);
    return actualResolution;
}

void Camera::setResolution(const cv::Size &resolution)
{
    std::unique_lock<std::mutex> lock(guard);
    this->resolution = resolution;
}

void Camera::takeSnapshoot(const std::string &fileName)
{
    std::unique_lock<std::mutex> lock(snapGuard);
    if (freeForSnap)
    {
        snapFileName = fileName;
        freeForSnap = false;
    }

}

bool Camera::canTakeSnapshoot() const
{
   std::unique_lock<std::mutex> lock(snapGuard);
   return freeForSnap;
}

int Camera::getDeviceCount()
{
    int availableCount = 0;
    int maxToTestTest = 10;
    for (int i = 0; i < maxToTestTest; ++i)
    {
        cv::VideoCapture vcap(i);
        if (vcap.isOpened())
        {
            ++availableCount;
        }
    }
    return availableCount;
}

bool Camera::calibrate(int squareSize,
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
    cv::Size imageSize(-1, -1);
    std::vector<std::vector<cv::Point2f>> imagePoints;
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
