/**
 * @file DailyLeaderboardWidget.cpp
 * @brief Implements the DailyLeaderboardWidget class.
 */

#include "DailyLeaderboardWidget.h"
#include <QSqlDatabase>
#include <QDebug>
#include <QFontMetrics>

DailyLeaderboardWidget::DailyLeaderboardWidget(const QString &connectionName, int dayNum, QWidget *parent)
    : QWidget(parent), m_connectionName(connectionName), m_dayNum(dayNum),
      leaderboardModel(new DailyLeaderboardModel(m_connectionName, m_dayNum, this)),
      leaderboardView(new QTableView(this))
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << QString("DailyLeaderboardWidget (Day %1): ERROR: Invalid or closed database connection passed to constructor.").arg(m_dayNum);
    }

    leaderboardView->setModel(leaderboardModel);
    configureTableView();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(leaderboardView);
    setLayout(mainLayout);
}

DailyLeaderboardWidget::~DailyLeaderboardWidget()
{
}

QSqlDatabase DailyLeaderboardWidget::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

void DailyLeaderboardWidget::configureTableView()
{
    leaderboardView->verticalHeader()->setVisible(false);
    leaderboardView->setSelectionBehavior(QAbstractItemView::SelectRows);
    leaderboardView->setSelectionMode(QAbstractItemView::NoSelection);
    leaderboardView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leaderboardView->horizontalHeader()->setStretchLastSection(true);

    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForRank(), QHeaderView::ResizeToContents);
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForPlayerName(), QHeaderView::Stretch);
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForDailyTotalPoints(), QHeaderView::ResizeToContents);
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForDailyNetPoints(), QHeaderView::ResizeToContents);
}

void DailyLeaderboardWidget::refreshData()
{
    leaderboardModel->refreshData();
}

QImage DailyLeaderboardWidget::exportToImage() const
{
    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount();

    if (rowCount == 0 || colCount == 0) {
        qDebug() << QString("DailyLeaderboardWidget (Day %1): exportToImage: No data available to export.").arg(m_dayNum);
        return QImage();
    }

    int titleHeight = 120;
    int headerHeight = 80;
    int rowHeight = 60;
    int padding = 0;

    QVector<int> columnWidths;
    columnWidths << 120;  // Rank
    columnWidths << 400; // Player Name
    columnWidths << 200; // Daily Total
    columnWidths << 200; // Daily Net

    int totalWidth = padding * 2;
    for (int width : columnWidths) {
        totalWidth += width;
    }

    int totalHeight = padding * 2;
    totalHeight += titleHeight;
    totalHeight += headerHeight;
    totalHeight += rowCount * rowHeight;

    if (totalWidth < 400) totalWidth = 400;
    if (totalHeight < 200) totalHeight = 200;

    QImage image(totalWidth, totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    if (!painter.isActive()) {
        qDebug() << QString("DailyLeaderboardWidget (Day %1): exportToImage: QPainter failed to start.").arg(m_dayNum);
        return QImage();
    }

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);

    painter.setFont(QFont("Arial", 32, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - padding * 2, titleHeight);
    painter.fillRect(titleRect, Qt::black);
    painter.drawText(titleRect, Qt::AlignCenter, QString("Day %1 Leaderboard").arg(m_dayNum));

    painter.setFont(QFont("Arial", 24, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;

    for (int i = 0; i < colCount; ++i) {
        QRect headerRect(currentX, currentY, columnWidths.at(i), headerHeight);
        painter.fillRect(headerRect, Qt::black);
        painter.drawText(headerRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(i, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += columnWidths.at(i);
    }

    painter.setFont(QFont("Arial", 20));
    painter.setPen(Qt::black);
    currentY += headerHeight;

    for (int row = 0; row < rowCount; ++row) {
        currentX = padding;

        QColor rowColor = ((row % 2) == 0) ? Qt::white : QColor(240, 240, 240);

        QVariant rankData = leaderboardModel->data(leaderboardModel->index(row, leaderboardModel->getColumnForRank()), Qt::DisplayRole);
        bool ok;
        int rank = rankData.toInt(&ok);
        if (ok && rank >= 1 && rank <= 3) {
            rowColor = QColor(255, 165, 0, 255);
        }

        painter.fillRect(padding, currentY, totalWidth - padding * 2, rowHeight, rowColor);

        for (int i = 0; i < colCount; ++i) {
            QRect dataRect(currentX, currentY, columnWidths.at(i), rowHeight);
            painter.drawText(dataRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, i), Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->data(leaderboardModel->index(row, i), Qt::DisplayRole).toString());
            currentX += columnWidths.at(i);
        }

        currentY += rowHeight;
    }

    painter.setPen(Qt::black);
    currentY = padding + titleHeight + headerHeight;
    for (int row = 0; row <= rowCount; ++row) {
        painter.drawLine(padding, currentY, totalWidth - padding, currentY);
        currentY += rowHeight;
    }

    QVector<int> verticalLineXPositions;
    verticalLineXPositions << padding;
    currentX = padding;
    for (int width : columnWidths) {
        currentX += width;
        verticalLineXPositions << currentX;
    }

    currentY = padding + titleHeight;
    for (int xPos : verticalLineXPositions) {
        painter.drawLine(xPos, currentY, xPos, totalHeight - padding);
    }

    painter.end();
    return image;
}
