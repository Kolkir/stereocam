#include "dmapsettingsmodel.h"

DMapSettingsModel::DMapSettingsModel(QObject *parent, DepthMapBuilder& dmapBuilder)
    : QAbstractTableModel(parent)
    , dmapBuilder(&dmapBuilder)
{

}

int DMapSettingsModel::rowCount(const QModelIndex &parent) const
{
    return ROWS;
}

int DMapSettingsModel::columnCount(const QModelIndex &parent) const
{
    return COLS;
}

QVariant DMapSettingsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        //names
        if (index.column() == 0)
        {
            switch(index.row())
            {
            case 0:
                return QString("NumDisparities");
            case 1:
                return QString("BlockSize");
            case 2:
                return QString("TextureThreshold");
            }
        }
        //values
        else if(index.column() == 1)
        {
            switch(index.row())
            {
            case 0:
                return dmapBuilder->getNumDisparities();
            case 1:
                return dmapBuilder->getBlockSize();
            case 2:
                return dmapBuilder->getTextureThreshold();
            }
        }
    }
    return QVariant();
}

QVariant DMapSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Name");
            case 1:
                return QString("Value");
            }
        }
    }
    return QVariant();
}

bool DMapSettingsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::EditRole)
    {
        if(index.column() == 1)
        {
            int ival = value.toInt();
            switch(index.row())
            {
            case 0:
                dmapBuilder->setNumDisparities(ival);
                break;
            case 1:
                dmapBuilder->setBlockSize(ival);
                break;
            case 2:
                dmapBuilder->setTextureThreshold(ival);
                break;           
            }
        }
    }
    return true;
}

Qt::ItemFlags DMapSettingsModel::flags(const QModelIndex & index) const
{
    if (index.column() == 1)
    {
        return  Qt::ItemIsEditable | Qt::ItemIsEnabled ;
    }
    return QAbstractTableModel::flags(index);
}
