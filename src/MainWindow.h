#pragma once

#include <QMainWindow>

class QLineEdit;
class QLabel;
class QPushButton;
class QCheckBox;
class QFrame;
class FileOrganizer;
class MobileServer;
class QrWindow;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(
    MobileServer *mobileServer,
    QWidget *parent = nullptr
);

private:
    MobileServer *mobileServer;
    QrWindow *qrWindow = nullptr;
    QLineEdit *pathEdit;

    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QLabel *statusLabel;
    QLabel *dropLabel;

    QPushButton *themeButton;
    QPushButton *browseButton;
    QPushButton *scanButton;
    QPushButton *undoButton;
    QPushButton *mobileButton;

    QCheckBox *dbCheckBox;
    QCheckBox *aiCheckBox;
    QCheckBox *othersCheckBox;
    QCheckBox *deleteEmptyCheckBox;

    QFrame *settingsFrame;

    bool darkMode = true;

    void applyTheme();

    void scanAndPreview();
    void showPreview(FileOrganizer *organizer);
};