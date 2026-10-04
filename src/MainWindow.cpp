#include "MainWindow.h"

#include "FileOrganizer.h"
#include "Database.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QStyle>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTreeWidget>

#include <functional>
#include <utility>
#include "MobileServer.h"
#include "QrWindow.h"

// ======================================================
// FOLDER DROP LABEL
// ======================================================

class FolderDropLabel : public QLabel
{
public:
    using DropHandler =
        std::function<void(const QString&)>;

    explicit FolderDropLabel(
        const QString &text,
        QWidget *parent = nullptr
    )
        : QLabel(text, parent)
    {
        setAcceptDrops(true);
        setMouseTracking(true);
    }

    void setDropHandler(
        DropHandler handler
    )
    {
        dropHandler = std::move(handler);
    }

protected:
    void dragEnterEvent(
        QDragEnterEvent *event
    ) override
    {
        if (event->mimeData()->hasUrls()) {

            setProperty("dragActive", true);
            style()->unpolish(this);
            style()->polish(this);
            update();

            event->acceptProposedAction();
            return;
        }

        event->ignore();
    }

    void dragLeaveEvent(
        QDragLeaveEvent *event
    ) override
    {
        setProperty("dragActive", false);
        style()->unpolish(this);
        style()->polish(this);
        update();

        QLabel::dragLeaveEvent(event);
    }

    void dropEvent(
        QDropEvent *event
    ) override
    {
        setProperty("dragActive", false);
        style()->unpolish(this);
        style()->polish(this);
        update();

        const QList<QUrl> urls =
            event->mimeData()->urls();

        for (const QUrl &url : urls) {

            if (!url.isLocalFile()) {
                continue;
            }

            const QString droppedPath =
                url.toLocalFile();

            QFileInfo info(droppedPath);

            if (!info.exists()) {
                continue;
            }

            const QString folderPath =
                info.isDir()
                    ? info.absoluteFilePath()
                    : info.absolutePath();

            if (dropHandler) {
                dropHandler(folderPath);
            }

            event->acceptProposedAction();
            return;
        }

        event->ignore();
    }

private:
    DropHandler dropHandler;
};


// ======================================================
// MAIN WINDOW
// ======================================================

