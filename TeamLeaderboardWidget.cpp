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
}


QImage TeamLeaderboardWidget::exportToImage() const {
    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount();

    if (rowCount == 0 || colCount == 0) 
    {
        qDebug() << "TeamLeaderboardWidget::exportToImage: No data to export.";
        return QImage();
    }

    // Estimate sizes
    int titleHeight = 200;
    int headerHeight = 120;
    int rowHeight = 100;
    int padding = 15;
    QVector<int> columnWidths = {180, 550, 400, 400, 400, 440}; // Rank, Team, D1, D2, D3, Overall

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
    painter.setPen(Qt::white);

    // Title
    painter.setFont(QFont("Arial", 64, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - 2 * padding, titleHeight);
    painter.fillRect(titleRect, Qt::black);
    painter.drawText(titleRect, Qt::AlignCenter, "Team Leaderboard");

    // Headers
    painter.setFont(QFont("Arial", 48, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;
    for (int col = 0; col < colCount; ++col) {
        if (leaderboardView->isColumnHidden(col)) continue;
        QRect headerRect(currentX, currentY, columnWidths.at(col), headerHeight);
        painter.fillRect(headerRect, Qt::black);
        painter.drawText(headerRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(col, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += columnWidths.at(col);
    }

    // Data Rows
    painter.setFont(QFont("Arial", 36));
    painter.setPen(Qt::black);
    currentY += headerHeight;
    for (int row = 0; row < rowCount; ++row) {
        currentX = padding;
        QColor rowColor = (row % 2 == 0) ? Qt::white : QColor(240, 240, 240);
        if (leaderboardModel->data(leaderboardModel->index(row, 0), Qt::DisplayRole).toInt() <= 1)
            rowColor = QColor(255, 165, 0, 255);

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

    // Draw horizontal lines
    currentY = padding + titleHeight + headerHeight;
    for (int row = 0; row <= rowCount; ++row) { // Draw line above headers and below each row
         painter.drawLine(padding, currentY, totalWidth - padding, currentY);
         currentY += rowHeight;
    }

    // Draw the vertical lines
    currentX = padding;
    currentY = padding + titleHeight + headerHeight;
    painter.drawLine(currentX, padding, currentX, totalHeight - padding);
    for (int col = 0; col < colCount; ++col) {
        if (leaderboardView->isColumnHidden(col)) continue;
        int colWidth = (col < columnWidths.size()) ? columnWidths.at(col) : 440;
        painter.drawLine(currentX, currentY, currentX, totalHeight - padding);
        currentX += colWidth;
    }
    painter.drawLine(currentX, padding, currentX, totalHeight - padding);
    
    painter.end();
    qDebug() << "TeamLeaderboardWidget: Image exported.";
    return image;
}
