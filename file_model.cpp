#include "file_model.h"

#include <QDebug>

FileNode::FileNode(const QString& filename):
    QStandardItem(filename) {

}

QList<QStringList>& parseFileListStr(const QString& fileListStr) {
    QStringList infoList = fileListStr.split(QRegExp("[\r][\n]"));
    for (int i = 0;i < infoList.length();i++) qDebug() << infoList[i];
}

FileModel::FileModel(QStandardItemModel* parent):
    QStandardItemModel(parent) {
    QStringList header;
    header << "文件名" << "文件大小" << "最近修改" << "权限" << "所有者/组";
    setHorizontalHeaderLabels(header);

}
