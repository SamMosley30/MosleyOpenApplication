#include "tournamentleaderboardwidget.h"
#include <QSqlDatabase>
#include <QDebug>
#include <QSet>
#include <QFontMetrics>


// Constructor
TournamentLeaderboardWidget::TournamentLeaderboardWidget(const QString &connectionName, QWidget *parent)
    : QWidget(parent)
    , m_connectionName(connectionName)
    , leaderboardModel(new TournamentLeaderboardModel(m_connectionName, this)) // Create the model
    , leaderboardView(new QTableView(this)) // Create the view
{
    // --- Setup Database Connection ---
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
         qDebug() << "TournamentLeaderboardWidget: ERROR: Invalid or closed database connection passed to constructor.";
         // Handle this error appropriately - maybe show a message or disable functionality
    }

    // --- Setup Table View ---
    leaderboardView->setModel(leaderboardModel); // Set the model on the view
    configureTableView(); // Configure view settings

    // --- Setup Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this); // Main layout for the widget
    mainLayout->addWidget(leaderboardView); // Add the table view

    setLayout(mainLayout); // Set the main layout for the widget

    // Initial data load is typically done by the parent (the Dialog)
    // refreshData(); // You could call this here, but better to let the dialog manage refresh
}

// Destructor
TournamentLeaderboardWidget::~TournamentLeaderboardWidget()
{
    // Model and view are parented to 'this', so they will be deleted automatically.
}

// Helper to get database connection
QSqlDatabase TournamentLeaderboardWidget::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// Helper to configure the table view settings
void TournamentLeaderboardWidget::configureTableView()
{
    leaderboardView->verticalHeader()->setVisible(false); // Hide vertical header (row numbers)
    leaderboardView->setSelectionBehavior(QAbstractItemView::SelectRows); // Select entire rows
    leaderboardView->setSelectionMode(QAbstractItemView::NoSelection);    // Make view read-only (no selection)
    leaderboardView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make view non-editable
    leaderboardView->horizontalHeader()->setStretchLastSection(true); // Stretch the last section

    // Configure specific column resize modes (matching the model's expected structure)
    // This assumes the model's column order: Rank (0), Player (1), Handicap (2),
    // Day 1 Total (3), Day 1 Net (4), Day 2 Total (5), Day 2 Net (6),
    // Day 3 Total (7), Day 3 Net (8), Overall Pts (9)
    leaderboardView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Rank
    leaderboardView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);        // Player Name
    leaderboardView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Handicap

    // Set resize modes for daily points columns (will be hidden if no scores for the day)
    // Columns 3-8 in the model are daily points
    for (int modelCol = 3; modelCol <= 8; ++modelCol) {
         leaderboardView->horizontalHeader()->setSectionResizeMode(modelCol, QHeaderView::ResizeToContents);
    }

    // Set resize mode for the Overall Total Pts column (the last column, index 9)
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->columnCount() - 1, QHeaderView::ResizeToContents);
}


// Public method to refresh the leaderboard data
void TournamentLeaderboardWidget::refreshData()
{
    qDebug() << "TournamentLeaderboardWidget: Refreshing data...";
    leaderboardModel->refreshData(); // Call the model's refresh method
    updateColumnVisibility(); // Update column visibility after refresh
    qDebug() << "TournamentLeaderboardWidget: Data refreshed.";
}

// Helper to update column visibility based on scores (for daily columns in overall)
void TournamentLeaderboardWidget::updateColumnVisibility()
{
    QSet<int> daysWithScores = leaderboardModel->getDaysWithScores();

    // Columns 3-8 in the model are daily points (Day 1 Total, Day 1 Net, Day 2 Total, Day 2 Net, Day 3 Total, Day 3 Net)
    for (int dayNum = 1; dayNum <= 3; ++dayNum) {
        int totalCol = leaderboardModel->getColumnForDailyGrossPoints(dayNum); // 4, 6, 8
        int netCol = leaderboardModel->getColumnForDailyNetPoints(dayNum);   // 5, 7, 9

        // Note: The model's getColumnForDailyTotalPoints/NetPoints return indices 4-9.
        // We need to map these to the correct columns in the view if the model
        // structure changes. Assuming the model structure is fixed for now.

        if (totalCol != -1) { // Check if the column index is valid
            bool hasScores = daysWithScores.contains(dayNum);
            leaderboardView->setColumnHidden(totalCol, !hasScores); // Hide if no scores for this day
        }
         if (netCol != -1) { // Check if the column index is valid
            bool hasScores = daysWithScores.contains(dayNum);
            leaderboardView->setColumnHidden(netCol, !hasScores); // Hide if no scores for this day
        }
    }
     qDebug() << "TournamentLeaderboardWidget: Column visibility updated.";
}


