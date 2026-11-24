#include "document_controller.h"

#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QThread>
#include <QUrlQuery>

DocumentController::DocumentController(QObject *parent)
    : QObject(parent)
{
}

void DocumentController::setText(const QString &text)
{
    if (m_readOnly)
        return;
    if (m_text == text)
        return;
    m_text = text;
    if (!m_dirty) {
        m_dirty = true;
        emit dirtyChanged();
    }
    emit textChanged();
}

void DocumentController::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;
    m_dirty = dirty;
    emit dirtyChanged();
}

void DocumentController::setLastError(const QString &error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

QString DocumentController::fileName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    return m_document.fileName();
}

QString DocumentController::displayNameForUrl(const QUrl &url)
{
    if (url.isLocalFile())
        return QFileInfo(url.toLocalFile()).fileName();

    const QString fileName = url.fileName();
    if (!fileName.isEmpty())
        return QUrl::fromPercentEncoding(fileName.toUtf8());

    // Some providers put the name in a query parameter.
    const QUrlQuery query(url);
    for (const char *key : {"displayName", "title", "name"}) {
        const QString value = query.queryItemValue(QString::fromLatin1(key));
        if (!value.isEmpty())
            return value;
    }
    return QStringLiteral("document.md");
}

bool DocumentController::adoptText(const QString &text, const QString &displayName, bool readOnly)
{
    m_text = text;
    m_displayName = displayName;
    m_readOnly = readOnly;
    m_dirty = false;
    setLastError(QString());
    emit identityChanged();
    emit dirtyChanged();
    emit textChanged();
    return true;
}

bool DocumentController::load(const QString &path)
{
    QString text;
    if (!m_document.load(path, &text)) {
        setLastError(m_document.lastError());
        return false;
    }
    m_displayName.clear();
    return adoptText(text, QString(), false);
}

bool DocumentController::loadUrl(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        setLastError(QStringLiteral("Missing document location."));
        return false;
    }

    if (url.isLocalFile())
        return load(url.toLocalFile());

    // Keep the URI fully encoded — Android permission grants match the exact string.
    const QString uri = url.toString(QUrl::FullyEncoded);
    QFile file(uri);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLastError(QStringLiteral("Could not open the document:\n%1").arg(uri));
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setLastError(QStringLiteral("Could not read the document:\n%1").arg(uri));
        return false;
    }

    // Provider VIEW grants are read-only; local Document path stays empty.
    m_document.clear();
    m_writableUrl = QUrl();
    return adoptText(QString::fromUtf8(bytes), displayNameForUrl(url), true);
}

bool DocumentController::save(const QString &path)
{
    if (m_readOnly && path.isEmpty() && !m_writableUrl.isValid()) {
        setLastError(QStringLiteral("This document is read-only. Use New or Open a local file to edit."));
        return false;
    }
    if (path.isEmpty() && m_document.path().isEmpty() && m_writableUrl.isValid()) {
        if (saveUrl(m_writableUrl))
            return true;
        // Grant revoked or provider gone — force Save As next.
        m_writableUrl = QUrl();
        emit identityChanged();
        setLastError(lastError() + QStringLiteral("\nChoose a new location with Save."));
        return false;
    }

    const QString target = path.isEmpty() ? m_document.path() : path;
    if (target.isEmpty()) {
        setLastError(QStringLiteral("Choose a file name before saving."));
        return false;
    }
    if (!m_document.save(target, m_text)) {
        setLastError(m_document.lastError());
        return false;
    }
    m_displayName.clear();
    m_writableUrl = QUrl();
    m_readOnly = false;
    m_dirty = false;
    setLastError(QString());
    emit identityChanged();
    emit dirtyChanged();
    return true;
}

bool DocumentController::saveUrl(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        setLastError(QStringLiteral("Choose a file name before saving."));
        return false;
    }
    if (url.isLocalFile())
        return save(url.toLocalFile());

    const QString uri = url.toString(QUrl::FullyEncoded);
    QFile file(uri);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setLastError(QStringLiteral("Could not write to the document:\n%1").arg(uri));
        return false;
    }
    const QByteArray bytes = m_text.toUtf8();
    if (file.write(bytes) != bytes.size() || !file.flush() || file.error() != QFileDevice::NoError) {
        setLastError(QStringLiteral("Could not write to the document:\n%1").arg(uri));
        return false;
    }
    // Android content:// "w" may keep trailing bytes when the new text is shorter.
    if (!file.resize(bytes.size()) || file.error() != QFileDevice::NoError) {
        setLastError(QStringLiteral("Could not finish saving:\n%1").arg(uri));
        return false;
    }
    file.close();
    if (file.error() != QFileDevice::NoError) {
        setLastError(QStringLiteral("Could not finish saving:\n%1").arg(uri));
        return false;
    }

    m_document.clear();
    m_writableUrl = url;
    m_displayName = displayNameForUrl(url);
    m_readOnly = false;
    m_dirty = false;
    setLastError(QString());
    emit identityChanged();
    emit dirtyChanged();
    return true;
}

void DocumentController::clear()
{
    m_document.clear();
    m_text.clear();
    m_displayName.clear();
    m_writableUrl = QUrl();
    m_readOnly = false;
    m_dirty = false;
    setLastError(QString());
    emit identityChanged();
    emit dirtyChanged();
    emit textChanged();
}

QString DocumentController::suggestedSavePath(const QString &folder) const
{
    return m_document.suggestedPath(folder, QStringLiteral(".md"));
}

QString DocumentController::localPathFromUrl(const QUrl &url) const
{
    return url.toLocalFile();
}

QString DocumentController::finalizeLocalSavePath(const QUrl &url) const
{
    QString path = url.toLocalFile();
    if (path.isEmpty())
        return path;
    // Match FileDialog defaultSuffix when the user typed a bare name.
    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".md");
    return path;
}

bool DocumentController::pathExists(const QString &path) const
{
    return !path.isEmpty() && QFileInfo::exists(path);
}

void DocumentController::setUiReady(bool ready)
{
    m_uiReady = ready;
    if (m_uiReady && m_pendingExternalUrl.isValid()) {
        const QUrl url = m_pendingExternalUrl;
        m_pendingExternalUrl = QUrl();
        emit externalOpenRequested(url);
    }
}

void DocumentController::handleUrl(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty())
        return;
    // Android may call the handler off the GUI thread; serialize on this object.
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, url]() { handleUrl(url); }, Qt::QueuedConnection);
        return;
    }
    if (!m_uiReady) {
        // Keep the latest pending URL; identical cold-start deliveries collapse here.
        m_pendingExternalUrl = url;
        return;
    }
    emit externalOpenRequested(url);
}
