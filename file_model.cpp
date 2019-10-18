#include "file_model.h"

#include <QDebug>

FileNode::FileNode(const QString& filename):
    QStandardItem(filename) {

}

QVector<QStringList>& FileNode::parseFileListStr(const QString& fileListStr) {
    // QStringList infoList = fileListStr.split(QRegExp("[\r][\n]"));
    QStringList infoList = fileListStr.split("\r\n", QString::SkipEmptyParts);
    QRegExp regex("[ ]+");
    QVector<QStringList> fileList;
    int len = infoList.length();
    for (int i = 0;i < len;i++) {
        qDebug() << infoList[i].section(' ', 8, -1, QString::SectionSkipEmpty);
    }
    // qDebug() << fileList;
    return fileList;
}

FileModel::FileModel(QStandardItemModel* parent):
    QStandardItemModel(parent) {
    QStringList header;
    header << "文件名" << "文件大小" << "最近修改" << "权限" << "所有者/组";
    setHorizontalHeaderLabels(header);

}
