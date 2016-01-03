#include "qframeconverter.h"

#include <QTimerEvent>
#include <QImage>
#include <QThread>

QFrameConverter::QFrameConverter(FrameProcessor& frameProcessor, QObject *parent)
    : QObject(parent)
    , frameProcessor(frameProcessor)
    , stopTimer(false)
{
    timer.start(0, this);
}

void QFrameConverter::timerEvent(QTimerEvent * ev)
{
    if (ev->timerId() != timer.timerId())
    {
        QObject::timerEvent(ev);
    }
    else
    {
        frameProcessor.getFrame(frame);

        if (!frame.empty())
        {
            QImage::Format format(QImage::Format_RGB888);
            switch (frame.channels())
            {
            case 1:
                format = QImage::Format_Grayscale8;
                break;
            case 3:
                format = QImage::Format_RGB888;
                break;
            default:
                Q_ASSERT(false);
            }

            const QImage image(frame.data, frame.cols, frame.rows, static_cast<int>(frame.step), format);

            Q_ASSERT(image.constBits() == frame.data);

            emit imageReady(image);
        }
    }

    if (stopTimer)
    {
        timer.stop();
        while(timer.isActive())
        {
            QThread::sleep(10);
        }
    }
}

void QFrameConverter::stop()
{
    stopTimer = true;
}

