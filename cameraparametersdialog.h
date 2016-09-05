#ifndef CAMERAPARAMETERSDIALOG_H
#define CAMERAPARAMETERSDIALOG_H

#include "cameraparametersmodel.h"

#include <QDialog>

#include <memory>

namespace Ui {
class CameraParametersDialog;
}

class CameraParametersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CameraParametersDialog(QWidget *parent = 0);
    ~CameraParametersDialog();

private slots:
    void on_cameraSelectCombo_currentIndexChanged(const QString &arg1);

private:
    Ui::CameraParametersDialog *ui;
    std::shared_ptr<CameraParametersModel> model;
};

#endif // CAMERAPARAMETERSDIALOG_H
