#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QStandardItemModel>

class FileNode: public QObject, public QStandardItem {
    Q_OBJECT

public:
    enum Type { FILE, DIR };
    explicit FileNode(const QString& fileName);
    void appendChildren(const QVector<QStringList>& fileList);
    Type getType();
    QString getPath();
    static QVector<QStringList> parseFileListStr(const QString& fileListStr);
};


class FileModel: public QStandardItemModel {
    Q_OBJECT

public:
    explicit FileModel(QObject* parent = nullptr);
    void addNode(QStandardItem* parent, QStandardItem* child);
    FileNode* getRoot();


    Qt::ItemFlags flags(const QModelIndex &index) const;

    QMimeData *mimeData(const QModelIndexList &indexes) const;

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    QStringList mimeTypes() const;


private:
    FileNode* root;
};

#endif // FILEMODEL_H
