#pragma once

#include <QDialog>
#include <QString>

class QLabel;

class QrWindow : public QDialog
{
public:
    explicit QrWindow(
        const QString &serverUrl,
        QWidget *parent = nullptr
    );

private:
    QString serverUrl;

    QLabel *qrLabel = nullptr;
    QLabel *urlLabel = nullptr;
};