MainWindow::MainWindow(
    MobileServer *mobileServer,
    QWidget *parent
)
    : QMainWindow(parent),
      mobileServer(mobileServer)
{
    setWindowTitle("PyClean — Desktop Core");
    resize(680, 650);
    setMinimumSize(600, 580);

    // ==================================================
    // CENTRAL WIDGET
    // ==================================================

    auto *centralWidget =
        new QWidget(this);

    centralWidget->setObjectName(
        "centralWidget"
    );

    auto *mainLayout =
        new QVBoxLayout(centralWidget);

    mainLayout->setContentsMargins(
        28, 22, 28, 18
    );

    mainLayout->setSpacing(12);


    // ==================================================
    // HEADER
    // ==================================================

    auto *headerLayout =
        new QHBoxLayout();

    headerLayout->setSpacing(12);


    titleLabel =
        new QLabel("PyClean");

    titleLabel->setObjectName(
        "titleLabel"
    );

    titleLabel->setToolTip(
        "PyClean C++ Edition"
    );


    themeButton =
        new QPushButton("☀  Light");

    themeButton->setObjectName(
        "themeButton"
    );

    themeButton->setCursor(
        Qt::PointingHandCursor
    );

    themeButton->setMinimumHeight(36);
    themeButton->setMinimumWidth(104);


    headerLayout->addWidget(
        titleLabel
    );

    headerLayout->addStretch();

    headerLayout->addWidget(
        themeButton
    );

    mainLayout->addLayout(
        headerLayout
    );


    // ==================================================
    // SUBTITLE / EDITION TAG
    // ==================================================

    auto *subtitleRow =
        new QHBoxLayout();

    subtitleRow->setSpacing(8);


    subtitleLabel =
        new QLabel(
            "Smart File Organizer • Local-first"
        );

    subtitleLabel->setObjectName(
        "subtitleLabel"
    );


    auto *editionLabel =
        new QLabel(
            "C++  •  QT 6  •  DESKTOP"
        );

    editionLabel->setObjectName(
        "editionLabel"
    );

    editionLabel->setAlignment(
        Qt::AlignRight |
        Qt::AlignVCenter
    );


    subtitleRow->addWidget(
        subtitleLabel
    );

    subtitleRow->addStretch();

    subtitleRow->addWidget(
        editionLabel
    );

    mainLayout->addLayout(
        subtitleRow
    );


    // ==================================================
    // DIRECTORY CARD
    // ==================================================

    auto *directoryFrame =
        new QFrame();

    directoryFrame->setObjectName(
        "directoryFrame"
    );

    auto *directoryLayout =
        new QVBoxLayout(directoryFrame);

    directoryLayout->setContentsMargins(
        16, 12, 16, 14
    );

    directoryLayout->setSpacing(8);


    auto *directoryTitle =
        new QLabel(
            "SOURCE DIRECTORY"
        );

    directoryTitle->setObjectName(
        "sectionLabel"
    );


    auto *directoryRow =
        new QHBoxLayout();

    directoryRow->setSpacing(8);


    pathEdit =
        new QLineEdit();

    pathEdit->setObjectName(
        "pathEdit"
    );

    pathEdit->setPlaceholderText(
        "Select a folder to organize..."
    );

    pathEdit->setMinimumHeight(40);

    pathEdit->setClearButtonEnabled(
        true
    );


    browseButton =
        new QPushButton(
            "Browse"
        );

    browseButton->setObjectName(
        "browseButton"
    );

    browseButton->setCursor(
        Qt::PointingHandCursor
    );

    browseButton->setMinimumWidth(96);
    browseButton->setMinimumHeight(40);


    directoryRow->addWidget(
        pathEdit,
        1
    );

    directoryRow->addWidget(
        browseButton
    );


    directoryLayout->addWidget(
        directoryTitle
    );

    directoryLayout->addLayout(
        directoryRow
    );


    mainLayout->addWidget(
        directoryFrame
    );


    // ==================================================
    // ENGINE SETTINGS
    // ==================================================

    settingsFrame =
        new QFrame();

    settingsFrame->setObjectName(
        "settingsFrame"
    );


    auto *settingsLayout =
        new QVBoxLayout(settingsFrame);

    settingsLayout->setContentsMargins(
        16, 12, 16, 14
    );

    settingsLayout->setSpacing(6);


    auto *settingsHeader =
        new QHBoxLayout();


    auto *settingsTitle =
        new QLabel(
            "ENGINE CONFIGURATION"
        );

    settingsTitle->setObjectName(
        "sectionLabel"
    );


    auto *settingsTag =
        new QLabel(
            "CORE"
        );

    settingsTag->setObjectName(
        "sectionTag"
    );

    settingsTag->setAlignment(
        Qt::AlignCenter
    );


    settingsHeader->addWidget(
        settingsTitle
    );

    settingsHeader->addStretch();

    settingsHeader->addWidget(
        settingsTag
    );


    dbCheckBox =
        new QCheckBox(
            "SQLite Persistent Audit Trail"
        );

    aiCheckBox =
        new QCheckBox(
            "AI NLP Classification"
        );

    othersCheckBox =
        new QCheckBox(
            "Auto-group unknown files → Others"
        );

    deleteEmptyCheckBox =
        new QCheckBox(
            "Remove empty folders on rollback"
        );


    dbCheckBox->setObjectName(
        "dbCheckBox"
    );

    aiCheckBox->setObjectName(
        "aiCheckBox"
    );

    othersCheckBox->setObjectName(
        "othersCheckBox"
    );

    deleteEmptyCheckBox->setObjectName(
        "deleteEmptyCheckBox"
    );


    dbCheckBox->setChecked(true);
    aiCheckBox->setChecked(true);
    othersCheckBox->setChecked(true);
    deleteEmptyCheckBox->setChecked(true);


    dbCheckBox->setToolTip(
        "Record successful file operations in SQLite."
    );

    aiCheckBox->setToolTip(
        "Use filename keywords when an extension is unknown or missing."
    );

    othersCheckBox->setToolTip(
        "Send files that remain unclassified to Others."
    );

    deleteEmptyCheckBox->setToolTip(
        "Remove empty category folders after rollback."
    );


    settingsLayout->addLayout(
        settingsHeader
    );

    settingsLayout->addSpacing(3);

    settingsLayout->addWidget(
        dbCheckBox
    );

    settingsLayout->addWidget(
        aiCheckBox
    );

    settingsLayout->addWidget(
        othersCheckBox
    );

    settingsLayout->addWidget(
        deleteEmptyCheckBox
    );


    mainLayout->addWidget(
        settingsFrame
    );


    // ==================================================
    // ACTION BUTTONS
    // ==================================================

    auto *actionLabel =
        new QLabel(
            "OPERATIONS"
        );

    actionLabel->setObjectName(
        "sectionLabel"
    );

    mainLayout->addWidget(
        actionLabel
    );


    auto *buttonLayout =
        new QHBoxLayout();

    buttonLayout->setSpacing(9);


    scanButton =
        new QPushButton(
            "↳  Scan & Review"
        );

    undoButton =
        new QPushButton(
            "↶  Undo Last Batch"
        );

    mobileButton =
        new QPushButton(
            "▣  Launch Agent"
        );


    scanButton->setObjectName(
        "scanButton"
    );

    undoButton->setObjectName(
        "undoButton"
    );

    mobileButton->setObjectName(
        "mobileButton"
    );


    scanButton->setCursor(
        Qt::PointingHandCursor
    );

    undoButton->setCursor(
        Qt::PointingHandCursor
    );

    mobileButton->setCursor(
        Qt::PointingHandCursor
    );


    scanButton->setMinimumHeight(48);
    undoButton->setMinimumHeight(48);
    mobileButton->setMinimumHeight(48);


    scanButton->setToolTip(
        "Scan the selected directory and review planned moves."
    );

    undoButton->setToolTip(
        "Restore the most recent completed operation."
    );

    mobileButton->setToolTip(
        "Start or stop the PyClean LAN mobile agent."
    );


    buttonLayout->addWidget(
        scanButton
    );

    buttonLayout->addWidget(
        undoButton
    );

    buttonLayout->addWidget(
        mobileButton
    );


    mainLayout->addLayout(
        buttonLayout
    );


    // ==================================================
    // DROP ZONE
    // ==================================================

    dropLabel =
        new FolderDropLabel(
            "DROP A FOLDER\n\n"
            "Drag & drop a directory to begin"
        );

    dropLabel->setObjectName(
        "dropLabel"
    );

    dropLabel->setAlignment(
        Qt::AlignCenter
    );

    dropLabel->setMinimumHeight(
        155
    );

    dropLabel->setCursor(
        Qt::PointingHandCursor
    );

    dropLabel->setToolTip(
        "Drop a folder here to select it."
    );


    mainLayout->addWidget(
        dropLabel,
        1
    );


    auto *dropZone =
        static_cast<FolderDropLabel*>(
            dropLabel
        );


    dropZone->setDropHandler(
        [this](const QString &folderPath) {

            pathEdit->setText(
                QDir::toNativeSeparators(
                    folderPath
                )
            );

            dropLabel->setText(
                "✓  FOLDER SELECTED\n\n"
                + QDir::toNativeSeparators(
                    folderPath
                )
            );

            statusLabel->setText(
                "●  Folder loaded via drag & drop"
            );
        }
    );


    // ==================================================
    // STATUS BAR AREA
    // ==================================================

    auto *statusFrame =
        new QFrame();

    statusFrame->setObjectName(
        "statusFrame"
    );

    statusFrame->setMaximumHeight(
        36
    );


    auto *statusLayout =
        new QHBoxLayout(statusFrame);

    statusLayout->setContentsMargins(
        10, 5, 10, 5
    );


    statusLabel =
        new QLabel(
            "●  READY"
        );

    statusLabel->setObjectName(
        "statusLabel"
    );


    auto *versionLabel =
        new QLabel(
            "PyClean Core • Release"
        );

    versionLabel->setObjectName(
        "versionLabel"
    );

    versionLabel->setAlignment(
        Qt::AlignRight |
        Qt::AlignVCenter
    );


    statusLayout->addWidget(
        statusLabel
    );

    statusLayout->addStretch();

    statusLayout->addWidget(
        versionLabel
    );


    mainLayout->addWidget(
        statusFrame
    );


    setCentralWidget(
        centralWidget
    );


    // ==================================================
    // BROWSE
    // ==================================================

    connect(
        browseButton,
        &QPushButton::clicked,
        this,
        [this]() {

            const QString selectedDirectory =
                QFileDialog::getExistingDirectory(
                    this,
                    "Select Folder",
                    pathEdit->text().trimmed()
                );

            if (
                selectedDirectory.isEmpty()
            ) {
                return;
            }

            pathEdit->setText(
                QDir::toNativeSeparators(
                    selectedDirectory
                )
            );

            dropLabel->setText(
                "FOLDER SELECTED\n\n"
                + QDir::toNativeSeparators(
                    selectedDirectory
                )
            );

            statusLabel->setText(
                "●  Folder selected"
            );
        }
    );


    // ==================================================
    // SCAN
    // ==================================================

    connect(
        scanButton,
        &QPushButton::clicked,
        this,
        &MainWindow::scanAndPreview
    );


    // ==================================================
    // UNDO
    // ==================================================

    connect(
        undoButton,
        &QPushButton::clicked,
        this,
        [this]() {

            const QString databasePath =
                QCoreApplication::applicationDirPath()
                + "/pyclean_logs.db";

            Database database(
                databasePath
            );

            if (!database.initialize()) {

                QMessageBox::warning(
                    this,
                    "Database Error",
                    database.errorMessage()
                );

                return;
            }

            FileOrganizer organizer(
                QString(),
                &database
            );

            statusLabel->setText(
                "●  Rolling back..."
            );


            if (
                !organizer.undoLastAction(
                    deleteEmptyCheckBox->isChecked()
                )
            ) {

                QMessageBox::information(
                    this,
                    "PyClean",
                    "There is no previous action to undo."
                );

                statusLabel->setText(
                    "●  Nothing to undo"
                );

                return;
            }


            QMessageBox::information(
                this,
                "Rollback Complete",
                QString(
                    "Rollback complete!\n\n"
                    "Restored: %1 file(s)"
                )
                .arg(
                    organizer.undoneCount()
                )
            );


            statusLabel->setText(
                QString(
                    "●  Restored %1 file(s)"
                )
                .arg(
                    organizer.undoneCount()
                )
            );
        }
    );


    // ==================================================
    // THEME TOGGLE
    // ==================================================

    connect(
        themeButton,
        &QPushButton::clicked,
        this,
        [this]() {

            darkMode = !darkMode;

            QSettings settings(
                QCoreApplication::applicationDirPath()
                + "/pyclean_config.ini",
                QSettings::IniFormat
            );

            settings.setValue(
                "is_dark_mode",
                darkMode
            );

            applyTheme();
        }
    );


    // ==================================================
    // LOAD SAVED THEME
    // ==================================================

    QSettings settings(
        QCoreApplication::applicationDirPath()
        + "/pyclean_config.ini",
        QSettings::IniFormat
    );

    darkMode =
        settings.value(
            "is_dark_mode",
            true
        ).toBool();


    // ==================================================
    // APPLY THEME
    // ==================================================

// ==================================================
// MOBILE AGENT TOGGLE
// ==================================================

connect(
    mobileButton,
    &QPushButton::clicked,
    this,
    [this]() {

        if (!this->mobileServer)
        {
            QMessageBox::warning(
                this,
                "PyClean Mobile",
                "The mobile server is unavailable."
            );

            return;
        }


        /*
         * Second click = stop the agent, matching the original Python
         * toggle behaviour.
         */
        if (this->mobileServer->isRunning())
        {
            this->mobileServer->stop();

            mobileButton->setText(
                "▣  Launch Agent"
            );

            statusLabel->setText(
                "●  Mobile agent stopped"
            );

            if (qrWindow)
            {
                qrWindow->close();
                qrWindow = nullptr;
            }

            return;
        }


        statusLabel->setText(
            "●  Starting mobile agent..."
        );


        if (!this->mobileServer->start(5000))
        {
            const QString details =
                this->mobileServer
                    ->errorMessage();

            QMessageBox::warning(
                this,
                "PyClean Mobile",
                details.isEmpty()
                    ? "The mobile server could not be started."
                    : "The mobile server could not be started.\n\n"
                      + details
            );

            statusLabel->setText(
                "●  Mobile agent failed to start"
            );

            return;
        }


        mobileButton->setText(
            "■  Stop Agent"
        );


        const QString serverUrl =
            QString(
                "http://%1:%2"
            )
            .arg(
                this->mobileServer->localIp()
            )
            .arg(
                this->mobileServer->port()
            );


        statusLabel->setText(
            QString(
                "●  Mobile agent online — %1:%2"
            )
            .arg(
                this->mobileServer->localIp()
            )
            .arg(
                this->mobileServer->port()
            )
        );


        qrWindow =
            new QrWindow(
                serverUrl,
                this
            );

        qrWindow->setAttribute(
            Qt::WA_DeleteOnClose
        );

        connect(
            qrWindow,
            &QObject::destroyed,
            this,
            [this]() {
                qrWindow = nullptr;
            }
        );

        qrWindow->show();
    }
);


// ==================================================
// APPLY THEME
// ==================================================

applyTheme();
}


