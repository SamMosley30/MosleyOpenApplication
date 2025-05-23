#include "TournamentLeaderboardWidget.h" 
#include "TournamentLeaderboardModel.h" 

#include <QSqlDatabase>
#include <QDebug>
#include <QPainter> 
#include <QFileDialog> 
#include <QMessageBox> 
#include <QVBoxLayout> // Required for QVBoxLayout
#include <QTableView>   // Required for QTableView
#include <QSqlError>

// Constructor for TournamentLeaderboardWidget (used by Mosley & Twisted Creek tabs)
TournamentLeaderboardWidget::TournamentLeaderboardWidget(const QString &connectionName, QWidget *parent)
    : QWidget(parent),
      m_connectionName(connectionName), // (1) Parameter 'connectionName' is stored in member 'm_connectionName'
      leaderboardModel(nullptr),      // Initialize pointers to nullptr
      leaderboardView(nullptr) {

    qDebug() << "TournamentLeaderboardWidget instance" << this << "CONSTRUCTOR START.";
    qDebug() << "  Parameter 'connectionName':" << connectionName;
    qDebug() << "  Member 'm_connectionName' (after direct initialization):" << this->m_connectionName;

    // (2) Member 'm_connectionName' is used to construct the model
    // Create model in the constructor body for more precise debugging
    QString nameToPassToModel = this->m_connectionName;
    qDebug() << "  Name being passed to TournamentLeaderboardModel constructor:" << nameToPassToModel;
    
    this->leaderboardModel = new TournamentLeaderboardModel(nameToPassToModel, this); 
    
    qDebug() << "  TournamentLeaderboardModel instance created at address:" << this->leaderboardModel;
    qDebug() << "  (Check model's constructor log for the name it actually received)";

    this->leaderboardView = new QTableView(this);

    leaderboardView->setModel(leaderboardModel); // Set model on view
    configureTableView();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(leaderboardView);
    setLayout(mainLayout);
    qDebug() << "TournamentLeaderboardWidget instance" << this << "CONSTRUCTOR END.";
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
    qDebug() << "TournamentLeaderboardWidget instance" << this << ": Refreshing data...";
    if (leaderboardModel) { 
        leaderboardModel->refreshData(); 
    } else {
        qWarning() << "TournamentLeaderboardWidget instance" << this << ": leaderboardModel is null in refreshData()!";
    }
    updateColumnVisibility(); 
    qDebug() << "TournamentLeaderboardWidget instance" << this << ": Data refresh complete.";
}

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
    
    qDebug() << "TournamentLeaderboardWidget instance" << this << ": Column visibility updated. Days with scores:" << daysWithScores;
}


QImage TournamentLeaderboardWidget::exportToImage() const {
    if (!leaderboardModel || !leaderboardView) return QImage();

    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount();

    if (rowCount == 0 || colCount == 0) {
        qDebug() << "TournamentLeaderboardWidget::exportToImage: No data to export.";
        return QImage();
    }

    int titleHeight = 50;
    int headerHeight = 30;
    int rowHeight = 25;
    int padding = 15;
    QVector<int> columnWidths = {50, 150, 60, 70, 70, 70, 70, 70, 70, 100}; 

    int totalWidth = padding * 2;
    for(int i=0; i < colCount; ++i) {
        if (!leaderboardView->isColumnHidden(i)) {
            if (i < columnWidths.size()) totalWidth += columnWidths.at(i);
            else totalWidth += 80; 
        }
    }
    
    int totalHeight = padding * 2 + titleHeight + headerHeight + (rowCount * rowHeight);

    QImage image(totalWidth, totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    QString leaderboardTitle = "Leaderboard"; // Generic title
    // Potentially get a more specific title if the dialog sets one on the widget
    if (!this->windowTitle().isEmpty() && this->windowTitle() != "QWidget") { // Basic check
        leaderboardTitle = this->windowTitle(); // Or a dedicated title property
    } else {
         // Try to infer from model context if possible and if model is TournamentLeaderboardModel
        const TournamentLeaderboardModel* tlm = qobject_cast<const TournamentLeaderboardModel*>(leaderboardModel);
        if (tlm) {
            // This requires TournamentLeaderboardModel to have a public getter for its context
            // e.g. if (tlm->getTournamentContext() == TournamentLeaderboardModel::MosleyOpen) leaderboardTitle = "Mosley Open";
            // else if (tlm->getTournamentContext() == TournamentLeaderboardModel::TwistedCreek) leaderboardTitle = "Twisted Creek";
        }
    }


    painter.setFont(QFont("Arial", 14, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - 2 * padding, titleHeight);
    painter.drawText(titleRect, Qt::AlignCenter, leaderboardTitle);

    painter.setFont(QFont("Arial", 10, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;
    for (int col = 0; col < colCount; ++col) {
        if (leaderboardView->isColumnHidden(col)) continue;
        int colWidth = (col < columnWidths.size()) ? columnWidths.at(col) : 80;
        QRect headerRect(currentX, currentY, colWidth, headerHeight);
        painter.drawText(headerRect, 
                         static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(col, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += colWidth;
    }

    painter.setFont(QFont("Arial", 9));
    currentY += headerHeight;
    for (int row = 0; row < rowCount; ++row) {
        currentX = padding;
        QColor rowColor = (row % 2 == 0) ? Qt::white : QColor(240, 240, 240);
        
        int visibleRowWidth = 0;
        for(int col_idx = 0; col_idx < colCount; ++col_idx) {
            if(!leaderboardView->isColumnHidden(col_idx)) {
                 visibleRowWidth += (col_idx < columnWidths.size()) ? columnWidths.at(col_idx) : 80;
            }
        }
        painter.fillRect(padding, currentY, visibleRowWidth, rowHeight, rowColor);


        for (int col = 0; col < colCount; ++col) {
            if (leaderboardView->isColumnHidden(col)) continue;
            int colWidth = (col < columnWidths.size()) ? columnWidths.at(col) : 80;
            QRect dataRect(currentX, currentY, colWidth, rowHeight);
            painter.drawText(dataRect, 
                             static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, col), Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->data(leaderboardModel->index(row, col), Qt::DisplayRole).toString());
            currentX += colWidth;
        }
        currentY += rowHeight;
    }
    
    painter.end();
    qDebug() << "TournamentLeaderboardWidget instance" << this << ": Image exported.";
    return image;
}
