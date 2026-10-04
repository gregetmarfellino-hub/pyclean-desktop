#include "MobileServer.h"

#include "CategoryRules.h"
#include "Database.h"
#include "FileOrganizer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QHostAddress>
#include <QHostInfo>
#include <QHttpHeaders>
#include <QHttpServer>
#include <QHttpServerConfiguration>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QSet>
#include <QTcpServer>
#include <QUuid>

namespace
{

using StatusCode =
    QHttpServerResponder::StatusCode;


QHttpServerResponse jsonResponse(
    const QJsonObject &object,
    StatusCode status = StatusCode::Ok
)
{
    return QHttpServerResponse(
        object,
        status
    );
}


QHttpServerResponse apiError(
    const QString &message,
    StatusCode status
)
{
    QJsonObject response;

    response["status"] =
        QStringLiteral("error");

    response["success"] =
        false;

    response["message"] =
        message;

    /*
     * Keep the C++ field too so newer clients can use either shape.
     */
    response["error"] =
        message;

    return jsonResponse(
        response,
        status
    );
}


QJsonObject parseJsonObject(
    const QHttpServerRequest &request
)
{
    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            request.body(),
            &error
        );

    if (
        error.error != QJsonParseError::NoError
        || !document.isObject()
    )
    {
        return {};
    }

    return document.object();
}


QString jsonPath(
    const QJsonObject &object
)
{
    QString path =
        object.value(
            QStringLiteral("path")
        ).toString();

    if (path.isEmpty())
    {
        path =
            object.value(
                QStringLiteral("target_dir")
            ).toString();
    }

    if (path.isEmpty())
    {
        path =
            object.value(
                QStringLiteral("targetDirectory")
            ).toString();
    }

    return path.trimmed();
}


bool isProtectedPath(
    const QString &path
)
{
    if (path.trimmed().isEmpty())
    {
        return true;
    }

    const QString clean =
        QDir::cleanPath(
            QDir::fromNativeSeparators(path)
        ).toLower();

    /*
     * Protect any filesystem root, not only C:\.
     */
    if (QDir(clean).isRoot())
    {
        return true;
    }

    static const QStringList protectedPaths = {
        QStringLiteral("c:/windows"),
        QStringLiteral("c:/program files"),
        QStringLiteral("c:/program files (x86)")
    };

    for (const QString &protectedPath : protectedPaths)
    {
        if (
            clean == protectedPath
            || clean.startsWith(
                protectedPath + QLatin1Char('/')
            )
        )
        {
            return true;
        }
    }

    /*
     * Protect the Users directory itself while still allowing normal
     * user-owned folders inside it.
     */
    if (clean == QStringLiteral("c:/users"))
    {
        return true;
    }

    return false;
}


