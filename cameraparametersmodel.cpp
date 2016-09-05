#include "cameraparametersmodel.h"

CameraParametersModel::CameraParametersModel(QObject *parent, const std::vector<camera::utils::CameraParameter> &parameters, int devId)
    : QAbstractTableModel(parent)
    , parameters(parameters)
    , dev(devId)
{

}

int CameraParametersModel::rowCount(const QModelIndex& /*parent*/) const
{
    return static_cast<int>(parameters.size());
}

int CameraParametersModel::columnCount(const QModelIndex& /*parent*/) const
{
    return COLS;
}

QVariant CameraParametersModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        //names
        if (index.column() == 0)
        {
            return QString::fromStdString(parameters.at(index.row()).name);
        }
        //min val
        else if(index.column() == 1)
        {
            return parameters.at(index.row()).minimum;
        }
        //max val
        else if(index.column() == 2)
        {
            return parameters.at(index.row()).maximum;
        }
        //val
        else if(index.column() == 3)
        {
            return parameters.at(index.row()).value;
        }
    }
    return QVariant();
}

QVariant CameraParametersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Name");
            case 1:
                return QString("minimum");
            case 2:
                return QString("maximum");
            case 3:
                return QString("value");
            }
        }
    }
    return QVariant();
}

bool CameraParametersModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::EditRole)
    {
        if(index.column() == 3)
        {
            int ival = value.toInt();
            parameters.at(index.row()).value = ival;
            try
            {
                camera::utils::setCameraParameter(dev.fd(), parameters.at(index.row()));
            }
            catch(std::runtime_error&)
            {
                camera::utils::getCameraParameters(dev.fd(), parameters);
            }
        }
    }
    return true;
}

Qt::ItemFlags CameraParametersModel::flags(const QModelIndex & index) const
{
    if (index.column() == 3)
    {
        return  Qt::ItemIsEditable | Qt::ItemIsEnabled ;
    }
    return QAbstractTableModel::flags(index);
}
