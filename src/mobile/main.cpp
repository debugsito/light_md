#include "document_controller.h"

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
    const QStringList args = QGuiApplication::arguments();
    if (args.size() > 1) {
        if (!document.load(args.at(1)))
            qWarning("Could not open %s: %s", qPrintable(args.at(1)), qPrintable(document.lastError()));
    } else {
        document.setText(QStringLiteral(
            "# light_md mobile\n\n"
            "Reader first. Tap **Edit** to change the text.\n\n"
            "- **Open** / **Save** use the system file picker\n"
            "- **Esc** or **Read** returns to the formatted page\n"
            "- Shared Document core with the desktop app\n"));
        document.setDirty(false);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("document"), &document);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/light_md_mobile/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
