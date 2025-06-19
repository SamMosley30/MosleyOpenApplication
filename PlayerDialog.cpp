#include "PlayerDialog.h"
#include "SpinBoxDelegate.h"    // For SpinBoxDelegate
#include "CheckBoxDelegate.h" // For CheckBoxDelegate
#include <QtWidgets>            // For QTableView, QPushButton, Layouts etc.
#include <QtSql>                // For QSqlTableModel, QSqlQuery, QSqlError

PlayerDialog::PlayerDialog(QSqlDatabase &db, QWidget *parent)
    : QDialog(parent)
    // No need to store 'db' as a member if only used for model initialization here
    , model(new QSqlTableModel(this, db)) // 'this' for parent, 'db' for database
    , tableView(new QTableView(this))
    , addButton(new QPushButton(tr("Add"), this))
    , removeButton(new QPushButton(tr("Remove"), this))
    , closeButton(new QPushButton(tr("Close"), this))
    , exportButton(new QPushButton(tr("Export to CSV"), this))
{
    model->setTable("players");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select(); // Initial population
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    model->setHeaderData(2, Qt::Horizontal, tr("Handicap"));
    model->setHeaderData(3, Qt::Horizontal, tr("Active"));

    tableView->setModel(model);
    tableView->hideColumn(0); // Hide 'id' column
    tableView->hideColumn(4); // Hide 'team' column
    tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    tableView->setItemDelegateForColumn(2, new SpinBoxDelegate(tableView));
    tableView->setItemDelegateForColumn(3, new CheckBoxDelegate(tableView));

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tableView);
    mainLayout->addLayout(buttonLayout);
    setWindowTitle(tr("Player Database"));
    resize(500,400);

    connect(addButton, &QPushButton::clicked, this, &PlayerDialog::addPlayer);
    connect(removeButton, &QPushButton::clicked, this, &PlayerDialog::removeSelected);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(exportButton, &QPushButton::clicked, this, &PlayerDialog::exportToCsv);
}

void PlayerDialog::addPlayer() {
    int row = model->rowCount();
    model->insertRow(row);
    // Set default values for the new row
    model->setData(model->index(row, 1), "New Player"); // Name column
    model->setData(model->index(row, 2), 0);          // Handicap column
    model->setData(model->index(row, 3), 0);          // Active column (0 for false/unchecked)

    // Explicitly submit changes to ensure it's written to the DB,
    // especially important in test environments or when OnFieldChange might not trigger.
    if (!model->submitAll())
        qWarning() << "PlayerDialog::addPlayer - submitAll() failed:" << model->lastError().text();
}

void PlayerDialog::removeSelected() {
    QModelIndexList selected = tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return; 
    }

    // Remove rows from bottom up to avoid index shifting issues
    for (int i = selected.count() - 1; i >= 0; --i) {
        // The QSqlTableModel::removeRow marks the row for deletion.
        // The actual deletion from DB happens on submitAll() or based on edit strategy.
        if (!model->removeRow(selected.at(i).row())) {
             qWarning() << "PlayerDialog::removeSelected - Failed to mark row for removal in model cache.";
        }
    }

    // Submit all pending changes (including row deletions)
    if (!model->submitAll()) {
        qCritical() << "PlayerDialog::removeSelected - submitAll() FAILED for deletions. Database Error:" << model->lastError();
        QMessageBox::critical(this, "Database Error",
                             "Failed to remove the selected player(s) from the database.\nError: " + model->lastError().text());
        model->revertAll(); // Revert changes in the model cache if DB commit failed
    }
}

void PlayerDialog::exportToCsv() {
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Export Player Data"), 
                                                    QDir::homePath(),         
                                                    tr("CSV Files (*.csv);;All Files (*)")
                                                    );
    if (filePath.isEmpty()) {
        return;
    }
    if (!filePath.toLower().endsWith(".csv")) {
        filePath += ".csv";
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("File Error"),
                              tr("Could not open file for writing: %1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8); 

    QStringList headerLabels;
    for (int i = 0; i < model->columnCount(); ++i) {
        if (tableView->isColumnHidden(i)) continue; // Skip hidden columns like 'id'
        QString headerText = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        headerLabels << QString("\"%1\"").arg(headerText.replace("\"", "\"\"")); 
    }
    out << headerLabels.join(",") << "\n"; 

    for (int row = 0; row < model->rowCount(); ++row) {
        QStringList rowData;
        for (int column = 0; column < model->columnCount(); ++column) {
            if (tableView->isColumnHidden(column)) continue;
            QVariant cellData = model->data(model->index(row, column), Qt::DisplayRole);
            QString cellString = cellData.toString(); 
            if (cellData.typeId() == QMetaType::Bool) { // Handle boolean for 'active' column
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
    QMessageBox::information(this, tr("Export Successful"),
                             tr("Player data exported to:\n%1").arg(QDir::toNativeSeparators(filePath)));
}
