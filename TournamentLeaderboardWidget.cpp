/**
 * @file TournamentLeaderboardWidget.cpp
 * @brief Implements the TournamentLeaderboardWidget class.
 */

#include "TournamentLeaderboardWidget.h"
#include "TournamentLeaderboardModel.h"

#include <QSqlDatabase>
#include <QDebug>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QTableView>
#include <QSqlError>

TournamentLeaderboardWidget::TournamentLeaderboardWidget(const QString &connectionName, QWidget *parent)
    : QWidget(parent),
      m_connectionName(connectionName),
      leaderboardModel(nullptr),
      leaderboardView(nullptr) {

    QString nameToPassToModel = this->m_connectionName;
    this->leaderboardModel = new TournamentLeaderboardModel(nameToPassToModel, this); 

    this->leaderboardView = new QTableView(this);

    leaderboardView->setModel(leaderboardModel);
    configureTableView();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(leaderboardView);
    setLayout(mainLayout);
}

TournamentLeaderboardWidget::~TournamentLeaderboardWidget() {}

QSqlDatabase TournamentLeaderboardWidget::database() const {
    QSqlDatabase db_check = QSqlDatabase::database(m_connectionName, false);
     if (!db_check.isValid()) {
        qWarning() << "TournamentLeaderboardWidget::database() - Connection name '" << m_connectionName << "' is NOT VALID. Available:" << QSqlDatabase::connectionNames();
    } else if (!db_check.isOpen()) {
        qWarning() << "TournamentLeaderboardWidget::database() - Connection '" << m_connectionName << "' is VALID but NOT OPEN. Last error:" << db_check.lastError().text();
    }
    return db_check;
}

/**
 * @brief Configures the table view settings.
 */
void TournamentLeaderboardWidget::configureTableView() {
    if (!leaderboardView) return;
    leaderboardView->verticalHeader()->setVisible(false);
    leaderboardView->setSelectionBehavior(QAbstractItemView::SelectRows);
    leaderboardView->setSelectionMode(QAbstractItemView::NoSelection);
    leaderboardView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leaderboardView->horizontalHeader()->setStretchLastSection(true);

    leaderboardView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);        
    leaderboardView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents); 
    leaderboardView->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents); 
}

void TournamentLeaderboardWidget::refreshData() {
    if (leaderboardModel) { 
        leaderboardModel->refreshData(); 
    } else {
        qWarning() << "TournamentLeaderboardWidget instance" << this << ": leaderboardModel is null in refreshData()!";
    }
    updateColumnVisibility(); 
}

/**
 * @brief Updates the visibility of the daily score columns.
 */
void TournamentLeaderboardWidget::updateColumnVisibility() {
    if (!leaderboardModel || !leaderboardView) return; 

    QSet<int> daysWithScores = leaderboardModel->getDaysWithScores();
    bool day1HasScores = daysWithScores.contains(1);
    bool day2HasScores = daysWithScores.contains(2);
    bool day3HasScores = daysWithScores.contains(3);

    leaderboardView->setColumnHidden(3, !day1HasScores); 
    leaderboardView->setColumnHidden(4, !day1HasScores); 
    leaderboardView->setColumnHidden(5, !day2HasScores); 
    leaderboardView->setColumnHidden(6, !day2HasScores); 
    leaderboardView->setColumnHidden(7, !day3HasScores); 
    leaderboardView->setColumnHidden(8, !day3HasScores);
}

