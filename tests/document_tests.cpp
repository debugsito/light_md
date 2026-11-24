#include "core/document.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class DocumentTests : public QObject
{
    Q_OBJECT

private slots:
    void unicodeRoundTrip();
    void wordCountWhitespace();
    void failedOpenPreservesDocument();
    void failedSaveAsPreservesIdentityAndCallerText();
    void overwriteExistingWithShorterText();
};

void DocumentTests::unicodeRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("café.md"));
    const QString original = QStringLiteral("# Título\n\nHola — 日本語 👋\n");

    Document doc;
    QVERIFY2(doc.save(path, original), qPrintable(doc.lastError()));
    QCOMPARE(doc.path(), path);

    Document loaded;
    QString text;
    QVERIFY2(loaded.load(path, &text), qPrintable(loaded.lastError()));
    QCOMPARE(text, original);
    QCOMPARE(loaded.fileName(), QStringLiteral("café.md"));
}

void DocumentTests::wordCountWhitespace()
{
    QCOMPARE(Document::wordCount(QString()), 0);
    QCOMPARE(Document::wordCount(QStringLiteral("   \n\t  ")), 0);
    QCOMPARE(Document::wordCount(QStringLiteral("one")), 1);
    QCOMPARE(Document::wordCount(QStringLiteral("  one  two\nthree  ")), 3);
}

void DocumentTests::failedOpenPreservesDocument()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString goodPath = dir.filePath(QStringLiteral("ok.md"));
    const QString body = QStringLiteral("keep me");

    Document doc;
    QVERIFY(doc.save(goodPath, body));

    QString text = QStringLiteral("caller buffer");
    QVERIFY(!doc.load(dir.filePath(QStringLiteral("missing.md")), &text));
    QVERIFY(!doc.lastError().isEmpty());
    QCOMPARE(doc.path(), goodPath);
    QCOMPARE(text, QStringLiteral("caller buffer"));
}

void DocumentTests::failedSaveAsPreservesIdentityAndCallerText()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString goodPath = dir.filePath(QStringLiteral("ok.md"));

    Document doc;
    QVERIFY(doc.save(goodPath, QStringLiteral("original")));

    // A path whose parent directory does not exist should fail without clobbering identity.
    const QString badPath = dir.filePath(QStringLiteral("no_such_dir/out.md"));
    const QString draft = QStringLiteral("unsaved draft text");
    QVERIFY(!doc.save(badPath, draft));
    QVERIFY(!doc.lastError().isEmpty());
    QCOMPARE(doc.path(), goodPath);

    QString reloaded;
    QVERIFY(doc.load(goodPath, &reloaded));
    QCOMPARE(reloaded, QStringLiteral("original"));
}

void DocumentTests::overwriteExistingWithShorterText()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("rewrite.md"));

    Document doc;
    QVERIFY(doc.save(path, QStringLiteral("# long original content that must shrink\n")));
    QVERIFY(doc.save(path, QStringLiteral("# short\n")));

    QString text;
    QVERIFY(doc.load(path, &text));
    QCOMPARE(text, QStringLiteral("# short\n"));
}

QTEST_MAIN(DocumentTests)
#include "document_tests.moc"
