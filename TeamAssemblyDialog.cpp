#include "TeamAssemblyDialog.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <algorithm> // For std::remove_if, std::sort, std::shuffle, std::any_of, std::none_of
#include <random>    // For std::mt19937, std::random_device

TeamAssemblyDialog::TeamAssemblyDialog(QSqlDatabase &db, QWidget *parent)
    : QDialog(parent), database(db) {
    teamsData.resize(6); // Initialize for 6 teams
    setupUI();
    loadActivePlayers(); // Load initial players and their persisted team assignments

    connect(refreshPlayersButton, &QPushButton::clicked, this, &TeamAssemblyDialog::loadActivePlayers);
    connect(closeButton, &QPushButton::clicked, this, &TeamAssemblyDialog::accept);
    connect(autoAssignButton, &QPushButton::clicked, this, &TeamAssemblyDialog::autoAssignTeams);
    connect(saveButton, &QPushButton::clicked, this, &TeamAssemblyDialog::saveTeams);
}

// Default destructor is sufficient as child widgets are handled by Qt

void TeamAssemblyDialog::setupUI() {
    setWindowTitle(tr("Assemble Teams (Drag & Drop)"));
    setMinimumSize(800, 600); 

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Active Players Section
    QGroupBox *activePlayersGroupBox = new QGroupBox(tr("Available Players"));
    QVBoxLayout *activePlayersLayout = new QVBoxLayout(activePlayersGroupBox);
    activePlayersListWidget = new PlayerListWidget(this);
    activePlayersListWidget->setObjectName("availablePlayersListWidget"); 
    refreshPlayersButton = new QPushButton(tr("Refresh Players"));
    activePlayersLayout->addWidget(refreshPlayersButton);
    activePlayersLayout->addWidget(activePlayersListWidget);

    connect(activePlayersListWidget, QOverload<const PlayerInfo&, PlayerListWidget*, PlayerListWidget*>::of(&PlayerListWidget::playerDropped),
        this, &TeamAssemblyDialog::handlePlayerDropped);

    // Teams Section
    QHBoxLayout *teamsLayout = new QHBoxLayout(); 

    for (int i = 0; i < 6; ++i) {
        QGroupBox *teamXGroupBox = new QGroupBox(tr("Team %1").arg(i + 1)); 
        QVBoxLayout *teamXLayout = new QVBoxLayout(teamXGroupBox);
        PlayerListWidget *teamXListWidget = new PlayerListWidget(this);
        teamXListWidget->setObjectName(QString("teamListWidget%1").arg(i)); 
        teamListWidgets.push_back(teamXListWidget);
        teamXLayout->addWidget(teamXListWidget);
        teamsLayout->addWidget(teamXGroupBox); 

        connect(teamXListWidget, QOverload<const PlayerInfo&, PlayerListWidget*, PlayerListWidget*>::of(&PlayerListWidget::playerDropped),
            this, &TeamAssemblyDialog::handlePlayerDropped);
    }

    // Buttons Layout
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    autoAssignButton = new QPushButton(tr("Auto-Assign")); 
    saveButton = new QPushButton(tr("Save Teams"));       
    closeButton = new QPushButton(tr("Close"));
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(autoAssignButton); 
    buttonsLayout->addWidget(saveButton);       
    buttonsLayout->addWidget(closeButton);

    mainLayout->addWidget(activePlayersGroupBox);
    mainLayout->addLayout(teamsLayout); 
    mainLayout->addLayout(buttonsLayout);
}


