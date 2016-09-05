#ifndef CAMERAPARAMETERSMODEL_H
#define CAMERAPARAMETERSMODEL_H

#include "camerautils.h"

#include <QAbstractTableModel>
#include <QString>

class CameraParametersModel : public QAbstractTableModel
{
Q_OBJECT

private:
    static const int COLS = 4;
public:
    CameraParametersModel(QObject *parent, const std::vector<camera::utils::CameraParameter>& parameters, int devId);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const ;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
private:
    std::vector<camera::utils::CameraParameter> parameters;
    camera::utils::ScopedVideoDevice dev;
};

#endif // CAMERAPARAMETERSMODEL_H
