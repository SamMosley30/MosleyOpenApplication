/**
 * @file SpinBoxDelegate.cpp
 * @brief Implements the SpinBoxDelegate class.
 */

#include "SpinBoxDelegate.h"
#include <QSpinBox>

/**
 * @brief Creates the editor widget for the delegate.
 * @param parent The parent widget for the editor.
 * @param option The style options for the item.
 * @param index The model index of the item.
 * @return The editor widget.
 */
QWidget *SpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    auto *sb = new QSpinBox(parent);
    sb->setRange(0, 72);
    return sb;
}

/**
 * @brief Sets the data for the editor.
 * @param editor The editor widget.
 * @param index The model index of the item.
 */
void SpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    int value = index.model()->data(index, Qt::EditRole).toInt();
    static_cast<QSpinBox*>(editor)->setValue(value);
}

/**
 * @brief Sets the data for the model.
 * @param editor The editor widget.
 * @param model The model.
 * @param index The model index of the item.
 */
void SpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    int value = static_cast<QSpinBox*>(editor)->value();
    model->setData(index, value, Qt::EditRole);
}
