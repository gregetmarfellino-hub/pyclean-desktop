#include "CategoryRules.h"

#include <QHash>

QString CategoryRules::categoryForExtension(const QString &extension)
{
    static const QHash<QString, QString> extensionMap = {
        // Images
        {".jpg", "Images"},
        {".jpeg", "Images"},
        {".png", "Images"},
        {".gif", "Images"},
        {".webp", "Images"},
        {".bmp", "Images"},
        {".svg", "Images"},
        {".ico", "Images"},

        // Documents
        {".pdf", "Documents"},
        {".docx", "Documents"},
        {".doc", "Documents"},
        {".txt", "Documents"},
        {".xlsx", "Documents"},
        {".pptx", "Documents"},
        {".csv", "Documents"},
        {".md", "Documents"},

        // Archives
        {".zip", "Archives"},
        {".rar", "Archives"},
        {".7z", "Archives"},
        {".tar", "Archives"},
        {".gz", "Archives"},
        {".iso", "Archives"},

        // Videos
        {".mp4", "Videos"},
        {".mkv", "Videos"},
        {".avi", "Videos"},
        {".mov", "Videos"},
        {".wmv", "Videos"},

        // Audio
        {".mp3", "Audio"},
        {".wav", "Audio"},
        {".flac", "Audio"},
        {".m4a", "Audio"},
        {".ogg", "Audio"},

        // Code
        {".py", "Code"},
        {".js", "Code"},
        {".html", "Code"},
        {".cpp", "Code"},
        {".c", "Code"},
        {".css", "Code"},
        {".java", "Code"},
        {".php", "Code"},
        {".json", "Code"},
        {".sql", "Code"},

        // Programs
        {".exe", "Programs"},
        {".msi", "Programs"},
        {".apk", "Programs"},

        // Design
        {".psd", "Design"},
        {".ai", "Design"},
        {".fig", "Design"},
        {".blend", "Design"}
    };

    QString normalized = extension.trimmed().toLower();

    if (!normalized.startsWith('.')) {
        normalized.prepend('.');
    }

    return extensionMap.value(normalized, "Others");
}

QStringList CategoryRules::extensionsForCategory(const QString &category)
{
    static const QHash<QString, QStringList> categoryMap = {
        {
            "Images",
            {
                ".jpg", ".jpeg", ".png", ".gif",
                ".webp", ".bmp", ".svg", ".ico"
            }
        },
        {
            "Documents",
            {
                ".pdf", ".docx", ".doc", ".txt",
                ".xlsx", ".pptx", ".csv", ".md"
            }
        },
        {
            "Archives",
            {
                ".zip", ".rar", ".7z", ".tar",
                ".gz", ".iso"
            }
        },
        {
            "Videos",
            {
                ".mp4", ".mkv", ".avi", ".mov", ".wmv"
            }
        },
        {
            "Audio",
            {
                ".mp3", ".wav", ".flac", ".m4a", ".ogg"
            }
        },
        {
            "Code",
            {
                ".py", ".js", ".html", ".cpp", ".c",
                ".css", ".java", ".php", ".json", ".sql"
            }
        },
        {
            "Programs",
            {
                ".exe", ".msi", ".apk"
            }
        },
        {
            "Design",
            {
                ".psd", ".ai", ".fig", ".blend"
            }
        }
    };

    return categoryMap.value(category, {});
}