#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "calibparamsdialog.h"
#include "camerasetupdialog.h"
#include "cameraparametersdialog.h"

#include <QtWidgets>

#include <functional>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    scaleFactor(1.0),
    currentX(0),
    currentY(0),
    colorViewType(COLOR_RGB),
    currentCamera{-1,-1},
    workingDir(QDir::currentPath()),
    dmapSettingsModel(0, depthMapBuilder)
{
    ui->setupUi(this);

    scaleStatusLabel = new QLabel(this);
    coordsStatusLabel = new QLabel(this);   

    ui->statusbar->addWidget(scaleStatusLabel);
    ui->statusbar->addWidget(coordsStatusLabel);

    ui->imageLabel1->installEventFilter(this);
    ui->imageLabel1->setMouseTracking(true);
    ui->imageLabel2->installEventFilter(this);
    ui->imageLabel2->setMouseTracking(true);
    ui->depthMapLabel->installEventFilter(this);

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    updateActions();

    //initialze camera connections
    camera[0].setFrameCallback(std::bind(&FrameProcessor::setFrame, &frameProcessor[0], std::placeholders::_1));
    converter[0].setFrameSource(frameProcessor[0]);
    connect(&converter[0], SIGNAL(imageReady(QImage)), this, SLOT(setImage1(QImage)));

    camera[1].setFrameCallback(std::bind(&FrameProcessor::setFrame, &frameProcessor[1], std::placeholders::_1));
    converter[1].setFrameSource(frameProcessor[1]);
    connect(&converter[1], SIGNAL(imageReady(QImage)), this, SLOT(setImage2(QImage)));

    depthMapBuilder.setLeftSource(frameProcessor[0]);
    depthMapBuilder.setRightSource(frameProcessor[1]);
    converter[2].setFrameSource(depthMapBuilder);
    connect(&converter[2], SIGNAL(imageReady(QImage)), this, SLOT(setDepthImage(QImage)));

    converterThread[0].start();
    converter[0].moveToThread(&converterThread[0]);

    converterThread[1].start();
    converter[1].moveToThread(&converterThread[1]);

    converterThread[2].start();
    converter[2].moveToThread(&converterThread[2]);

    ui->depthSettingsTableView->setModel(&dmapSettingsModel);

    ui->actionCameraView->setChecked(true);
    this->converter[0].pause(false);
    this->converter[1].pause(false);
    this->converter[2].pause(true);
    depthMapBuilder.stopProcessing();

    ui->viewStackedWidget->setCurrentIndex(0);


    cloud.reset (new PointCloudT);    
    viewer.reset (new pcl::visualization::PCLVisualizer ("viewer", false));
    viewer->setBackgroundColor (0.1, 0.1, 0.1);
    viewer->addCoordinateSystem (1.0);
    ui->pc3dVtk->SetRenderWindow (viewer->getRenderWindow ());
    viewer->setupInteractor (ui->pc3dVtk->GetInteractor (), ui->pc3dVtk->GetRenderWindow ());
    ui->pc3dVtk->update ();
    //viewer->addPointCloud (cloud, "cloud");
    //viewer->resetCamera ();
    //ui->pc3dVtk->update ();
}