// ======================================================
// THEME
// ======================================================

void MainWindow::applyTheme()
{
    if (darkMode) {

        setStyleSheet(R"(
            QMainWindow {
                background-color: #171722;
                color: #d9e0f2;
            }

            QWidget#centralWidget {
                background-color: #171722;
            }

            QLabel {
                color: #d9e0f2;
                background: transparent;
            }

            QLabel#titleLabel {
                color: #f2f4ff;
                font-size: 25px;
                font-weight: 800;
                letter-spacing: 1px;
            }

            QLabel#subtitleLabel {
                color: #9ca8c3;
                font-size: 11px;
            }

            QLabel#editionLabel {
                color: #89b4fa;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 1px;
            }

            QLabel#sectionLabel {
                color: #aeb9d3;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 1px;
            }

            QLabel#sectionTag {
                color: #89b4fa;
                background-color: #202438;
                border: 1px solid #303852;
                border-radius: 7px;
                padding: 3px 8px;
                font-size: 8px;
                font-weight: bold;
            }

            QFrame#directoryFrame,
            QFrame#settingsFrame {
                background-color: #20202d;
                border: 1px solid #303044;
                border-radius: 10px;
            }

            QLineEdit#pathEdit {
                background-color: #11111b;
                color: #f2f4ff;
                border: 1px solid #35384c;
                border-radius: 7px;
                padding: 0 12px;
                selection-background-color: #89b4fa;
                selection-color: #11111b;
                font-size: 10px;
            }

            QLineEdit#pathEdit:hover {
                border: 1px solid #4d5572;
            }

            QLineEdit#pathEdit:focus {
                border: 1px solid #89b4fa;
            }

            QPushButton {
                border: none;
                border-radius: 8px;
                padding: 8px 13px;
                font-weight: bold;
                font-size: 10px;
            }

            QPushButton:pressed {
                padding-top: 10px;
                padding-bottom: 6px;
            }

            QPushButton#themeButton,
            QPushButton#browseButton {
                background-color: #2b2d40;
                color: #d9e0f2;
                border: 1px solid #3a3e55;
            }

            QPushButton#themeButton:hover,
            QPushButton#browseButton:hover {
                background-color: #34384e;
                border: 1px solid #505873;
            }

            QPushButton#themeButton:pressed,
            QPushButton#browseButton:pressed {
                background-color: #242638;
            }

            QPushButton#scanButton {
                background-color: #89b4fa;
                color: #11111b;
            }

            QPushButton#scanButton:hover {
                background-color: #a5c6ff;
            }

            QPushButton#scanButton:pressed {
                background-color: #709eea;
            }

            QPushButton#undoButton {
                background-color: #f38ba8;
                color: #21121a;
            }

            QPushButton#undoButton:hover {
                background-color: #f6a1b8;
            }

            QPushButton#undoButton:pressed {
                background-color: #dc7694;
            }

            QPushButton#mobileButton {
                background-color: #a6e3a1;
                color: #102015;
            }

            QPushButton#mobileButton:hover {
                background-color: #b8edb3;
            }

            QPushButton#mobileButton:pressed {
                background-color: #8fd58a;
            }

            QCheckBox {
                color: #cbd3e7;
                spacing: 9px;
                padding: 3px 2px;
                font-size: 10px;
            }

            QCheckBox:hover {
                color: #f2f4ff;
            }

            QCheckBox::indicator {
                width: 17px;
                height: 17px;
                border-radius: 5px;
                border: 1px solid #4a5068;
                background-color: #151520;
            }

            QCheckBox::indicator:hover {
                border: 1px solid #89b4fa;
            }

            QCheckBox::indicator:checked {
                background-color: #89b4fa;
                border: 1px solid #89b4fa;
            }

            QCheckBox::indicator:pressed {
                background-color: #6e96d6;
            }

            QLabel#dropLabel {
                background-color: #1b1b29;
                color: #8994ad;
                border: 1px dashed #424861;
                border-radius: 12px;
                padding: 20px;
                font-size: 11px;
                font-weight: 600;
            }

            QLabel#dropLabel:hover {
                color: #b7c2dc;
                border: 1px dashed #627096;
                background-color: #1e1f2f;
            }

            QLabel#dropLabel[dragActive="true"] {
                color: #89b4fa;
                border: 2px dashed #89b4fa;
                background-color: #20273a;
            }

            QFrame#statusFrame {
                background-color: #1d1e2b;
                border: 1px solid #2d3041;
                border-radius: 8px;
            }

            QLabel#statusLabel {
                color: #a6e3a1;
                font-size: 9px;
                font-weight: bold;
            }

            QLabel#versionLabel {
                color: #68728a;
                font-size: 8px;
            }

            QScrollArea {
                background-color: #191925;
                border: 1px solid #303044;
                border-radius: 8px;
            }

            QScrollBar:vertical {
                background: #191925;
                width: 10px;
                margin: 3px;
            }

            QScrollBar::handle:vertical {
                background: #424861;
                border-radius: 5px;
                min-height: 30px;
            }

            QScrollBar::handle:vertical:hover {
                background: #59627d;
            }

            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {
                height: 0px;
            }

            QDialog {
                background-color: #171722;
                color: #d9e0f2;
            }

            QDialog QLabel {
                color: #d9e0f2;
            }

            QDialog QCheckBox {
                background-color: #20202d;
                border: 1px solid #2f3345;
                border-radius: 7px;
                padding: 8px;
            }

            QDialog QCheckBox:hover {
                background-color: #27293a;
                border: 1px solid #414960;
            }

        )");

        themeButton->setText(
            "☀  Light"
        );
    }
    else {

        setStyleSheet(R"(
            QMainWindow {
                background-color: #f4f6fa;
                color: #202735;
            }

            QWidget#centralWidget {
                background-color: #f4f6fa;
            }

            QLabel {
                color: #202735;
                background: transparent;
            }

            QLabel#titleLabel {
                color: #172033;
                font-size: 25px;
                font-weight: 800;
                letter-spacing: 1px;
            }

            QLabel#subtitleLabel {
                color: #697386;
                font-size: 11px;
            }

            QLabel#editionLabel {
                color: #2367b1;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 1px;
            }

            QLabel#sectionLabel {
                color: #58657a;
                font-size: 9px;
                font-weight: bold;
                letter-spacing: 1px;
            }

            QLabel#sectionTag {
                color: #2367b1;
                background-color: #e6effa;
                border: 1px solid #cbdcf0;
                border-radius: 7px;
                padding: 3px 8px;
                font-size: 8px;
                font-weight: bold;
            }

            QFrame#directoryFrame,
            QFrame#settingsFrame {
                background-color: #ffffff;
                border: 1px solid #dce1ea;
                border-radius: 10px;
            }

            QLineEdit#pathEdit {
                background-color: #fbfcfe;
                color: #182131;
                border: 1px solid #cfd6e2;
                border-radius: 7px;
                padding: 0 12px;
                selection-background-color: #2367b1;
                selection-color: #ffffff;
                font-size: 10px;
            }

            QLineEdit#pathEdit:hover {
                border: 1px solid #aeb9c9;
            }

            QLineEdit#pathEdit:focus {
                border: 1px solid #2367b1;
            }

            QPushButton {
                border: none;
                border-radius: 8px;
                padding: 8px 13px;
                font-weight: bold;
                font-size: 10px;
            }

            QPushButton:pressed {
                padding-top: 10px;
                padding-bottom: 6px;
            }

            QPushButton#themeButton,
            QPushButton#browseButton {
                background-color: #e9edf3;
                color: #303a4c;
                border: 1px solid #d2d9e3;
            }

            QPushButton#themeButton:hover,
            QPushButton#browseButton:hover {
                background-color: #dfe5ed;
                border: 1px solid #bdc7d5;
            }

            QPushButton#themeButton:pressed,
            QPushButton#browseButton:pressed {
                background-color: #d5dce6;
            }

            QPushButton#scanButton {
                background-color: #2367b1;
                color: #ffffff;
            }

            QPushButton#scanButton:hover {
                background-color: #2f78c8;
            }

            QPushButton#scanButton:pressed {
                background-color: #1d5795;
            }

            QPushButton#undoButton {
                background-color: #d94b68;
                color: #ffffff;
            }

            QPushButton#undoButton:hover {
                background-color: #e15e78;
            }

            QPushButton#undoButton:pressed {
                background-color: #bd3d57;
            }

            QPushButton#mobileButton {
                background-color: #2b8a57;
                color: #ffffff;
            }

            QPushButton#mobileButton:hover {
                background-color: #349c64;
            }

            QPushButton#mobileButton:pressed {
                background-color: #24764b;
            }

            QCheckBox {
                color: #39465a;
                spacing: 9px;
                padding: 3px 2px;
                font-size: 10px;
            }

            QCheckBox:hover {
                color: #172033;
            }

            QCheckBox::indicator {
                width: 17px;
                height: 17px;
                border-radius: 5px;
                border: 1px solid #b8c1ce;
                background-color: #ffffff;
            }

            QCheckBox::indicator:hover {
                border: 1px solid #2367b1;
            }

            QCheckBox::indicator:checked {
                background-color: #2367b1;
                border: 1px solid #2367b1;
            }

            QCheckBox::indicator:pressed {
                background-color: #1d5795;
            }

            QLabel#dropLabel {
                background-color: #fbfcfe;
                color: #7a8699;
                border: 1px dashed #c5ccd8;
                border-radius: 12px;
                padding: 20px;
                font-size: 11px;
                font-weight: 600;
            }

            QLabel#dropLabel:hover {
                color: #526075;
                border: 1px dashed #8e9aae;
                background-color: #f7f9fc;
            }

            QLabel#dropLabel[dragActive="true"] {
                color: #2367b1;
                border: 2px dashed #2367b1;
                background-color: #edf5fd;
            }

            QFrame#statusFrame {
                background-color: #ffffff;
                border: 1px solid #dce1ea;
                border-radius: 8px;
            }

            QLabel#statusLabel {
                color: #27824f;
                font-size: 9px;
                font-weight: bold;
            }

            QLabel#versionLabel {
                color: #8b96a8;
                font-size: 8px;
            }

            QScrollArea {
                background-color: #ffffff;
                border: 1px solid #dce1ea;
                border-radius: 8px;
            }

            QScrollBar:vertical {
                background: #f1f3f7;
                width: 10px;
                margin: 3px;
            }

            QScrollBar::handle:vertical {
                background: #b8c1ce;
                border-radius: 5px;
                min-height: 30px;
            }

            QScrollBar::handle:vertical:hover {
                background: #9ca8b9;
            }

            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {
                height: 0px;
            }

            QDialog {
                background-color: #f4f6fa;
                color: #202735;
            }

            QDialog QLabel {
                color: #202735;
            }

            QDialog QCheckBox {
                background-color: #ffffff;
                border: 1px solid #dce1ea;
                border-radius: 7px;
                padding: 8px;
            }

            QDialog QCheckBox:hover {
                background-color: #f7f9fc;
                border: 1px solid #c2cad6;
            }

        )");

        themeButton->setText(
            "☾  Dark"
        );
    }
}


