#include "document.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

void Document::clear()
{
    m_path.clear();
    m_lastError.clear();
}

void Document::setPath(const QString &path)
{
    m_path = path;
    m_lastError.clear();
}

QString Document::fileName() const
{
    if (m_path.isEmpty())
        return QStringLiteral("New document");
    return QFileInfo(m_path).fileName();
}

bool Document::load(const QString &path, QString *text)
{
    if (!text) {
        m_lastError = QStringLiteral("Internal error: missing text buffer.");
        return false;
    }

    QFile file(path);
    // Text mode keeps Windows CRLF ↔ LF conversion like the previous editor I/O.
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Could not read the file:\n%1").arg(path);
        return false;
    }

    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        m_lastError = QStringLiteral("Could not read the file:\n%1").arg(path);
        return false;
    }

    *text = QString::fromUtf8(bytes);
    m_path = path;
    m_lastError.clear();
    return true;
}

bool Document::save(const QString &path, const QString &text)
{
    QSaveFile file(path);
    // Text mode preserves prior Windows CRLF output; QSaveFile still commits atomically.
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Could not write to the file:\n%1").arg(path);
        return false;
    }

    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        m_lastError = QStringLiteral("Could not write to the file:\n%1").arg(path);
        return false;
    }

    if (!file.commit()) {
        m_lastError = QStringLiteral("Could not finish saving:\n%1").arg(path);
        return false;
    }

    m_path = path;
    m_lastError.clear();
    return true;
}

QString Document::suggestedPath(const QString &folder, const QString &extension) const
{
    const QString base = m_path.isEmpty() ? QStringLiteral("document")
                                          : QFileInfo(m_path).completeBaseName();
    if (folder.isEmpty())
        return base + extension;
    return QDir(folder).filePath(base + extension);
}

int Document::wordCount(const QString &text)
{
    static const QRegularExpression spaces(QStringLiteral("\\s+"));
    return text.split(spaces, Qt::SkipEmptyParts).size();
}
