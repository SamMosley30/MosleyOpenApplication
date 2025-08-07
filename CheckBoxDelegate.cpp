/**
 * @file CheckBoxDelegate.cpp
 * @brief Implements the CheckBoxDelegate class.
 */

#include "CheckBoxDelegate.h"
#include <QCheckBox>

/**
 * @brief Creates the editor widget for the delegate.
 * @param parent The parent widget for the editor.
 * @param option The style options for the item.
 * @param index The model index of the item.
 * @return The editor widget.
 */
QWidget *CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    return new QCheckBox(parent);
}

/**
 * @brief Sets the data for the editor.
 * @param editor The editor widget.
 * @param index The model index of the item.
 */
void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    bool checked = index.model()->data(index, Qt::EditRole).toBool();
    static_cast<QCheckBox*>(editor)->setChecked(checked);
}

/**
 * @brief Sets the data for the model.
 * @param editor The editor widget.
 * @param model The model.
 * @param index The model index of the item.
 */
void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    bool checked = static_cast<QCheckBox*>(editor)->isChecked();
    model->setData(index, checked, Qt::EditRole);
}

/**
 * @brief Updates the geometry of the editor.
 * @param editor The editor widget.
 * @param option The style options for the item.
 * @param index The model index of the item.
 */
void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
    editor->setGeometry(option.rect);
}
