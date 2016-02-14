#include "qframeconverter.h"

#include <QTimerEvent>
#include <QImage>
#include <QThread>

QFrameConverter::QFrameConverter(QObject *parent)
    : QObject(parent)
    , stopTimer(false)
{

}

QFrameConverter::QFrameConverter(FrameSource& frameSource, QObject *parent)
    : QObject(parent)
    , frameSource(&frameSource)
    , stopTimer(false)
{
    timer.start(0, this);
}

void QFrameConverter::pause(bool val)
{
    pauseProcessing = val;
}

void QFrameConverter::timerEvent(QTimerEvent * ev)
{
    if (ev->timerId() != timer.timerId())
    {
        QObject::timerEvent(ev);
    }
    else
    {
        if (!pauseProcessing)
        {
            frameSource->getFrame(frame);

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

void QFrameConverter::setFrameSource(FrameSource &frameSource)
{
    this->frameSource = &frameSource;
    timer.start(0, this);
}

