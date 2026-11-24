#include "document_controller.h"

#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("light_md"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.1.0"));
    QGuiApplication::setOrganizationName(QStringLiteral("light_md"));
    QGuiApplication::setDesktopFileName(QStringLiteral("light_md"));

    // Material reads well on phones and follows the system light/dark setting.
    QQuickStyle::setStyle(QStringLiteral("Material"));

    DocumentController document;

    // Android VIEW intents arrive as content:// (and sometimes file://) URLs.
    // Register before loading QML so cold-start opens are queued, not dropped.
    QDesktopServices::setUrlHandler(QStringLiteral("content"), &document, "handleUrl");
    QDesktopServices::setUrlHandler(QStringLiteral("file"), &document, "handleUrl");

    bool opened = false;
    const QStringList args = QGuiApplication::arguments();
    if (args.size() > 1) {
        const QUrl argUrl = QUrl::fromUserInput(args.at(1), QDir::currentPath());
        if (argUrl.isLocalFile()) {
            if (!document.load(argUrl.toLocalFile()))
                qWarning("Could not open %s: %s", qPrintable(args.at(1)),
                         qPrintable(document.lastError()));
        } else {
            document.handleUrl(argUrl);
        }
        opened = true;
    }

    if (!opened) {
        document.setText(QStringLiteral(
            "# light_md mobile\n\n"
            "Reader first. Tap **Edit** to change the text.\n\n"
            "- **Open** / **Save** use the system file picker\n"
            "- On Android, open a `.md` from Files with **Open with → light_md**\n"
            "- Provider documents open read-only in this build\n"
            "- **Esc** or **Read** returns to the formatted page\n"));
        document.setDirty(false);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("document"), &document);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/light_md_mobile/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        QDesktopServices::unsetUrlHandler(QStringLiteral("content"));
        QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
        return 1;
    }

    const int code = app.exec();
    QDesktopServices::unsetUrlHandler(QStringLiteral("content"));
    QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
    return code;
}
