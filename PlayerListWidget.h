/**
 * @file PlayerListWidget.h
 * @brief Contains the declaration of the PlayerListWidget class.
 */

#ifndef PLAYERLISTWIDGET_H
#define PLAYERLISTWIDGET_H

#include <QListWidget>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QApplication>
#include <QDataStream>
#include "CommonStructs.h"

/**
 * @brief Custom MIME type for player data.
 */
const QString PLAYER_MIME_TYPE = "application/x-playerinfo";

/**
 * @class PlayerListWidget
 * @brief A custom QListWidget that supports drag and drop of players.
 *
 * This widget is used to display lists of players and allows them to be
 * dragged and dropped between different PlayerListWidget instances.
 */
class PlayerListWidget : public QListWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a PlayerListWidget object.
     * @param parent The parent widget.
     */
    explicit PlayerListWidget(QWidget *parent = nullptr);

    /**
     * @brief Gets the PlayerInfo from a QListWidgetItem.
     * @param item The item to get the data from.
     * @return The PlayerInfo associated with the item.
     */
    PlayerInfo getPlayerInfoFromItem(QListWidgetItem *item) const;

    /**
     * @brief Adds a player to the list.
     * @param player The player to add.
     */
    void addPlayer(const PlayerInfo& player);

signals:
    /**
     * @brief Emitted when a player is dropped onto the list.
     * @param player The player that was dropped.
     * @param sourceList The list the player was dragged from.
     * @param targetList The list the player was dropped onto.
     */
    void playerDropped(const PlayerInfo& player, PlayerListWidget* sourceList, PlayerListWidget* targetList);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void startDrag();
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

private:
    QPoint startPos;             ///< The starting position of a drag operation.
    QListWidgetItem *draggedItem; ///< The item being dragged from this list.
};

#endif // PLAYERLISTWIDGET_H