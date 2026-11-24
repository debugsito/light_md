#pragma once

#include "core/document.h"

#include <QObject>
#include <QString>

// QML-facing document API for the mobile UI.
// On mobile the controller owns the working text; desktop keeps QPlainTextEdit
// as the authority and talks to Document directly.
class DocumentController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString path READ path NOTIFY identityChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY identityChanged)
    Q_PROPERTY(bool hasPath READ hasPath NOTIFY identityChanged)
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY textChanged)

public:
    explicit DocumentController(QObject *parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString path() const { return m_document.path(); }
    QString fileName() const { return m_document.fileName(); }
    bool hasPath() const { return m_document.hasPath(); }
    bool dirty() const { return m_dirty; }
    void setDirty(bool dirty);
    QString lastError() const { return m_lastError; }
    int wordCount() const { return Document::wordCount(m_text); }

    Q_INVOKABLE bool load(const QString &path);
    Q_INVOKABLE bool save(const QString &path = QString());
    Q_INVOKABLE void clear();
    Q_INVOKABLE QString suggestedSavePath(const QString &folder) const;

signals:
    void textChanged();
    void identityChanged();
    void dirtyChanged();
    void lastErrorChanged();

private:
    void setLastError(const QString &error);

    Document m_document;
    QString m_text;
    QString m_lastError;
    bool m_dirty = false;
};
