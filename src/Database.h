#pragma once

#include <QString>
#include <QList>

struct FileLog
{
    int id;
    QString batchId;
    QString filename;
    QString category;
    QString sourcePath;
    QString targetPath;
    QString timestamp;
};

class Database
{
public:
    explicit Database(const QString &databasePath);
    ~Database();

    bool initialize();

    bool recordMove(
        const QString &filename,
        const QString &category,
        const QString &sourcePath,
        const QString &targetPath,
        const QString &batchId
    );

    QList<FileLog> recentLogs(int limit = 100) const;

    QList<FileLog> logsForBatch(
        const QString &batchId
    ) const;

    QString latestBatchId() const;

    bool removeLog(int id);

    QString errorMessage() const;

private:
    QString databasePath;
    QString connectionName;
    QString lastError;

    bool initialized = false;
};