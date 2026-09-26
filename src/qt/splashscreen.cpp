// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2018-2022 The BitBlocks developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "splashscreen.h"

#include "clientversion.h"
#include "init.h"
#include "networkstyle.h"
#include "ui_interface.h"
#include "util.h"
#include "version.h"

#ifdef ENABLE_WALLET
#include "wallet.h"
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>

#include <boost/bind/bind.hpp>
using namespace boost::placeholders;

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle* networkStyle) : QWidget(0, f), curAlignment(0)
{
    // Reference points for the wide, compact splash artwork.
    const int paddingLeft = 24;
    const int productBaseline = 340;
    const int copyrightBaseline = 404;

    // define text to place
    QString titleText = tr("BitBlocks Core");
    QString versionText = QString(tr("Version %1")).arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightTextBtc = QChar(0xA9) + QString(tr(" 2009-2014 The Bitcoin developers"));
    QString copyrightTextDash = QChar(0xA9) + QString(tr(" 2014-2015 The Dash developers"));
    QString copyrightTextPivx = QChar(0xA9) + QString(tr(" 2015-2017 The Pivx developers"));
    QString copyrightTextBbk = QChar(0xA9) + QString(" 2018-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The BitBlocks Core developers"));
    QString titleAddText = networkStyle->getTitleAddText();

    QString font = QApplication::font().family();

    // load the bitmap for writing some text over it
    pixmap = networkStyle->getSplashImage().scaled(QSize(768, 512), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(QColor(58, 94, 96));

    QFont productFont(font, 16);
    productFont.setWeight(QFont::DemiBold);
    pixPaint.setFont(productFont);
    pixPaint.drawText(paddingLeft, productBaseline, titleText);

    pixPaint.setFont(QFont(font, 11));
    pixPaint.drawText(paddingLeft, productBaseline + 20, versionText);

    // draw copyright stuff
    pixPaint.setFont(QFont(font, 8));
    pixPaint.drawText(paddingLeft, copyrightBaseline, copyrightTextBtc);
    pixPaint.drawText(paddingLeft, copyrightBaseline + 12, copyrightTextDash);
    pixPaint.drawText(paddingLeft, copyrightBaseline + 24, copyrightTextPivx);
    pixPaint.drawText(paddingLeft, copyrightBaseline + 36, copyrightTextBbk);

    // draw additional text if special network
    if (!titleAddText.isEmpty()) {
        QFont boldFont = QFont(font, 10);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        QFontMetrics fm = pixPaint.fontMetrics();
        int titleAddTextWidth = fm.width(titleAddText);
        pixPaint.drawText(pixmap.width() - titleAddTextWidth - 18, 28, titleAddText);
    }

    pixPaint.end();

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), pixmap.size());
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

void SplashScreen::slotFinish(QWidget* mainWin)
{
    Q_UNUSED(mainWin);
    hide();
}

static void InitMessage(SplashScreen* splash, const std::string& message)
{
    QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom | Qt::AlignHCenter),
        Q_ARG(QColor, QColor(0, 0, 0)));
}

static void ShowProgress(SplashScreen* splash, const std::string& title, int nProgress)
{
    InitMessage(splash, title + strprintf("%d", nProgress) + "%");
}

#ifdef ENABLE_WALLET
static void ConnectWallet(SplashScreen* splash, CWallet* wallet)
{
    wallet->ShowProgress.connect(boost::bind(ShowProgress, splash, _1, _2));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.connect(boost::bind(ConnectWallet, this, _1));
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.InitMessage.disconnect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    if (pwalletMain)
        pwalletMain->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#endif
}

void SplashScreen::showMessage(const QString& message, int alignment, const QColor& color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
    QRect r = rect().adjusted(5, 5, -5, -5);
    painter.setPen(curColor);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent* event)
{
    StartShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
