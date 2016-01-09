#ifndef CALIBPARAMS_H
#define CALIBPARAMS_H

#include <QDialog>

namespace Ui {
class CalibParamsDialog;
}

class CalibParamsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibParamsDialog(QWidget *parent = 0);
    ~CalibParamsDialog();

    int getSquareSize() const;
    int getWCount() const;
    int getHCount() const;

private:
    Ui::CalibParamsDialog *ui;
};

#endif // CALIBPARAMS_H
