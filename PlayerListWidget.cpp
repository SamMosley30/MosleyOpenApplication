/**
 * @file PlayerListWidget.cpp
 * @brief Implements the PlayerListWidget class.
 */

#include "PlayerListWidget.h"
#include <QDebug>
#include <QApplication>
#include <QDataStream>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>

PlayerListWidget::PlayerListWidget(QWidget *parent)
    : QListWidget(parent), draggedItem(nullptr) {
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
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
            startDrag();
        }
    }
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
    QByteArray itemDataByteArray;
    QDataStream dataStream(&itemDataByteArray, QIODevice::WriteOnly);
    dataStream << player.id << player.name << player.handicap;

    mimeData->setData(PLAYER_MIME_TYPE, itemDataByteArray);
    mimeData->setText(player.name);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    if (drag->exec(Qt::MoveAction) == Qt::MoveAction) {
        delete takeItem(row(draggedItem));
    } else {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Drag for" << player.name << "did not result in MoveAction (e.g., cancelled).";
    }
    draggedItem = nullptr;
}

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

    if (sourceListWidget == this) {
        event->acceptProposedAction();
        return;
    }

    QByteArray itemDataByteArray = event->mimeData()->data(PLAYER_MIME_TYPE);
    QDataStream dataStream(&itemDataByteArray, QIODevice::ReadOnly);
    PlayerInfo player;
    dataStream >> player.id >> player.name >> player.handicap;

    if (player.id != -1) {
        this->addPlayer(player);
        event->acceptProposedAction();
        emit playerDropped(player, sourceListWidget, this);
    } else {
        qDebug() << "PlayerListWidget (" << this->objectName() << "): Drop ignored - invalid player ID from MIME data.";
        event->ignore();
    }
}

void PlayerListWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    QListWidget::dragLeaveEvent(event);
}
