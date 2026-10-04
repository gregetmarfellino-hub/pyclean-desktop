#include "Database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>

Database::Database(const QString &databasePath)
    : databasePath(databasePath)
{
    connectionName =
        "PyCleanDB_" +
        QUuid::createUuid().toString(
            QUuid::WithoutBraces
        );
}

Database::~Database()
{
    if (!QSqlDatabase::contains(connectionName))
    {
        return;
    }

    {
        QSqlDatabase db =
            QSqlDatabase::database(
                connectionName,
                false
            );

        if (db.isValid())
        {
            db.close();
        }
    }

    QSqlDatabase::removeDatabase(connectionName);
}

bool Database::initialize()
{
    lastError.clear();

    if (initialized)
    {
        return true;
    }

    QSqlDatabase db =
        QSqlDatabase::addDatabase(
            "QSQLITE",
            connectionName
        );

    db.setDatabaseName(databasePath);

    if (!db.open())
    {
        lastError =
            "Failed to open database:\n" +
            db.lastError().text();

        return false;
    }

    QSqlQuery query(db);

    const QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS file_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT,
            category TEXT,
            source_path TEXT,
            target_path TEXT,
            batch_id TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createTableSql))
    {
        lastError =
            "Failed to create file_logs table:\n" +
            query.lastError().text();

        return false;
    }

    /*
     * If an older test database exists, add the new
     * batch_id column without destroying the database.
     */
    bool hasBatchId = false;

    QSqlQuery pragmaQuery(db);

    if (pragmaQuery.exec(
            "PRAGMA table_info(file_logs)"
        ))
    {
        while (pragmaQuery.next())
        {
            QString columnName =
                pragmaQuery.value(1).toString();

            if (columnName == "batch_id")
            {
                hasBatchId = true;
                break;
            }
        }
    }

    if (!hasBatchId)
    {
        QSqlQuery alterQuery(db);

        if (!alterQuery.exec(
                "ALTER TABLE file_logs "
                "ADD COLUMN batch_id TEXT"
            ))
        {
            lastError =
                "Failed to add batch_id column:\n" +
                alterQuery.lastError().text();

            return false;
        }
    }

    initialized = true;

    return true;
}

bool Database::recordMove(
    const QString &filename,
    const QString &category,
    const QString &sourcePath,
    const QString &targetPath,
    const QString &batchId
)
{
    lastError.clear();

    if (!initialized)
    {
        lastError =
            "Database has not been initialized.";

        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::database(
            connectionName
        );

    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO file_logs
        (
            filename,
            category,
            source_path,
            target_path,
            batch_id
        )
        VALUES
        (
            :filename,
            :category,
            :source_path,
            :target_path,
            :batch_id
        )
    )");

    query.bindValue(
        ":filename",
        filename
    );

    query.bindValue(
        ":category",
        category
    );

    query.bindValue(
        ":source_path",
        sourcePath
    );

    query.bindValue(
        ":target_path",
        targetPath
    );

    query.bindValue(
        ":batch_id",
        batchId
    );

    if (!query.exec())
    {
        lastError =
            "Failed to record file move:\n" +
            query.lastError().text();

        return false;
    }

    return true;
}

QList<FileLog> Database::recentLogs(int limit) const
{
    QList<FileLog> logs;

    if (!initialized)
    {
        return logs;
    }

    QSqlDatabase db =
        QSqlDatabase::database(
            connectionName
        );

    QSqlQuery query(db);

    query.prepare(R"(
        SELECT
            id,
            batch_id,
            filename,
            category,
            source_path,
            target_path,
            timestamp
        FROM file_logs
        ORDER BY id DESC
        LIMIT :limit
    )");

    query.bindValue(":limit", limit);

    if (!query.exec())
    {
        return logs;
    }

    while (query.next())
    {
        FileLog log;

        log.id =
            query.value(0).toInt();

        log.batchId =
            query.value(1).toString();

        log.filename =
            query.value(2).toString();

        log.category =
            query.value(3).toString();

        log.sourcePath =
            query.value(4).toString();

        log.targetPath =
            query.value(5).toString();

        log.timestamp =
            query.value(6).toString();

        logs.append(log);
    }

    return logs;
}

QList<FileLog> Database::logsForBatch(
    const QString &batchId
) const
{
    QList<FileLog> logs;

    if (!initialized || batchId.isEmpty())
    {
        return logs;
    }

    QSqlDatabase db =
        QSqlDatabase::database(
            connectionName
        );

    QSqlQuery query(db);

    query.prepare(R"(
        SELECT
            id,
            batch_id,
            filename,
            category,
            source_path,
            target_path,
            timestamp
        FROM file_logs
        WHERE batch_id = :batch_id
        ORDER BY id DESC
    )");

    query.bindValue(
        ":batch_id",
        batchId
    );

    if (!query.exec())
    {
        return logs;
    }

    while (query.next())
    {
        FileLog log;

        log.id =
            query.value(0).toInt();

        log.batchId =
            query.value(1).toString();

        log.filename =
            query.value(2).toString();

        log.category =
            query.value(3).toString();

        log.sourcePath =
            query.value(4).toString();

        log.targetPath =
            query.value(5).toString();

        log.timestamp =
            query.value(6).toString();

        logs.append(log);
    }

    return logs;
}

QString Database::latestBatchId() const
{
    if (!initialized)
    {
        return QString();
    }

    QSqlDatabase db =
        QSqlDatabase::database(
            connectionName
        );

    QSqlQuery query(db);

    if (!query.exec(R"(
        SELECT batch_id
        FROM file_logs
        WHERE batch_id IS NOT NULL
          AND batch_id != ''
        ORDER BY id DESC
        LIMIT 1
    )"))
    {
        return QString();
    }

    if (!query.next())
    {
        return QString();
    }

    return query.value(0).toString();
}

bool Database::removeLog(int id)
{
    lastError.clear();

    if (!initialized)
    {
        lastError =
            "Database has not been initialized.";

        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::database(
            connectionName
        );

    QSqlQuery query(db);

    query.prepare(
        "DELETE FROM file_logs WHERE id = :id"
    );

    query.bindValue(":id", id);

    if (!query.exec())
    {
        lastError =
            "Failed to remove database log:\n" +
            query.lastError().text();

        return false;
    }

    return true;
}

QString Database::errorMessage() const
{
    return lastError;
}