QImage TournamentLeaderboardWidget::exportToImage() const {
    if (!leaderboardModel || !leaderboardView) return QImage();

    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount();

    if (rowCount == 0 || colCount == 0) {
        qDebug() << "TournamentLeaderboardWidget::exportToImage: No data to export.";
        return QImage();
    }

    int titleHeight = 100;
    int headerHeight = 60;
    int rowHeight = 50;
    int padding = 15;
    QVector<int> columnWidths = {100, 220, 220, 220, 220, 220, 220, 220, 220, 200}; 

    int totalWidth = padding * 2;
    for(int i=0; i < colCount; ++i) {
        if (!leaderboardView->isColumnHidden(i)) {
            if (i < columnWidths.size()) totalWidth += columnWidths.at(i);
            else totalWidth += 220; 
        }
    }
    
    int totalHeight = padding * 2 + titleHeight + headerHeight + (rowCount * rowHeight);

    QImage image(totalWidth, totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing);

    QString leaderboardTitle = "Leaderboard";
    if (!this->windowTitle().isEmpty() && this->windowTitle() != "QWidget") {
        leaderboardTitle = this->windowTitle();
    } else if (leaderboardModel->getTournamentContext() == TournamentLeaderboardModel::MosleyOpen) {
        leaderboardTitle = "Mosley Open";
    } else if (leaderboardModel->getTournamentContext() == TournamentLeaderboardModel::TwistedCreek) {
        leaderboardTitle = "Twisted Creek";
    }

    painter.setFont(QFont("Arial", 32, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - 2 * padding, titleHeight);
    painter.fillRect(titleRect, Qt::black);
    painter.drawText(titleRect, Qt::AlignCenter, leaderboardTitle);

    painter.setFont(QFont("Arial", 24, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;
    for (int col = 0; col < colCount; ++col) {
        if (leaderboardView->isColumnHidden(col)) continue;
        int colWidth = (col < columnWidths.size()) ? columnWidths.at(col) : 80;
        QRect headerRect(currentX, currentY, colWidth, headerHeight);
        painter.fillRect(headerRect, Qt::black);
        painter.drawText(headerRect, 
                         static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(col, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += colWidth;
    }

    painter.setFont(QFont("Arial", 18));
    painter.setPen(Qt::black);
    painter.drawLine(padding, padding, totalWidth - padding, padding);
    currentY += headerHeight;
    for (int row = 0; row < rowCount; ++row) {
        currentX = padding;
        QColor rowColor = (row % 2 == 0) ? Qt::white : QColor(240, 240, 240);
        if (leaderboardModel->data(leaderboardModel->index(row, 0), Qt::DisplayRole).toInt() <= 3) {
            rowColor = QColor(255, 165, 0, 255);
        }
        
        int visibleRowWidth = 0;
        for(int col_idx = 0; col_idx < colCount; ++col_idx) {
            if(!leaderboardView->isColumnHidden(col_idx)) {
                 visibleRowWidth += (col_idx < columnWidths.size()) ? columnWidths.at(col_idx) : 220;
            }
        }
        painter.fillRect(padding, currentY, visibleRowWidth, rowHeight, rowColor);
        painter.drawLine(currentX, currentY + rowHeight, totalWidth - padding, currentY + rowHeight);

        for (int col = 0; col < colCount; ++col) {
            if (leaderboardView->isColumnHidden(col)) continue;
            int colWidth = (col < columnWidths.size()) ? columnWidths.at(col) : 220;
            QRect dataRect(currentX, currentY, colWidth, rowHeight);
            painter.drawText(dataRect, 
                             static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, col), Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->data(leaderboardModel->index(row, col), Qt::DisplayRole).toString());
            currentX += colWidth;
        }
        currentY += rowHeight;
    }

    currentX = padding;
    currentY = padding + titleHeight + headerHeight;
    painter.drawLine(currentX, padding, currentX, totalHeight - padding);
    for (int col = 0; col < colCount; ++col) {
        if (leaderboardView->isColumnHidden(col)) continue;
        int colWidth = (col < columnWidths.size()) ? columnWidths.at(col) : 220;
        painter.drawLine(currentX, currentY, currentX, totalHeight - padding);
        currentX += colWidth;
    }
    painter.drawLine(currentX, padding, currentX, totalHeight - padding);
    
    painter.end();
    return image;
}