QString multipartField(
    const QByteArray &headers
)
{
    static const QRegularExpression regex(
        QStringLiteral(
            R"REGEX(name="([^"]+)")REGEX"
        )
    );

    const QRegularExpressionMatch match =
        regex.match(
            QString::fromUtf8(headers)
        );

    if (!match.hasMatch())
    {
        return {};
    }

    return match.captured(1);
}


QString multipartFilename(
    const QByteArray &headers
)
{
    static const QRegularExpression regex(
        QStringLiteral(
            R"REGEX(filename="([^"]*)")REGEX"
        )
    );

    const QRegularExpressionMatch match =
        regex.match(
            QString::fromUtf8(headers)
        );

    if (!match.hasMatch())
    {
        return {};
    }

    return match.captured(1);
}


QByteArray multipartBoundary(
    const QByteArray &contentType
)
{
    const QByteArray lower =
        contentType.toLower();

    constexpr auto prefix =
        "boundary=";

    const qsizetype position =
        lower.indexOf(prefix);

    if (position < 0)
    {
        return {};
    }

    QByteArray boundary =
        contentType.mid(
            position
            + static_cast<qsizetype>(
                sizeof("boundary=") - 1
            )
        ).trimmed();

    /*
     * Ignore any parameter after the boundary when the boundary is not
     * quoted. Typical Qt/Flutter clients do not add one, but accepting it
     * costs almost nothing.
     */
    if (!boundary.startsWith('"'))
    {
        const qsizetype semicolon =
            boundary.indexOf(';');

        if (semicolon >= 0)
        {
            boundary =
                boundary.left(
                    semicolon
                ).trimmed();
        }
    }

    if (
        boundary.size() >= 2
        && boundary.startsWith('"')
        && boundary.endsWith('"')
    )
    {
        boundary =
            boundary.mid(
                1,
                boundary.size() - 2
            );
    }

    return boundary;
}


struct MultipartUpload
{
    QString targetDirectory;
    QString filename;

    qsizetype fileOffset = -1;
    qsizetype fileLength = 0;
};


bool parseMultipartUpload(
    const QHttpServerRequest &request,
    MultipartUpload &upload,
    QByteArray &body,
    QString &errorMessage
)
{
    const QByteArray contentType =
        request.value(
            "Content-Type"
        );

    if (
        !contentType.toLower().startsWith(
            "multipart/form-data"
        )
    )
    {
        errorMessage =
            QStringLiteral(
                "Expected multipart/form-data."
            );

        return false;
    }

    const QByteArray rawBoundary =
        multipartBoundary(
            contentType
        );

    if (rawBoundary.isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Missing multipart boundary."
            );

        return false;
    }

    const QByteArray boundary =
        QByteArrayLiteral("--")
        + rawBoundary;

    body =
        request.body();

    qsizetype searchPosition = 0;

    while (true)
    {
        const qsizetype boundaryIndex =
            body.indexOf(
                boundary,
                searchPosition
            );

        if (boundaryIndex < 0)
        {
            break;
        }

        qsizetype headersStart =
            boundaryIndex
            + boundary.size();

        if (
            body.mid(
                headersStart,
                2
            ) == "--"
        )
        {
            break;
        }

        if (
            body.mid(
                headersStart,
                2
            ) == "\r\n"
        )
        {
            headersStart += 2;
        }
        else if (
            body.mid(
                headersStart,
                1
            ) == "\n"
        )
        {
            ++headersStart;
        }

        qsizetype headerSeparatorSize = 4;

        qsizetype contentStart =
            body.indexOf(
                "\r\n\r\n",
                headersStart
            );

        if (contentStart < 0)
        {
            headerSeparatorSize = 2;

            contentStart =
                body.indexOf(
                    "\n\n",
                    headersStart
                );
        }

        if (contentStart < 0)
        {
            break;
        }

        const QByteArray headers =
            body.mid(
                headersStart,
                contentStart - headersStart
            );

        contentStart +=
            headerSeparatorSize;

        const qsizetype nextBoundary =
            body.indexOf(
                boundary,
                contentStart
            );

        if (nextBoundary < 0)
        {
            break;
        }

        qsizetype contentLength =
            nextBoundary
            - contentStart;

        /*
         * Remove only the protocol CRLF before the next boundary.
         * Do not strip arbitrary trailing newlines from uploaded files.
         */
        if (
            contentLength >= 2
            && body.mid(
                contentStart + contentLength - 2,
                2
            ) == "\r\n"
        )
        {
            contentLength -= 2;
        }
        else if (
            contentLength >= 1
            && body.at(
                contentStart + contentLength - 1
            ) == '\n'
        )
        {
            --contentLength;
        }

        const QString fieldName =
            multipartField(
                headers
            );

        const QString filename =
            multipartFilename(
                headers
            );

        if (!filename.isEmpty())
        {
            upload.filename =
                QFileInfo(filename)
                    .fileName();

            upload.fileOffset =
                contentStart;

            upload.fileLength =
                contentLength;
        }
        else if (
            fieldName == QStringLiteral("path")
            || fieldName == QStringLiteral("target_dir")
            || fieldName == QStringLiteral("targetDirectory")
        )
        {
            upload.targetDirectory =
                QString::fromUtf8(
                    body.constData()
                        + contentStart,
                    contentLength
                ).trimmed();
        }

        /*
         * Re-process the boundary we just found on the next iteration.
         *
         * Advancing past it would skip the following multipart section.
         * Flutter sends target_dir first and the file second, so the old
         * logic parsed target_dir and jumped straight to the closing
         * boundary without ever seeing the file.
         */
        searchPosition =
            nextBoundary;
    }

    if (
        upload.filename.isEmpty()
        || upload.fileOffset < 0
    )
    {
        errorMessage =
            QStringLiteral(
                "No uploaded file found."
            );

        return false;
    }

    return true;
}



QString uniqueDestinationPath(
    const QString &desiredPath
)
{
    /*
     * Fast path: no collision.
     */
    if (!QFileInfo::exists(desiredPath))
    {
        return desiredPath;
    }

    const QFileInfo desiredInfo(
        desiredPath
    );

    const QString directory =
        desiredInfo.absolutePath();

    const QString fileName =
        desiredInfo.fileName();

    const QString suffix =
        desiredInfo.suffix();

    QString baseName;

    if (suffix.isEmpty())
    {
        baseName =
            fileName;
    }
    else
    {
        /*
         * Preserve everything before the final extension.
         *
         * "archive.tar.gz" -> "archive.tar"
         * "report.pdf"     -> "report"
         */
        baseName =
            fileName.left(
                fileName.size()
                - suffix.size()
                - 1
            );
    }

    for (int counter = 1; ; ++counter)
    {
        QString candidateName;

        if (suffix.isEmpty())
        {
            candidateName =
                QStringLiteral(
                    "%1 (%2)"
                )
                .arg(baseName)
                .arg(counter);
        }
        else
        {
            candidateName =
                QStringLiteral(
                    "%1 (%2).%3"
                )
                .arg(baseName)
                .arg(counter)
                .arg(suffix);
        }

        const QString candidatePath =
            QDir(directory)
                .filePath(
                    candidateName
                );

        if (!QFileInfo::exists(candidatePath))
        {
            return candidatePath;
        }
    }
}


bool saveUpload(
    const MultipartUpload &upload,
    const QByteArray &body,
    const QString &targetDirectory,
    QString &savedPath,
    QString &errorMessage
)
{
    const QFileInfo directoryInfo(
        targetDirectory
    );

    /*
     * Match the Python server: uploads target an existing directory rather
     * than silently creating a misspelled path.
     */
    if (
        !directoryInfo.exists()
        || !directoryInfo.isDir()
    )
    {
        errorMessage =
            QStringLiteral(
                "Target directory does not exist."
            );

        return false;
    }

    if (upload.filename.isEmpty())
    {
        errorMessage =
            QStringLiteral(
                "Invalid uploaded filename."
            );

        return false;
    }

    const QString desiredPath =
        QDir(targetDirectory)
            .filePath(
                upload.filename
            );

    savedPath =
        uniqueDestinationPath(
            desiredPath
        );

    QFile output(
        savedPath
    );

    if (
        !output.open(
            QIODevice::WriteOnly
            | QIODevice::NewOnly
        )
    )
    {
        errorMessage =
            QStringLiteral(
                "Unable to save uploaded file."
            );

        return false;
    }

    const qint64 written =
        output.write(
            body.constData()
                + upload.fileOffset,
            upload.fileLength
        );

    output.close();

    if (written != upload.fileLength)
    {
        QFile::remove(
            savedPath
        );

        errorMessage =
            QStringLiteral(
                "Failed while writing uploaded file."
            );

        return false;
    }

    return true;
}


bool moveFileFast(
    const QString &sourcePath,
    const QString &targetPath
)
{
    /*
     * Final safety guard. Callers already resolve collisions before reaching
     * this function, but never overwrite if the filesystem changed meanwhile.
     */
    if (QFileInfo::exists(targetPath))
    {
        return false;
    }

    /*
     * Same-volume move: filesystem rename is effectively O(1).
     */
    if (
        QFile::rename(
            sourcePath,
            targetPath
        )
    )
    {
        return true;
    }

    /*
     * Cross-volume fallback.
     */
    if (
        QFile::copy(
            sourcePath,
            targetPath
        )
    )
    {
        if (
            QFile::remove(
                sourcePath
            )
        )
        {
            return true;
        }

        QFile::remove(
            targetPath
        );
    }

    return false;
}


struct SortResult
{
    int moved = 0;
    int failed = 0;
};


SortResult executeRemoteSort(
    const QString &targetDirectory,
    Database *database
)
{
    SortResult result;

    QDir directory(
        targetDirectory
    );

    const QFileInfoList files =
        directory.entryInfoList(
            QDir::Files
            | QDir::NoDotAndDotDot,
            QDir::Name
        );

    static const QSet<QString> protectedNames = {
        QStringLiteral("pyclean_mobile"),
        QStringLiteral("pyclean_cpp.exe"),
        QStringLiteral("pyclean_logs.db"),
        QStringLiteral("pyclean_config.json"),
        QStringLiteral("pyclean_config.ini")
    };

    /*
     * A single batch id means desktop rollback can also reverse the most
     * recent remote sort.
     */
    const QString batchId =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );

    const bool canLog =
        database != nullptr
        && database->initialize();

    for (const QFileInfo &fileInfo : files)
    {
        if (
            protectedNames.contains(
                fileInfo.fileName()
                    .toLower()
            )
        )
        {
            continue;
        }

        /*
         * The original Flask remote endpoint was extension-only and sent
         * every unknown extension to Others. Keep that behaviour separate
         * from the desktop AI classifier.
         */
        const QString category =
            CategoryRules::categoryForExtension(
                fileInfo.suffix()
            );

        const QString categoryDirectory =
            directory.filePath(
                category
            );

        if (
            !QDir().exists(
                categoryDirectory
            )
            && !QDir().mkpath(
                categoryDirectory
            )
        )
        {
            ++result.failed;
            continue;
        }

        const QString desiredTargetPath =
            QDir(categoryDirectory)
                .filePath(
                    fileInfo.fileName()
                );

        const QString actualTargetPath =
            uniqueDestinationPath(
                desiredTargetPath
            );

        if (
            !moveFileFast(
                fileInfo.absoluteFilePath(),
                actualTargetPath
            )
        )
        {
            ++result.failed;
            continue;
        }

        ++result.moved;

        if (canLog)
        {
            /*
             * Store the exact collision-safe path so desktop rollback can
             * restore this remote operation correctly.
             */
            database->recordMove(
                fileInfo.fileName(),
                category,
                fileInfo.absoluteFilePath(),
                actualTargetPath,
                batchId
            );
        }
    }

    return result;
}


