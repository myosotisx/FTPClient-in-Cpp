#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QStandardItemModel>

class FileNode: public QObject, public QStandardItem {
    Q_OBJECT

public:
    enum Type { EMPTY, FILE, DIR };
    explicit FileNode(const QString& fileName);
    void appendChildren(const QVector<QStringList>& fileList);
    void clearChildren();
    Type getType();
    QString getPath();
    static QVector<QStringList> parseFileListStr(const QString& fileListStr);
    static void appendFakeNode(FileNode* node);
    static FileNode* findNodeByPath(FileNode* root, const QString& path);

private:

};


class FileModel: public QStandardItemModel {
    Q_OBJECT

public:
    explicit FileModel(QObject* parent = nullptr, const QString& rootPath = "/");
    void addNode(QStandardItem* parent, QStandardItem* child);
    FileNode* getRoot();
    void initRoot(const QString& rootPath);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    QStringList mimeTypes() const;

signals:
    void transfer(const QString& srcPath, const QString& srcFile,
                  const QString& dstPath);


private:
    FileNode* root;
};

#endif // FILEMODEL_H
