#include "camerasetupdialog.h"
#include "ui_camerasetupdialog.h"


CameraSetupDialog::CameraSetupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CameraSetupDialog)
{
    ui->setupUi(this);

    auto devList = camera::utils::getDeviceList();

    for (auto& dev : devList)
    {
        QString camName = QString("%1 : %2").arg(dev.id).arg(QString::fromStdString(dev.name));
        ui->cameraLeft_cb->addItem(camName,QVariant(dev.id));
    }

}

CameraSetupDialog::~CameraSetupDialog()
{
    delete ui;
}

int CameraSetupDialog::getLeftDeviceId()
{
    return ui->cameraLeft_cb->currentData().value<int>();
}

int CameraSetupDialog::getRightDeviceId()
{
    return ui->cameraRight_cb->currentData().value<int>();
}

camera::utils::VideoDevFormat CameraSetupDialog::getDeviceFormat()
{
    auto resIndex = ui->resolution_cb->currentIndex();
    return commonFormats[resIndex];
}

void CameraSetupDialog::on_cameraLeft_cb_currentIndexChanged(const QString &arg1)
{
    ui->cameraRight_cb->clear();

    auto devList = camera::utils::getDeviceList();

    for (auto& dev : devList)
    {
        QString camName = QString("%1 : %2").arg(dev.id).arg(QString::fromStdString(dev.name));
        if (camName != arg1)
        {
            ui->cameraRight_cb->addItem(camName,QVariant(dev.id));
        }
    }

    updateResolutions();
}

void CameraSetupDialog::on_cameraRight_cb_currentIndexChanged(const QString &arg1)
{
    updateResolutions();
}

void CameraSetupDialog::updateResolutions()
{
    auto rightDevId = ui->cameraRight_cb->currentData().value<int>();
    auto leftDevId = ui->cameraLeft_cb->currentData().value<int>();

    auto rightFormats = camera::utils::getDeviceFormats(rightDevId);
    auto leftFormats = camera::utils::getDeviceFormats(leftDevId);

    commonFormats.clear();

    for (auto& rfmt : rightFormats)
    {
        for (auto& lfmt : leftFormats)
        {
            if (rfmt.pixelformat == lfmt.pixelformat &&
                rfmt.width == lfmt.width &&
                rfmt.height == lfmt.height)
            {
                commonFormats.push_back(rfmt);
                break;
            }
        }
    }

    ui->resolution_cb->clear();
    for (auto& fmt : commonFormats)
    {
        ui->resolution_cb->addItem(QString::fromStdString(fmt.description));
    }
}
