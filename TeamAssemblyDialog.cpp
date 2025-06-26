#include "TeamAssemblyDialog.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <algorithm> // For std::remove_if, std::sort, std::shuffle, std::any_of, std::none_of
#include <random>    // For std::mt19937, std::random_device

TeamAssemblyDialog::TeamAssemblyDialog(QSqlDatabase &db, QWidget *parent)
    : QDialog(parent), database(db) 
{
    setupUI();
    loadActivePlayers(); // Load initial players and their persisted team assignments

    connect(addTeamButton, &QPushButton::clicked, this, &TeamAssemblyDialog::addTeam);
    connect(removeTeamButton, &QPushButton::clicked, this, &TeamAssemblyDialog::removeTeam);
    connect(refreshPlayersButton, &QPushButton::clicked, this, &TeamAssemblyDialog::loadActivePlayers);
    connect(closeButton, &QPushButton::clicked, this, &TeamAssemblyDialog::accept);
    connect(autoAssignButton, &QPushButton::clicked, this, &TeamAssemblyDialog::autoAssignTeams);
    connect(saveButton, &QPushButton::clicked, this, &TeamAssemblyDialog::saveTeams);
}

// Default destructor is sufficient as child widgets are handled by Qt

void TeamAssemblyDialog::createTeamGroupBox(int teamIndex, const QString& teamName)
{
    QGroupBox *teamXGroupBox = new QGroupBox(this);
    teamXGroupBox->setObjectName(QString("teamGroupBox%1").arg(teamIndex));
    teamXGroupBox->setTitle(QString("Team %1").arg(teamIndex + 1)); // Set title for identification

    QVBoxLayout *teamXLayout = new QVBoxLayout(teamXGroupBox);

    QLineEdit *teamNameEdit = new QLineEdit(teamName, this);
    teamNameEdit->setObjectName(QString("teamNameEdit%1").arg(teamIndex));
    teamNameEditLines.push_back(teamNameEdit);
    teamXLayout->addWidget(teamNameEdit);

    PlayerListWidget *teamXListWidget = new PlayerListWidget(this);
    teamXListWidget->setObjectName(QString("teamListWidget%1").arg(teamIndex));
    teamListWidgets.push_back(teamXListWidget);
    teamXLayout->addWidget(teamXListWidget, 1);

    // Find the teamsLayout which is inside the scroll area
    qobject_cast<QHBoxLayout*>(teamXGroupBox->parentWidget()->layout())->addWidget(teamXGroupBox);

    connect(teamXListWidget, QOverload<const PlayerInfo&, PlayerListWidget*, PlayerListWidget*>::of(&PlayerListWidget::playerDropped),
        this, &TeamAssemblyDialog::handlePlayerDropped);
}

void TeamAssemblyDialog::addTeam() {
    // Find the highest existing team ID to determine the new ID
    QSqlQuery maxIdQuery("SELECT MAX(id) FROM teams", database);
    int maxId = 0;
    if (maxIdQuery.exec() && maxIdQuery.next()) {
        maxId = maxIdQuery.value(0).toInt();
    }
    int newTeamId = maxId + 1;

    // Insert the new team into the database
    QSqlQuery insertQuery(database);
    insertQuery.prepare("INSERT INTO teams (id, name) VALUES (:id, :name)");
    insertQuery.bindValue(":id", newTeamId);
    insertQuery.bindValue(":name", QString("Team %1").arg(newTeamId));
    if (!insertQuery.exec()) {
        QMessageBox::critical(this, "Database Error", "Could not add the new team to the database.");
        return;
    }
    
    // Reload everything from the database to reflect the change
    loadActivePlayers();
}