MainWindow::~MainWindow()
{
    for(int i = 0; i < camNumber + 1; ++i)
    {
        converter[i].stop();
        converterThread[i].quit();
        converterThread[i].wait();
    }
    delete ui;
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    auto imagePaint = [&](QLabel* imageLabel, QImage& img) -> bool
    {
        if (imageLabel != nullptr)
        {
            QPaintEvent* paintEvent = dynamic_cast<QPaintEvent*>(event);
            if (paintEvent != nullptr && !img.isNull())
            {
                QPainter painter(imageLabel);

                if (ui->scrollArea->widgetResizable())
                {
                    painter.drawImage(0,0, img.scaled(paintEvent->rect().size()));
                }
                else
                {
                    painter.drawImage(paintEvent->rect(), img, paintEvent->rect());
                }
                return true;
            }
        }
        return false;
    };

    if ((target == ui->imageLabel1 || target == ui->imageLabel2)
         && event->type() == QEvent::MouseMove)
    {
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (mouseEvent != nullptr)
        {
            currentX = mouseEvent->x();
            currentY = mouseEvent->y();
            updateStatusBar();
            ui->imageLabel1->update();
            ui->imageLabel2->update();
        }
    }
    else if ((target == ui->imageLabel1 || target == ui->imageLabel2)
             && event->type() == QEvent::Paint)
    {
        int cam = target == ui->imageLabel1 ? 0 : 1;
        return imagePaint(dynamic_cast<QLabel*>(target), currentQImage[cam]);
    }
    else if (target == ui->depthMapLabel && event->type() == QEvent::Paint)
    {
       return imagePaint(dynamic_cast<QLabel*>(target), depthQImage);
    }
    return QMainWindow::eventFilter(target, event);
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionFit_to_window_triggered()
{    
    ui->scrollArea->setWidgetResizable(true);
    scaleFactor = 1.0;
    for (int i = 0; i < camNumber; ++i)
    {
        frameProcessor[i].setOutScaleFactor(scaleFactor);
    }
    updateStatusBar();
}

void MainWindow::on_action100_triggered()
{
    ui->scrollArea->setWidgetResizable(false);
    scaleFactor = 1.0;
    for (int i = 0; i < camNumber; ++i)
    {
        frameProcessor[i].setOutScaleFactor(scaleFactor);
        switch(i)
        {
            case 0:
                ui->imageLabel1->resize(currentSize);
                break;
            case 1:
                ui->imageLabel2->resize(currentSize);
                break;
            default:
                throw std::logic_error("There is no camera with index" + std::to_string(i));
        }
    }

    QSize totalSize(currentSize.width() * 2, currentSize.height());
    ui->scrollArea->widget()->resize(totalSize);
    updateStatusBar();
}

void MainWindow::scaleImage(double factor)
{
    ui->scrollArea->setWidgetResizable(false);

    double scrollfactor = (scaleFactor + factor) / scaleFactor;
    scaleFactor += factor;

    for (int i = 0; i < camNumber; ++i)
    {
        frameProcessor[i].setOutScaleFactor(scaleFactor);

        switch(i)
        {
            case 0:
                ui->imageLabel1->resize(currentSize);
                break;
            case 1:
                ui->imageLabel2->resize(currentSize);
                break;
            default:
                throw std::logic_error("There is no camera with index" + std::to_string(i));
        }
    }

    QSize totalSize(currentSize.width() * 2, currentSize.height());
    ui->scrollArea->widget()->resize(totalSize);

    adjustScrollBar(ui->scrollArea->horizontalScrollBar(), scrollfactor);
    adjustScrollBar(ui->scrollArea->verticalScrollBar(), scrollfactor);
    updateStatusBar();
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}

void MainWindow::on_actionZoom_In_triggered()
{
    scaleImage(1);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    scaleImage(-1);
}

void MainWindow::updateActions()
{
    bool imageExist = false;
    int canSnap = 0;
    int realCamNum = 0;

    bool enable = false;
    for (int i = 0; i < camNumber; ++i)
    {
        if (!currentQImage[i].isNull())
        {
            imageExist = true;
            enable = true;
            canSnap += camera[i].canTakeSnapshoot() ? 1 : 0;
            ++realCamNum;
        }       
    }

    ui->actionSnapshot->setEnabled(canSnap > 0);
    ui->actionLoad_Calibration->setEnabled(enable);
    ui->actionUndistort->setEnabled(enable);
    ui->actionLoad_Stereo_Calibration->setEnabled(enable);
    ui->actionCameraParameters->setEnabled(enable);

    ui->actionDepthMapView->setEnabled(realCamNum > 1);
    ui->actionPC3DView->setEnabled(realCamNum > 1);
    ui->actionDepth_Map_snaphot->setEnabled(realCamNum > 1);

    if (imageExist)
    {                
        ui->actionZoom_In->setEnabled(true);
        ui->actionZoom_Out->setEnabled(true);
        ui->action100->setEnabled(true);
        ui->actionFit_to_window->setEnabled(true);
        ui->actionRGB->setEnabled(true);
        ui->actionGray->setEnabled(true);
        ui->actionRed_channel->setEnabled(true);
        ui->actionGreen_channel->setEnabled(true);
        ui->actionBlue_channel->setEnabled(true);       
    }
    else
    {
        ui->actionZoom_In->setEnabled(false);
        ui->actionZoom_Out->setEnabled(false);
        ui->action100->setEnabled(false);
        ui->actionFit_to_window->setEnabled(false);
        ui->actionRGB->setEnabled(false);
        ui->actionGray->setEnabled(false);
        ui->actionRed_channel->setEnabled(false);
        ui->actionGreen_channel->setEnabled(false);
        ui->actionBlue_channel->setEnabled(false);       
    }
}

void MainWindow::updateStatusBar()
{
    scaleStatusLabel->setText(QString("Scale : %1 ").arg(static_cast<int>(scaleFactor * 100)));
    coordsStatusLabel->setText(QString("Pos : %1, %2 ").arg(static_cast<int>(currentX / scaleFactor)).arg(static_cast<int>(currentY / scaleFactor)));
}

void MainWindow::updateColorViewType(COLOR_TYPE type)
{
    colorViewType = type;
   
    ui->actionRGB->setChecked(colorViewType == COLOR_RGB);
    ui->actionGray->setChecked(colorViewType == COLOR_GRAY);
    ui->actionRed_channel->setChecked(colorViewType == COLOR_RED);
    ui->actionGreen_channel->setChecked(colorViewType == COLOR_GREEN);
    ui->actionBlue_channel->setChecked(colorViewType == COLOR_BLUE);

    for (int i = 0; i < camNumber; ++i)
    {
        switch(type)
        {
        case COLOR_RGB:
            frameProcessor[i].setOutChannel(-1);
            frameProcessor[i].setOutGray(false);
            break;
        case COLOR_GRAY:
            frameProcessor[i].setOutChannel(-1);
            frameProcessor[i].setOutGray(true);
            break;
        case COLOR_RED:
            frameProcessor[i].setOutChannel(0);
            frameProcessor[i].setOutGray(false);
            break;
        case COLOR_GREEN:
            frameProcessor[i].setOutChannel(1);
            frameProcessor[i].setOutGray(false);
            break;
        case COLOR_BLUE:
            frameProcessor[i].setOutChannel(2);
            frameProcessor[i].setOutGray(false);
            break;
        }
    }
}

void MainWindow::on_actionRGB_triggered()
{
    updateColorViewType(COLOR_RGB);
}

void MainWindow::on_actionGray_triggered()
{
    updateColorViewType(COLOR_GRAY);
}

void MainWindow::on_actionRed_channel_triggered()
{
    updateColorViewType(COLOR_RED);
}

void MainWindow::on_actionGreen_channel_triggered()
{
    updateColorViewType(COLOR_GREEN);
}

void MainWindow::on_actionBlue_channel_triggered()
{
    updateColorViewType(COLOR_BLUE);
}

void MainWindow::closeEvent(QCloseEvent*)
{
}

void MainWindow::setImage(const QImage &img, int imgIndex)
{
    currentQImage[imgIndex] = img;

    QSize totalSize(currentSize.width() * 2, currentSize.height());
    ui->scrollArea->widget()->resize(totalSize);

    update();
    updateActions();
}

void MainWindow::setImage1(const QImage &img)
{
    currentSize = img.size();

    setImage(img, 0);

    ui->imageLabel1->update();
}

void MainWindow::setImage2(const QImage &img)
{
    currentSize = img.size();

    setImage(img, 1);

    ui->imageLabel2->update();
}

void MainWindow::setDepthImage(const QImage &img)
{
    currentSize = img.size();

    depthQImage = img;

    ui->scrollArea->widget()->resize(currentSize);

    update();
    updateActions();

    ui->depthMapLabel->update();
}

void MainWindow::on_actionSnapshot_triggered()
{
    QDir wdir(workingDir);
    if (!wdir.exists())
    {
        QDir().mkdir(workingDir);
    }

    for (int i = 0; i < camNumber; ++i)
    {
        const QString filename = workingDir + utils::getTimestampFileName(QString("/snap_%1").arg(i),"png");

        if (camera[i].canTakeSnapshoot())
        {
            camera[i].takeSnapshoot(filename.toStdString());
        }
    }
    updateActions();
}

void MainWindow::on_actionWork_Directory_triggered()
{
    QString dirname = QFileDialog::getExistingDirectory(this,
                                                       tr("Select a Directory"),
                                                       workingDir);
    if( !dirname.isNull() )
    {
        workingDir = dirname;
    }
}

void MainWindow::on_actionCalibrate_triggered()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select images"), workingDir);

    if (!fileNames.empty())
    {
        std::vector<std::string> files;
        files.reserve(fileNames.size());

        for(auto& s : fileNames)
        {
            files.push_back(s.toStdString());
        }

        CalibParamsDialog* calibParamsDlg = new CalibParamsDialog(this);

        calibParamsDlg->exec();

        const QString dataFileName = workingDir + utils::getTimestampFileName("/calib-data", "yml");
        bool ok = camera::utils::calibrate(calibParamsDlg->getSquareSize(),
                                    calibParamsDlg->getWCount(),
                                    calibParamsDlg->getHCount(),
                                    files,
                                    dataFileName.toStdString());
        if (ok)
        {
            QMessageBox::information(this, tr("Calibration"), tr("Finished succesfully!"), QMessageBox::Ok);
        }
        else
        {
            QMessageBox::warning(this, tr("Calibration"), tr("Failed!"));
        }
    }
}

