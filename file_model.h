#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QStandardItemModel>

class FileNode: public QObject, public QStandardItem {
    Q_OBJECT

public:
    FileNode(const QString& fileName);
    QList<QStringList>& parseFileListStr(const QString& fileListStr);
    void addChildren(const QList<QStringList>& fileList);
};

class FileModel: public QStandardItemModel {
    Q_OBJECT

public:
    FileModel(QStandardItemModel* parent = nullptr);
    void addNode(QStandardItem* parent, QStandardItem* child);
};

#endif // FILEMODEL_H
