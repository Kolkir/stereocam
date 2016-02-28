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
#include "depthmapbuilder.h"
#include "qframeconverter.h"
#include "dmapsettingsmodel.h"

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
    static const int camNumber = 2;
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

    void setImage1(const QImage & img);
    void setImage2(const QImage & img);
    void setDepthImage(const QImage & img);

    void on_actionSnapshot_triggered();

    void on_actionWork_Directory_triggered();

    void on_actionCalibrate_triggered();

    void on_actionUndistort_triggered();

    void on_actionLoad_Calibration_triggered();

    void on_actionCameraSetup_triggered();

    void on_actionCameraView_triggered();

    void on_actionDepthMapView_triggered();

    void on_actionStereo_Calibrate_triggered();

    void on_actionLoad_Stereo_Calibration_triggered();

    void on_actionDepth_Map_snaphot_triggered();

private:

    void setImage(const QImage &img, int imgIndex);
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void updateActions();
    void updateStatusBar();

    bool eventFilter(QObject *target, QEvent *event);

    void updateColorViewType(COLOR_TYPE type);

private:

    QImage currentQImage[camNumber]; //for paint event
    QImage depthQImage;
    QSize currentSize;

    Ui::MainWindow *ui;
    
    double scaleFactor;
    int currentX;
    int currentY;
    
    QLabel* scaleStatusLabel;
    QLabel* coordsStatusLabel;

    COLOR_TYPE colorViewType;    

    FrameProcessor frameProcessor[camNumber];
    Camera camera[camNumber];
    int currentCamera[camNumber];

    QThread converterThread[camNumber + 1];
    QFrameConverter converter[camNumber + 1];

    QString workingDir;

    DepthMapBuilder depthMapBuilder;
    DMapSettingsModel dmapSettingsModel;
};

#endif // MAINWINDOW_H