QByteArray dashboardHtml(
    const QString &ip,
    quint16 port
)
{
    const QString html =
        QStringLiteral(
R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>PyClean Mobile Agent</title>
<style>
body {
    background:#1e1e2e;
    color:#cdd6f4;
    font-family:"Segoe UI",sans-serif;
    margin:0;
    padding:20px;
    text-align:center;
}
.card {
    background:#2a2a3c;
    border:1px solid #45475a;
    border-radius:12px;
    max-width:380px;
    margin:30px auto;
    padding:20px;
}
h1 {
    color:#89b4fa;
    font-size:20px;
    margin-bottom:5px;
}
p {
    color:#a6adc8;
    font-size:13px;
}
.badge {
    display:inline-block;
    background:#a6e3a1;
    color:#11111b;
    font-weight:bold;
    padding:4px 12px;
    border-radius:20px;
    font-size:11px;
    margin-top:10px;
}
.info {
    background:#181825;
    color:#a6e3a1;
    border-radius:8px;
    padding:12px;
    text-align:left;
    font-family:monospace;
    font-size:12px;
    margin-top:15px;
}
</style>
</head>
<body>
<div class="card">
<h1>PyClean Mobile Agent</h1>
<p>IoT Server Active &amp; Listening to PC Engine</p>
<div class="badge">ONLINE</div>
<div class="info">
Server IP : %1<br>
Port      : %2<br>
Status    : Ready for Flutter Remote
</div>
</div>
</body>
</html>)HTML"
        )
        .arg(
            ip.toHtmlEscaped()
        )
        .arg(
            port
        );

    return html.toUtf8();
}

} // namespace


