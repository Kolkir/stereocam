#include "frameprocessor.h"

FrameProcessor::FrameProcessor()
    : outScaleFactor(1)
    , outChannel(-1)
    , outGray(false)    
    , outUndistort(false)
    , stop(false)
{

}

FrameProcessor::~FrameProcessor()
{
    stopProcessing();
}

void FrameProcessor::setFrame(const cv::Mat frame)
{
    std::unique_lock<std::mutex> lock(processGuard);
    this->frame = frame;
}

void FrameProcessor::getFrame(cv::Mat &frame)
{
    std::unique_lock<std::mutex> lock(outGuard);
    outFrame.copyTo(frame);
}

void FrameProcessor::processing()
{
    std::vector<cv::Mat> channels(3);
    cv::Mat tmp;
    double scaleFactor = 1;
    int channel = -1;
    bool gray = false;
    bool undistort = false;
    cv::Mat frameUndistort;


    int maxMeanFilter = 5;
    std::vector<cv::Mat> meanFilterBuff(maxMeanFilter);
    cv::Mat filterValues(maxMeanFilter,1,CV_8U);
    int currMeanFilter = 0;

    bool done =false;
    while(!done)
    {
        {
            std::unique_lock<std::mutex> lock(processGuard);
            if (!frame.empty())
            {
                frame.copyTo(tmp);
            }
        }
        {
            std::unique_lock<std::mutex> lock(outGuard);
            scaleFactor = outScaleFactor;
            channel  = outChannel;
            gray = outGray;
            undistort = outUndistort;
            done = stop;
        }

        if (!tmp.empty())
        {
            if (undistort)
            {
                //TODO: fix sync outGuard
                if (!cameraMatrix.empty())
                {
                    cv::undistort(tmp, frameUndistort, cameraMatrix, distCoeffs);
                }
                else if (!mapx.empty())
                {
                    cv::remap(tmp, frameUndistort, mapx, mapy, cv::INTER_LINEAR);
                    frameUndistort = frameUndistort(remapRoi);
                }
                frameUndistort.copyTo(tmp);
            }

            if (gray)
            {
                cv::cvtColor(tmp, tmp, CV_BGR2GRAY);
            }
            else
            {
                cv::cvtColor(tmp, tmp, CV_BGR2RGB);
                if (scaleFactor != 1)
                {
                    cv::resize(tmp, tmp, cv::Size(0,0), scaleFactor, scaleFactor, cv::INTER_NEAREST);
                }
                if (channel >= 0 && channel < tmp.channels())
                {
                    channels.clear();
                    cv::split(tmp, channels);
                    channels[channel].copyTo(tmp);
                }
            }

            //noise filter
            /*if (tmp.channels() == 1)
            {
                tmp.copyTo(meanFilterBuff[currMeanFilter]);
                ++currMeanFilter;
                if(currMeanFilter >= maxMeanFilter)
                {
                    currMeanFilter  = 0;
                }

                for(int y = 0; y < tmp.rows; ++y)
                {
                    for(int x = 0; x < tmp.cols; ++x)
                    {
                        for (int i = 0; i < maxMeanFilter; ++i)
                        {
                            if (!meanFilterBuff[i].empty())
                            {
                                filterValues.at<uchar>(i,0) = meanFilterBuff[i].at<uchar>(y,x);
                            }
                        }
                        auto pixelMean = cv::mean(filterValues);
                        tmp.at<uchar>(y,x) = pixelMean[0];
                    }
                }
            }*/

            //draw center lines
            /*{
                cv::Scalar color(0,255,0);
                if (tmp.channels() == 1)
                {
                    color = cv::Scalar(255);
                }
                int offset = 50;
                //vertical
                cv::Point vstart(tmp.cols/2,0);
                cv::Point vend(tmp.cols/2,tmp.rows-1);
                cv::line(tmp, vstart, vend, color, 1);

                for (int li = -offset; li <= offset; li+=10)
                {
                    cv::Point hstart2(tmp.cols/2-li, tmp.rows/2-abs(li));
                    cv::Point hend2(tmp.cols/2-li,tmp.rows/2+abs(li));
                    cv::line(tmp, hstart2, hend2, color, 1);
                }

                //horisontal
                cv::Point hstart(0, tmp.rows/2);
                cv::Point hend(tmp.cols-1,tmp.rows/2);
                cv::line(tmp, hstart, hend, color, 1);                

                for (int li = -offset; li <= offset; li+=10)
                {
                    cv::Point hstart2(tmp.cols/2-abs(li), tmp.rows/2-li);
                    cv::Point hend2(tmp.cols/2+abs(li),tmp.rows/2-li);
                    cv::line(tmp, hstart2, hend2, color, 1);
                }
            }*/
            {
                std::unique_lock<std::mutex> lock(outGuard);
                tmp.copyTo(outFrame);
            }
        }
    }
}

void FrameProcessor::startProcessing()
{
    stop = false;
    thread = std::move(std::thread(std::bind(&FrameProcessor::processing,this)));
}

void FrameProcessor::stopProcessing()
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

void FrameProcessor::setOutScaleFactor(double factor)
{
    std::unique_lock<std::mutex> lock(outGuard);
    outScaleFactor = factor;
}

void FrameProcessor::setOutChannel(int index)
{
    std::unique_lock<std::mutex> lock(outGuard);
    outChannel = index;
}

void FrameProcessor::setOutGray(bool gray)
{
    std::unique_lock<std::mutex> lock(outGuard);
    outGray = gray;
}

void FrameProcessor::setApplyUndistort(bool undistort)
{
    std::unique_lock<std::mutex> lock(outGuard);
    outUndistort = undistort;
}

bool FrameProcessor::isUndistortApplied() const
{
    return outUndistort;
}

bool FrameProcessor::loadCalibrationParams(const std::string &fileName)
{
    std::unique_lock<std::mutex> lock(outGuard);
    try
    {
        cv::FileStorage storage(fileName, cv::FileStorage::READ);
        storage ["cameraMatrix"] >> cameraMatrix;
        storage ["distCoeffs"] >> distCoeffs;
        return true;
    }
    catch(...)
    {
        return false;
    }
}

void FrameProcessor::setUndistortMappings(const cv::Mat &mapx, const cv::Mat &mapy, const cv::Rect &roi)
{
    std::unique_lock<std::mutex> lock(outGuard);
    this->mapx = mapx.clone();
    this->mapy = mapy.clone();
    this->remapRoi = roi;
}
