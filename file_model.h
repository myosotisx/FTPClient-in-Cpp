#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QStandardItemModel>
#include <QAbstractTableModel>
#include <QMimeDatabase>
#include <QFileIconProvider>
#include <QStyledItemDelegate>

static QMimeDatabase mime_database;
static QFileIconProvider iconProvider;

class FileNode: public QObject, public QStandardItem {
    Q_OBJECT

public:
    enum { CompareRole = Qt::UserRole+1024 };
    enum Type { EMPTY = 0, FILE = 1, DIR  = 2};
    explicit FileNode(const QString& filename = "(empty)");
    explicit FileNode(const QIcon& icon, const QString& filename = "(empty)");
    void appendChildren(const QVector<QStringList>& fileList);
    FileNode* appendChildDir();
    void clearChildren();
    Type getType();
    long long getSize();
    QString getPath();
    QString getFilePath();
    QString getParentPath();
    static QVector<QStringList> parseFileListStr(const QString& fileListStr);
    static void appendFakeNode(FileNode* node);
    static FileNode* findNodeByPath(FileNode* root, const QString& path);
    static QIcon autoGetIcon(Type type, const QString& filename = "(empty)");

private:

};

class FileModel: public QStandardItemModel {
    Q_OBJECT

public:
    explicit FileModel(const QString& _id, QObject* parent = nullptr, const QString& rootPath = "/");
    void addNode(QStandardItem* parent, QStandardItem* child);
    FileNode* getRoot();
    void initRoot(const QString& rootPath);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    QStringList mimeTypes() const;

public slots:
    void checkTextChanged(const QModelIndex& index);

signals:
    void transfer(const QString& srcPath, const QString& srcFile,
                  const QString& dstPath);
    void textChanged(const QModelIndex& index, const QString& oldPath);
    void rootChanged(const QString& oldRoot);

private:

    QString id;
    FileNode* root;
};

class FileListModel: public QStandardItemModel {
    Q_OBJECT
public:
    explicit FileListModel(QObject* parent = nullptr);
    void appendFileItem(const QString& filename, const QString& type, double progress, long long size);
    void updateProgress(int row, double percent);

};

class ProgressDelegate: public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ProgressDelegate(QObject* parent = nullptr);
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

};

#endif // FILEMODEL_H
