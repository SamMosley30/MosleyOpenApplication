/**
 * @file CheckBoxDelegate.h
 * @brief Contains the declaration of the CheckBoxDelegate class.
 */

#ifndef CHECKBOXDELEGATE_H
#define CHECKBOXDELEGATE_H

#include <QStyledItemDelegate>

/**
 * @class CheckBoxDelegate
 * @brief A delegate for rendering and editing boolean values as checkboxes in a view.
 *
 * This class provides a checkbox editor for items in a model.
 */
class CheckBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    /**
     * @brief Constructs a CheckBoxDelegate object.
     * @param parent The parent object.
     */
    explicit CheckBoxDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    /**
     * @brief Creates the editor widget for the delegate.
     * @param parent The parent widget for the editor.
     * @param option The style options for the item.
     * @param index The model index of the item.
     * @return The editor widget.
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    /**
     * @brief Sets the data for the editor.
     * @param editor The editor widget.
     * @param index The model index of the item.
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    /**
     * @brief Sets the data for the model.
     * @param editor The editor widget.
     * @param model The model.
     * @param index The model index of the item.
     */
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    /**
     * @brief Updates the geometry of the editor.
     * @param editor The editor widget.
     * @param option The style options for the item.
     * @param index The model index of the item.
     */
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};

#endif // CHECKBOXDELEGATE_H