#include "PlayerDialog.h"
#include "SpinBoxDelegate.h"
#include "CheckBoxDelegate.h"
#include <QtWidgets>
#include <QtSql>

PlayerDialog::PlayerDialog(QSqlDatabase &db, QWidget *parent)
    : QDialog(parent)
    , model(new QSqlTableModel(this, db))
    , tableView(new QTableView(this))
    , addButton(new QPushButton(tr("Add"), this))
    , removeButton(new QPushButton(tr("Remove"), this))
    , closeButton(new QPushButton(tr("Close"), this))
    , exportButton(new QPushButton(tr("Export to CSV"), this))
{
    model->setTable("players");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    model->setHeaderData(2, Qt::Horizontal, tr("Handicap"));
    model->setHeaderData(3, Qt::Horizontal, tr("Active"));

    tableView->setModel(model);
    tableView->hideColumn(0);
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
    connect(exportButton, &QPushButton::clicked, this, &PlayerDialog::exportToCsv); // Connect to the new slot
}

void PlayerDialog::addPlayer() {
    int row = model->rowCount();
    model->insertRow(row);
    model->setData(model->index(row, 1), "New Player");
    model->setData(model->index(row, 2), 0);
    model->setData(model->index(row, 3), 0);
}

void PlayerDialog::removeSelected() {
    QModelIndexList selected = tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return; 
    }

    qDebug() << "Attempting to remove" << selected.count() << "rows.";

    // Mark rows for removal in the cache
    for (int i = selected.count() - 1; i >= 0; --i) {
        int rowToRemove = selected.at(i).row();
        qDebug() << "Marking row" << rowToRemove << "for removal.";
        if (!model->removeRow(rowToRemove)) {
             qWarning() << "Failed to mark row" << rowToRemove << "for removal in model cache.";
             // Maybe continue, maybe return, depends on desired behavior
        }
    }

    qDebug() << "Calling submitAll() to commit removals...";
    bool success = model->submitAll();
    qDebug() << "submitAll() returned:" << success; // Log the EXACT return value

    if (success) {
        qDebug() << "submitAll() reported SUCCESS.";
        // Even on success, check for potential lingering errors (less common)
        if (model->lastError().isValid() && model->lastError().type() != QSqlError::NoError) {
             qWarning() << "WARNING: submitAll() returned true, but there's a lingering model error:" << model->lastError();
        }
        // The view should update automatically here if signals worked.
    } else {
        qCritical() << "submitAll() reported FAILURE."; // Use qCritical for emphasis
        qCritical() << "Database Error:" << model->lastError(); // Log the error
        QMessageBox::critical(this, "Database Error",
                             "Failed to remove the selected player(s) from the database.\nError: " + model->lastError().text());
        
        qDebug() << "Calling revertAll() due to submission failure.";
        model->revertAll(); 
    }

    // *** KEY DIAGNOSTIC STEP ***
    // Force the model to re-select data from the database AFTER the submit attempt.
    // This will synchronize the model/view with the *actual* database state.
    qDebug() << "Forcing model refresh with select().";
    model->select(); 
    qDebug() << "Model refresh complete.";
}

void PlayerDialog::exportToCsv() {
    // Use QFileDialog to get the file path from the user
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Export Player Data"), // Dialog title
                                                    QDir::homePath(),         // Default directory
                                                    tr("CSV Files (*.csv);;All Files (*)") // File filters
                                                    );

    // Check if the user cancelled the dialog
    if (filePath.isEmpty()) {
        return;
    }

    // Ensure the file path has a .csv extension
    if (!filePath.toLower().endsWith(".csv")) {
        filePath += ".csv";
    }

    // Open the file for writing
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // Show an error message if the file cannot be opened
        QMessageBox::critical(this, tr("File Error"),
                              tr("Could not open file for writing: %1").arg(file.errorString()));
        return;
    }

    // Create a text stream to write to the file
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8); // Use UTF-8 encoding for broader compatibility

    // --- Write the header row ---
    // Iterate through the visible columns of the model
    QStringList headerLabels;
    // Get the horizontal header from the view (to check for hidden columns if needed)
    QHeaderView *header = tableView->horizontalHeader();
    for (int i = 0; i < model->columnCount(); ++i) {
        // Check if the column is visible in the view (optional, but good practice)
        // if (!tableView->isColumnHidden(i)) {
            // Get the header data for the column (DisplayRole for text)
            QString headerText = model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            // Handle potential commas or quotes in header text for CSV
            headerLabels << QString("\"%1\"").arg(headerText.replace("\"", "\"\"")); // Enclose in quotes and escape existing quotes
        // }
    }
    out << headerLabels.join(",") << "\n"; // Join headers with commas and add a newline

    // --- Write the data rows ---
    // Iterate through all rows in the model
    for (int row = 0; row < model->rowCount(); ++row) {
        QStringList rowData;
        // Iterate through all columns in the model
        for (int column = 0; column < model->columnCount(); ++column) {
            // Check if the column is visible (optional)
            // if (!tableView->isColumnHidden(column)) {
                // Get the data for the cell (DisplayRole)
                QVariant cellData = model->data(model->index(row, column), Qt::DisplayRole);
                QString cellString = cellData.toString(); // Convert data to string

                // Handle potential commas, quotes, or newlines in cell data for CSV
                // Basic handling: enclose in quotes if it contains comma, quote, or newline
                if (cellString.contains(',') || cellString.contains('"') || cellString.contains('\n') || cellString.contains('\r')) {
                     rowData << QString("\"%1\"").arg(cellString.replace("\"", "\"\"")); // Enclose and escape quotes
                } else {
                     rowData << cellString; // Just add the string if no special characters
                }
            // }
        }
        out << rowData.join(",") << "\n"; // Join cell data with commas and add a newline
    }

    // Close the file (QFile will auto-close when it goes out of scope or file.close() is called)
    // file.close(); // Explicitly close if needed

    // Optional: Show a success message
    QMessageBox::information(this, tr("Export Successful"),
                             tr("Player data exported to:\n%1").arg(QDir::toNativeSeparators(filePath)));
}
