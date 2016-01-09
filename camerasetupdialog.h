#ifndef CAMERADIALOG_H
#define CAMERADIALOG_H

#include "camerautils.h"
#include <QDialog>

namespace Ui {
class CameraSetupDialog;
}

class CameraSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CameraSetupDialog(QWidget *parent = 0);
    ~CameraSetupDialog();

    int getLeftDeviceId();
    int getRightDeviceId();

    camera::utils::VideoDevFormat getDeviceFormat();

private slots:
    void on_cameraLeft_cb_currentIndexChanged(const QString &arg1);

    void on_cameraRight_cb_currentIndexChanged(const QString &arg1);

private:
    void updateResolutions();
private:
    Ui::CameraSetupDialog *ui;
    std::vector<camera::utils::VideoDevFormat> commonFormats;
};

#endif // CAMERADIALOG_H
