#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTextBrowser>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow
{
public:
    MainWindow()
    {
        setWindowTitle(QStringLiteral("light_md - Editor de Markdown sencillo"));
        resize(1100, 700);

        auto *central = new QWidget(this);
        auto *root = new QVBoxLayout(central);

        // Barra de archivo: palabras de todos los dias, sin terminos tecnicos.
        auto *fileBar = new QHBoxLayout;
        auto *btnNew = new QPushButton(QStringLiteral("Nuevo"), central);
        auto *btnOpen = new QPushButton(QStringLiteral("Abrir..."), central);
        auto *btnSave = new QPushButton(QStringLiteral("Guardar"), central);
        auto *btnSaveAs = new QPushButton(QStringLiteral("Guardar como..."), central);
        fileBar->addWidget(btnNew);
        fileBar->addWidget(btnOpen);
        fileBar->addWidget(btnSave);
        fileBar->addWidget(btnSaveAs);
        fileBar->addStretch();
        root->addLayout(fileBar);

        // Barra de formato: el usuario escribe con botones, no con simbolos.
        auto *formatBar = new QHBoxLayout;
        auto *btnTitle = new QPushButton(QStringLiteral("Titulo grande"), central);
        auto *btnSubtitle = new QPushButton(QStringLiteral("Subtitulo"), central);
        auto *btnBold = new QPushButton(QStringLiteral("Negrita"), central);
        auto *btnItalic = new QPushButton(QStringLiteral("Cursiva"), central);
        auto *btnBullets = new QPushButton(QStringLiteral("Lista con puntos"), central);
        auto *btnNumbered = new QPushButton(QStringLiteral("Lista numerada"), central);
        auto *btnQuote = new QPushButton(QStringLiteral("Cita"), central);
        auto *btnLink = new QPushButton(QStringLiteral("Enlace"), central);
        formatBar->addWidget(btnTitle);
        formatBar->addWidget(btnSubtitle);
        formatBar->addWidget(btnBold);
        formatBar->addWidget(btnItalic);
        formatBar->addWidget(btnBullets);
        formatBar->addWidget(btnNumbered);
        formatBar->addWidget(btnQuote);
        formatBar->addWidget(btnLink);
        formatBar->addStretch();
        root->addLayout(formatBar);

        // Dos paneles: a la izquierda se escribe, a la derecha se lee bonito.
        auto *splitter = new QSplitter(Qt::Horizontal, central);
        m_editor = new QPlainTextEdit(splitter);
        m_editor->setPlaceholderText(QStringLiteral("Escribe aqui tu texto..."));
        m_preview = new QTextBrowser(splitter);
        m_preview->setOpenExternalLinks(true);
        splitter->addWidget(m_editor);
        splitter->addWidget(m_preview);
        splitter->setSizes({550, 550});
        root->addWidget(splitter, 1);

        m_status = new QLabel(central);
        root->addWidget(m_status);

        setCentralWidget(central);

        m_editor->setPlainText(QStringLiteral(
            "# Bienvenido a light_md\n\n"
            "Escribe a la **izquierda** y mira el resultado a la *derecha*.\n\n"
            "## Para empezar\n\n"
            "- Pulsa los botones de arriba para dar formato\n"
            "- No necesitas saber Markdown\n"
            "- Guarda tu texto con el boton Guardar\n"));

        updatePreview();

        connect(btnNew, &QPushButton::clicked, this, [this] { newDocument(); });
        connect(btnOpen, &QPushButton::clicked, this, [this] { openDocument(); });
        connect(btnSave, &QPushButton::clicked, this, [this] { saveDocument(); });
        connect(btnSaveAs, &QPushButton::clicked, this, [this] { saveDocumentAs(); });

        connect(btnTitle, &QPushButton::clicked, this, [this] { prefixLines(QStringLiteral("# ")); });
        connect(btnSubtitle, &QPushButton::clicked, this, [this] { prefixLines(QStringLiteral("## ")); });
        connect(btnQuote, &QPushButton::clicked, this, [this] { prefixLines(QStringLiteral("> ")); });
        connect(btnBullets, &QPushButton::clicked, this, [this] { prefixLines(QStringLiteral("- ")); });
        connect(btnNumbered, &QPushButton::clicked, this, [this] { numberedLines(); });
        connect(btnBold, &QPushButton::clicked, this, [this] { wrapSelection(QStringLiteral("**")); });
        connect(btnItalic, &QPushButton::clicked, this, [this] { wrapSelection(QStringLiteral("*")); });
        connect(btnLink, &QPushButton::clicked, this, [this] { insertLink(); });

        connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
            m_modified = true;
            updatePreview();
        });
        m_modified = false;
    }

