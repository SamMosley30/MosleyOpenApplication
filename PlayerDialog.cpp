/**
 * @file PlayerDialog.cpp
 * @brief Implements the PlayerDialog class.
 */

#include "PlayerDialog.h"
#include "SpinBoxDelegate.h"
#include "CheckBoxDelegate.h"
#include <QtWidgets>
#include <QtSql>

/**
 * @brief Constructs a PlayerDialog object.
 *
 * This constructor sets up the UI for the player dialog, including the table view,
 * buttons, and layout. It also initializes the model and connects signals and slots.
 *
 * @param db The database connection to use.
 * @param exporter The exporter to use for exporting data.
 * @param parent The parent widget.
 */
PlayerDialog::PlayerDialog(QSqlDatabase &db, Exporter *exporter, QWidget *parent)
    : QDialog(parent)
    , model(new QSqlTableModel(this, db))
    , tableView(new QTableView(this))
    , addButton(new QPushButton(tr("Add"), this))
    , removeButton(new QPushButton(tr("Remove"), this))
    , closeButton(new QPushButton(tr("Close"), this))
    , exportButton(new QPushButton(tr("Export"), this))
    , m_exporter(exporter)
{
    model->setTable("players");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    model->setHeaderData(2, Qt::Horizontal, tr("Handicap"));
    model->setHeaderData(3, Qt::Horizontal, tr("Active"));

    tableView->setModel(model);
    tableView->hideColumn(0);
    tableView->hideColumn(4);
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
    connect(exportButton, &QPushButton::clicked, this, &PlayerDialog::exportData);
}

/**
 * @brief Adds a new player to the database.
 *
 * This slot is called when the "Add" button is clicked. It inserts a new row
 * into the model and sets default values for the new player.
 */
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

/**
 * @brief Removes the selected player(s) from the database.
 *
 * This slot is called when the "Remove" button is clicked. It removes the
 * selected rows from the table view and the database.
 */
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

/**
 * @brief Exports the player list using the provided exporter.
 *
 * This slot is called when the "Export" button is clicked. It calls the
 * exporter to export the player data.
 */
void PlayerDialog::exportData() {
    if (m_exporter) {
        m_exporter->exportData(model);
    }
}
