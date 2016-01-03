#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "calibparams.h"

#include <QtWidgets>

#include <functional>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    scaleFactor(1.0),
    currentX(0),
    currentY(0),
    colorViewType(COLOR_RGB),
    currentCamera(-1),
    converter(frameProcessor),
    workingDir(QDir::currentPath())
{
    ui->setupUi(this);

    scaleStatusLabel = new QLabel(this);
    coordsStatusLabel = new QLabel(this);
    colorStatusLabel = new QLabel(this);
    resolutionStatusLabel = new QLabel(this);

    ui->statusbar->addWidget(scaleStatusLabel);
    ui->statusbar->addWidget(coordsStatusLabel);
    ui->statusbar->addWidget(colorStatusLabel);
    ui->statusbar->addWidget(resolutionStatusLabel);

    ui->imageLabel->installEventFilter(this);
    ui->imageLabel->setMouseTracking(true);

    signalMapper = new QSignalMapper(this);

    //initialize camera menu
    int camerasCount = Camera::getDeviceCount();
    if (camerasCount > 0)
    {
        currentCamera = 0;
        for (int i = 0; i < camerasCount; ++i)
        {
            auto action = ui->menuCamera->addAction(QString("Camera : %1 ").arg(i));
            action->setCheckable(true);
            if (i == currentCamera)
            {
                action->setChecked(true);
            }

            connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));
            signalMapper->setMapping(action, i);
        }

        connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(on_actionCamera_triggered(int)));

        camera.setFrameCallback(std::bind(&FrameProcessor::setFrame, &frameProcessor, std::placeholders::_1));

        if (camera.startCapture(currentCamera))
        {
            frameProcessor.startProcessing();

            converterThread.start();
            converter.moveToThread(&converterThread);

            connect(&converter, SIGNAL(imageReady(QImage)), this, SLOT(setImage(QImage)));
        }
        else
        {
            throw std::logic_error("Camera initialization failed");
        }
    }

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    updateActions();
}

MainWindow::~MainWindow()
{
    converter.stop();
    converterThread.quit();
    converterThread.wait();
    delete ui;
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    if (target == ui->imageLabel && event->type() == QEvent::MouseMove)
    {
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (mouseEvent != nullptr)
        {
            currentX = mouseEvent->x();
            currentY = mouseEvent->y();
            updateStatusBar();
            ui->imageLabel->update();
        }
    }
    else if (target == ui->imageLabel && event->type() == QEvent::Paint)
    {
        QPaintEvent* paintEvent = dynamic_cast<QPaintEvent*>(event);
        if (paintEvent != nullptr && !currentQImage.isNull())
        {
            QPainter painter(ui->imageLabel);

            if (ui->scrollArea->widgetResizable())
            {
                painter.drawImage(0,0, currentQImage.scaled(paintEvent->rect().size()));
            }
            else
            {
                painter.drawImage(paintEvent->rect(), currentQImage, paintEvent->rect());
            }           

            return true;
        }
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
    frameProcessor.setOutScaleFactor(scaleFactor);
    updateStatusBar();
}

void MainWindow::on_action100_triggered()
{
    ui->scrollArea->setWidgetResizable(false);
    scaleFactor = 1.0;
    frameProcessor.setOutScaleFactor(scaleFactor);
    ui->imageLabel->resize(currentSize);
    ui->scrollArea->widget()->resize(currentSize);
    updateStatusBar();
}

void MainWindow::scaleImage(double factor)
{
    ui->scrollArea->setWidgetResizable(false);

    double scrollfactor = (scaleFactor + factor) / scaleFactor;
    scaleFactor += factor;

    frameProcessor.setOutScaleFactor(scaleFactor);

    ui->imageLabel->resize(currentSize);
    ui->scrollArea->widget()->resize(currentSize);

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
    if (!currentQImage.isNull())
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

        ui->action320_x_240->setEnabled(true);
        ui->action640_x_480->setEnabled(true);
        ui->action1024_x_768->setEnabled(true);
        ui->action1280_x_1024->setEnabled(true);

        if (camera.canTakeSnapshoot())
        {
            ui->actionSnapshoot->setEnabled(true);
        }
        else
        {
            ui->actionSnapshoot->setEnabled(false);
        }
        ui->actionCalibrate->setEnabled(true);
        ui->actionApply_undistort->setEnabled(true);
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

        ui->action320_x_240->setEnabled(false);
        ui->action640_x_480->setEnabled(false);
        ui->action1024_x_768->setEnabled(false);
        ui->action1280_x_1024->setEnabled(false);
        ui->actionSnapshoot->setEnabled(false);
        ui->actionCalibrate->setEnabled(false);
        ui->actionApply_undistort->setEnabled(false);
    }
}

