#ifndef PLAYERLISTWIDGET_H
#define PLAYERLISTWIDGET_H

#include <QListWidget>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QApplication> // For QApplication::startDragDistance()
#include <QDataStream>  // For serializing PlayerInfo
#include "CommonStructs.h" // Your PlayerInfo struct

// Define a custom MIME type for player data
const QString PLAYER_MIME_TYPE = "application/x-playerinfo";

class PlayerListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit PlayerListWidget(QWidget *parent = nullptr);
    PlayerInfo getPlayerInfoFromItem(QListWidgetItem *item) const;
    void addPlayer(const PlayerInfo& player);

signals:
    void playerDropped(const PlayerInfo& player, PlayerListWidget* targetList); // Emitted by target
    void playerAboutToBeRemoved(QListWidgetItem* item); // Emitted by source before removing

protected:
    // Dragging from this list
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void startDrag(); // Helper to initiate the drag

    // Dropping onto this list
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;


private:
    QPoint startPos; // To determine when a drag should start
    QListWidgetItem *draggedItem; // Keep track of the item being dragged from this list
};

#endif // PLAYERLISTWIDGET_H