// ======================================================
// SCAN
// ======================================================

void MainWindow::scanAndPreview()
{
    const QString directory =
        pathEdit->text().trimmed();

    if (directory.isEmpty()) {

        QMessageBox::warning(
            this,
            "PyClean",
            "Please select a folder first."
        );

        return;
    }


    const QFileInfo directoryInfo(
        directory
    );

    if (
        !directoryInfo.exists()
        || !directoryInfo.isDir()
    ) {

        QMessageBox::warning(
            this,
            "PyClean",
            "The selected path is not a valid folder."
        );

        return;
    }


    const QString databasePath =
        QCoreApplication::applicationDirPath()
        + "/pyclean_logs.db";


    /*
     * Keep these objects on the stack.
     *
     * The old version allocated both Database and FileOrganizer with new
     * and then manually deleted them on every exit path. Stack lifetime is
     * cheaper, simpler, and guarantees cleanup on every return.
     */
    Database database(
        databasePath
    );


    Database *activeDatabase =
        nullptr;


    if (dbCheckBox->isChecked()) {

        if (!database.initialize()) {

            QMessageBox::warning(
                this,
                "Database Error",
                database.errorMessage()
            );

            return;
        }

        activeDatabase =
            &database;
    }


    FileOrganizer organizer(
        directory,
        activeDatabase
    );


    statusLabel->setText(
        "●  Scanning directory..."
    );


    if (
        !organizer.scan(
            aiCheckBox->isChecked(),
            othersCheckBox->isChecked()
        )
    ) {

        QMessageBox::warning(
            this,
            "Scan Error",
            organizer.errorMessage()
        );

        statusLabel->setText(
            "●  READY"
        );

        return;
    }


    if (
        organizer.plannedMoves().isEmpty()
    ) {

        QMessageBox::information(
            this,
            "PyClean",
            "No files were found that need organizing."
        );

        statusLabel->setText(
            "●  Nothing to organize"
        );

        return;
    }


    showPreview(
        &organizer
    );
}


