#include "core/document.h"
#include "theme.h"

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(ThemeManager *themes, const QString &initialFile = QString())
        : m_themes(themes)
    {
        resize(960, 640);

        auto *central = new QWidget(this);
        auto *root = new QVBoxLayout(central);
        root->setContentsMargins(8, 8, 8, 8);

        m_splitter = new QSplitter(Qt::Horizontal, central);
        m_editor = new QPlainTextEdit(m_splitter);
        m_editor->setPlaceholderText(QStringLiteral("Type your text here..."));
        m_editor->setUndoRedoEnabled(true);
        m_editor->viewport()->installEventFilter(this);
        m_preview = new QTextBrowser(m_splitter);
        // Never navigate the preview itself — long draw.io / https links would
        // blank the pane. Always hand the URL to the system browser/app.
        m_preview->setOpenLinks(false);
        connect(m_preview, &QTextBrowser::anchorClicked, this, [](const QUrl &url) {
            if (url.isValid())
                QDesktopServices::openUrl(url);
        });
        QFont readFont = m_preview->font();
        readFont.setPointSizeF(readFont.pointSizeF() + 2);
        m_preview->setFont(readFont);
        QFont writeFont = m_editor->font();
        writeFont.setPointSizeF(writeFont.pointSizeF() + 1);
        m_editor->setFont(writeFont);
        m_splitter->addWidget(m_editor);
        m_splitter->addWidget(m_preview);
        m_splitter->setStretchFactor(0, 1);
        m_splitter->setStretchFactor(1, 1);
        m_splitter->setSizes({480, 480});
        root->addWidget(m_splitter, 1);

        m_status = new QLabel(central);
        root->addWidget(m_status);

        setCentralWidget(central);
        setAcceptDrops(true);
        buildMenus();

        // Preview updates quickly; word count is cheaper on a slower timer.
        m_previewTimer.setSingleShot(true);
        m_previewTimer.setInterval(120);
        connect(&m_previewTimer, &QTimer::timeout, this, [this] { updatePreview(); });
        m_statusTimer.setSingleShot(true);
        m_statusTimer.setInterval(350);
        connect(&m_statusTimer, &QTimer::timeout, this, [this] { updateStatus(); });

        connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
            m_previewTimer.start();
            m_statusTimer.start();
        });
        connect(m_editor->document(), &QTextDocument::modificationChanged, this, [this](bool changed) {
            setWindowModified(changed);
            updateStatus();
        });

        QSettings settings;
        const QByteArray geometry = settings.value(QStringLiteral("geometry")).toByteArray();
        if (!geometry.isEmpty())
            restoreGeometry(geometry);

        if (!initialFile.isEmpty()) {
            loadFile(initialFile);
        } else {
            m_editor->setPlainText(QStringLiteral(
                "# Welcome to light_md\n\n"
                "Documents open in **Reader** mode so you can focus on the text.\n\n"
                "## Getting started\n\n"
                "- Open any `.md` file to read it\n"
                "- Press **Ctrl+E** (Options → Edit document) when you want to change it\n"
                "- Press **Ctrl+R** or **Esc** to return to Read only\n"
                "- Every action has a shortcut under Options\n"));
            m_editor->document()->setModified(false);
            updateTitle();
            updatePreview();
            setEditMode(false);
        }
    }

    bool loadFile(const QString &path)
    {
        QString text;
        // On failure Document keeps its previous path and leaves the editor alone.
        if (!m_document.load(path, &text)) {
            QMessageBox::warning(this, QStringLiteral("Could not open"), m_document.lastError());
            return false;
        }
        m_editor->setPlainText(text);
        m_editor->document()->setModified(false);
        rememberFolder(path);
        updateTitle();
        updateStatus();
        m_preview->verticalScrollBar()->setValue(0);
        updatePreview();
        setEditMode(false);
        return true;
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        if (!askSaveIfModified()) {
            event->ignore();
            return;
        }
        QSettings().setValue(QStringLiteral("geometry"), saveGeometry());
        event->accept();
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        // Esc returns to Reader without stealing Esc from dialogs (those are modal).
        if (event->key() == Qt::Key_Escape && m_editMode) {
            setEditMode(false);
            event->accept();
            return;
        }
        QMainWindow::keyPressEvent(event);
    }

    // Dropping a document on the window opens it instead of pasting its path.
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (droppedFile(event->mimeData()).isEmpty())
            return;
        event->acceptProposedAction();
    }

    void dropEvent(QDropEvent *event) override
    {
        const QString path = droppedFile(event->mimeData());
        if (path.isEmpty())
            return;
        event->acceptProposedAction();
        if (askSaveIfModified())
            loadFile(path);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_editor->viewport()) {
            if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
                auto *drag = static_cast<QDragMoveEvent *>(event);
                if (!droppedFile(drag->mimeData()).isEmpty()) {
                    drag->acceptProposedAction();
                    return true;
                }
            } else if (event->type() == QEvent::Drop) {
                auto *drop = static_cast<QDropEvent *>(event);
                if (!droppedFile(drop->mimeData()).isEmpty()) {
                    dropEvent(drop);
                    return true;
                }
            }
        }
        return QMainWindow::eventFilter(watched, event);
    }

