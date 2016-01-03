#include "calibparams.h"
#include "ui_calibparams.h"

CalibParams::CalibParams(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibParams)
{
    ui->setupUi(this);
}

CalibParams::~CalibParams()
{
    delete ui;
}

int CalibParams::getSquareSize() const
{
    return ui->squareSizeSpin->value();
}

int CalibParams::getWCount() const
{
    return ui->wcountSpin->value();
}

int CalibParams::getHCount() const
{
    return ui->hcountSpin->value();
}