void MainWindow::on_actionUndistort_triggered()
{    
    frameProcessor[0].setApplyUndistort(!frameProcessor[0].isUndistortApplied());
    frameProcessor[1].setApplyUndistort(!frameProcessor[1].isUndistortApplied());
    ui->actionUndistort->setChecked(frameProcessor[0].isUndistortApplied() &&
                                    frameProcessor[1].isUndistortApplied());
}


void MainWindow::on_actionLoad_Calibration_triggered()
{
    QStringList items;
    auto devList = camera::utils::getDeviceList();

    for (auto& dev : devList)
    {
        if (dev.id == camera[0].getId() ||
            dev.id == camera[1].getId())
        {
            QString camName = QString("%1 : %2").arg(dev.id).arg(QString::fromStdString(dev.name));
            items.append(camName);
        }
    }
    QString res = QInputDialog::getItem(this, "Choose camera for applying calibration", "Camera :", items);
    if (!res.isEmpty())
    {
        QStringList slist = res.split(':');
        auto itemId = slist.at(0).toInt();

        auto devId = camera[0].getId() == itemId ? 0 : 1;

        QString fileName = QFileDialog::getOpenFileName(this, "Select file with calibration data", workingDir);
        if (!fileName.isNull())
        {
            if (frameProcessor[devId].loadCalibrationParams(fileName.toStdString()))
            {
                QMessageBox::information(this, tr("Calibration"), tr("Loaded succesfully!"), QMessageBox::Ok);
            }
            else
            {
                QMessageBox::warning(this, tr("Calibration"), tr("Load failed!"));
            }
        }
    }
}


