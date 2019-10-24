#include "file_model.h"

#include <QMimeData>
#include <QDebug>
#include <QApplication>
#include <QStyle>
#include <QFileIconProvider>

FileNode::FileNode(const QString& filename):
    QStandardItem(filename) {}

FileNode::FileNode(const QIcon& icon, const QString& filename) :
    QStandardItem(icon, filename) {}

void FileNode::appendChildren(const QVector<QStringList>& fileList) {
    int len = fileList.length();
    clearChildren();
    for (int i = 0;i < len;i++) {
        FileNode* node = new FileNode(fileList[i][0]);
        appendRow(node);
        setChild(node->index().row(), 1, new FileNode(fileList[i][1]));
        setChild(node->index().row(), 2, new FileNode(fileList[i][2]));
        setChild(node->index().row(), 3, new FileNode(fileList[i][3]));
        setChild(node->index().row(), 4, new FileNode(fileList[i][4]));
        node->setIcon(autoGetIcon(node->getType(), node->text()));
        if (fileList[i][3][0] == 'd') {
            appendFakeNode(node);
        }
    }
    if (!rowCount()) appendFakeNode(this);
}

FileNode* FileNode::appendChildDir() {
    FileNode* node = new FileNode(autoGetIcon(DIR), "New_Directory");
    appendRow(node);
    setChild(node->index().row(), 1, new FileNode(""));
    setChild(node->index().row(), 2, new FileNode(""));
    setChild(node->index().row(), 3, new FileNode("d---------"));
    setChild(node->index().row(), 4, new FileNode(""));
    return node;
}

FileNode::Type FileNode::getType() {
    QStandardItem* _this = this;
    if (!_this->parent()) return DIR; // 根目录
    QModelIndex index = model()->indexFromItem(this).siblingAtColumn(3);
    QString text = model()->itemFromIndex(index)->text();
    if (text[0] == 'd') return DIR;
    else if (text.isEmpty()) return EMPTY;
    else return FILE;
}

QString FileNode::getPath() {
    if (getType() == EMPTY) return QString();
    QStandardItem* p = this;
    if (getType() == FILE) {
        p = p->parent();
    }
    QString path;
    if (p->text().contains(' ')) path = "\""+p->text()+"\"";
    else path = p->text();
    p = p->parent();
    while (p) {
        int len = p->text().length();
        QString dirName;
        if (p->text().contains(' ')) dirName = "\""+p->text()+"\"";
        else dirName = p->text();
        if (len && p->text()[len-1] == '/') {
            path = dirName+path;
        }
        else path = dirName+'/'+path;
        p = p->parent();
    }
    return path;
}

QString FileNode::getFilePath() {
    if (getType() == FILE) {
        QString path = getPath();
        if (path.endsWith('/')) return path+text();
        else return path+"/"+text();
    }
    else return getPath();
}

QString FileNode::getParentPath() {
    QStandardItem* _this = this;
    FileNode* parent = dynamic_cast<FileNode*>(_this->parent());
    if (parent) return parent->getPath();
    else return getPath();
}

void FileNode::clearChildren() {
    removeRows(0, rowCount());
}

