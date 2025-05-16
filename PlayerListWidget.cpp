#include "PlayerListWidget.h"
#include <QDebug>

PlayerListWidget::PlayerListWidget(QWidget *parent)
    : QListWidget(parent), draggedItem(nullptr) {
    setDragEnabled(true);       // Important for allowing drags to start
    setAcceptDrops(true);       // Important for allowing drops
    setDropIndicatorShown(true); // Visual feedback
    // Set default drag drop mode if you only want to move items within the same widget
    // or copy items. For inter-widget movement, we handle it manually.
    // setDefaultDropAction(Qt::MoveAction);
}

PlayerInfo PlayerListWidget::getPlayerInfoFromItem(QListWidgetItem *item) const {
    PlayerInfo player;
    if (item) {
        player.id = item->data(Qt::UserRole).toInt();
        player.name = item->text();
        player.handicap = item->data(Qt::UserRole + 1).toInt();
    } else {
        player.id = -1; // Indicate invalid player
    }
    return player;
}

void PlayerListWidget::addPlayer(const PlayerInfo& player) {
    QListWidgetItem *item = new QListWidgetItem(player.name, this);
    item->setData(Qt::UserRole, player.id);
    item->setData(Qt::UserRole + 1, player.handicap);
    // addItem(item); // Already added by passing 'this' to QListWidgetItem constructor
}


// --- Dragging FROM this list ---

void PlayerListWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        startPos = event->pos(); // Store the starting position of the mouse press
        draggedItem = itemAt(event->pos()); // Get the item under the cursor
    }
    QListWidget::mousePressEvent(event); // Call base class implementation
}

void PlayerListWidget::mouseMoveEvent(QMouseEvent *event) {
    // If the left button is pressed and the mouse has moved sufficiently
    if ((event->buttons() & Qt::LeftButton) && draggedItem) {
        if ((event->pos() - startPos).manhattanLength() >= QApplication::startDragDistance()) {
            startDrag();
        }
    }
    QListWidget::mouseMoveEvent(event);
}

void PlayerListWidget::startDrag() {
    if (!draggedItem) return;

    PlayerInfo player = getPlayerInfoFromItem(draggedItem);
    if (player.id == -1) return;

    QMimeData *mimeData = new QMimeData;
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << player.id << player.name << player.handicap;

    mimeData->setData(PLAYER_MIME_TYPE, itemData);
    mimeData->setText(player.name); // For simple text drops if needed

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    // Optional: Set a pixmap for the drag cursor
    // QPixmap pixmap = draggedItem->icon().pixmap(32, 32); // or render the item to a pixmap
    // drag->setPixmap(pixmap);
    // drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));

    emit playerAboutToBeRemoved(draggedItem); // Signal that this item is being dragged out

    // Qt::MoveAction will remove the item from the source if the drop is successful on a target that accepts it.
    // If you want to copy, use Qt::CopyAction.
    // If the drop is on a different PlayerListWidget, we'll handle removal/addition manually in dropEvent.
    if (drag->exec(Qt::MoveAction) == Qt::MoveAction) {
        // If the action was a move, and the target didn't already handle removal (e.g., internal move),
        // or if we want to ensure it's removed from the source after a successful drop elsewhere.
        // For inter-widget moves, it's often better to remove the item from the source
        // *after* it has been successfully added to the target.
        // The current setup in dropEvent handles adding to target and source removal.
    }
    draggedItem = nullptr; // Reset after drag operation
}


// --- Dropping ONTO this list ---

void PlayerListWidget::dragEnterEvent(QDragEnterEvent *event) {
    // Check if the MIME data format is one we can handle
    if (event->mimeData()->hasFormat(PLAYER_MIME_TYPE)) {
        event->acceptProposedAction(); // Accept the drag
    } else {
        event->ignore(); // Reject the drag
    }
}

void PlayerListWidget::dragMoveEvent(QDragMoveEvent *event) {
    // Can be used to change the drop action (e.g., based on modifier keys)
    // or to do fine-grained checks.
    if (event->mimeData()->hasFormat(PLAYER_MIME_TYPE)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void PlayerListWidget::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat(PLAYER_MIME_TYPE)) {
        QByteArray itemData = event->mimeData()->data(PLAYER_MIME_TYPE);
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        PlayerInfo player;
        dataStream >> player.id >> player.name >> player.handicap;

        if (player.id != -1) {
            // Add the player to this list
            this->addPlayer(player);

            // If the source was another PlayerListWidget, remove the item from it.
            PlayerListWidget *sourceList = qobject_cast<PlayerListWidget*>(event->source());
            if (sourceList && sourceList != this) {
                // Find and remove the item from the source list.
                // This is a bit simplified; ideally, the source's `draggedItem` or
                // the `playerAboutToBeRemoved` signal would be used more directly.
                for (int i = 0; i < sourceList->count(); ++i) {
                    QListWidgetItem* itemInSource = sourceList->item(i);
                    if (sourceList->getPlayerInfoFromItem(itemInSource).id == player.id) {
                        delete sourceList->takeItem(i);
                        break;
                    }
                }
            }
            event->acceptProposedAction();
            emit playerDropped(player, this);
            return;
        }
    }
    event->ignore();
}

void PlayerListWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    // Called when a drag operation leaves the widget
    QListWidget::dragLeaveEvent(event);
}