private:
    void updatePreview()
    {
        const QString text = m_editor->toPlainText();
        m_preview->setMarkdown(text);
        const int words = text.split(QChar(' '), Qt::SkipEmptyParts).size();
        m_status->setText(QStringLiteral("Palabras: %1    Archivo: %2%3")
                              .arg(words)
                              .arg(m_filePath.isEmpty() ? QStringLiteral("(sin guardar)") : m_filePath)
                              .arg(m_modified ? QStringLiteral("  *") : QString()));
    }

    void prefixLines(const QString &prefix)
    {
        QTextCursor cursor = m_editor->textCursor();
        cursor.beginEditBlock();
        if (!cursor.hasSelection()) {
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.insertText(prefix);
        } else {
            const int start = cursor.selectionStart();
            const int end = cursor.selectionEnd();
            cursor.setPosition(start);
            cursor.movePosition(QTextCursor::StartOfLine);
            const int blockStart = cursor.position();
            cursor.setPosition(end);
            cursor.movePosition(QTextCursor::EndOfLine);
            const int blockEnd = cursor.position();
            cursor.setPosition(blockStart);
            while (cursor.position() < blockEnd) {
                cursor.movePosition(QTextCursor::StartOfLine);
                cursor.insertText(prefix);
                if (!cursor.movePosition(QTextCursor::NextBlock))
                    break;
            }
        }
        cursor.endEditBlock();
        m_editor->setFocus();
    }

    void numberedLines()
    {
        QTextCursor cursor = m_editor->textCursor();
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.insertText(QStringLiteral("1. "));
        cursor.endEditBlock();
        m_editor->setFocus();
    }

    void wrapSelection(const QString &marks)
    {
        QTextCursor cursor = m_editor->textCursor();
        cursor.beginEditBlock();
        if (cursor.hasSelection()) {
            const QString selected = cursor.selectedText();
            cursor.insertText(marks + selected + marks);
        } else {
            cursor.insertText(marks + QStringLiteral("texto") + marks);
        }
        cursor.endEditBlock();
        m_editor->setFocus();
    }

    void insertLink()
    {
        QTextCursor cursor = m_editor->textCursor();
        cursor.beginEditBlock();
        const QString selected = cursor.hasSelection() ? cursor.selectedText()
                                                        : QStringLiteral("texto del enlace");
        cursor.insertText(QStringLiteral("[%1](https://ejemplo.com)").arg(selected));
        cursor.endEditBlock();
        m_editor->setFocus();
    }

    bool askSaveIfModified()
    {
        if (!m_modified)
            return true;
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Guardar cambios"),
            QStringLiteral("Tienes cambios sin guardar. Quieres guardarlos?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
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
        m_filePath.clear();
        m_modified = false;
        updatePreview();
    }

    void openDocument()
    {
        if (!askSaveIfModified())
            return;
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Abrir documento"), QString(),
            QStringLiteral("Documentos Markdown (*.md *.markdown *.txt);;Todos los archivos (*)"));
        if (path.isEmpty())
            return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("No se pudo abrir"),
                                 QStringLiteral("No se pudo leer el archivo elegido."));
            return;
        }
        m_editor->setPlainText(QString::fromUtf8(file.readAll()));
        m_filePath = path;
        m_modified = false;
        updatePreview();
    }

    bool writeTo(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("No se pudo guardar"),
                                 QStringLiteral("No se pudo escribir en el archivo elegido."));
            return false;
        }
        file.write(m_editor->toPlainText().toUtf8());
        m_filePath = path;
        m_modified = false;
        updatePreview();
        return true;
    }

    bool saveDocument()
    {
        if (m_filePath.isEmpty())
            return saveDocumentAs();
        return writeTo(m_filePath);
    }

    bool saveDocumentAs()
    {
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Guardar como"), QStringLiteral("documento.md"),
            QStringLiteral("Documentos Markdown (*.md);;Todos los archivos (*)"));
        if (path.isEmpty())
            return false;
        return writeTo(path);
    }

    QPlainTextEdit *m_editor = nullptr;
    QTextBrowser *m_preview = nullptr;
    QLabel *m_status = nullptr;
    QString m_filePath;
    bool m_modified = false;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("light_md"));
    MainWindow window;
    window.show();
    return app.exec();
}
