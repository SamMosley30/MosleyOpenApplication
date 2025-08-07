/**
 * @file SpinBoxDelegate.h
 * @brief Contains the declaration of the SpinBoxDelegate class.
 */

#ifndef SPINBOXDELEGATE_H
#define SPINBOXDELEGATE_H

#include <QStyledItemDelegate>

/**
 * @class SpinBoxDelegate
 * @brief A delegate for rendering and editing integer values as spin boxes in a view.
 *
 * This class provides a QSpinBox editor for items in a model.
 */
class SpinBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    /**
     * @brief Constructs a SpinBoxDelegate object.
     * @param parent The parent object.
     */
    explicit SpinBoxDelegate(QObject *parent = nullptr)
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
};

#endif // SPINBOXDELEGATE_H