#ifndef TEAMASSEMBLYDIALOG_H
#define TEAMASSEMBLYDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <vector>
#include "CommonStructs.h"
#include "PlayerListWidget.h" // Include the custom PlayerListWidget

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
    void handlePlayerDropped(const PlayerInfo& player, PlayerListWidget* targetList);

private:
    QSqlDatabase &database;

    PlayerListWidget *activePlayersListWidget; 
    std::vector<PlayerListWidget*> teamListWidgets; 
    
    QPushButton *refreshPlayersButton;
    QPushButton *autoAssignButton; 
    QPushButton *saveButton;       
    QPushButton *closeButton;

    // Internal data models for players
    std::vector<PlayerInfo> availablePlayersData; // Players not yet in a team
    std::vector<std::vector<PlayerInfo>> teamsData; // Players organized by team index

    void setupUI();
};

#endif // TEAMASSEMBLYDIALOG_H
