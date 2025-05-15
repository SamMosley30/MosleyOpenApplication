#ifndef COMMONSTRUCTS_H
#define COMMONSTRUCTS_H

#include <QString>

// Structure to hold information about an active player
struct PlayerInfo {
    int id;
    QString name;
    int handicap;
    // Add other player details if needed for display or calculation
};

#endif // COMMONSTRUCTS_H