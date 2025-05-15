#include "CheckBoxDelegate.h"
#include <QCheckBox>

QWidget *CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    return new QCheckBox(parent);
}

void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    bool checked = index.model()->data(index, Qt::EditRole).toBool();
    static_cast<QCheckBox*>(editor)->setChecked(checked);
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    bool checked = static_cast<QCheckBox*>(editor)->isChecked();
    model->setData(index, checked, Qt::EditRole);
}

void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
    editor->setGeometry(option.rect);
}