MobileServer::MobileServer(
    FileOrganizer *organizer,
    Database *database,
    QObject *parent
)
    : QObject(parent),
      database(database)
{
    /*
     * Kept in the constructor signature for compatibility with the rest of
     * the project, but mobile jobs now create their own local sort state.
     */
    Q_UNUSED(organizer);

    serverIp =
        detectLocalIp();
}


bool MobileServer::start(
    quint16 port
)
{
    lastError.clear();

    if (isRunning())
    {
        return true;
    }

    /*
     * Recreate the HTTP/TCP pair on each start. This makes repeated
     * Start -> Stop -> Start cycles deterministic and avoids stale bindings.
     */
    stop();

    httpServer =
        new QHttpServer(this);

    tcpServer =
        new QTcpServer(this);

    serverPort =
        port;

    QHttpServerConfiguration configuration =
        httpServer->configuration();

    /*
     * Qt's HTTP server buffers request bodies, so cap mobile uploads instead
     * of allowing a phone to exhaust the desktop process.
     */
    configuration.setMaximumBodySize(
        64 * 1024 * 1024
    );

    httpServer->setConfiguration(
        configuration
    );

    setupRoutes();

    if (
        !tcpServer->listen(
            QHostAddress::Any,
            serverPort
        )
    )
    {
        lastError =
            tcpServer->errorString();

        stop();
        return false;
    }

    if (
        !httpServer->bind(
            tcpServer
        )
    )
    {
        lastError =
            QStringLiteral(
                "Qt HTTP server could not bind to the TCP listener."
            );

        stop();
        return false;
    }

    serverPort =
        tcpServer->serverPort();

    serverIp =
        detectLocalIp();

    return true;
}


