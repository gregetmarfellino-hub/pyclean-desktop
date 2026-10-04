#include "FileOrganizer.h"

#include "CategoryRules.h"
#include "Database.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QUuid>

namespace
{

struct KnowledgeGroup
{
    QString category;
    QStringList keywords;
};

const QList<KnowledgeGroup>& knowledgeBase()
{
    /*
     * Construct this table once and reuse it for every file.
     *
     * The old implementation rebuilt an entire QMap<QString, QStringList>
     * on every classifier call. Keeping a single static table removes that
     * repeated allocation/construction work.
     *
     * Keep the group order identical to the Python version. When two groups
     * have the same score, the first one wins just like the original Python
     * dictionary iteration order.
     */
    static const QList<KnowledgeGroup> base = {
        {
            QStringLiteral("Documents"),
            {
                QStringLiteral("laporan"),
                QStringLiteral("tugas"),
                QStringLiteral("skripsi"),
                QStringLiteral("jurnal"),
                QStringLiteral("makalah"),
                QStringLiteral("rekap"),
                QStringLiteral("data"),
                QStringLiteral("draft"),
                QStringLiteral("ujian"),
                QStringLiteral("note"),
                QStringLiteral("report"),
                QStringLiteral("doc"),
                QStringLiteral("invoice"),
                QStringLiteral("cv")
            }
        },
        {
            QStringLiteral("Images"),
            {
                QStringLiteral("foto"),
                QStringLiteral("gambar"),
                QStringLiteral("screenshot"),
                QStringLiteral("ss"),
                QStringLiteral("profil"),
                QStringLiteral("design"),
                QStringLiteral("logo"),
                QStringLiteral("icon"),
                QStringLiteral("img"),
                QStringLiteral("pic"),
                QStringLiteral("meme")
            }
        },
        {
            QStringLiteral("Videos"),
            {
                QStringLiteral("video"),
                QStringLiteral("rekaman"),
                QStringLiteral("gameplay"),
                QStringLiteral("clip"),
                QStringLiteral("zoom"),
                QStringLiteral("tutorial"),
                QStringLiteral("vlog"),
                QStringLiteral("movie"),
                QStringLiteral("vid")
            }
        },
        {
            QStringLiteral("Audio"),
            {
                QStringLiteral("musik"),
                QStringLiteral("lagu"),
                QStringLiteral("podcast"),
                QStringLiteral("audio"),
                QStringLiteral("sound"),
                QStringLiteral("voice"),
                QStringLiteral("bgm"),
                QStringLiteral("ost")
            }
        },
        {
            QStringLiteral("Archives"),
            {
                QStringLiteral("installer"),
                QStringLiteral("setup"),
                QStringLiteral("patch"),
                QStringLiteral("crack"),
                QStringLiteral("backup"),
                QStringLiteral("v1"),
                QStringLiteral("v2"),
                QStringLiteral("update"),
                QStringLiteral("pack"),
                QStringLiteral("archive")
            }
        },
        {
            QStringLiteral("Code"),
            {
                QStringLiteral("script"),
                QStringLiteral("main"),
                QStringLiteral("test"),
                QStringLiteral("bot"),
                QStringLiteral("config"),
                QStringLiteral("server"),
                QStringLiteral("client"),
                QStringLiteral("app"),
                QStringLiteral("index"),
                QStringLiteral("src"),
                QStringLiteral("query")
            }
        },
        {
            QStringLiteral("Programs"),
            {
                QStringLiteral("software"),
                QStringLiteral("tool"),
                QStringLiteral("program"),
                QStringLiteral("game"),
                QStringLiteral("mod"),
                QStringLiteral("portable")
            }
        },
        {
            QStringLiteral("Design"),
            {
                QStringLiteral("mockup"),
                QStringLiteral("template"),
                QStringLiteral("ui"),
                QStringLiteral("ux"),
                QStringLiteral("render"),
                QStringLiteral("3d"),
                QStringLiteral("asset")
            }
        }
    };

    return base;
}

QString mlPredictCategory(const QString &filename)
{
    QString text = filename.toLower();

    text.replace(
        QLatin1Char('_'),
        QLatin1Char(' ')
    );

    text.replace(
        QLatin1Char('-'),
        QLatin1Char(' ')
    );

    QString bestCategory =
        QStringLiteral("Others");

    int bestScore = 0;

    for (const KnowledgeGroup &group : knowledgeBase())
    {
        int score = 0;

        for (const QString &keyword : group.keywords)
        {
            if (text.contains(keyword))
            {
                ++score;
            }
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestCategory = group.category;
        }
    }

    return bestCategory;
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

} // namespace

QString PlannedMove::methodText() const
{
    switch (method)
    {
        case Method::ExtensionMatch:
            return QStringLiteral("Extension Match");

        case Method::AiPredicted:
            return QStringLiteral("AI Predicted");

        case Method::Fallback:
            return QStringLiteral("Fallback");
    }

    return QStringLiteral("Unknown");
}

FileOrganizer::FileOrganizer(
    const QString &sourceDirectory,
    Database *database
)
    : sourceDirectory(sourceDirectory),
      database(database)
{
}

bool FileOrganizer::scan(
    bool useAi,
    bool autoOthers
)
{
    moves.clear();
    lastError.clear();

    const QDir sourceDir(sourceDirectory);

    if (!sourceDir.exists())
    {
        lastError =
            QStringLiteral(
                "Source directory does not exist."
            );

        return false;
    }

    const QFileInfoList files =
        sourceDir.entryInfoList(
            QDir::Files |
            QDir::NoDotAndDotDot,
            QDir::Name
        );

    /*
     * Worst case every file becomes a planned move.
     * Reserving once avoids repeated QList growth while scanning.
     */
    moves.reserve(files.size());

    for (const QFileInfo &fileInfo : files)
    {
        const QString extension =
            fileInfo.suffix()
                .trimmed()
                .toLower();

        QString category;

        PlannedMove::Method method =
            PlannedMove::Method::ExtensionMatch;

        /*
         * Stage 1: extension classification.
         */
        if (!extension.isEmpty())
        {
            category =
                CategoryRules::categoryForExtension(
                    extension
                );
        }

        const bool extensionMatched =
            !category.isEmpty() &&
            category != QStringLiteral("Others");

        if (extensionMatched)
        {
            method =
                PlannedMove::Method::ExtensionMatch;
        }
        else
        {
            /*
             * Match the Python behavior:
             * unknown/no extension is only considered further when the
             * AI/NLP option is enabled.
             */
            if (!useAi)
            {
                continue;
            }

            /*
             * Python used os.path.splitext(filename)[0].
             * completeBaseName() gives equivalent behavior for filenames
             * such as "laporan_final.xyz".
             */
            const QString predicted =
                mlPredictCategory(
                    fileInfo.completeBaseName()
                );

            if (predicted != QStringLiteral("Others"))
            {
                category = predicted;

                method =
                    PlannedMove::Method::AiPredicted;
            }
            else
            {
                if (!autoOthers)
                {
                    continue;
                }

                category =
                    QStringLiteral("Others");

                method =
                    PlannedMove::Method::Fallback;
            }
        }

        const QString desiredTargetPath =
            sourceDir.filePath(
                category +
                QDir::separator() +
                fileInfo.fileName()
            );

        /*
         * Preview the collision-safe name the user is most likely to get.
         */
        const QString targetPath =
            uniqueDestinationPath(
                desiredTargetPath
            );

        moves.append({
            fileInfo.absoluteFilePath(),
            targetPath,
            category,
            method
        });
    }

    return true;
}

bool FileOrganizer::executeMoves()
{
    QList<int> allIndices;
    allIndices.reserve(moves.size());

    for (int i = 0; i < moves.size(); ++i)
    {
        allIndices.append(i);
    }

    return executeSelectedMoves(allIndices);
}

bool FileOrganizer::executeSelectedMoves(
    const QList<int> &selectedIndices
)
{
    movedFiles = 0;
    failedFiles = 0;
    currentBatchId.clear();
    lastError.clear();

    if (selectedIndices.isEmpty())
    {
        lastError =
            QStringLiteral(
                "No files were selected."
            );

        return false;
    }

    currentBatchId =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );

