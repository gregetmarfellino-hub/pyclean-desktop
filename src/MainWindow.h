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
    QPushButton *settingsButton;
    QPushButton *browseButton;
    QPushButton *scanButton;
    QPushButton *undoButton;
    QPushButton *mobileButton;

    bool darkMode = true;

    bool dbEnabled = true;
    bool aiEnabled = true;
    bool othersEnabled = true;
    bool deleteEmptyEnabled = true;
    bool mobileShareEnabled = false;
    bool mobileReadOnlyEnabled = true;

    void applyTheme();
    void openSettingsDialog();

    void scanAndPreview();
    void showPreview(FileOrganizer *organizer);
};