void MobileServer::stop()
{
    if (tcpServer)
    {
        tcpServer->close();
        delete tcpServer;
        tcpServer = nullptr;
    }

    if (httpServer)
    {
        delete httpServer;
        httpServer = nullptr;
    }
}


bool MobileServer::isRunning() const
{
    return (
        tcpServer != nullptr
        && tcpServer->isListening()
    );
}


QString MobileServer::localIp() const
{
    return serverIp;
}


quint16 MobileServer::port() const
{
    return serverPort;
}


QString MobileServer::errorMessage() const
{
    return lastError;
}


QString MobileServer::detectLocalIp() const
{
    const QList<QNetworkInterface> interfaces =
        QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces)
    {
        const auto flags =
            interface.flags();

        if (
            !flags.testFlag(
                QNetworkInterface::IsUp
            )
            || !flags.testFlag(
                QNetworkInterface::IsRunning
            )
            || flags.testFlag(
                QNetworkInterface::IsLoopBack
            )
        )
        {
            continue;
        }

        const QList<QNetworkAddressEntry> entries =
            interface.addressEntries();

        for (const QNetworkAddressEntry &entry : entries)
        {
            const QHostAddress address =
                entry.ip();

            if (
                address.protocol()
                    != QAbstractSocket::IPv4Protocol
                || address.isLoopback()
            )
            {
                continue;
            }

            const QString candidate =
                address.toString();

            /*
             * Skip APIPA addresses when a normal LAN address is available.
             */
            if (
                candidate.startsWith(
                    QStringLiteral("169.254.")
                )
            )
            {
                continue;
            }

            return candidate;
        }
    }

    return QStringLiteral(
        "127.0.0.1"
    );
}


