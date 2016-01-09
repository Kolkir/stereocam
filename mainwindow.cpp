#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "calibparamsdialog.h"
#include "camerasetupdialog.h"

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

    ui->imageLabel1->installEventFilter(this);
    ui->imageLabel1->setMouseTracking(true);
    ui->imageLabel2->installEventFilter(this);
    ui->imageLabel2->setMouseTracking(true);


    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    updateActions();
}

MainWindow::~MainWindow()
{
    for(int i = 0; i < camNumber; ++i)
    {
        converter[i].stop();
        converterThread[i].quit();
        converterThread[i].wait();
    }
    delete ui;
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
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
        auto imagePaint = [&](QLabel* imageLabel, int cam) -> bool
        {
            if (imageLabel != nullptr)
            {
                QPaintEvent* paintEvent = dynamic_cast<QPaintEvent*>(event);
                if (paintEvent != nullptr && !currentQImage[cam].isNull())
                {
                    QPainter painter(imageLabel);

                    if (ui->scrollArea->widgetResizable())
                    {
                        painter.drawImage(0,0, currentQImage[cam].scaled(paintEvent->rect().size()));
                    }
                    else
                    {
                        painter.drawImage(paintEvent->rect(), currentQImage[cam], paintEvent->rect());
                    }
                    return true;
                }
            }
            return false;
        };

        int cam = target == ui->imageLabel1 ? 0 : 1;
        return imagePaint(dynamic_cast<QLabel*>(target), cam);
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
    bool canSnap = false;

    bool enable = false;
    for (int i = 0; i < camNumber; ++i)
    {
        if (!currentQImage[i].isNull())
        {
            imageExist = true;
            enable = true;
            canSnap &= camera[i].canTakeSnapshoot();
        }       
    }

    ui->actionSnapshot->setEnabled(canSnap);
    ui->actionLoad_Calibration->setEnabled(enable);
    ui->actionCalibrate->setEnabled(enable);
    ui->actionUndistort->setEnabled(enable);

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

    auto res1 = camera[0].getResolution();
    auto res2 = camera[1].getResolution();
    resolutionStatusLabel->setText(QString("Resolution : %1 x %2, %3 x %4").arg(res1.width).arg(res1.height).arg(res2.width).arg(res2.height));
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

void MainWindow::on_actionUndistort_triggered()
{    
    frameProcessor[0].setApplyUndistort(!frameProcessor[0].isUndistortApplied());
    frameProcessor[1].setApplyUndistort(!frameProcessor[1].isUndistortApplied());
    ui->actionUndistort->setChecked(frameProcessor[0].isUndistortApplied() &&
                                    frameProcessor[1].isUndistortApplied());
}


void MainWindow::on_actionLoad_Calibration_triggered()
{
    /*
    QString fileName = QFileDialog::getOpenFileName(this, "Select file with calibration data", workingDir);
    if (!fileName.isNull())
    {
        if (frameProcessor[0].loadCalibrationParams(fileName.toStdString()))
        {
            QMessageBox::information(this, tr("Calibration"), tr("Loaded succesfully!"), QMessageBox::Ok);
        }
        else
        {
            QMessageBox::warning(this, tr("Calibration"), tr("Load failed!"));
        }
    }
    */
}


void MainWindow::on_actionCameraSetup_triggered()
{
    CameraSetupDialog* camSetupDlg = new CameraSetupDialog(this);

    camSetupDlg->exec();
}
