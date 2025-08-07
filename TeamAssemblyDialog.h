/**
 * @file TeamAssemblyDialog.h
 * @brief Contains the declaration of the TeamAssemblyDialog class.
 */

#ifndef TEAMASSEMBLYDIALOG_H
#define TEAMASSEMBLYDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QGroupBox>
#include <vector>
#include "CommonStructs.h"
#include "PlayerListWidget.h"

/**
 * @struct TeamData
 * @brief Holds data for a single team, including its members.
 */
struct TeamData {
    int id;                     ///< The unique identifier for the team.
    QString name;               ///< The name of the team.
    std::vector<PlayerInfo> members; ///< The players who are members of the team.
};

/**
 * @class TeamAssemblyDialog
 * @brief A dialog for assembling players into teams.
 *
 * This dialog provides an interface for creating teams, assigning players to them
 * manually via drag and drop, or automatically assigning them.
 */
class TeamAssemblyDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TeamAssemblyDialog object.
     * @param db The database connection to use.
     * @param parent The parent widget.
     */
    explicit TeamAssemblyDialog(QSqlDatabase &db, QWidget *parent = nullptr);

    /**
     * @brief Refreshes the data in the dialog.
     */
    void refresh();

private slots:
    void loadActivePlayers();
    void autoAssignTeams();
    void saveTeams();
    void handlePlayerDropped(const PlayerInfo& player, PlayerListWidget* sourceList, PlayerListWidget* targetList);
    void addTeam();
    void removeTeam();

private:
    QSqlDatabase &database;

    PlayerListWidget *activePlayersListWidget;
    QHBoxLayout *teamsLayout;
    std::vector<PlayerListWidget*> teamListWidgets;
    std::vector<QLineEdit*> teamNameEditLines;

    QPushButton *addTeamButton;
    QPushButton *removeTeamButton;
    QPushButton *refreshPlayersButton;
    QPushButton *autoAssignButton;
    QPushButton *saveButton;
    QPushButton *closeButton;

    std::vector<PlayerInfo> availablePlayersData;
    std::vector<TeamData> teamsData;

    void setupUI();
    void createTeamGroupBox(int teamIndex, const QString& teamName);
};

#endif // TEAMASSEMBLYDIALOG_H