void MobileServer::setupRoutes()
{
    if (!httpServer)
    {
        return;
    }

    /*
     * CORS + Flask-equivalent no-cache behaviour.
     */
    httpServer->addAfterRequestHandler(
        httpServer,
        [](
            const QHttpServerRequest &request,
            QHttpServerResponse &response
        )
        {
            Q_UNUSED(request);

            QHttpHeaders headers =
                response.headers();

            headers.append(
                "Access-Control-Allow-Origin",
                "*"
            );

            headers.append(
                "Access-Control-Allow-Methods",
                "GET, POST, OPTIONS"
            );

            headers.append(
                "Access-Control-Allow-Headers",
                "Content-Type"
            );

            headers.append(
                "Cache-Control",
                "no-store, no-cache, must-revalidate, max-age=0"
            );

            response.setHeaders(
                std::move(headers)
            );
        }
    );


    /*
     * CORS preflight for every current /api/<endpoint>.
     */
    httpServer->route(
        "/api/<arg>",
        QHttpServerRequest::Method::Options,
        [](const QString &)
        {
            return QHttpServerResponse(
                StatusCode::Ok
            );
        }
    );


    /*
     * GET /
     * Restore the small browser dashboard from the Flask agent.
     */
    httpServer->route(
        "/",
        QHttpServerRequest::Method::Get,
        [this]()
        {
            return QHttpServerResponse(
                QByteArrayLiteral(
                    "text/html; charset=utf-8"
                ),
                dashboardHtml(
                    localIp(),
                    port()
                ),
                StatusCode::Ok
            );
        }
    );


    /*
     * GET /api/status
     *
     * Python-compatible keys are preserved; C++-specific fields are additive.
     */
    httpServer->route(
        "/api/status",
        QHttpServerRequest::Method::Get,
        [this]()
        {
            QJsonObject response;

            response["status"] =
                QStringLiteral("online");

            response["device_name"] =
                QHostInfo::localHostName();

            response["ip"] =
                localIp();

            response["port"] =
                static_cast<int>(
                    port()
                );

            response["message"] =
                QStringLiteral(
                    "PyClean Engine PC Siap!"
                );

            response["server"] =
                QStringLiteral(
                    "PyClean C++"
                );

            response["running"] =
                isRunning();

            return jsonResponse(
                response
            );
        }
    );


    /*
     * GET /api/logs
     */
    httpServer->route(
        "/api/logs",
        QHttpServerRequest::Method::Get,
        [this]()
        {
            if (
                database
                && !database->initialize()
            )
            {
                return apiError(
                    database->errorMessage(),
                    StatusCode::InternalServerError
                );
            }

            QJsonArray logs;

            if (database)
            {
                const QList<FileLog> recent =
                    database->recentLogs(
                        20
                    );

                for (const FileLog &log : recent)
                {
                    QJsonObject item;

                    item["id"] =
                        log.id;

                    item["filename"] =
                        log.filename;

                    item["category"] =
                        log.category;

                    item["source_path"] =
                        log.sourcePath;

                    item["target_path"] =
                        log.targetPath;

                    item["timestamp"] =
                        log.timestamp;

                    logs.append(
                        item
                    );
                }
            }

            QJsonObject response;

            response["status"] =
                QStringLiteral("success");

            response["logs"] =
                logs;

            return jsonResponse(
                response
            );
        }
    );


    /*
     * POST /api/browse
     */
    httpServer->route(
        "/api/browse",
        QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &request)
        {
            const QJsonObject body =
                parseJsonObject(
                    request
                );

            QString path =
                jsonPath(
                    body
                );

            if (path.isEmpty())
            {
                path =
                    QDir::homePath();
            }

            const QFileInfo directoryInfo(
                path
            );

            if (
                !directoryInfo.exists()
                || !directoryInfo.isDir()
            )
            {
                return apiError(
                    QStringLiteral(
                        "Path '%1' tidak ditemukan."
                    ).arg(path),
                    StatusCode::BadRequest
                );
            }

            if (!directoryInfo.isReadable())
            {
                return apiError(
                    QStringLiteral(
                        "Akses Ditolak: Folder ini dilindungi Windows!"
                    ),
                    StatusCode::Forbidden
                );
            }

            QDir directory(
                directoryInfo.absoluteFilePath()
            );

            const QFileInfoList entries =
                directory.entryInfoList(
                    QDir::AllEntries
                    | QDir::NoDotAndDotDot,
                    QDir::DirsFirst
                    | QDir::Name
                    | QDir::IgnoreCase
                );

            QJsonArray items;

            for (const QFileInfo &entry : entries)
            {
                QJsonObject item;

                item["name"] =
                    entry.fileName();

                item["path"] =
                    QDir::toNativeSeparators(
                        entry.absoluteFilePath()
                    );

                item["is_dir"] =
                    entry.isDir();

                item["size"] =
                    entry.isDir()
                    ? 0
                    : static_cast<qint64>(
                        entry.size()
                    );

                items.append(
                    item
                );
            }

            QDir parent =
                directory;

            parent.cdUp();

            const QString currentPath =
                QDir::toNativeSeparators(
                    directory.absolutePath()
                );

            QJsonObject response;

            response["status"] =
                QStringLiteral("success");

            response["current_path"] =
                currentPath;

            response["parent_path"] =
                QDir::toNativeSeparators(
                    parent.absolutePath()
                );

            response["items"] =
                items;

            /*
             * Keep the newer C++ aliases as additive fields.
             */
            response["name"] =
                directory.dirName();

            response["path"] =
                currentPath;

            return jsonResponse(
                response
            );
        }
    );


    /*
     * POST /api/execute-sort
     */
    httpServer->route(
        "/api/execute-sort",
        QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest &request)
        {
            const QJsonObject body =
                parseJsonObject(
                    request
                );

            QString targetDirectory =
                body.value(
                    QStringLiteral(
                        "target_dir"
                    )
                ).toString()
                 .trimmed();

            if (targetDirectory.isEmpty())
            {
                targetDirectory =
                    QDir::home()
                        .filePath(
                            QStringLiteral(
                                "Downloads"
                            )
                        );
            }

            const QFileInfo directoryInfo(
                targetDirectory
            );

            if (
                !directoryInfo.exists()
                || !directoryInfo.isDir()
            )
            {
                return apiError(
                    QStringLiteral(
                        "Folder %1 tidak ditemukan di PC."
                    ).arg(targetDirectory),
                    StatusCode::BadRequest
                );
            }

            const SortResult result =
                executeRemoteSort(
                    targetDirectory,
                    database
                );

            QJsonObject response;

            response["status"] =
                QStringLiteral("success");

            response["success"] =
                true;

            response["message"] =
                QStringLiteral(
                    "Berhasil merapikan %1 file!"
                ).arg(
                    result.moved
                );

            /*
             * Python field.
             */
            response["moved_files"] =
                result.moved;

            /*
             * Newer C++ aliases.
             */
            response["moved"] =
                result.moved;

            response["failed"] =
                result.failed;

            return jsonResponse(
                response
            );
        }
    );


    /*
     * POST /api/delete
     */
    httpServer->route(
        "/api/delete",
        QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &request)
        {
            const QJsonObject body =
                parseJsonObject(
                    request
                );

            const QString path =
                jsonPath(
                    body
                );

            if (path.isEmpty())
            {
                return apiError(
                    QStringLiteral(
                        "Path file/folder tidak valid!"
                    ),
                    StatusCode::BadRequest
                );
            }

            if (isProtectedPath(path))
            {
                return apiError(
                    QStringLiteral(
                        "BAHAYA: Folder sistem tidak boleh dihapus!"
                    ),
                    StatusCode::Forbidden
                );
            }

            const QFileInfo info(
                path
            );

            if (!info.exists())
            {
                return apiError(
                    QStringLiteral(
                        "Path file/folder tidak valid!"
                    ),
                    StatusCode::BadRequest
                );
            }

            const bool success =
                info.isDir()
                ? QDir(path)
                    .removeRecursively()
                : QFile::remove(path);

            if (!success)
            {
                return apiError(
                    QStringLiteral(
                        "Gagal menghapus item."
                    ),
                    StatusCode::InternalServerError
                );
            }

            const QString name =
                info.fileName();

            QJsonObject response;

            response["status"] =
                QStringLiteral("success");

            response["success"] =
                true;

            response["message"] =
                QStringLiteral(
                    "Berhasil menghapus: %1"
                ).arg(name);

            response["deleted"] =
                path;

            return jsonResponse(
                response
            );
        }
    );


    /*
     * POST /api/upload
     */
    httpServer->route(
        "/api/upload",
        QHttpServerRequest::Method::Post,
        [](const QHttpServerRequest &request)
        {
            MultipartUpload upload;
            QByteArray body;
            QString errorMessage;

            if (
                !parseMultipartUpload(
                    request,
                    upload,
                    body,
                    errorMessage
                )
            )
            {
                return apiError(
                    errorMessage,
                    StatusCode::BadRequest
                );
            }

            QString targetDirectory =
                upload.targetDirectory
                    .trimmed();

            /*
             * Match Flask: default upload location is the user's home,
             * not Downloads.
             */
            if (targetDirectory.isEmpty())
            {
                targetDirectory =
                    QDir::homePath();
            }

            if (isProtectedPath(
                    targetDirectory
                ))
            {
                return apiError(
                    QStringLiteral(
                        "Protected target directory."
                    ),
                    StatusCode::Forbidden
                );
            }

            QString savedPath;

            if (
                !saveUpload(
                    upload,
                    body,
                    targetDirectory,
                    savedPath,
                    errorMessage
                )
            )
            {
                return apiError(
                    errorMessage,
                    StatusCode::BadRequest
                );
            }

            QJsonObject response;

            response["status"] =
                QStringLiteral("success");

            response["success"] =
                true;

            response["message"] =
                QStringLiteral(
                    "Berhasil upload '%1' ke PC!"
                ).arg(
                    upload.filename
                );

            response["filename"] =
                upload.filename;

            response["path"] =
                QDir::toNativeSeparators(
                    savedPath
                );

            return jsonResponse(
                response
            );
        }
    );
}