QVector<QStringList> FileNode::parseFileListStr(const QString& fileListStr) {
    QStringList infoList = fileListStr.split("\r\n", QString::SkipEmptyParts);
    QRegExp regex("[ ]+");
    QVector<QStringList> fileList;
    QString filename, size, modifyDate, authority, ownerNGroup;
    int len = infoList.length();
    for (int i = 0;i < len;i++) {
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
    return fileList;
}

void FileNode::appendFakeNode(FileNode* node) {
    if (node->rowCount()) return;
    FileNode* fakeNode = new FileNode("(empty)");
    node->appendRow(fakeNode);
    node->setChild(fakeNode->index().row(), 1, new FileNode(""));
    node->setChild(fakeNode->index().row(), 2, new FileNode(""));
    node->setChild(fakeNode->index().row(), 3, new FileNode(""));
    node->setChild(fakeNode->index().row(), 4, new FileNode(""));
}

FileNode* FileNode::findNodeByPath(FileNode* root, const QString& path) {
    int index = path.indexOf(root->text());
    if (index) return nullptr;

    QStringList dirList = path.mid(root->text().length()).split('/', QString::SkipEmptyParts);
    int len = dirList.length();
    QStandardItem* p = root;
    for (int i = 0;i < len;i++) {
        QString dirName;
        if (dirList[i][0] == "\""
            && dirList[i][dirList[i].length()-1] == "\""
            && dirList[i].contains(' ')) {
            dirName = dirList[i].mid(1, dirList[i].length()-2);
        }
        else dirName = dirList[i];
        int rowCount = p->rowCount();
        bool found = false;
        for (int j = 0;j < rowCount;j++) {
            if (p->child(j)->text() == dirName) {
                found = true;
                p = p->child(j);
                break;
            }
        }
        if (!found) return nullptr;
    }
    FileNode* node = dynamic_cast<FileNode*>(p);
    if (node) {
        return node;
    }
    else return nullptr;
}

QIcon FileNode::autoGetIcon(Type type, const QString &filename) {
    if (type == DIR) {
        return iconProvider.icon(QFileIconProvider::Folder);
    }
    else {
        QIcon icon;
        QList<QMimeType> mime_types = mime_database.mimeTypesForFileName(filename);
        // qDebug() << mime_types.count();
        for (int i=0; i < mime_types.count() && icon.isNull(); i++) {
            icon = QIcon::fromTheme(mime_types[i].iconName());
            // qDebug() << mime_types[i].iconName();
            // qDebug() << icon;
        }

        if (icon.isNull()) {
            return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        }
        else return icon;
    }
}


FileModel::FileModel(const QString &_id, QObject* parent, const QString& rootPath):
    QStandardItemModel(parent)
    , id(_id)
    , root(nullptr) {
    QStringList header;
    header << "Path" << "Size" << "Last Modify" << "Authority" << "Owner/Group";
    setHorizontalHeaderLabels(header);
    initRoot(rootPath);

    connect(this, &QAbstractItemModel::dataChanged, this, &FileModel::checkTextChanged);
}

void FileModel::initRoot(const QString &rootPath) {
    if (!root) {
        root = new FileNode(rootPath);
        appendRow(root);
    }
    else {
        root->clearChildren();
        root->setText(rootPath);
    }
    FileNode::appendFakeNode(root);
}

FileNode* FileModel::getRoot() {
    return root;
}

Qt::ItemFlags FileModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    QStandardItem* item = itemFromIndex(index);
    FileNode* node = dynamic_cast<FileNode*>(item);
    if (index.column() != 0 || !node || node->getType() == FileNode::EMPTY) {
        return flags;
    }
    else if (!item->parent()) {
        flags = flags | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
    }
    else {
        flags = flags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
        return flags;
    }
}

QMimeData* FileModel::mimeData(const QModelIndexList &indexes) const {
    if (indexes.count() <= 0) return 0;

    QMimeData *data = new QMimeData;
    FileNode* node = dynamic_cast<FileNode*>(itemFromIndex(indexes.at(0)));
    if (!node) return 0;

    if (node->getType() == FileNode::FILE) {
        // 仅允许拖拽文件
        data->setData("id", this->id.toLocal8Bit());
        data->setData("path", node->getPath().toLocal8Bit());
        data->setData("filename", node->text().toLocal8Bit());
    }
    else return 0;

    return data;
}

bool FileModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    if (QString::fromLocal8Bit(data->data("id")) == this->id) return false;

    QModelIndex index = parent.siblingAtColumn(0);
    FileNode* node = dynamic_cast<FileNode*>(itemFromIndex(index));
    if (!node) return false;

    QString path = QString::fromLocal8Bit(data->data("path"));
    QString filename = QString::fromLocal8Bit(data->data("filename"));
    qDebug() << "path:" << path << "filename:" << filename;

    qDebug() << node->getPath();

    emit transfer(path, filename, node->getPath());
    return true;
}

QStringList FileModel::mimeTypes() const {
    QStringList types;
    types << "path" << "filename";
    return types;
}

void FileModel::checkTextChanged(const QModelIndex& index) {
    if (index.data(FileNode::CompareRole).isNull()) {
        // 初始化过程中设置CompareRole
        setData(index, index.data(Qt::EditRole), FileNode::CompareRole);
        return;
    }
    if (index.data(Qt::EditRole) == index.data(FileNode::CompareRole)) return;
    else {
        QStandardItem* item = itemFromIndex(index);
        FileNode* node = dynamic_cast<FileNode*>(item);
        if (!node) return;
        if (!item->parent()) {
            QString oldRoot = index.data(FileNode::CompareRole).toString();
            emit rootChanged(oldRoot);
        }
        else {
            QString path = node->getParentPath();
            if (path.endsWith('/')) path = path+index.data(FileNode::CompareRole).toString();
            else path = path+"/"+index.data(FileNode::CompareRole).toString();
            emit textChanged(index, path);
        }
        setData(index, index.data(Qt::EditRole), FileNode::CompareRole);
    }
}