    for (const int index : selectedIndices)
    {
        if (
            index < 0 ||
            index >= moves.size()
        )
        {
            ++failedFiles;
            continue;
        }

        const PlannedMove &move =
            moves.at(index);

        const QFileInfo sourceInfo(
            move.sourcePath
        );

        const QDir sourceDir(
            sourceInfo.absolutePath()
        );

        if (!sourceDir.exists())
        {
            ++failedFiles;
            continue;
        }

        const QFileInfo previewTargetInfo(
            move.targetPath
        );

        const QString targetDirectoryPath =
            previewTargetInfo.absolutePath();

        if (!QDir().exists(targetDirectoryPath))
        {
            if (!QDir().mkpath(targetDirectoryPath))
            {
                ++failedFiles;
                continue;
            }
        }

        /*
         * Rebuild the desired destination from the original source filename,
         * then resolve collisions against the live filesystem.
         *
         * This also handles the race where a file appears after Scan Preview
         * but before Confirm & Execute.
         */
        const QString desiredTargetPath =
            QDir(targetDirectoryPath)
                .filePath(
                    sourceInfo.fileName()
                );

        const QString actualTargetPath =
            uniqueDestinationPath(
                desiredTargetPath
            );

        if (
            moveFile(
                move.sourcePath,
                actualTargetPath
            )
        )
        {
            ++movedFiles;

            if (database != nullptr)
            {
                /*
                 * Log the ACTUAL collision-safe destination so rollback knows
                 * exactly which file to restore.
                 */
                database->recordMove(
                    sourceInfo.fileName(),
                    move.category,
                    move.sourcePath,
                    actualTargetPath,
                    currentBatchId
                );
            }
        }
        else
        {
            ++failedFiles;
        }
    }

