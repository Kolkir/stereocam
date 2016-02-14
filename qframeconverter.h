#ifndef QFRAMECONVERTER_H
#define QFRAMECONVERTER_H

#include "framesource.h"

#include <QObject>
#include <qbasictimer.h>

#include <opencv2/opencv.hpp>

class QFrameConverter : public QObject
{
    Q_OBJECT
public:
    explicit QFrameConverter(QObject *parent = 0);
    explicit QFrameConverter(FrameSource& frameSOurce, QObject *parent = 0);

    void timerEvent(QTimerEvent * ev) override;

    Q_SIGNAL void imageReady(const QImage &);

    void stop();

    void pause(bool val);

    void setFrameSource(FrameSource& frameSource);

private:
    FrameSource* frameSource;
    QBasicTimer timer;
    cv::Mat frame;
    bool stopTimer;
    bool pauseProcessing;
};

#endif // QFRAMECONVERTER_H
