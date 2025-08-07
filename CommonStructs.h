/**
 * @file CommonStructs.h
 * @brief Contains common data structures used throughout the application.
 */

#ifndef COMMONSTRUCTS_H
#define COMMONSTRUCTS_H

#include <QString>

/**
 * @struct PlayerInfo
 * @brief Holds information about an active player.
 *
 * This structure is used to store the basic details of a player,
 * such as their ID, name, and handicap.
 */
struct PlayerInfo {
    int id;             ///< The unique identifier for the player.
    QString name;       ///< The name of the player.
    int handicap;       ///< The player's handicap.
};

#endif // COMMONSTRUCTS_H