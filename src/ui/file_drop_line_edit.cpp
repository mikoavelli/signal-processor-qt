/// @file file_drop_line_edit.cpp
/// @brief Read-only line edit that accepts a single local file via drag&drop.
#include "file_drop_line_edit.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QResizeEvent>
#include <QUrl>
#include <QtGlobal>

namespace sp::ui {

std::optional<QString> extractLocalFile(const QMimeData *mimeData)
{
    if (mimeData == nullptr || !mimeData->hasUrls()) {
        return std::nullopt;
    }
    const QList<QUrl> urls = mimeData->urls();
    if (urls.size() != 1) {
        return std::nullopt;
    }
    const QUrl &url = urls.constFirst();
    if (!url.isLocalFile()) {
        return std::nullopt;
    }
    const QString path = QFileInfo(url.toLocalFile()).absoluteFilePath();
    if (!QFileInfo(path).isFile()) {
        return std::nullopt;
    }
    return path;
}

FileDropLineEdit::FileDropLineEdit(const QString &placeholder,
    std::function<void(const QString &)> onFileDropped, QWidget *parent)
    : QLineEdit(parent)
    , m_onFileDropped(std::move(onFileDropped))
{
    setReadOnly(true);
    setPlaceholderText(placeholder);
    setAcceptDrops(true);
}

void FileDropLineEdit::setFilePath(const QString &path)
{
    m_fullPath = path;
    setToolTip(m_fullPath);
    updateDisplayText();
}

void FileDropLineEdit::updateDisplayText()
{
    if (m_fullPath.isEmpty()) {
        setText(QString());
        return;
    }
    const int availableWidth = qMax(10, contentsRect().width() - 12);
    const QString elided = fontMetrics().elidedText(m_fullPath, Qt::ElideMiddle,
        availableWidth);
    setText(elided);
    setCursorPosition(0);
}

void FileDropLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    updateDisplayText();
}

void FileDropLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (extractLocalFile(event->mimeData()).has_value()) {
        setStyleSheet(QStringLiteral("border: 2px dashed #2563EB;"));
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void FileDropLineEdit::dragMoveEvent(QDragMoveEvent *event)
{
    if (extractLocalFile(event->mimeData()).has_value()) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void FileDropLineEdit::dragLeaveEvent(QDragLeaveEvent *event)
{
    setStyleSheet(QString());
    event->accept();
}

void FileDropLineEdit::dropEvent(QDropEvent *event)
{
    setStyleSheet(QString());
    const std::optional<QString> filePath = extractLocalFile(event->mimeData());
    if (filePath.has_value()) {
        m_onFileDropped(filePath.value());
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

} // namespace sp::ui
