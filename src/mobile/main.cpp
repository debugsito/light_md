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

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    DocumentController document;
    const QStringList args = QGuiApplication::arguments();
    if (args.size() > 1)
        document.load(args.at(1));
    else {
        document.setText(QStringLiteral(
            "# light_md mobile\n\n"
            "Reader first. Tap **Edit** to change the text.\n\n"
            "- Open and Save use the system file dialog\n"
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
