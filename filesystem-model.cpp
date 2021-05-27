#include "filesystem-model.h"

#include <QIcon>

FileSystemModel::FileSystemModel(QObject *parent) : QStandardItemModel(parent)
{

}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole) {
        return QIcon::fromTheme("folder");
    }
    if (role == Qt::UserRole) {
        return "file://" + QStandardItemModel::data(index).toString();
    } else {
        return QStandardItemModel::data(index, role);
    }
}
