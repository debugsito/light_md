#pragma once

#include <QString>

// Local-file document identity and UTF-8 load/save.
// The UI keeps the live text (editor undo/cursor); this class owns only the
// path and the durable I/O so desktop and a future mobile UI can share it.
class Document
{
public:
    QString path() const { return m_path; }
    QString fileName() const;
    QString lastError() const { return m_lastError; }
    bool hasPath() const { return !m_path.isEmpty(); }

    void clear();
    void setPath(const QString &path);

    // On failure: returns false, sets lastError, leaves *text and path unchanged.
    bool load(const QString &path, QString *text);
    // On failure: returns false, sets lastError, leaves path unchanged.
    // On success: writes atomically (QSaveFile) and updates path.
    bool save(const QString &path, const QString &text);

    QString suggestedPath(const QString &folder, const QString &extension) const;

    static int wordCount(const QString &text);

private:
    QString m_path;
    mutable QString m_lastError;
};
