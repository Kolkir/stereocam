#include "calibparamsdialog.h"
#include "ui_calibparamsdialog.h"

CalibParamsDialog::CalibParamsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibParamsDialog)
{
    ui->setupUi(this);
}

CalibParamsDialog::~CalibParamsDialog()
{
    delete ui;
}

int CalibParamsDialog::getSquareSize() const
{
    return ui->squareSizeSpin->value();
}

int CalibParamsDialog::getWCount() const
{
    return ui->wcountSpin->value();
}

int CalibParamsDialog::getHCount() const
{
    return ui->hcountSpin->value();
}
