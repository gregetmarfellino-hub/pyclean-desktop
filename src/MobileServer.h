#pragma once

#include <QObject>
#include <QString>

class QHttpServer;
class QTcpServer;
class FileOrganizer;
class Database;

class MobileServer : public QObject
{
public:
    explicit MobileServer(
        FileOrganizer *organizer,
        Database *database,
        QObject *parent = nullptr
    );

    bool start(quint16 port = 5000);
    void stop();

    [[nodiscard]]
    bool isRunning() const;

    [[nodiscard]]
    QString localIp() const;

    [[nodiscard]]
    quint16 port() const;

    [[nodiscard]]
    QString errorMessage() const;

    void setSharedRoot(const QString &path);
    void clearSharedRoot();

    [[nodiscard]]
    QString sharedRoot() const;

    [[nodiscard]]
    bool hasSharedRoot() const;

    void setReadOnly(bool enabled);

    [[nodiscard]]
    bool isReadOnly() const;

private:
    QHttpServer *httpServer = nullptr;
    QTcpServer *tcpServer = nullptr;

    Database *database = nullptr;

    quint16 serverPort = 5000;
    QString serverIp;
    QString lastError;

    QString sharedRootPath;
    bool readOnlyMode = false;

    void setupRoutes();
    QString detectLocalIp() const;

    [[nodiscard]]
    bool isPathAllowed(const QString &path) const;

    [[nodiscard]]
    bool isSharedRootPath(const QString &path) const;
};