    return movedFiles > 0;
}

bool FileOrganizer::undoLastAction(
    bool deleteEmptyFolders
)
{
    undoneFiles = 0;
    lastError.clear();

    if (!database)
    {
        lastError =
            QStringLiteral(
                "Database is not available."
            );

        return false;
    }

    /*
     * Undo only the newest sorting batch.
     */
    const QString latestBatchId =
        database->latestBatchId();

    if (latestBatchId.isEmpty())
    {
        lastError =
            QStringLiteral(
                "There are no file operations to undo."
            );

        return false;
    }

    const QList<FileLog> logs =
        database->logsForBatch(
            latestBatchId
        );

    if (logs.isEmpty())
    {
        lastError =
            QStringLiteral(
                "There are no file operations to undo."
            );

        return false;
    }

    bool everythingSucceeded = true;

    for (const FileLog &log : logs)
    {
        if (!QFile::exists(log.targetPath))
        {
            everythingSucceeded = false;

            lastError =
                QStringLiteral(
                    "Cannot undo:\n\n"
                ) +
                log.targetPath +
                QStringLiteral(
                    "\n\nThe moved file could not be found."
                );

            continue;
        }

        const QFileInfo sourceInfo(
            log.sourcePath
        );

        QDir sourceDir =
            sourceInfo.absoluteDir();

        if (!sourceDir.exists())
        {
            if (!sourceDir.mkpath(QStringLiteral(".")))
            {
                everythingSucceeded = false;

                lastError =
                    QStringLiteral(
                        "Cannot recreate original directory:\n\n"
                    ) +
                    sourceDir.absolutePath();

                continue;
            }
        }

        if (
            !moveFile(
                log.targetPath,
                log.sourcePath
            )
        )
        {
            everythingSucceeded = false;
            continue;
        }

        if (!database->removeLog(log.id))
        {
            everythingSucceeded = false;

            lastError =
                QStringLiteral(
                    "File restored, but its database "
                    "log could not be removed:\n\n"
                ) +
                log.filename;

            continue;
        }

        ++undoneFiles;

        /*
         * rmdir() removes the category folder only when it is empty.
         */
        if (deleteEmptyFolders)
        {
            const QFileInfo targetInfo(
                log.targetPath
            );

            QDir categoryDirectory =
                targetInfo.absoluteDir();

            const QString categoryName =
                categoryDirectory.dirName();

            QDir parentDirectory =
                categoryDirectory;

            if (parentDirectory.cdUp())
            {
                parentDirectory.rmdir(
                    categoryName
                );
            }
        }
    }

    return everythingSucceeded;
}

bool FileOrganizer::moveFile(
    const QString &sourcePath,
    const QString &targetPath
)
{
    /*
     * Fast path: same-volume rename.
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
     * Fallback for moves that cannot be performed as a rename
     * (for example, between different volumes).
     */
    if (
        QFile::copy(
            sourcePath,
            targetPath
        )
    )
    {
        if (QFile::remove(sourcePath))
        {
            return true;
        }

        /*
         * Source deletion failed, so remove the copied target to avoid
         * pretending the move completed successfully.
         */
        QFile::remove(targetPath);
    }

    lastError =
        QStringLiteral(
            "Failed to move:\n"
        ) +
        sourcePath +
        QStringLiteral(
            "\n\nTo:\n"
        ) +
        targetPath;

    return false;
}

const QList<PlannedMove>&
FileOrganizer::plannedMoves() const
{
    return moves;
}

QString FileOrganizer::errorMessage() const
{
    return lastError;
}

int FileOrganizer::movedCount() const
{
    return movedFiles;
}

int FileOrganizer::failedCount() const
{
    return failedFiles;
}

int FileOrganizer::undoneCount() const
{
    return undoneFiles;
}