void TeamAssemblyDialog::removeTeam() {
    // Find the highest team ID to remove
    QSqlQuery maxIdQuery("SELECT MAX(id) FROM teams", database);
    int maxId = 0;
    if (maxIdQuery.exec() && maxIdQuery.next()) {
        maxId = maxIdQuery.value(0).toInt();
    }

    if (maxId == 0) {
        QMessageBox::information(this, "No Teams", "There are no teams to remove.");
        return;
    }
    
    // For simplicity, we only allow removing the last team.
    // A more complex implementation could allow choosing which team to remove.
    QString teamNameToRemove = "team";
    QSqlQuery nameQuery(database);
    nameQuery.prepare("SELECT name FROM teams WHERE id = :id");
    nameQuery.bindValue(":id", maxId);
    if(nameQuery.exec() && nameQuery.next()) teamNameToRemove = nameQuery.value(0).toString();

    auto reply = QMessageBox::question(this, "Confirm Removal", 
        QString("Are you sure you want to remove '%1'? All players on this team will be unassigned.").arg(teamNameToRemove),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;

    database.transaction();
    QSqlQuery updatePlayersQuery(database);
    updatePlayersQuery.prepare("UPDATE players SET team_id = NULL WHERE team_id = :id");
    updatePlayersQuery.bindValue(":id", maxId);
    
    QSqlQuery deleteTeamQuery(database);
    deleteTeamQuery.prepare("DELETE FROM teams WHERE id = :id");
    deleteTeamQuery.bindValue(":id", maxId);

    if (updatePlayersQuery.exec() && deleteTeamQuery.exec()) {
        database.commit();
    } else {
        database.rollback();
        QMessageBox::critical(this, "Database Error", "Could not remove the team due to a database error.");
        return;
    }

    loadActivePlayers();
}

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
    QScrollArea *teamsScrollArea = new QScrollArea(this);
    teamsScrollArea->setWidgetResizable(true);
    QWidget *teamsScrollWidget = new QWidget;
    teamsLayout = new QHBoxLayout(teamsScrollWidget);
    teamsLayout->setObjectName("teamsLayout");
    teamsScrollWidget->setLayout(teamsLayout);
    teamsScrollArea->setWidget(teamsScrollWidget);

    // Buttons Layout
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    addTeamButton = new QPushButton(tr("Add Team"), this);
    removeTeamButton = new QPushButton(tr("Remove Team"), this);
    autoAssignButton = new QPushButton(tr("Auto-Assign")); 
    saveButton = new QPushButton(tr("Save Teams"));       
    closeButton = new QPushButton(tr("Close"));
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(addTeamButton);
    buttonsLayout->addWidget(removeTeamButton);
    buttonsLayout->addWidget(autoAssignButton); 
    buttonsLayout->addWidget(saveButton);       
    buttonsLayout->addWidget(closeButton);

    mainLayout->addWidget(activePlayersGroupBox);
    mainLayout->addWidget(teamsScrollArea, 1);
    mainLayout->addLayout(buttonsLayout);
}


