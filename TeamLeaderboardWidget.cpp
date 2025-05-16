#include "TeamLeaderboardWidget.h"
#include <QSqlDatabase>
#include <QDebug>
#include <QPainter> // For exportToImage
#include <QFileDialog> // For exportToImage (if saving directly)
#include <QMessageBox> // For exportToImage messages

TeamLeaderboardWidget::TeamLeaderboardWidget(const QString &connectionName, QWidget *parent)
    : QWidget(parent),
      m_connectionName(connectionName),
      leaderboardModel(new TeamLeaderboardModel(connectionName, this)),
      leaderboardView(new QTableView(this)) {

    leaderboardView->setModel(leaderboardModel);
    configureTableView();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(leaderboardView);
    setLayout(mainLayout);
}

TeamLeaderboardWidget::~TeamLeaderboardWidget() {}

QSqlDatabase TeamLeaderboardWidget::database() const {
    return QSqlDatabase::database(m_connectionName);
}

void TeamLeaderboardWidget::configureTableView() {
    leaderboardView->verticalHeader()->setVisible(false);
    leaderboardView->setSelectionBehavior(QAbstractItemView::SelectRows);
    leaderboardView->setSelectionMode(QAbstractItemView::NoSelection);
    leaderboardView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leaderboardView->horizontalHeader()->setStretchLastSection(true);

    // Column widths: Rank, Team Name, Day 1, Day 2, Day 3, Overall
    leaderboardView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Rank
    leaderboardView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);        // Team Name
    leaderboardView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Day 1
    leaderboardView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Day 2
    leaderboardView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Day 3
    leaderboardView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // Overall
}

void TeamLeaderboardWidget::refreshData() {
    qDebug() << "TeamLeaderboardWidget: Refreshing data...";
    leaderboardModel->refreshData();
    updateColumnVisibility();
    qDebug() << "TeamLeaderboardWidget: Data refreshed.";
}

void TeamLeaderboardWidget::updateColumnVisibility() {
    QSet<int> daysWithScores = leaderboardModel->getDaysWithScores();
    // Columns: 0:Rank, 1:Team, 2:Day1, 3:Day2, 4:Day3, 5:Overall
    leaderboardView->setColumnHidden(2, !daysWithScores.contains(1)); // Hide Day 1 if no scores
    leaderboardView->setColumnHidden(3, !daysWithScores.contains(2)); // Hide Day 2 if no scores
    leaderboardView->setColumnHidden(4, !daysWithScores.contains(3)); // Hide Day 3 if no scores
    qDebug() << "TeamLeaderboardWidget: Column visibility updated based on days with scores:" << daysWithScores;
}


QImage TeamLeaderboardWidget::exportToImage() const {
    // Basic implementation, can be enhanced like your TournamentLeaderboardWidget::exportToImage
    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount();

    if (rowCount == 0 || colCount == 0) {
        qDebug() << "TeamLeaderboardWidget::exportToImage: No data to export.";
        // QMessageBox::information(const_cast<TeamLeaderboardWidget*>(this), "Export Image", "No data available to export.");
        return QImage();
    }

    // Estimate sizes
    int titleHeight = 50;
    int headerHeight = 30;
    int rowHeight = 25;
    int padding = 15;
    QVector<int> columnWidths = {60, 150, 80, 80, 80, 100}; // Rank, Team, D1, D2, D3, Overall

    int totalWidth = padding * 2;
    for(int i=0; i < colCount; ++i) {
        if (!leaderboardView->isColumnHidden(i)) {
            totalWidth += columnWidths.at(i);
        }
    }
    
    int totalHeight = padding * 2 + titleHeight + headerHeight + (rowCount * rowHeight);

    QImage image(totalWidth, totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // Title
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - 2 * padding, titleHeight);
    painter.drawText(titleRect, Qt::AlignCenter, "Team Leaderboard");

    // Headers
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;
    for (int col = 0; col < colCount; ++col) {
        if (leaderboardView->isColumnHidden(col)) continue;
        QRect headerRect(currentX, currentY, columnWidths.at(col), headerHeight);
        painter.drawText(headerRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(col, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += columnWidths.at(col);
    }

    // Data Rows
    painter.setFont(QFont("Arial", 9));
    currentY += headerHeight;
    for (int row = 0; row < rowCount; ++row) {
        currentX = padding;
        QColor rowColor = (row % 2 == 0) ? Qt::white : QColor(240, 240, 240);
        painter.fillRect(padding, currentY, totalWidth - 2 * padding, rowHeight, rowColor);

        for (int col = 0; col < colCount; ++col) {
            if (leaderboardView->isColumnHidden(col)) continue;
            QRect dataRect(currentX, currentY, columnWidths.at(col), rowHeight);
            painter.drawText(dataRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, col), Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->data(leaderboardModel->index(row, col), Qt::DisplayRole).toString());
            currentX += columnWidths.at(col);
        }
        currentY += rowHeight;
    }
    
    painter.end();
    qDebug() << "TeamLeaderboardWidget: Image exported.";
    return image;
}
