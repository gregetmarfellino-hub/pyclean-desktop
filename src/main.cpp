#include <QApplication>
#include <QDebug>
#include <QDir>

#include "Database.h"
#include "MainWindow.h"
#include "MobileServer.h"

int main(
    int argc,
    char *argv[]
)
{
    QApplication app(
        argc,
        argv
    );

    // ---------------------------------------------------------
    // Application database
    // ---------------------------------------------------------

    const QString databasePath =
        QDir(
            QCoreApplication::
                applicationDirPath()
        ).filePath(
            "pyclean_logs.db"
        );

    Database database(
        databasePath
    );

    if (!database.initialize())
    {
        qWarning()
            << "Database initialization failed:"
            << database.errorMessage();
    }


    // ---------------------------------------------------------
    // Mobile HTTP server
    // ---------------------------------------------------------
    //
    // Do NOT auto-start here.
    // The desktop "Launch Agent" button now owns the same
    // start/stop lifecycle that the original Python app had.
    //

    MobileServer mobileServer(
        nullptr,
        &database
    );


    // ---------------------------------------------------------
    // Desktop GUI
    // ---------------------------------------------------------

    MainWindow window(
        &mobileServer
    );

    window.show();

    return app.exec();
}
