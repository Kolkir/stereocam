#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qsize.h>
#include <qsignalmapper.h>
#include <qthread.h>

#include "camera.h"
#include "frameprocessor.h"
#include "qframeconverter.h"

#include <opencv2/opencv.hpp>

#include <memory>

namespace Ui {
class MainWindow;
}

enum COLOR_TYPE
{
    COLOR_RGB,
    COLOR_GRAY,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:

    void on_actionExit_triggered();
    void on_actionFit_to_window_triggered();
    void on_action100_triggered();
    void on_actionZoom_In_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionRGB_triggered();
    void on_actionGray_triggered();
    void on_actionRed_channel_triggered();
    void on_actionGreen_channel_triggered();
    void on_actionBlue_channel_triggered();
    void on_actionCamera_triggered(int id);
    void setImage(const QImage & img);

    void on_action320_x_240_triggered();

    void on_action640_x_480_triggered();

    void on_action1024_x_768_triggered();

    void on_action1280_x_1024_triggered();

    void on_actionSnapshoot_triggered();

    void on_actionWork_Directory_triggered();

    void on_actionCalibrate_triggered();

    void on_actionApply_undistort_triggered();

    void on_actionLoad_calibration_triggered();

private:

    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void updateActions();
    void updateStatusBar();

    bool eventFilter(QObject *target, QEvent *event);

    void updateColorViewType(COLOR_TYPE type);

private:

    QImage currentQImage; //for paint event
    QSize currentSize;

    Ui::MainWindow *ui;
    
    double scaleFactor;
    int currentX;
    int currentY;
    
    QLabel* scaleStatusLabel;
    QLabel* coordsStatusLabel;
    QLabel* colorStatusLabel;
    QLabel* resolutionStatusLabel;

    COLOR_TYPE colorViewType;    

    FrameProcessor frameProcessor;
    Camera camera;
    int currentCamera;

    QSignalMapper *signalMapper;

    QThread converterThread;
    QFrameConverter converter;

    QString workingDir;
};

#endif // MAINWINDOW_H