void MainWindow::on_actionCameraSetup_triggered()
{
    CameraSetupDialog* camSetupDlg = new CameraSetupDialog(this);

    auto ret = camSetupDlg->exec();

    if (ret == QDialog::Accepted)
    {
        //stop
        depthMapBuilder.stopProcessing();

        frameProcessor[0].stopProcessing();
        frameProcessor[1].stopProcessing();
        camera[0].stopCapture();
        camera[1].stopCapture();

        //start
        camera[0].startCapture(camSetupDlg->getLeftDeviceId(), camSetupDlg->getDeviceFormat());
        frameProcessor[0].startProcessing();

        camera[1].startCapture(camSetupDlg->getRightDeviceId(), camSetupDlg->getDeviceFormat());
        frameProcessor[1].startProcessing();       
    }
}

void MainWindow::on_actionCameraView_triggered()
{
    ui->actionCameraView->setChecked(true);
    if (ui->viewStackedWidget->currentIndex() != 0)
    {
        if(ui->actionDepthMapView->isChecked())
        {
            ui->actionDepthMapView->setChecked(false);
        }

        if(ui->actionPC3DView->isChecked())
        {
            ui->actionPC3DView->setChecked(false);
        }

        this->converter[0].pause(false);
        this->converter[1].pause(false);
        this->converter[2].pause(true);

        ui->viewStackedWidget->setCurrentIndex(0);

        depthMapBuilder.stopProcessing();
    }
}

void MainWindow::on_actionDepthMapView_triggered()
{
    ui->actionDepthMapView->setChecked(true);
    if (ui->viewStackedWidget->currentIndex() != 1)
    {
        if(ui->actionCameraView->isChecked())
        {
            ui->actionCameraView->setChecked(false);
        }

        if(ui->actionPC3DView->isChecked())
        {
            ui->actionPC3DView->setChecked(false);
        }


        this->converter[0].pause(true);
        this->converter[1].pause(true);
        this->converter[2].pause(false);

        ui->viewStackedWidget->setCurrentIndex(1);

        depthMapBuilder.startProcessing();
    }
}

