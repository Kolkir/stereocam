#ifndef DMAPSETTINGSMODEL_H
#define DMAPSETTINGSMODEL_H

#include "depthmapbuilder.h"

#include <QAbstractTableModel>
#include <QString>

class DMapSettingsModel : public QAbstractTableModel
{
Q_OBJECT

private:
    static const int COLS = 2;
    static const int ROWS = 3;
public:
    DMapSettingsModel(QObject *parent, DepthMapBuilder& dmapBuilder);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const ;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
private:
    DepthMapBuilder* dmapBuilder;
};

#endif // DMAPSETTINGSMODEL_H