void TeamAssemblyDialog::loadActivePlayers() {
    // Clear UI lists first
    activePlayersListWidget->clear();
    for(auto& list_widget : teamListWidgets) {
        list_widget->clear();
    }

    // Clear internal data models
    availablePlayersData.clear();
    for(auto& team_data : teamsData) {
        team_data.clear();
    }

    if (!database.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Database is not open."));
        return;
    }

    QSqlQuery query(database);
    // Fetch active players and their team_id
    if (query.exec("SELECT id, name, handicap, team_id FROM players WHERE active = 1 ORDER BY name")) {
        while (query.next()) {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            QVariant teamIdVariant = query.value("team_id");
            
            int teamId = 0; // Default to 0 (no team or available)
            if (!teamIdVariant.isNull()) {
                teamId = teamIdVariant.toInt();
            }

            if (teamId >= 1 && teamId <= 6) { // Player is assigned to a valid team (1-6)
                int teamIndex = teamId - 1; // Convert to 0-based index
                if (teamIndex < teamsData.size()) {
                    teamsData[teamIndex].push_back(player);
                    teamListWidgets[teamIndex]->addPlayer(player); // Add to UI
                } else {
                     qWarning() << "Player" << player.name << "has invalid team_id" << teamId << "from DB. Placing in available.";
                     availablePlayersData.push_back(player);
                     activePlayersListWidget->addPlayer(player); // Add to UI
                }
            } else { // Player is not on a team (team_id is NULL or 0 or invalid)
                availablePlayersData.push_back(player);
                activePlayersListWidget->addPlayer(player); // Add to UI
            }
        }
        qDebug() << "Loaded active players. Available:" << availablePlayersData.size() 
                 << "Team0:" << teamsData[0].size() << "Team1:" << teamsData[1].size()
                 << "Team2:" << teamsData[2].size() << "Team3:" << teamsData[3].size();
    } else {
        QMessageBox::critical(this, tr("Database Query Error"), query.lastError().text());
    }
}

void TeamAssemblyDialog::handlePlayerDropped(const PlayerInfo& player, PlayerListWidget* sourceList, PlayerListWidget* targetList) {
    qDebug() << "Player" << player.name << "(ID:" << player.id << ") dropped. Source:" << (sourceList ? sourceList->objectName() : "null") << "Target:" << targetList->objectName();

    // A drop within the same list is just a reorder. No data model change needed.
    if (!sourceList || !targetList || sourceList == targetList)
        return;

    // Step 1: Find the source and target internal data vectors
    std::vector<PlayerInfo>* sourceData = nullptr;
    if (sourceList == activePlayersListWidget) {
        sourceData = &availablePlayersData;
    } else {
        for (size_t i = 0; i < teamListWidgets.size(); ++i) {
            if (teamListWidgets[i] == sourceList) {
                sourceData = &teamsData[i];
                break;
            }
        }
    }

    std::vector<PlayerInfo>* targetData = nullptr;
    if (targetList == activePlayersListWidget) {
        targetData = &availablePlayersData;
    } else { 
        for (size_t i = 0; i < teamListWidgets.size(); ++i) {
            if (teamListWidgets[i] == targetList) {
                targetData = &teamsData[i];
                break; 
            }
        }
    }

    if (!sourceData || !targetData) {
        qWarning() << "Could not map source or target list to an internal data model.";
        return;
    }

    // Step 2: Move the player from the source vector to the target vector
    sourceData->erase(std::remove_if(sourceData->begin(), sourceData->end(),
                                     [&](const PlayerInfo& p) { return p.id == player.id; }),
                      sourceData->end());

    targetData->push_back(player);
    
    qDebug() << "Successfully moved player" << player.name << "in the internal data model.";
}

