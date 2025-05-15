#include "SpinBoxDelegate.h"
#include <QSpinBox>

QWidget *SpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    auto *sb = new QSpinBox(parent);
    sb->setRange(0, 72);
    return sb;
}

void SpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    int value = index.model()->data(index, Qt::EditRole).toInt();
    static_cast<QSpinBox*>(editor)->setValue(value);
}

void SpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    int value = static_cast<QSpinBox*>(editor)->value();
    model->setData(index, value, Qt::EditRole);
}
