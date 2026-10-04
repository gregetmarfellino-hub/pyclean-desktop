#pragma once

#include <QString>
#include <QStringList>

class CategoryRules
{
public:
    static QString categoryForExtension(const QString &extension);

    static QStringList extensionsForCategory(const QString &category);
};