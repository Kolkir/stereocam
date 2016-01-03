#ifndef CALIBPARAMS_H
#define CALIBPARAMS_H

#include <QDialog>

namespace Ui {
class CalibParams;
}

class CalibParams : public QDialog
{
    Q_OBJECT

public:
    explicit CalibParams(QWidget *parent = 0);
    ~CalibParams();

    int getSquareSize() const;
    int getWCount() const;
    int getHCount() const;

private:
    Ui::CalibParams *ui;
};

#endif // CALIBPARAMS_H
