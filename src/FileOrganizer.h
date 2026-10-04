#pragma once

#include <QList>
#include <QString>

class Database;

struct PlannedMove
{
    enum class Method
    {
        ExtensionMatch,
        AiPredicted,
        Fallback
    };

    QString sourcePath;
    QString targetPath;
    QString category;

    Method method = Method::ExtensionMatch;

    [[nodiscard]]
    QString methodText() const;
};

class FileOrganizer
{
public:
    explicit FileOrganizer(
        const QString &sourceDirectory,
        Database *database = nullptr
    );

    bool scan(
        bool useAi = true,
        bool autoOthers = true
    );

    bool executeMoves();

    bool executeSelectedMoves(
        const QList<int> &selectedIndices
    );

    bool undoLastAction(
        bool deleteEmptyFolders = true
    );

    [[nodiscard]]
    const QList<PlannedMove>& plannedMoves() const;

    [[nodiscard]]
    QString errorMessage() const;

    [[nodiscard]]
    int movedCount() const;

    [[nodiscard]]
    int failedCount() const;

    [[nodiscard]]
    int undoneCount() const;

private:
    QString sourceDirectory;
    Database *database = nullptr;

    QList<PlannedMove> moves;

    QString lastError;
    QString currentBatchId;

    int movedFiles = 0;
    int failedFiles = 0;
    int undoneFiles = 0;

    bool moveFile(
        const QString &sourcePath,
        const QString &targetPath
    );
};
