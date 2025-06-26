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
#include "PlayerListWidget.h" // Include the custom PlayerListWidget

struct TeamData {
    int id;
    QString name;
    std::vector<PlayerInfo> members;
};

class TeamAssemblyDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeamAssemblyDialog(QSqlDatabase &db, QWidget *parent = nullptr);
    // ~TeamAssemblyDialog(); // Default destructor is fine

private slots:
    void loadActivePlayers();
    void autoAssignTeams(); 
    void saveTeams();       
    // This slot is called when a player is successfully dropped onto any PlayerListWidget
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

    std::vector<PlayerInfo> availablePlayersData; // Players not yet in a team
    std::vector<TeamData> teamsData; // Players organized by team index

    void setupUI();
    void createTeamGroupBox(int teamIndex, const QString& teamName);
};

#endif // TEAMASSEMBLYDIALOG_H
