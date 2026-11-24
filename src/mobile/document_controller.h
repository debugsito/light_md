#pragma once

#include "core/document.h"

#include <QObject>
#include <QString>
#include <QUrl>

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
    Q_PROPERTY(bool hasSaveTarget READ hasSaveTarget NOTIFY identityChanged)
    Q_PROPERTY(bool readOnly READ readOnly NOTIFY identityChanged)
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY textChanged)

public:
    explicit DocumentController(QObject *parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString path() const { return m_document.path(); }
    QString fileName() const;
    bool hasPath() const { return m_document.hasPath(); }
    bool hasSaveTarget() const { return m_document.hasPath() || m_writableUrl.isValid(); }
    bool readOnly() const { return m_readOnly; }
    bool dirty() const { return m_dirty; }
    void setDirty(bool dirty);
    QString lastError() const { return m_lastError; }
    int wordCount() const { return Document::wordCount(m_text); }

    Q_INVOKABLE bool load(const QString &path);
    Q_INVOKABLE bool loadUrl(const QUrl &url);
    Q_INVOKABLE bool save(const QString &path = QString());
    Q_INVOKABLE bool saveUrl(const QUrl &url);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QString suggestedSavePath(const QString &folder) const;
    // QML FileDialog urls do not expose toLocalFile(); resolve in C++.
    Q_INVOKABLE QString localPathFromUrl(const QUrl &url) const;
    Q_INVOKABLE QString finalizeLocalSavePath(const QUrl &url) const;
    Q_INVOKABLE bool pathExists(const QString &path) const;

    // Android VIEW intents / desktop URL opens. Queues until the UI is ready.
    Q_INVOKABLE void setUiReady(bool ready);

public slots:
    void handleUrl(const QUrl &url);

signals:
    void textChanged();
    void identityChanged();
    void dirtyChanged();
    void lastErrorChanged();
    void externalOpenRequested(const QUrl &url);

private:
    void setLastError(const QString &error);
    bool adoptText(const QString &text, const QString &displayName, bool readOnly);
    static QString displayNameForUrl(const QUrl &url);

    Document m_document;
    QString m_text;
    QString m_displayName;
    QString m_lastError;
    QUrl m_pendingExternalUrl;
    QUrl m_writableUrl; // SAF content:// create/write target (Android)
    bool m_dirty = false;
    bool m_readOnly = false;
    bool m_uiReady = false;
};
