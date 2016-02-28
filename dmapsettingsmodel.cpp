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
                return QString("MinDisparity");
            case 1:
                return QString("NumDisparities");
            case 2:
                return QString("BlockSize");
            case 3:
                return QString("P1");
            case 4:
                return QString("P2");
            case 5:
                return QString("disp12MaxDiff");
            case 6:
                return QString("preFilterCap");
            case 7:
                return QString("uniquenessRatio");
            case 8:
                return QString("speckleWindowSize");
            case 9:
                return QString("speckleRange");
            case 10:
                return QString("mode");
            }
        }
        //values
        else if(index.column() == 1)
        {
            switch(index.row())
            {
            case 0:
                return dmapBuilder->getMinDisparity();
            case 1:
                return dmapBuilder->getNumDisparities();
            case 2:
                return dmapBuilder->getBlockSize();
            case 3:
                return dmapBuilder->getP1();
            case 4:
                return dmapBuilder->getP2();
            case 5:
                return dmapBuilder->getDisp12MaxDiff();
            case 6:
                return dmapBuilder->getPreFilterCap();
            case 7:
                return dmapBuilder->getUniquenessRatio();
            case 8:
                return dmapBuilder->getSpeckleWindowSize();
            case 9:
                return dmapBuilder->getSpeckleRange();
            case 10:
                return dmapBuilder->getMode();
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
                dmapBuilder->setMinDisparity(ival);
                break;
            case 1:
                dmapBuilder->setNumDisparities(ival);
                break;
            case 2:
                dmapBuilder->setBlockSize(ival);
                break;           
            case 3:
                dmapBuilder->setP1(ival);
                break;
            case 4:
                dmapBuilder->setP2(ival);
                break;
            case 5:
                dmapBuilder->setDisp12MaxDiff(ival);
                break;
            case 6:
                dmapBuilder->setPreFilterCap(ival);
                break;
            case 7:
                dmapBuilder->setUniquenessRatio(ival);
                break;
            case 8:
                dmapBuilder->setSpeckleWindowSize(ival);
                break;
            case 9:
                dmapBuilder->setSpeckleRange(ival);
                break;
            case 10:
                dmapBuilder->setMode(ival);
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