// ======================================================
// PREVIEW
// ======================================================

void MainWindow::showPreview(
    FileOrganizer *organizer
)
{
    if (!organizer)
    {
        return;
    }

    const auto &moves =
        organizer->plannedMoves();

    int renamedCount = 0;

    for (const PlannedMove &move : moves)
    {
        const QFileInfo sourceInfo(move.sourcePath);
        const QFileInfo targetInfo(move.targetPath);

        if (sourceInfo.fileName() != targetInfo.fileName())
        {
            ++renamedCount;
        }
    }


    // ======================================================
    // DIALOG
    // ======================================================

    QDialog dialog(this);

    dialog.setObjectName("previewDialog");
    dialog.setWindowTitle("PyClean — Review Changes");
    dialog.resize(900, 640);
    dialog.setMinimumSize(720, 520);
    dialog.setModal(true);

    const QString previewStyle = darkMode
        ? QStringLiteral(R"(
            QDialog#previewDialog {
                background:#171722;
                color:#d9e0f2;
            }
            QLabel#previewTitle {
                color:#f2f4ff;
                font-size:22px;
                font-weight:800;
            }
            QLabel#previewSubtitle,
            QLabel#previewSelection {
                color:#9ca8c3;
                font-size:10px;
            }
            QLabel#previewBadge {
                color:#89b4fa;
                background:#20273a;
                border:1px solid #34405f;
                border-radius:9px;
                padding:5px 10px;
                font-size:9px;
                font-weight:800;
            }
            QTreeWidget#previewTree {
                background:#1b1b29;
                color:#e4e8f5;
                border:1px solid #303044;
                border-radius:10px;
                outline:none;
                font-size:10px;
                padding:4px;
            }
            QTreeWidget#previewTree::item {
                min-height:38px;
                border-bottom:1px solid #292b3d;
                padding:4px 6px;
            }
            QTreeWidget#previewTree::item:hover {
                background:#222536;
            }
            QTreeWidget#previewTree QHeaderView::section {
                background:#20202d;
                color:#98a5bf;
                border:none;
                border-bottom:1px solid #34364a;
                padding:9px 8px;
                font-size:9px;
                font-weight:800;
            }
            QTreeWidget#previewTree::indicator {
                width:17px;
                height:17px;
                border-radius:5px;
                border:1px solid #4a5068;
                background:#151520;
            }
            QTreeWidget#previewTree::indicator:hover {
                border-color:#89b4fa;
            }
            QTreeWidget#previewTree::indicator:checked {
                background:#89b4fa;
                border-color:#89b4fa;
            }
            QPushButton#previewGhost {
                background:#242638;
                color:#cbd3e7;
                border:1px solid #383d55;
                border-radius:7px;
                padding:7px 12px;
                font-size:9px;
                font-weight:700;
            }
            QPushButton#previewGhost:hover {
                background:#2e3247;
            }
            QPushButton#previewCancel {
                background:#292b3d;
                color:#d9e0f2;
                border:1px solid #3a3e55;
                border-radius:8px;
                padding:9px 18px;
                font-weight:700;
            }
            QPushButton#previewCancel:hover {
                background:#34384e;
            }
            QPushButton#previewExecute {
                background:#89b4fa;
                color:#11111b;
                border:none;
                border-radius:8px;
                padding:9px 20px;
                font-weight:800;
            }
            QPushButton#previewExecute:hover {
                background:#a5c6ff;
            }
            QPushButton#previewExecute:disabled {
                background:#3b4257;
                color:#747d95;
            }
            QScrollBar:vertical {
                background:#191925;
                width:10px;
                margin:3px;
            }
            QScrollBar::handle:vertical {
                background:#424861;
                border-radius:5px;
                min-height:32px;
            }
            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {
                height:0px;
            }
        )")
        : QStringLiteral(R"(
            QDialog#previewDialog {
                background:#f4f6fa;
                color:#202735;
            }
            QLabel#previewTitle {
                color:#172033;
                font-size:22px;
                font-weight:800;
            }
            QLabel#previewSubtitle,
            QLabel#previewSelection {
                color:#697386;
                font-size:10px;
            }
            QLabel#previewBadge {
                color:#2367b1;
                background:#e8f1fb;
                border:1px solid #c9ddef;
                border-radius:9px;
                padding:5px 10px;
                font-size:9px;
                font-weight:800;
            }
            QTreeWidget#previewTree {
                background:#ffffff;
                color:#202735;
                border:1px solid #dce1ea;
                border-radius:10px;
                outline:none;
                font-size:10px;
                padding:4px;
            }
            QTreeWidget#previewTree::item {
                min-height:38px;
                border-bottom:1px solid #edf0f5;
                padding:4px 6px;
            }
            QTreeWidget#previewTree::item:hover {
                background:#f4f7fb;
            }
            QTreeWidget#previewTree QHeaderView::section {
                background:#f6f8fb;
                color:#627087;
                border:none;
                border-bottom:1px solid #dce1ea;
                padding:9px 8px;
                font-size:9px;
                font-weight:800;
            }
            QTreeWidget#previewTree::indicator {
                width:17px;
                height:17px;
                border-radius:5px;
                border:1px solid #b8c1ce;
                background:#ffffff;
            }
            QTreeWidget#previewTree::indicator:hover {
                border-color:#2367b1;
            }
            QTreeWidget#previewTree::indicator:checked {
                background:#2367b1;
                border-color:#2367b1;
            }
            QPushButton#previewGhost {
                background:#eef2f7;
                color:#435069;
                border:1px solid #d3dbe6;
                border-radius:7px;
                padding:7px 12px;
                font-size:9px;
                font-weight:700;
            }
            QPushButton#previewGhost:hover {
                background:#e3e9f1;
            }
            QPushButton#previewCancel {
                background:#e9edf3;
                color:#303a4c;
                border:1px solid #d2d9e3;
                border-radius:8px;
                padding:9px 18px;
                font-weight:700;
            }
            QPushButton#previewExecute {
                background:#2367b1;
                color:#ffffff;
                border:none;
                border-radius:8px;
                padding:9px 20px;
                font-weight:800;
            }
            QPushButton#previewExecute:hover {
                background:#2f78c8;
            }
            QPushButton#previewExecute:disabled {
                background:#cbd2dc;
                color:#8b96a8;
            }
        )");

    dialog.setStyleSheet(previewStyle);


    // ======================================================
    // LAYOUT + HEADER
    // ======================================================

    auto *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(24, 22, 24, 20);
    mainLayout->setSpacing(12);

    auto *headerRow = new QHBoxLayout();

    auto *title = new QLabel("Review Changes");
    title->setObjectName("previewTitle");

    auto *countBadge = new QLabel(
        QString("%1 FILES").arg(moves.size())
    );
    countBadge->setObjectName("previewBadge");
    countBadge->setAlignment(Qt::AlignCenter);

    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(countBadge);

    auto *subtitle = new QLabel();
    subtitle->setObjectName("previewSubtitle");

    subtitle->setText(
        renamedCount > 0
            ? QString(
                "Review destinations before execution • "
                "%1 name conflict(s) will be safely renamed."
              ).arg(renamedCount)
            : QStringLiteral(
                "Review destinations before execution. "
                "Uncheck anything you want to exclude."
              )
    );

    mainLayout->addLayout(headerRow);
    mainLayout->addWidget(subtitle);


    // ======================================================
    // SELECTION TOOLBAR
    // ======================================================

    auto *selectionRow = new QHBoxLayout();
    selectionRow->setSpacing(8);

    auto *selectionLabel = new QLabel();
    selectionLabel->setObjectName("previewSelection");

    auto *selectAllButton = new QPushButton("Select All");
    selectAllButton->setObjectName("previewGhost");
    selectAllButton->setCursor(Qt::PointingHandCursor);

    auto *clearButton = new QPushButton("Clear");
    clearButton->setObjectName("previewGhost");
    clearButton->setCursor(Qt::PointingHandCursor);

    selectionRow->addWidget(selectionLabel);
    selectionRow->addStretch();
    selectionRow->addWidget(selectAllButton);
    selectionRow->addWidget(clearButton);

    mainLayout->addLayout(selectionRow);


    // ======================================================
    // TABLE PREVIEW
    // ======================================================

    auto *tree = new QTreeWidget();
    tree->setObjectName("previewTree");
    tree->setColumnCount(3);
    tree->setHeaderLabels({"FILE", "DESTINATION", "METHOD"});
    tree->setRootIsDecorated(false);
    tree->setUniformRowHeights(true);
    tree->setAnimated(false);
    tree->setIndentation(0);
    tree->setSelectionMode(QAbstractItemView::NoSelection);
    tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    for (int i = 0; i < moves.size(); ++i)
    {
        const PlannedMove &move = moves.at(i);
        const QFileInfo sourceInfo(move.sourcePath);
        const QFileInfo targetInfo(move.targetPath);

        const bool renamed =
            sourceInfo.fileName() != targetInfo.fileName();

        auto *item = new QTreeWidgetItem(tree);

        item->setFlags(
            item->flags()
            | Qt::ItemIsUserCheckable
            | Qt::ItemIsEnabled
        );

        item->setCheckState(0, Qt::Checked);
        item->setText(0, sourceInfo.fileName());

        QString destination = QString("%1  /  %2")
            .arg(move.category)
            .arg(targetInfo.fileName());

        if (renamed)
        {
            destination += "   •   renamed";
        }

        item->setText(1, destination);
        item->setText(2, move.methodText());
        item->setTextAlignment(2, Qt::AlignCenter);
        item->setData(0, Qt::UserRole, i);

        item->setToolTip(0, move.sourcePath);
        item->setToolTip(1, move.targetPath);
        item->setToolTip(
            2,
            QString("Classification method: %1")
                .arg(move.methodText())
        );
    }

    mainLayout->addWidget(tree, 1);


    // ======================================================
    // FIXED FOOTER
    // ======================================================

    auto *footerRow = new QHBoxLayout();
    footerRow->setSpacing(10);

    auto *cancelButton = new QPushButton("Cancel");
    cancelButton->setObjectName("previewCancel");
    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setMinimumSize(110, 40);

    auto *confirmButton = new QPushButton();
    confirmButton->setObjectName("previewExecute");
    confirmButton->setCursor(Qt::PointingHandCursor);
    confirmButton->setMinimumSize(170, 40);
    confirmButton->setDefault(true);

    footerRow->addStretch();
    footerRow->addWidget(cancelButton);
    footerRow->addWidget(confirmButton);

    mainLayout->addLayout(footerRow);


    // ======================================================
    // LIVE SELECTION STATE
    // ======================================================

    const auto refreshSelection =
        [tree, selectionLabel, confirmButton]()
        {
            int selectedCount = 0;
            const int totalCount = tree->topLevelItemCount();

            for (int i = 0; i < totalCount; ++i)
            {
                if (
                    tree->topLevelItem(i)->checkState(0)
                    == Qt::Checked
                )
                {
                    ++selectedCount;
                }
            }

            selectionLabel->setText(
                QString("%1 of %2 selected")
                    .arg(selectedCount)
                    .arg(totalCount)
            );

            confirmButton->setEnabled(selectedCount > 0);

            confirmButton->setText(
                QString("Execute %1 Move%2")
                    .arg(selectedCount)
                    .arg(selectedCount == 1 ? "" : "s")
            );
        };

    connect(
        tree,
        &QTreeWidget::itemChanged,
        &dialog,
        [refreshSelection](QTreeWidgetItem *, int)
        {
            refreshSelection();
        }
    );

    connect(
        selectAllButton,
        &QPushButton::clicked,
        &dialog,
        [tree, refreshSelection]()
        {
            const QSignalBlocker blocker(tree);

            for (int i = 0; i < tree->topLevelItemCount(); ++i)
            {
                tree->topLevelItem(i)->setCheckState(0, Qt::Checked);
            }

            refreshSelection();
        }
    );

    connect(
        clearButton,
        &QPushButton::clicked,
        &dialog,
        [tree, refreshSelection]()
        {
            const QSignalBlocker blocker(tree);

            for (int i = 0; i < tree->topLevelItemCount(); ++i)
            {
                tree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
            }

            refreshSelection();
        }
    );

    connect(
        cancelButton,
        &QPushButton::clicked,
        &dialog,
        &QDialog::reject
    );

    connect(
        confirmButton,
        &QPushButton::clicked,
        &dialog,
        &QDialog::accept
    );

    refreshSelection();


    // ======================================================
    // SHOW
    // ======================================================

    if (dialog.exec() != QDialog::Accepted)
    {
        statusLabel->setText("●  Scan cancelled");
        return;
    }


    // ======================================================
    // SELECTED MOVES
    // ======================================================

    QList<int> selectedIndices;
    selectedIndices.reserve(tree->topLevelItemCount());

    for (int i = 0; i < tree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = tree->topLevelItem(i);

        if (item->checkState(0) == Qt::Checked)
        {
            selectedIndices.append(
                item->data(0, Qt::UserRole).toInt()
            );
        }
    }

    if (selectedIndices.isEmpty())
    {
        statusLabel->setText("●  Nothing selected");
        return;
    }

    statusLabel->setText(
        QString("●  Organizing %1 file(s)...")
            .arg(selectedIndices.size())
    );


    // ======================================================
    // EXECUTE
    // ======================================================

    if (!organizer->executeSelectedMoves(selectedIndices))
    {
        QMessageBox::warning(
            this,
            "PyClean",
            "No files could be moved.\n\n"
            + organizer->errorMessage()
        );

        statusLabel->setText("●  Move failed");
        return;
    }


    // ======================================================
    // COMPLETE
    // ======================================================

    QMessageBox::information(
        this,
        "File Organized!",
        QString(
            "File Organized!\n\n"
            "Moved: %1\n"
            "Failed: %2"
        )
        .arg(organizer->movedCount())
        .arg(organizer->failedCount())
    );

    statusLabel->setText(
        QString("●  Moved %1 file(s)")
            .arg(organizer->movedCount())
    );
}