private:
    QAction *addOption(QMenu *menu, const QString &text, const QString &tip, const QKeySequence &keys)
    {
        auto *action = menu->addAction(text);
        action->setStatusTip(tip);
        action->setToolTip(tip);
        action->setShortcutContext(Qt::ApplicationShortcut);
        if (!keys.isEmpty())
            action->setShortcut(keys);
        return action;
    }

    void buildMenus()
    {
        auto *options = menuBar()->addMenu(QStringLiteral("&Options"));

        auto *actNew = addOption(options, QStringLiteral("&New"), QStringLiteral("Start a blank document"),
                                 QKeySequence::New);
        auto *actOpen = addOption(options, QStringLiteral("&Open..."), QStringLiteral("Open a saved document"),
                                  QKeySequence::Open);
        auto *actSave = addOption(options, QStringLiteral("&Save"), QStringLiteral("Save your changes"),
                                  QKeySequence::Save);
        auto *actSaveAs = addOption(options, QStringLiteral("Save &as..."),
                                    QStringLiteral("Save a copy with a different name"),
                                    QKeySequence::SaveAs);
        options->addSeparator();
        auto *actPdf = addOption(options, QStringLiteral("Export to &PDF..."),
                                 QStringLiteral("Create a nicely formatted PDF to send or share"),
                                 QKeySequence(QStringLiteral("Ctrl+Shift+E")));
        auto *actPrint = addOption(options, QStringLiteral("&Print..."), QStringLiteral("Print the document"),
                                   QKeySequence::Print);
        options->addSeparator();

        m_editAction = addOption(options, QStringLiteral("&Edit document"),
                                 QStringLiteral("Show the writing pane"),
                                 QKeySequence(QStringLiteral("Ctrl+E")));
        m_editAction->setCheckable(true);

        auto *actRead = addOption(options, QStringLiteral("&Read only"),
                                  QStringLiteral("Hide the writing pane and just read"),
                                  QKeySequence(QStringLiteral("Ctrl+R")));

        options->addSeparator();
        m_formatMenu = options->addMenu(QStringLiteral("&Format"));
        auto *actUndo = addOption(m_formatMenu, QStringLiteral("&Undo"),
                                  QStringLiteral("Take back the last change"), QKeySequence::Undo);
        auto *actRedo = addOption(m_formatMenu, QStringLiteral("&Redo"),
                                  QStringLiteral("Bring back the change you undid"), QKeySequence::Redo);
        m_formatMenu->addSeparator();
        auto *actTitle = addOption(m_formatMenu, QStringLiteral("&Big title"),
                                   QStringLiteral("Turn the line into a big title"),
                                   QKeySequence(QStringLiteral("Ctrl+1")));
        auto *actSubtitle = addOption(m_formatMenu, QStringLiteral("&Subtitle"),
                                      QStringLiteral("Turn the line into a subtitle"),
                                      QKeySequence(QStringLiteral("Ctrl+2")));
        auto *actBold = addOption(m_formatMenu, QStringLiteral("B&old"),
                                  QStringLiteral("Make the selected text bold"), QKeySequence::Bold);
        auto *actItalic = addOption(m_formatMenu, QStringLiteral("&Italic"),
                                    QStringLiteral("Make the selected text italic"), QKeySequence::Italic);
        auto *actBullets = addOption(m_formatMenu, QStringLiteral("Bullet &list"),
                                     QStringLiteral("Put a bullet in front of each selected line"),
                                     QKeySequence(QStringLiteral("Ctrl+Shift+8")));
        auto *actNumbered = addOption(m_formatMenu, QStringLiteral("&Numbered list"),
                                      QStringLiteral("Number the selected lines: 1, 2, 3..."),
                                      QKeySequence(QStringLiteral("Ctrl+Shift+7")));
        auto *actQuote = addOption(m_formatMenu, QStringLiteral("&Quote"),
                                   QStringLiteral("Mark the lines as a quote"),
                                   QKeySequence(QStringLiteral("Ctrl+Shift+Q")));
        auto *actLink = addOption(m_formatMenu, QStringLiteral("Lin&k"),
                                  QStringLiteral("Add a link to a web page"),
                                  QKeySequence(QStringLiteral("Ctrl+K")));

        options->addSeparator();
        auto *themeMenu = options->addMenu(QStringLiteral("&Theme"));
        auto *themeGroup = new QActionGroup(this);
        themeGroup->setExclusive(true);
        struct ThemeShortcut {
            ThemeManager::Kind kind;
            const char *keys;
        };
        const ThemeShortcut themeShortcuts[] = {
            {ThemeManager::Kind::System, "Ctrl+Shift+1"},
            {ThemeManager::Kind::Minimal, "Ctrl+Shift+2"},
            {ThemeManager::Kind::Omarchy, "Ctrl+Shift+3"},
        };
        for (const ThemeShortcut &entry : themeShortcuts) {
            auto *action = addOption(themeMenu, ThemeManager::kindLabel(entry.kind),
                                     QStringLiteral("Use the %1 theme").arg(ThemeManager::kindLabel(entry.kind)),
                                     QKeySequence(QString::fromLatin1(entry.keys)));
            action->setCheckable(true);
            action->setData(ThemeManager::kindToString(entry.kind));
            themeGroup->addAction(action);
            if (entry.kind == m_themes->kind())
                action->setChecked(true);
            connect(action, &QAction::triggered, this, [this, action] {
                const auto kind = ThemeManager::kindFromString(action->data().toString());
                m_themes->setKind(kind);
                QSettings().setValue(QStringLiteral("theme"), ThemeManager::kindToString(kind));
            });
        }

        auto *actHelp = addOption(options, QStringLiteral("&Help"), QStringLiteral("How to use light_md"),
                                  QKeySequence::HelpContents);

        connect(actNew, &QAction::triggered, this, [this] { newDocument(); });
        connect(actOpen, &QAction::triggered, this, [this] { openDocument(); });
        connect(actSave, &QAction::triggered, this, [this] { saveDocument(); });
        connect(actSaveAs, &QAction::triggered, this, [this] { saveDocumentAs(); });
        connect(actPdf, &QAction::triggered, this, [this] { exportPdf(); });
        connect(actPrint, &QAction::triggered, this, [this] { printDocument(); });
        connect(actHelp, &QAction::triggered, this, [this] { showHelp(); });
        connect(m_editAction, &QAction::toggled, this, [this](bool on) { setEditMode(on); });
        connect(actRead, &QAction::triggered, this, [this] { setEditMode(false); });

        connect(actUndo, &QAction::triggered, m_editor, &QPlainTextEdit::undo);
        connect(actRedo, &QAction::triggered, m_editor, &QPlainTextEdit::redo);
        connect(m_editor, &QPlainTextEdit::undoAvailable, actUndo, &QAction::setEnabled);
        connect(m_editor, &QPlainTextEdit::redoAvailable, actRedo, &QAction::setEnabled);
        actUndo->setEnabled(false);
        actRedo->setEnabled(false);

        connect(actTitle, &QAction::triggered, this, [this] { setHeading(QStringLiteral("# ")); });
        connect(actSubtitle, &QAction::triggered, this, [this] { setHeading(QStringLiteral("## ")); });
        connect(actQuote, &QAction::triggered, this, [this] { prefixLines([](int) { return QStringLiteral("> "); }); });
        connect(actBullets, &QAction::triggered, this, [this] { prefixLines([](int) { return QStringLiteral("- "); }); });
        connect(actNumbered, &QAction::triggered, this,
                [this] { prefixLines([](int n) { return QStringLiteral("%1. ").arg(n); }); });
        connect(actBold, &QAction::triggered, this, [this] { wrapSelection(QStringLiteral("**")); });
        connect(actItalic, &QAction::triggered, this, [this] { wrapSelection(QStringLiteral("*")); });
        connect(actLink, &QAction::triggered, this, [this] { insertLink(); });
    }

    void setEditMode(bool editing)
    {
        if (m_editMode == editing && m_editor->isVisible() == editing) {
            if (m_editAction && m_editAction->isChecked() != editing) {
                QSignalBlocker blocker(m_editAction);
                m_editAction->setChecked(editing);
            }
            return;
        }
        m_editMode = editing;
        m_editor->setVisible(editing);
        if (m_formatMenu)
            m_formatMenu->setEnabled(editing);
        if (m_editAction && m_editAction->isChecked() != editing) {
            QSignalBlocker blocker(m_editAction);
            m_editAction->setChecked(editing);
        }
        if (editing) {
            if (m_splitter->sizes().value(0, 0) < 80)
                m_splitter->setSizes({480, 480});
            m_editor->setFocus(Qt::ShortcutFocusReason);
        } else {
            m_preview->setFocus(Qt::OtherFocusReason);
        }
        updateStatus();
    }

    static QString droppedFile(const QMimeData *mime)
    {
        if (!mime->hasUrls())
            return QString();
        for (const QUrl &url : mime->urls()) {
            if (url.isLocalFile())
                return url.toLocalFile();
        }
        return QString();
    }

    void updatePreview()
    {
        // Keep the reading position; otherwise the pane jumps to the top on every keystroke.
        QScrollBar *bar = m_preview->verticalScrollBar();
        const int scroll = bar->value();
        m_preview->setMarkdown(m_editor->toPlainText());
        bar->setValue(scroll);
    }

    void updateStatus()
    {
        const int words = Document::wordCount(m_editor->toPlainText());
        const bool modified = m_editor->document()->isModified();
        const QString mode = m_editMode ? QStringLiteral("Editing") : QStringLiteral("Reading");
        m_status->setText(QStringLiteral("%1    Words: %2    File: %3%4")
                              .arg(mode)
                              .arg(words)
                              .arg(m_document.hasPath() ? QDir::toNativeSeparators(m_document.path())
                                                        : QStringLiteral("(not saved)"))
                              .arg(modified ? QStringLiteral("    (unsaved changes)") : QString()));
    }

    void updateTitle()
    {
        setWindowTitle(QStringLiteral("%1[*] - light_md").arg(m_document.fileName()));
    }

    void rememberFolder(const QString &path)
    {
        if (!path.isEmpty())
            QSettings().setValue(QStringLiteral("lastFolder"), QFileInfo(path).absolutePath());
    }

    QString lastFolder() const
    {
        return QSettings().value(QStringLiteral("lastFolder")).toString();
    }

    template <typename Fn>
    void forEachSelectedLine(Fn fn)
    {
        if (!m_editMode)
            setEditMode(true);
        QTextCursor cursor = m_editor->textCursor();
        QTextDocument *doc = m_editor->document();
        QTextBlock first = doc->findBlock(cursor.selectionStart());
        QTextBlock last = doc->findBlock(cursor.selectionEnd());
        if (cursor.hasSelection() && last != first && cursor.selectionEnd() == last.position())
            last = last.previous();
        const bool single = first == last;

        cursor.beginEditBlock();
        for (QTextBlock block = first; block.isValid(); block = block.next()) {
            if (single || !block.text().trimmed().isEmpty())
                fn(block);
            if (block == last)
                break;
        }
        cursor.endEditBlock();
        m_editor->setFocus();
    }

    template <typename PrefixFn>
    void prefixLines(PrefixFn prefixFor)
    {
        int n = 0;
        forEachSelectedLine([&](const QTextBlock &block) {
            QTextCursor c(block);
            c.insertText(prefixFor(++n));
        });
    }

    void setHeading(const QString &marks)
    {
        static const QRegularExpression existing(QStringLiteral("^#{1,6}\\s*"));
        forEachSelectedLine([&](const QTextBlock &block) {
            QTextCursor c(block);
            const QRegularExpressionMatch match = existing.match(block.text());
            if (match.hasMatch())
                c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, match.capturedLength());
            c.insertText(marks);
        });
    }

    void wrapSelection(const QString &marks)
    {
        if (!m_editMode)
            setEditMode(true);
        QTextCursor cursor = m_editor->textCursor();
        cursor.beginEditBlock();
        if (cursor.hasSelection()) {
            const QString selected = cursor.selectedText();
            cursor.insertText(marks + selected + marks);
        } else {
            const QString sample = QStringLiteral("text");
            cursor.insertText(marks + sample + marks);
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, marks.size());
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, sample.size());
        }
        cursor.endEditBlock();
        m_editor->setTextCursor(cursor);
        m_editor->setFocus();
    }

    void insertLink()
    {
        if (!m_editMode)
            setEditMode(true);
        QTextCursor cursor = m_editor->textCursor();
        bool ok = false;
        QString address = QInputDialog::getText(this, QStringLiteral("Add link"),
                                                QStringLiteral("Web page address:"), QLineEdit::Normal,
                                                QStringLiteral("https://"), &ok)
                              .trimmed();
        if (!ok || address.isEmpty() || address == QStringLiteral("https://")) {
            m_editor->setFocus();
            return;
        }
        if (!address.contains(QStringLiteral("://")) && !address.startsWith(QStringLiteral("mailto:")))
            address.prepend(QStringLiteral("https://"));

        const bool hadSelection = cursor.hasSelection();
        const QString label = hadSelection ? cursor.selectedText() : QStringLiteral("link text");
        cursor.beginEditBlock();
        cursor.insertText(QStringLiteral("[%1](%2)").arg(label, address));
        cursor.endEditBlock();
        if (!hadSelection) {
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, address.size() + 3);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, label.size());
        }
        m_editor->setTextCursor(cursor);
        m_editor->setFocus();
    }

    bool askSaveIfModified()
    {
        if (!m_editor->document()->isModified())
            return true;
        QMessageBox box(QMessageBox::Question, QStringLiteral("Save changes"),
                        QStringLiteral("You have unsaved changes. Do you want to save them?"),
                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
        box.button(QMessageBox::Save)->setText(QStringLiteral("Save"));
        box.button(QMessageBox::Discard)->setText(QStringLiteral("Don't save"));
        box.button(QMessageBox::Cancel)->setText(QStringLiteral("Cancel"));
        box.setDefaultButton(QMessageBox::Save);
        const int answer = box.exec();
        if (answer == QMessageBox::Cancel)
            return false;
        if (answer == QMessageBox::Save)
            return saveDocument();
        return true;
    }

    void newDocument()
    {
        if (!askSaveIfModified())
            return;
        m_editor->clear();
        m_editor->document()->setModified(false);
        m_document.clear();
        updateTitle();
        updateStatus();
        updatePreview();
        setEditMode(true);
    }

    void openDocument()
    {
        if (!askSaveIfModified())
            return;
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Open document"), lastFolder(),
            QStringLiteral("Markdown documents (*.md *.markdown *.txt);;All files (*)"));
        if (path.isEmpty())
            return;
        loadFile(path);
    }

    bool writeTo(const QString &path)
    {
        if (!m_document.save(path, m_editor->toPlainText())) {
            QMessageBox::warning(this, QStringLiteral("Could not save"), m_document.lastError());
            return false;
        }
        m_editor->document()->setModified(false);
        rememberFolder(path);
        updateTitle();
        updateStatus();
        return true;
    }

    bool saveDocument()
    {
        if (!m_document.hasPath())
            return saveDocumentAs();
        return writeTo(m_document.path());
    }

    QString suggestedPath(const QString &extension) const
    {
        const QString folder =
            m_document.hasPath() ? QFileInfo(m_document.path()).absolutePath() : lastFolder();
        return m_document.suggestedPath(folder, extension);
    }

    bool saveDocumentAs()
    {
        QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Save as"), suggestedPath(QStringLiteral(".md")),
            QStringLiteral("Markdown documents (*.md);;All files (*)"));
        if (path.isEmpty())
            return false;
        if (QFileInfo(path).suffix().isEmpty())
            path += QStringLiteral(".md");
        return writeTo(path);
    }

    void exportPdf()
    {
        QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export to PDF"),
                                                    suggestedPath(QStringLiteral(".pdf")),
                                                    QStringLiteral("PDF document (*.pdf)"));
        if (path.isEmpty())
            return;
        if (QFileInfo(path).suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) != 0)
            path += QStringLiteral(".pdf");
        updatePreview();
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(path);
        printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
        m_preview->document()->print(&printer);
        if (!QFileInfo::exists(path)) {
            QMessageBox::warning(this, QStringLiteral("Could not export"),
                                 QStringLiteral("Could not create the PDF:\n%1").arg(path));
            return;
        }
        m_status->setText(QStringLiteral("PDF created: %1").arg(QDir::toNativeSeparators(path)));
    }

    void printDocument()
    {
        updatePreview();
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dialog(&printer, this);
        dialog.setWindowTitle(QStringLiteral("Print"));
        if (dialog.exec() == QDialog::Accepted)
            m_preview->document()->print(&printer);
    }

    void showHelp()
    {
        QMessageBox::information(
            this, QStringLiteral("How to use light_md"),
            QStringLiteral(
                "<p>Documents open in <b>Reader</b> mode. Use <b>Options</b> for every action.</p>"
                "<p><b>View</b><br>"
                "Ctrl+E — Edit document<br>"
                "Ctrl+R or Esc — Read only</p>"
                "<p><b>File</b><br>"
                "Ctrl+N New · Ctrl+O Open · Ctrl+S Save · Ctrl+Shift+S Save as<br>"
                "Ctrl+Shift+E Export PDF · Ctrl+P Print</p>"
                "<p><b>Format</b> (while editing)<br>"
                "Ctrl+Z Undo · Ctrl+Shift+Z / Ctrl+Y Redo<br>"
                "Ctrl+B Bold · Ctrl+I Italic · Ctrl+K Link<br>"
                "Ctrl+1 Big title · Ctrl+2 Subtitle<br>"
                "Ctrl+Shift+8 Bullet list · Ctrl+Shift+7 Numbered list · Ctrl+Shift+Q Quote</p>"
                "<p><b>Theme</b><br>"
                "Ctrl+Shift+1 System · Ctrl+Shift+2 Minimal · Ctrl+Shift+3 Omarchy</p>"
                "<p>F1 — Help. You can also drag a document onto the window to open it.</p>"));
    }

    ThemeManager *m_themes = nullptr;
    Document m_document;
    QSplitter *m_splitter = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QTextBrowser *m_preview = nullptr;
    QLabel *m_status = nullptr;
    QAction *m_editAction = nullptr;
    QMenu *m_formatMenu = nullptr;
    QTimer m_previewTimer;
    QTimer m_statusTimer;
    bool m_editMode = true;
};

static ThemeManager::Kind loadInitialTheme()
{
    const QSettings settings;
    if (settings.contains(QStringLiteral("theme")))
        return ThemeManager::kindFromString(settings.value(QStringLiteral("theme")).toString());
    // First run on Omarchy: follow the desktop theme. Elsewhere keep System
    // so Windows/macOS behavior stays unchanged.
    if (ThemeManager::omarchyAvailable())
        return ThemeManager::Kind::Omarchy;
    return ThemeManager::Kind::System;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("light_md"));
    QApplication::setApplicationVersion(QStringLiteral("1.1.0"));
    QApplication::setOrganizationName(QStringLiteral("light_md"));
    // Matches light_md.desktop so Wayland/Omarchy pick up the launcher icon.
    QApplication::setDesktopFileName(QStringLiteral("light_md"));

    ThemeManager themes;
    themes.setKind(loadInitialTheme());

    const QStringList args = QApplication::arguments();
    const QString initialFile = args.size() > 1 ? args.at(1) : QString();
    MainWindow window(&themes, initialFile);
    window.show();
    return app.exec();
}
