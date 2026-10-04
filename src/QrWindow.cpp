#include "QrWindow.h"

#include "qrcodegen/qrcodegen.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QSizePolicy>

QrWindow::QrWindow(
    const QString &serverUrl,
    QWidget *parent
)
    : QDialog(parent),
      serverUrl(serverUrl)
{
    setWindowTitle("PyClean Mobile");
    setFixedSize(360, 470);

    setStyleSheet(
        "QDialog {"
        "    background-color: #1e1e2e;"
        "    color: #cdd6f4;"
        "}"
        "QLabel {"
        "    color: #cdd6f4;"
        "}"
    );

    auto *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        20,
        20,
        20,
        20
    );

    layout->setSpacing(12);

    // ==================================================
    // TITLE
    // ==================================================

    auto *titleLabel =
        new QLabel(
            "PyClean Mobile",
            this
        );

    titleLabel->setAlignment(
        Qt::AlignCenter
    );

    titleLabel->setFont(
        QFont(
            "Segoe UI",
            16,
            QFont::Bold
        )
    );

    layout->addWidget(titleLabel);

    // ==================================================
    // SUBTITLE
    // ==================================================

    auto *subtitleLabel =
        new QLabel(
            "Scan this QR code with PyClean Remote",
            this
        );

    subtitleLabel->setAlignment(
        Qt::AlignCenter
    );

    subtitleLabel->setFont(
        QFont(
            "Segoe UI",
            9
        )
    );

    layout->addWidget(subtitleLabel);

    // ==================================================
    // QR CODE
    // ==================================================

    qrLabel =
        new QLabel(this);

    qrLabel->setAlignment(
        Qt::AlignCenter
    );

    qrLabel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );

    layout->addWidget(
        qrLabel,
        1
    );

    // ==================================================
    // SERVER URL
    // ==================================================

    urlLabel =
        new QLabel(
            serverUrl,
            this
        );

    urlLabel->setAlignment(
        Qt::AlignCenter
    );

    urlLabel->setWordWrap(true);

    urlLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse
    );

    urlLabel->setFont(
        QFont(
            "Consolas",
            10
        )
    );

    urlLabel->setStyleSheet(
        "QLabel {"
        "    color: #89b4fa;"
        "    padding: 8px;"
        "}"
    );

    layout->addWidget(
        urlLabel
    );

    // ==================================================
    // GENERATE QR
    // ==================================================

    const QByteArray utf8Url =
        serverUrl.toUtf8();

    const qrcodegen::QrCode qr =
        qrcodegen::QrCode::encodeText(
            utf8Url.constData(),
            qrcodegen::QrCode::Ecc::MEDIUM
        );

    const int qrSize =
        qr.getSize();

    // Keep a quiet zone around the QR code.
    const int border = 4;

    const int scale = 6;

    const int imageSize =
        (qrSize + border * 2) * scale;

    QImage image(
        imageSize,
        imageSize,
        QImage::Format_RGB32
    );

    image.fill(Qt::white);

    QPainter painter(&image);

    painter.setPen(Qt::NoPen);

    painter.setBrush(Qt::black);

    for (int y = 0; y < qrSize; ++y)
    {
        for (int x = 0; x < qrSize; ++x)
        {
            if (qr.getModule(x, y))
            {
                painter.drawRect(
                    (x + border) * scale,
                    (y + border) * scale,
                    scale,
                    scale
                );
            }
        }
    }

    painter.end();

    qrLabel->setPixmap(
        QPixmap::fromImage(image)
    );
}