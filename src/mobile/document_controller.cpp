#include "document_controller.h"

DocumentController::DocumentController(QObject *parent)
    : QObject(parent)
{
}

void DocumentController::setText(const QString &text)
{
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

bool DocumentController::load(const QString &path)
{
    QString text;
    if (!m_document.load(path, &text)) {
        setLastError(m_document.lastError());
        return false;
    }
    m_text = text;
    m_dirty = false;
    setLastError(QString());
    emit identityChanged();
    emit dirtyChanged();
    emit textChanged();
    return true;
}

bool DocumentController::save(const QString &path)
{
    const QString target = path.isEmpty() ? m_document.path() : path;
    if (target.isEmpty()) {
        setLastError(QStringLiteral("Choose a file name before saving."));
        return false;
    }
    if (!m_document.save(target, m_text)) {
        setLastError(m_document.lastError());
        return false;
    }
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