// Public method to export the leaderboard as an image
QImage TournamentLeaderboardWidget::exportToImage() const
{
    // --- Determine Image Dimensions ---
    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount(); // Total columns in the model

    if (rowCount == 0 || colCount == 0) {
        qDebug() << "TournamentLeaderboardWidget::exportToImage: No data available to export.";
        return QImage(); // Return null image
    }

    // Estimate sizes (adjust these values based on desired appearance)
    int titleHeight = 120;
    int headerHeight = 80;
    int rowHeight = 60;
    int padding = 20; // Padding around content

    // Estimate column widths (adjust these based on your data and desired layout)
    // These should ideally match the widths used in headerData and data drawing
    // Note: These widths should correspond to the *model's* columns (0-9)
    QVector<int> columnWidths;
    columnWidths << 120;  // Column 0: Rank
    columnWidths << 240; // Column 1: Player Name (Increased width)
    columnWidths << 280;  // Column 2: Handicap
    // Daily columns will be added here based on day number
    // Overall Pts will be the last column

    // Add widths for daily columns (assume they are all the same width for simplicity here)
    int dailyColWidth = 240;
    for (int i = 0; i < 3; ++i) { // For Day 1, Day 2, Day 3 (Total and Net)
        columnWidths << dailyColWidth; // Total Pts
        columnWidths << dailyColWidth; // Net Pts
    }

    columnWidths << 280; // Overall Pts (Now the last column)


    // Calculate total width based on *visible* columns in the *view*
    int totalWidth = padding * 2; // Left and right padding
    QSet<int> daysWithScores = leaderboardModel->getDaysWithScores(); // Get days with scores from the model

    // Add width for static columns (0-2: Rank, Player, Handicap)
    for (int i = 0; i <= 2; ++i) {
        // Check view visibility, not model visibility, for drawing
        if (!leaderboardView->isColumnHidden(i)) {
             totalWidth += columnWidths.at(i);
        }
    }

    // Add width for daily columns only if the day has scores AND the column is not hidden in the view
    for (int dayNum = 1; dayNum <= 3; ++dayNum) {
        int totalColModelIndex = leaderboardModel->getColumnForDailyGrossPoints(dayNum); // 4, 6, 8
        int netColModelIndex = leaderboardModel->getColumnForDailyNetPoints(dayNum);   // 5, 7, 9

        if (totalColModelIndex != -1 && !leaderboardView->isColumnHidden(totalColModelIndex)) {
             totalWidth += columnWidths.at(totalColModelIndex);
        }
        if (netColModelIndex != -1 && !leaderboardView->isColumnHidden(netColModelIndex)) {
             totalWidth += columnWidths.at(netColModelIndex);
        }
    }

    // Add width for the Overall Pts column (the last column)
    int overallColModelIndex = colCount - 1; // Should be 9
     if (!leaderboardView->isColumnHidden(overallColModelIndex)) {
        totalWidth += columnWidths.last();
     }


    // Calculate total height
    int totalHeight = padding * 2; // Top and bottom padding
    totalHeight += titleHeight;    // Title height
    totalHeight += headerHeight;   // Header height
    totalHeight += rowCount * rowHeight; // Height for all data rows

    // Ensure minimum size
    if (totalWidth < 800) totalWidth = 800;
    if (totalHeight < 400) totalHeight = 400;


    // --- Create the Paint Device (QImage) ---
    QImage image(totalWidth, totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white); // Fill with a background color

    // --- Create a QPainter ---
    QPainter painter(&image);
    if (!painter.isActive()) {
        qDebug() << "TournamentLeaderboardWidget::exportToImage: QPainter failed to start.";
        return QImage(); // Return null image
    }

    // --- Set Drawing Properties ---
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother text/lines
    painter.setPen(Qt::white); // Default pen color

    // --- Draw Content ---

    // Draw Title (Use a generic title here, or pass one in)
    painter.setFont(QFont("Arial", 32, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - padding * 2, titleHeight);
    painter.drawText(titleRect, Qt::AlignCenter, "Mosley Open Leaderboard"); // Generic title


    // Draw Headers
    painter.setFont(QFont("Arial", 32, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;
    int currentModelColumn = 0; // Keep track of the model's column index (0-9)

    // Draw static headers (Columns 0-2: Rank, Player, Handicap)
    for (int i = 0; i <= 2; ++i) {
        if (!leaderboardView->isColumnHidden(currentModelColumn)) {
            QRect headerRect(currentX, currentY, columnWidths.at(currentModelColumn), headerHeight);
            painter.drawText(headerRect, Qt::AlignmentFlag::AlignCenter,
                             leaderboardModel->headerData(currentModelColumn, Qt::Horizontal, Qt::DisplayRole).toString());
            currentX += columnWidths.at(currentModelColumn);
        }
        currentModelColumn++; // Always increment model column index
    }

    // Draw daily headers (Columns 3-8 in model), only if the day has scores AND column is visible
    for (int dayNum = 1; dayNum <= 3; ++dayNum) {
        int totalColModelIndex = leaderboardModel->getColumnForDailyGrossPoints(dayNum); // 4, 6, 8
        int netColModelIndex = leaderboardModel->getColumnForDailyNetPoints(dayNum);   // 5, 7, 9

        if (totalColModelIndex != -1 && !leaderboardView->isColumnHidden(totalColModelIndex)) {
            // Draw Day X Total header
            QRect totalHeaderRect(currentX, currentY, columnWidths.at(totalColModelIndex), headerHeight);
             painter.drawText(totalHeaderRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(totalColModelIndex, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->headerData(totalColModelIndex, Qt::Horizontal, Qt::DisplayRole).toString());
            currentX += columnWidths.at(totalColModelIndex);
        }
        if (netColModelIndex != -1 && !leaderboardView->isColumnHidden(netColModelIndex)) {
            // Draw Day X Net header
            QRect netHeaderRect(currentX, currentY, columnWidths.at(netColModelIndex), headerHeight);
            painter.drawText(netHeaderRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(netColModelIndex, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->headerData(netColModelIndex, Qt::Horizontal, Qt::DisplayRole).toString());
            currentX += columnWidths.at(netColModelIndex);
        }
    }

    // Draw Overall Pts header (the last column in the model)
    overallColModelIndex = colCount - 1; // Should be 9
    if (!leaderboardView->isColumnHidden(overallColModelIndex)) {
        QRect overallHeaderRect(currentX, currentY, columnWidths.at(overallColModelIndex), headerHeight);
        painter.drawText(overallHeaderRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(overallColModelIndex, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(overallColModelIndex, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += columnWidths.at(overallColModelIndex);
    }


    // Draw Data Rows
    painter.setPen(Qt::black); // Default pen color
    painter.setFont(QFont("Arial", 20)); // Font for data
    currentY += headerHeight; // Start drawing rows below headers

    for (int row = 0; row < rowCount; ++row) {
        currentX = padding; // Reset X for each row
        currentModelColumn = 0; // Reset model column index for each row

        // --- Determine row background color ---
        QColor rowColor = ((row % 2) == 0) ? Qt::lightGray : Qt::gray; // Alternating gray/white

        // Check rank for top 3 (Assuming rank is column 0)
        QVariant rankData = leaderboardModel->data(leaderboardModel->index(row, 0), Qt::DisplayRole);
        bool ok;
        int rank = rankData.toInt(&ok);
        if (ok && rank >= 1 && rank <= 3) {
            rowColor = QColor(255, 215, 0); // Top 3 are yellow
        }

        // Fill row background (covers the area of visible columns)
        int visibleWidth = 0;
         for (int i = 0; i < colCount; ++i) {
            if (!leaderboardView->isColumnHidden(i)) {
                 visibleWidth += columnWidths.at(i);
            }
         }
        painter.fillRect(padding, currentY, visibleWidth, rowHeight, rowColor);


        // Draw static data columns (Columns 0-2: Rank, Player, Handicap)
        for (int i = 0; i <= 2; ++i) {
            if (!leaderboardView->isColumnHidden(currentModelColumn)) {
                QRect dataRect(currentX, currentY, columnWidths.at(currentModelColumn), rowHeight);
                painter.drawText(dataRect, Qt::AlignmentFlag::AlignCenter,
                                 leaderboardModel->data(leaderboardModel->index(row, currentModelColumn), Qt::DisplayRole).toString());
                currentX += columnWidths.at(currentModelColumn);
            }
            currentModelColumn++; // Always increment model column index
        }

        // Draw daily data columns (Columns 3-8 in model), only if the day has scores AND column is visible
        for (int dayNum = 1; dayNum <= 3; ++dayNum) {
            int totalColModelIndex = leaderboardModel->getColumnForDailyGrossPoints(dayNum); // 4, 6, 8
            int netColModelIndex = leaderboardModel->getColumnForDailyNetPoints(dayNum);   // 5, 7, 9

            if (totalColModelIndex != -1 && !leaderboardView->isColumnHidden(totalColModelIndex)) {
                // Draw Day X Total data
                QRect totalDataRect(currentX, currentY, columnWidths.at(totalColModelIndex), rowHeight);
                painter.drawText(totalDataRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, totalColModelIndex), Qt::TextAlignmentRole).toInt()),
                                 leaderboardModel->data(leaderboardModel->index(row, totalColModelIndex), Qt::DisplayRole).toString());
                currentX += columnWidths.at(totalColModelIndex);
            }
            if (netColModelIndex != -1 && !leaderboardView->isColumnHidden(netColModelIndex)) {
                // Draw Day X Net data
                QRect netDataRect(currentX, currentY, columnWidths.at(netColModelIndex), rowHeight);
                painter.drawText(netDataRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, netColModelIndex), Qt::TextAlignmentRole).toInt()),
                                 leaderboardModel->data(leaderboardModel->index(row, netColModelIndex), Qt::DisplayRole).toString());
                currentX += columnWidths.at(netColModelIndex);
            }
        }

        // Draw Overall Pts data (the last column in the model)
        int overallColModelIndex = colCount - 1; // Should be 9
        if (!leaderboardView->isColumnHidden(overallColModelIndex)) {
            QRect overallDataRect(currentX, currentY, columnWidths.at(overallColModelIndex), rowHeight);
            painter.drawText(overallDataRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, overallColModelIndex), Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->data(leaderboardModel->index(row, overallColModelIndex), Qt::DisplayRole).toString());
            currentX += columnWidths.at(overallColModelIndex);
        }


        currentY += rowHeight; // Move to the next row position
    }

    // Optional: Draw grid lines or borders
    // Draw horizontal lines
    currentY = padding + titleHeight;
    painter.drawLine(padding, currentY, totalWidth - padding, currentY);
    currentY += headerHeight;
    for (int row = 0; row <= rowCount; ++row) { // Draw line above headers and below each row
         painter.drawLine(padding, currentY, totalWidth - padding, currentY);
         currentY += rowHeight;
    }
    // Draw vertical lines
    // Calculate the x-positions of the left edge of each VISIBLE column and the right edge of the last VISIBLE column.
    QVector<int> visibleColumnXPositions;
    visibleColumnXPositions << padding; // Left border

    currentX = padding;
    int currentModelColumnIndex = 0;
    for (int i = 0; i < colCount; ++i) {
         bool isVisibleColumn = !leaderboardView->isColumnHidden(currentModelColumnIndex); // Check view visibility

         int columnWidth = columnWidths.at(currentModelColumnIndex); // Use the estimated width

         if (isVisibleColumn) {
              currentX += columnWidth;
              visibleColumnXPositions << currentX; // Add the right edge of this visible column
         } else {
              currentX += columnWidth; // Still move X for hidden columns to correctly position subsequent visible columns
         }
         currentModelColumnIndex++;
    }

    currentY = padding + titleHeight; // Start drawing vertical lines below the title
    for (int xPos : visibleColumnXPositions) {
        if (xPos == visibleColumnXPositions.first())
        {
            painter.drawLine(xPos, padding, xPos, totalHeight - padding);
        }
        else
        {
            painter.drawLine(xPos, currentY, xPos, totalHeight - padding);
        }
    }

    painter.drawLine(totalWidth - padding, padding, totalWidth - padding, totalHeight - padding);


    // --- End Painting ---
    painter.end();

    // Return the created image
    return image;
}
