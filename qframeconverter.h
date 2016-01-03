#ifndef QFRAMECONVERTER_H
#define QFRAMECONVERTER_H

#include "frameprocessor.h"

#include <QObject>
#include <qbasictimer.h>

#include <opencv2/opencv.hpp>

class QFrameConverter : public QObject
{
    Q_OBJECT
public:
    explicit QFrameConverter(FrameProcessor& frameProcessor, QObject *parent = 0);

    void timerEvent(QTimerEvent * ev) override;

    Q_SIGNAL void imageReady(const QImage &);

    void stop();

private:
    FrameProcessor& frameProcessor;
    QBasicTimer timer;
    cv::Mat frame;
    bool stopTimer;
};

#endif // QFRAMECONVERTER_H
