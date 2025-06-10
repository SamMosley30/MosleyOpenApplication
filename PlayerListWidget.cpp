#include "PlayerListWidget.h"
#include <QDebug>
#include <QApplication> // For QApplication::startDragDistance()
#include <QDataStream>  // For serializing PlayerInfo
#include <QMimeData>    // For QMimeData
#include <QDrag>        // For QDrag
#include <QMouseEvent>  // For QMouseEvent / QDragEnterEvent etc.

PlayerListWidget::PlayerListWidget(QWidget *parent)
    : QListWidget(parent), draggedItem(nullptr) {
    setDragEnabled(true);       
    setAcceptDrops(true);       
    setDropIndicatorShown(true); 
    // We are manually handling the move/copy logic in conjunction with QDrag,
    // so explicit setDefaultDropAction() or setDragDropMode() might interfere
    // or be redundant if not carefully managed with the overridden events.
    // The key is how drag->exec() and our dropEvent interact.
}

PlayerInfo PlayerListWidget::getPlayerInfoFromItem(QListWidgetItem *item) const {
    PlayerInfo player;
    if (item) {
        player.id = item->data(Qt::UserRole).toInt();
        player.name = item->text();
        player.handicap = item->data(Qt::UserRole + 1).toInt();
    } else {
        player.id = -1; 
    }
    return player;
}

void PlayerListWidget::addPlayer(const PlayerInfo& player) {
    // Check if a player with the same ID already exists to prevent UI duplicates
    // This is a safeguard, the main duplication is handled in dropEvent for self-drops.
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(Qt::UserRole).toInt() == player.id) {
            qDebug() << "PlayerListWidget::addPlayer - Player" << player.name << "already exists in this list. Not re-adding.";
            return; 
        }
    }
    QListWidgetItem *item = new QListWidgetItem(player.name, this);
    item->setData(Qt::UserRole, player.id);
    item->setData(Qt::UserRole + 1, player.handicap);
}

// --- Dragging FROM this list ---

void PlayerListWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        startPos = event->pos(); 
        draggedItem = itemAt(event->pos()); 
    }
    QListWidget::mousePressEvent(event); 
}

void PlayerListWidget::mouseMoveEvent(QMouseEvent *event) {
    if ((event->buttons() & Qt::LeftButton) && draggedItem) {
        if ((event->pos() - startPos).manhattanLength() >= QApplication::startDragDistance()) {
            startDrag(); // Call our drag initiation
        }
    }
    // No call to QListWidget::mouseMoveEvent(event) here, as we are fully handling
    // the drag initiation if conditions are met. If not met, default behavior is fine.
}

void PlayerListWidget::startDrag() {
    if (!draggedItem) {
        qDebug() << "startDrag called with no draggedItem.";
        return;
    }

    PlayerInfo player = getPlayerInfoFromItem(draggedItem);
    if (player.id == -1) {
        qDebug() << "startDrag: Invalid player data from draggedItem.";
        return;
    }

    QMimeData *mimeData = new QMimeData;
    QByteArray itemDataByteArray; // Renamed for clarity
    QDataStream dataStream(&itemDataByteArray, QIODevice::WriteOnly);
    dataStream << player.id << player.name << player.handicap;

    mimeData->setData(PLAYER_MIME_TYPE, itemDataByteArray);
    mimeData->setText(player.name); 

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // If the move is successful, remove the item from the source list.
    if (drag->exec(Qt::MoveAction) == Qt::MoveAction) {
        // FIX: Manually delete the item from the source list widget.
        delete takeItem(row(draggedItem));
        
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Drag for" << player.name << "completed with MoveAction.";
    } else {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Drag for" << player.name << "did not result in MoveAction (e.g., cancelled).";
    }
    draggedItem = nullptr; // Reset after drag operation
}


// --- Dropping ONTO this list ---

void PlayerListWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat(PLAYER_MIME_TYPE)) {
        event->acceptProposedAction(); 
    } else {
        event->ignore(); 
    }
}

void PlayerListWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat(PLAYER_MIME_TYPE)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void PlayerListWidget::dropEvent(QDropEvent *event) {
    if (!event->mimeData()->hasFormat(PLAYER_MIME_TYPE)) {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Drop ignored - wrong MIME type.";
        event->ignore();
        return;
    }

    PlayerListWidget *sourceListWidget = qobject_cast<PlayerListWidget*>(event->source());

    // Case 1: Drag and drop within the SAME list (reordering)
    if (sourceListWidget == this) {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Internal drop (reorder). Accepting.";
        // The item is just being reordered.
        // The QDrag operation with Qt::MoveAction (from startDrag) should handle the actual move of the item
        // in the UI. We don't need to add it again or explicitly remove it from itself.
        // We just accept the event to allow Qt to complete the move/reorder.
        event->acceptProposedAction();
        // Calling the base class QListWidget::dropEvent(event) might be necessary if Qt's default
        // drag-and-drop handling for internal moves isn't automatically reordering the visual item
        // when only acceptProposedAction() is called in an overridden dropEvent.
        // For now, let's assume acceptProposedAction() is sufficient for Qt to handle the reorder.
        // If items visually disappear or don't reorder on self-drop, uncommenting the next line is a good test.
        // QListWidget::dropEvent(event); 
        return; 
    }

    // Case 2: Drop from a DIFFERENT PlayerListWidget
    qDebug() << "PlayerListWidget (" << this->objectName() << "): Drop received from external source:" << (sourceListWidget ? sourceListWidget->objectName() : "Unknown source");
    QByteArray itemDataByteArray = event->mimeData()->data(PLAYER_MIME_TYPE);
    QDataStream dataStream(&itemDataByteArray, QIODevice::ReadOnly);
    PlayerInfo player;
    dataStream >> player.id >> player.name >> player.handicap;

    if (player.id != -1) {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Adding player from drop:" << player.name;
        this->addPlayer(player); // Add the player to this (target) list's UI
        event->acceptProposedAction(); // Accept the drop action.
        
        emit playerDropped(player, sourceListWidget, this); 
    } else {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Drop ignored - invalid player ID from MIME data.";
        event->ignore();
    }
}

void PlayerListWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    QListWidget::dragLeaveEvent(event);
}
