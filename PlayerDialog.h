/**
 * @file PlayerDialog.h
 * @brief Contains the declaration of the PlayerDialog class.
 */

#ifndef PLAYERDIALOG_H
#define PLAYERDIALOG_H

#include <QDialog>
#include <QSqlTableModel>

class QTableView;
class QPushButton;

/**
 * @class PlayerDialog
 * @brief A dialog for managing players in the database.
 *
 * This dialog displays a list of players from the database in a table view.
 * It allows adding new players, removing existing players, and exporting the player list to a CSV file.
 */
class PlayerDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Constructs a PlayerDialog object.
     * @param db The database connection to use.
     * @param parent The parent widget.
     */
    explicit PlayerDialog(QSqlDatabase &db, QWidget *parent = nullptr);

public slots:
    /**
     * @brief Adds a new player to the database.
     */
    void addPlayer();

    /**
     * @brief Removes the selected player(s) from the database.
     */
    void removeSelected();

    /**
     * @brief Exports the player list to a CSV file.
     */
    void exportToCsv();

private:
    QSqlTableModel *model;        ///< The model for the player data.
    QTableView *tableView;        ///< The table view for displaying player data.
    QPushButton *addButton;       ///< The button for adding a new player.
    QPushButton *removeButton;    ///< The button for removing selected players.
    QPushButton *closeButton;     ///< The button for closing the dialog.
    QPushButton *exportButton;    ///< The button for exporting data to CSV.
};

#endif // PLAYERDIALOG_H