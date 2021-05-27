#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QStandardItemModel>

class FileSystemModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit FileSystemModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

signals:

};
#endif // FILESYSTEMMODEL_H
