/// @file file_drop_line_edit.h
/// @brief Read-only line edit that accepts a single local file via drag&drop.
#pragma once

#include <QLineEdit>
#include <functional>
#include <optional>

class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QResizeEvent;
class QMimeData;

namespace sp::ui {

/// @brief Returns the single local file carried by @p mimeData, if any.
/// @param mimeData MIME data of the drag.
/// @return Path of the file, or empty if there is not exactly one local file.
[[nodiscard]] std::optional<QString> extractLocalFile(const QMimeData *mimeData);

/// @brief Line edit that displays an elided path and accepts dropped files.
class FileDropLineEdit : public QLineEdit {
    Q_OBJECT

public:
    /// @brief Creates the editor.
    /// @param placeholder Placeholder text shown while empty.
    /// @param onFileDropped Called with the resolved path of the dropped file.
    /// @param parent Parent widget.
    explicit FileDropLineEdit(const QString &placeholder,
        std::function<void(const QString &)> onFileDropped, QWidget *parent = nullptr);

    /// @brief Sets the full path to display (middle-elided to fit).
    /// @param path Resolved path of the spectrum file.
    void setFilePath(const QString &path);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void updateDisplayText();

    std::function<void(const QString &)> m_onFileDropped; ///< Called on a successful drop.
    QString m_fullPath; ///< Path shown in the tooltip and elided in the text.
};

} // namespace sp::ui