void MainWindow::updateStatusBar()
{
    scaleStatusLabel->setText(QString("Scale : %1 ").arg(static_cast<int>(scaleFactor * 100)));
    coordsStatusLabel->setText(QString("Pos : %1, %2 ").arg(static_cast<int>(currentX / scaleFactor)).arg(static_cast<int>(currentY / scaleFactor)));

    if (!currentQImage.isNull())
    {
        if (currentX < currentQImage.width() &&
            currentY < currentQImage.height())
        {
            QRgb color = currentQImage.pixel(currentX, currentY);
            colorStatusLabel->setText(QString("Color : %1 %2 %3").arg(qRed(color)).arg(qGreen(color)).arg(qBlue(color)));
        }

        auto res = camera.getResolution();
        resolutionStatusLabel->setText(QString("Resolution : %1 x %2").arg(res.width).arg(res.height));
    }
}

void MainWindow::updateColorViewType(COLOR_TYPE type)
{
    colorViewType = type;
   
    ui->actionRGB->setChecked(colorViewType == COLOR_RGB);
    ui->actionGray->setChecked(colorViewType == COLOR_GRAY);
    ui->actionRed_channel->setChecked(colorViewType == COLOR_RED);
    ui->actionGreen_channel->setChecked(colorViewType == COLOR_GREEN);
    ui->actionBlue_channel->setChecked(colorViewType == COLOR_BLUE);

    switch(type)
    {
    case COLOR_RGB:
        frameProcessor.setOutChannel(-1);
        frameProcessor.setOutGray(false);
        break;
    case COLOR_GRAY:
        frameProcessor.setOutChannel(-1);
        frameProcessor.setOutGray(true);
        break;
    case COLOR_RED:
        frameProcessor.setOutChannel(0);
        frameProcessor.setOutGray(false);
        break;
    case COLOR_GREEN:
        frameProcessor.setOutChannel(1);
        frameProcessor.setOutGray(false);
        break;
    case COLOR_BLUE:
        frameProcessor.setOutChannel(2);
        frameProcessor.setOutGray(false);
        break;
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

void MainWindow::on_actionCamera_triggered(int id)
{
    for(auto a : ui->menuCamera->actions())
    {
        a->setChecked(false);
    }
    QAction* action = static_cast<QAction*>(signalMapper->mapping(id));
    action->setChecked(true);

    camera.startCapture(id);
}

void MainWindow::setImage(const QImage &img)
{
    currentQImage = img;
    currentSize = img.size();

    ui->imageLabel->resize(currentSize);
    ui->scrollArea->widget()->resize(currentSize);

    ui->imageLabel->update();
    update();
    updateActions();
}

void MainWindow::on_action320_x_240_triggered()
{
    camera.setResolution(cv::Size(320,200));
}

void MainWindow::on_action640_x_480_triggered()
{
    camera.setResolution(cv::Size(640,480));
}

void MainWindow::on_action1024_x_768_triggered()
{
    camera.setResolution(cv::Size(1024,768));
}

void MainWindow::on_action1280_x_1024_triggered()
{
    camera.setResolution(cv::Size(1280,1024));
}

void MainWindow::on_actionSnapshoot_triggered()
{
    QDir wdir(workingDir);
    if (!wdir.exists())
    {
        QDir().mkdir(workingDir);
    }

    const QString filename = workingDir + utils::getTimestampFileName("/snap","png");

    camera.takeSnapshoot(filename.toStdString());
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

        CalibParams* calibParamsDlg = new CalibParams(this);

        calibParamsDlg->exec();

        const QString dataFileName = workingDir + utils::getTimestampFileName("/calib-data", "yml");
        bool ok = Camera::calibrate(calibParamsDlg->getSquareSize(),
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

void MainWindow::on_actionApply_undistort_triggered()
{
    frameProcessor.setApplyUndistort(!frameProcessor.isUndistortApplied());
    ui->actionApply_undistort->setChecked(frameProcessor.isUndistortApplied());
}

void MainWindow::on_actionLoad_calibration_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select file with calibration data", workingDir);
    if (!fileName.isNull())
    {
        if (frameProcessor.loadCalibrationParams(fileName.toStdString()))
        {
            QMessageBox::information(this, tr("Calibration"), tr("Loaded succesfully!"), QMessageBox::Ok);
        }
        else
        {
            QMessageBox::warning(this, tr("Calibration"), tr("Load failed!"));
        }
    }
}
