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

private:
    QHttpServer *httpServer = nullptr;
    QTcpServer *tcpServer = nullptr;

    Database *database = nullptr;

    quint16 serverPort = 5000;
    QString serverIp;
    QString lastError;

    void setupRoutes();
    QString detectLocalIp() const;
};