void TeamAssemblyDialog::loadActivePlayers() {
    // Clear all internal data and UI elements
    availablePlayersData.clear();
    teamsData.clear();
    activePlayersListWidget->clear();

    QLayoutItem* item;
    while ((item = teamsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    teamListWidgets.clear();
    teamNameEditLines.clear();

    if (!database.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Database is not open."));
        return;
    }

    // Load teams from the database to build the UI
    QSqlQuery teamQuery(database);
    if (teamQuery.exec("SELECT id, name FROM teams ORDER BY id")) {
        while (teamQuery.next()) {
            TeamData team;
            team.id = teamQuery.value("id").toInt();
            team.name = teamQuery.value("name").toString();
            teamsData.push_back(team);

            // Create the UI group box for this team
            QGroupBox *teamXGroupBox = new QGroupBox(tr("Team %1").arg(team.id));
            QVBoxLayout *teamXLayout = new QVBoxLayout(teamXGroupBox);
            
            QLineEdit *teamNameEdit = new QLineEdit(team.name, this);
            teamNameEditLines.push_back(teamNameEdit);
            teamXLayout->addWidget(teamNameEdit);
            
            PlayerListWidget *teamXListWidget = new PlayerListWidget(this);
            teamListWidgets.push_back(teamXListWidget);
            teamXLayout->addWidget(teamXListWidget, 1);
            
            teamsLayout->addWidget(teamXGroupBox);

            connect(teamXListWidget, QOverload<const PlayerInfo&, PlayerListWidget*, PlayerListWidget*>::of(&PlayerListWidget::playerDropped),
                    this, &TeamAssemblyDialog::handlePlayerDropped);
        }
    } else {
        QMessageBox::critical(this, tr("Database Error"), tr("Failed to load teams: %1").arg(teamQuery.lastError().text()));
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

            if (teamId >= 1 && static_cast<size_t>(teamId) <= teamsData.size()) { // Player is assigned to a valid team
                int teamIndex = teamId - 1; // Convert to 0-based index
                if (teamIndex < teamsData.size()) {
                    teamsData[teamIndex].members.push_back(player);
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
    } else {
        QMessageBox::critical(this, tr("Database Query Error"), query.lastError().text());
    }
}

void TeamAssemblyDialog::handlePlayerDropped(const PlayerInfo& player, PlayerListWidget* sourceList, PlayerListWidget* targetList) {
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
                sourceData = &teamsData[i].members;
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
                targetData = &teamsData[i].members;
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
}

void TeamAssemblyDialog::autoAssignTeams() {
    std::vector<PlayerInfo> allPlayersToAssign;
    // Consolidate from internal models, as UI might not be source of truth during this operation
    for (const auto& p : availablePlayersData) allPlayersToAssign.push_back(p);
    for (const auto& team : teamsData) {
        for (const auto& p : team.members) allPlayersToAssign.push_back(p);
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
    for(auto& team : teamsData) team.members.clear();
    for(auto& team_ui_list : teamListWidgets) team_ui_list->clear();

    int teamIdx = 0;
    for(const auto& player : allPlayersToAssign) {
        teamsData[teamIdx].members.push_back(player);
        teamIdx = (teamIdx + 1) % teamsData.size(); 
    }

    // Repopulate UI lists from the new internal models
    for(const auto& player : availablePlayersData) { 
        activePlayersListWidget->addPlayer(player);
    }
    for(size_t i = 0; i < teamsData.size(); ++i) {
        for(const auto& player : teamsData[i].members) {
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

    // First, clear existing teams and re-insert to handle additions/deletions
    QSqlQuery clearTeamsQuery(database);
    if (!clearTeamsQuery.exec("DELETE FROM teams")) {
        qWarning() << "Failed to clear teams table:" << clearTeamsQuery.lastError().text();
        QSqlDatabase::database().rollback();
        return;
    }

    bool allSuccessful = true;

    // Save the team names and player assignments
    for (size_t i = 0; i < teamsData.size(); ++i) {
        QSqlQuery teamInsertQuery(database);
        teamInsertQuery.prepare("INSERT INTO teams (id, name) VALUES (:id, :name)");
        teamInsertQuery.bindValue(":id", teamsData[i].id);
        teamInsertQuery.bindValue(":name", teamNameEditLines[i]->text());
        if (!teamInsertQuery.exec()) {
            qWarning() << "Failed to save team name for team ID" << teamsData[i].id << ":" << teamInsertQuery.lastError().text();
            allSuccessful = false;
        }

        for (const auto& player : teamsData[i].members) {
            QSqlQuery query(database);
            query.prepare("UPDATE players SET team_id = :teamId WHERE id = :playerId");
            query.bindValue(":teamId", teamsData[i].id);
            query.bindValue(":playerId", player.id);
            if (!query.exec()) {
                qWarning() << "Failed to save team for player" << player.name << "(ID:" << player.id << "):" << query.lastError().text();
                allSuccessful = false;
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
        }
    }

    if (allSuccessful) {
        if (QSqlDatabase::database().commit()) {
            QMessageBox::information(this, tr("Save Successful"), tr("Team assignments have been saved to the database."));
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