void TeamAssemblyDialog::autoAssignTeams() {
    qDebug() << "Auto-Assigning Teams...";
    std::vector<PlayerInfo> allPlayersToAssign;
    // Consolidate from internal models, as UI might not be source of truth during this operation
    for (const auto& p : availablePlayersData) allPlayersToAssign.push_back(p);
    for (const auto& team_vec : teamsData) {
        for (const auto& p : team_vec) allPlayersToAssign.push_back(p);
    }

    std::sort(allPlayersToAssign.begin(), allPlayersToAssign.end(), [](const PlayerInfo& a, const PlayerInfo& b){ return a.id < b.id; });
    allPlayersToAssign.erase(std::unique(allPlayersToAssign.begin(), allPlayersToAssign.end(), [](const PlayerInfo& a, const PlayerInfo& b){ return a.id == b.id; }), allPlayersToAssign.end());

    if (allPlayersToAssign.empty()) {
        QMessageBox::information(this, tr("No Players"), tr("No players available to assign."));
        return;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(allPlayersToAssign.begin(), allPlayersToAssign.end(), g);

    // Clear existing internal team data and UI lists
    activePlayersListWidget->clear();
    availablePlayersData.clear();
    for(auto& team_vec : teamsData) team_vec.clear();
    for(auto& team_ui_list : teamListWidgets) team_ui_list->clear();

    int teamIdx = 0;
    for(const auto& player : allPlayersToAssign) {
        teamsData[teamIdx].push_back(player);
        teamIdx = (teamIdx + 1) % teamsData.size(); 
    }

    // Repopulate UI lists from the new internal models
    for(const auto& player : availablePlayersData) { 
        activePlayersListWidget->addPlayer(player);
    }
    for(size_t i = 0; i < teamsData.size(); ++i) {
        for(const auto& player : teamsData[i]) {
            teamListWidgets[i]->addPlayer(player);
        }
    }
    QMessageBox::information(this, tr("Auto-Assign Complete"), tr("Players have been distributed into teams."));
}

void TeamAssemblyDialog::saveTeams() {
    if (!database.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Database is not open. Cannot save teams."));
        return;
    }

    QSqlDatabase::database().transaction(); // Start a transaction for atomic updates

    bool allSuccessful = true;

    // Update players assigned to teams
    for (size_t i = 0; i < teamsData.size(); ++i) {
        int teamNumberToSave = i + 1; // Team ID in DB will be 1, 2, 3, 4, 5, 6
        for (const auto& player : teamsData[i]) {
            QSqlQuery query(database);
            query.prepare("UPDATE players SET team_id = :teamId WHERE id = :playerId");
            query.bindValue(":teamId", teamNumberToSave);
            query.bindValue(":playerId", player.id);
            if (!query.exec()) {
                qWarning() << "Failed to save team for player" << player.name << "(ID:" << player.id << "):" << query.lastError().text();
                allSuccessful = false;
            } else {
                 qDebug() << "Saved player" << player.name << "to team_id" << teamNumberToSave;
            }
        }
    }

    // Update players in the "available" list (set team_id to NULL)
    for (const auto& player : availablePlayersData) {
        QSqlQuery query(database);
        query.prepare("UPDATE players SET team_id = NULL WHERE id = :playerId");
        query.bindValue(":playerId", player.id);
        if (!query.exec()) {
            qWarning() << "Failed to clear team for player" << player.name << "(ID:" << player.id << "):" << query.lastError().text();
            allSuccessful = false;
        } else {
            qDebug() << "Cleared team_id for available player" << player.name;
        }
    }

    if (allSuccessful) {
        if (QSqlDatabase::database().commit()) {
            QMessageBox::information(this, tr("Save Successful"), tr("Team assignments have been saved to the database."));
            qDebug() << "Team assignments saved and transaction committed.";
        } else {
            QMessageBox::critical(this, tr("Transaction Error"), tr("Failed to commit team assignments to the database: %1").arg(QSqlDatabase::database().lastError().text()));
            qWarning() << "Transaction commit failed:" << QSqlDatabase::database().lastError().text();
             QSqlDatabase::database().rollback(); // Rollback on commit failure
        }
    } else {
        QSqlDatabase::database().rollback(); // Rollback if any query failed
        QMessageBox::warning(this, tr("Save Failed"), tr("Some team assignments could not be saved. Changes have been rolled back. Check console for errors."));
        qWarning() << "One or more team assignment saves failed. Transaction rolled back.";
    }
}
