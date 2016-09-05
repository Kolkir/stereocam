#include "cameraparametersdialog.h"
#include "ui_cameraparametersdialog.h"

#include "camerautils.h"

CameraParametersDialog::CameraParametersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CameraParametersDialog)
{
    ui->setupUi(this);

    auto devList = camera::utils::getDeviceList();

    for (auto& dev : devList)
    {
        QString camName = QString("%1 : %2").arg(dev.id).arg(QString::fromStdString(dev.name));
        ui->cameraSelectCombo->addItem(camName,QVariant(dev.id));
    }
}

CameraParametersDialog::~CameraParametersDialog()
{
    delete ui;
}

void CameraParametersDialog::on_cameraSelectCombo_currentIndexChanged(const QString& /*arg1*/)
{
    auto devId = ui->cameraSelectCombo->currentData().value<int>();

    std::vector<camera::utils::CameraParameter> parameters;

    camera::utils::ScopedVideoDevice dev(devId);

    camera::utils::getCameraParameters(dev.fd(), parameters);

    model = std::make_shared<CameraParametersModel>(nullptr, parameters, devId);

    ui->parametersTable->setModel(model.get());
}
