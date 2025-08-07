/**
 * @file CsvExporter.cpp
 * @brief Implements the CsvExporter class.
 */

#include "CsvExporter.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QMetaType>
#include <QStringConverter>

CsvExporter::CsvExporter(QWidget *parent)
    : m_parent(parent)
{
}

bool CsvExporter::exportData(QAbstractItemModel *model) {
    if (!model) {
        return false;
    }

    QString filePath = QFileDialog::getSaveFileName(m_parent,
                                                    QObject::tr("Export Data"),
                                                    QDir::homePath(),
                                                    QObject::tr("CSV Files (*.csv);;All Files (*)"));
    if (filePath.isEmpty()) {
        return false;
    }
    if (!filePath.toLower().endsWith(".csv")) {
        filePath += ".csv";
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(m_parent, QObject::tr("File Error"),
                              QObject::tr("Could not open file for writing: %1").arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QStringList headerLabels;
    for (int i = 0; i < model->columnCount(); ++i) {
        // Assuming the model is associated with a view, but we can't access it here.
        // We will export all columns. If a view is available, it should be used to check for hidden columns.
        QString headerText = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        headerLabels << QString("\"%1\"").arg(headerText.replace("\"", "\"\""));
    }
    out << headerLabels.join(",") << "\n";

    for (int row = 0; row < model->rowCount(); ++row) {
        QStringList rowData;
        for (int column = 0; column < model->columnCount(); ++column) {
            QVariant cellData = model->data(model->index(row, column), Qt::DisplayRole);
            QString cellString = cellData.toString();
            if (cellData.typeId() == QMetaType::Bool) {
                cellString = cellData.toBool() ? "1" : "0";
            }
            if (cellString.contains(',') || cellString.contains('"') || cellString.contains('\n') || cellString.contains('\r')) {
                 rowData << QString("\"%1\"").arg(cellString.replace("\"", "\"\""));
            } else {
                 rowData << cellString;
            }
        }
        out << rowData.join(",") << "\n";
    }

    QMessageBox::information(m_parent, QObject::tr("Export Successful"),
                             QObject::tr("Data exported to:\n%1").arg(QDir::toNativeSeparators(filePath)));
    return true;
}