void MainWindow::on_actionStereo_Calibrate_triggered()
{
    QStringList fileNamesLeft = QFileDialog::getOpenFileNames(this, tr("Select images from left camera"), workingDir);
    QStringList fileNamesRight = QFileDialog::getOpenFileNames(this, tr("Select images from right camera"), workingDir);

    if (!fileNamesLeft.empty() &&
        !fileNamesRight.empty())
    {
        std::vector<std::string> filesLeft;
        filesLeft.reserve(fileNamesLeft.size());

        for(auto& s : fileNamesLeft)
        {
            filesLeft.push_back(s.toStdString());
        }

        std::vector<std::string> filesRight;
        filesRight.reserve(fileNamesRight.size());

        for(auto& s : fileNamesRight)
        {
            filesRight.push_back(s.toStdString());
        }

        CalibParamsDialog* calibParamsDlg = new CalibParamsDialog(this);

        calibParamsDlg->exec();

        const QString dataFileName = workingDir + utils::getTimestampFileName("/stereo-calib-data", "yml");
        bool ok = camera::utils::stereoCalibrate(calibParamsDlg->getSquareSize(),
                                    calibParamsDlg->getWCount(),
                                    calibParamsDlg->getHCount(),
                                    filesLeft,
                                    filesRight,
                                    dataFileName.toStdString());
        if (ok)
        {
            QMessageBox::information(this, tr("Stereo Calibration"), tr("Finished succesfully!"), QMessageBox::Ok);
        }
        else
        {
            QMessageBox::warning(this, tr("Stereo Calibration"), tr("Failed!"));
        }
    }
}

void MainWindow::on_actionLoad_Stereo_Calibration_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select file with calibration data", workingDir);
    if (!fileName.isNull())
    {
        if (depthMapBuilder.loadCalibrationParams(fileName.toStdString()))
        {
            cv::Size imgSize(this->currentSize.width(), this->currentSize.height());
            cv::Mat mapx, mapy;
            cv::Rect roi;

            depthMapBuilder.getLeftMapping(imgSize, mapx, mapy, roi);
            frameProcessor[0].setUndistortMappings(mapx, mapy, roi);

            depthMapBuilder.getRightMapping(imgSize, mapx, mapy, roi);
            frameProcessor[1].setUndistortMappings(mapx, mapy, roi);

            QMessageBox::information(this, tr("Stereo Calibration"), tr("Loaded succesfully!"), QMessageBox::Ok);
        }
        else
        {
            QMessageBox::warning(this, tr("Stereo Calibration"), tr("Load failed!"));
        }
    }
}

void MainWindow::on_actionDepth_Map_snaphot_triggered()
{
    QDir wdir(workingDir);
    if (!wdir.exists())
    {
        QDir().mkdir(workingDir);
    }

    const QString filename = workingDir + utils::getTimestampFileName(QString("/depthmap"),"png");

    depthMapBuilder.saveDepthMap(filename.toStdString());

    updateActions();
}


void MainWindow::on_actionPC3DView_triggered()
{
    ui->actionPC3DView->setChecked(true);
    if (ui->viewStackedWidget->currentIndex() != 2)
    {
        if(ui->actionDepthMapView->isChecked())
        {
            ui->actionDepthMapView->setChecked(false);
        }

        if(ui->actionCameraView->isChecked())
        {
            ui->actionCameraView->setChecked(false);
        }

        this->converter[0].pause(true);
        this->converter[1].pause(true);
        this->converter[2].pause(true);

        depthMapBuilder.stopProcessing();
        ui->viewStackedWidget->setCurrentIndex(2);


        std::vector<cv::Vec3f> points;
        std::vector<cv::Vec3b> colors;
        depthMapBuilder.getPoints(points, colors);

        cloud->points.resize (points.size());

        // Fill the cloud with points
        for (size_t i = 0; i < cloud->points.size (); ++i)
        {
            cloud->points[i].x = points[i][0];
            cloud->points[i].y = points[i][1];
            cloud->points[i].z = points[i][2];

            cloud->points[i].r = colors[i][0];
            cloud->points[i].g = colors[i][1];
            cloud->points[i].b = colors[i][2];
        }

        cloud->width = points.size();
        cloud->height = 1;

        if (!viewer->updatePointCloud(cloud, "cloud"))
        {
            viewer->addPointCloud (cloud, "cloud");
        }

        viewer->updateCamera();
        ui->pc3dVtk->update ();
    }
}

void MainWindow::on_actionCameraParameters_triggered()
{
    CameraParametersDialog* cameraParamsDlg = new CameraParametersDialog(this);

    cameraParamsDlg->exec();
}
