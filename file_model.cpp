#include "file_model.h"

#include <QMimeData>
#include <QDebug>

FileNode::FileNode(const QString& filename):
    QStandardItem(filename) {

}

void FileNode::appendChildren(const QVector<QStringList>& fileList) {
    int len = fileList.length();
    for (int i = 0;i < len;i++) {
        FileNode* node = new FileNode(fileList[i][0]);
        appendRow(node);
        setChild(node->index().row(), 1, new FileNode(fileList[i][1]));
        setChild(node->index().row(), 2, new FileNode(fileList[i][2]));
        setChild(node->index().row(), 3, new FileNode(fileList[i][3]));
        setChild(node->index().row(), 4, new FileNode(fileList[i][4]));
    }
}

FileNode::Type FileNode::getType() {
    QModelIndex index = model()->indexFromItem(this).siblingAtColumn(3);
    if (model()->itemFromIndex(index)->text()[0] == 'd') return DIR;
    else return FILE;
}

QString FileNode::getPath() {
    QStandardItem* p = this;
    QString path = p->text();
    p = p->parent();
    while (p) {
        int len = p->text().length();
        if (len && p->text()[len-1] == '/') {
            path = p->text()+path;
        }
        else path = p->text()+'/'+path;
        p = p->parent();
    }
    return path;
}

QVector<QStringList> FileNode::parseFileListStr(const QString& fileListStr) {
    // QStringList infoList = fileListStr.split(QRegExp("[\r][\n]"));
    QStringList infoList = fileListStr.split("\r\n", QString::SkipEmptyParts);
    QRegExp regex("[ ]+");
    QVector<QStringList> fileList;
    QString filename, size, modifyDate, authority, ownerNGroup;
    int len = infoList.length();
    for (int i = 0;i < len;i++) {
        // qDebug() << infoList[i].section(' ', 8, -1, QString::SectionSkipEmpty);
        filename = infoList[i].section(' ', 8, -1, QString::SectionSkipEmpty);
        size = infoList[i].section(' ', 4, 4, QString::SectionSkipEmpty);
        modifyDate = infoList[i].section(' ', 5, 7, QString::SectionSkipEmpty);
        authority = infoList[i].section(' ', 0, 0, QString::SectionSkipEmpty);
        ownerNGroup = infoList[i].section(' ', 2, 2, QString::SectionSkipEmpty)+" "
                      +infoList[i].section(' ', 3, 3, QString::SectionSkipEmpty);

        if (!filename.isEmpty()) {
            QStringList file;
            file << filename << size << modifyDate << authority << ownerNGroup;
            fileList.append(file);
        }
    }
    qDebug() << fileList;
    return fileList;
}

FileModel::FileModel(QObject* parent):
    QStandardItemModel(parent) {
    QStringList header;
    header << "文件名" << "文件大小" << "最近修改" << "权限" << "所有者/组";
    setHorizontalHeaderLabels(header);

    root = new FileNode("/");
    appendRow(root);
}

FileNode* FileModel::getRoot() {
    return root;
}

Qt::ItemFlags FileModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags = flags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    return flags;
}

QMimeData* FileModel::mimeData(const QModelIndexList &indexes) const {
    if (indexes.count() <= 0) return 0;

    FileNode* node = dynamic_cast<FileNode*>(itemFromIndex(indexes.at(0)));

    qDebug() << node->getPath();

    QMimeData *data = new QMimeData;
    data->setData("filename", itemFromIndex(indexes.at(0))->text().toLocal8Bit());
    data->setData("size", itemFromIndex(indexes.at(1))->text().toLocal8Bit());
    return data;
}

bool FileModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    QString filename = QString::fromLocal8Bit(data->data("filename"));
    QString size = QString::fromLocal8Bit(data->data("size"));
    qDebug() << "filename:" << filename << "size:" << size;
}

QStringList FileModel::mimeTypes() const {
    QStringList types;
    types << "filename" << "size";
    return types;
}


