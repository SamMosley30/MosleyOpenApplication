#include "dailyleaderboardwidget.h"
#include <QSqlDatabase>
#include <QDebug>
#include <QFontMetrics>

// Constructor
DailyLeaderboardWidget::DailyLeaderboardWidget(const QString &connectionName, int dayNum, QWidget *parent)
    : QWidget(parent)
    , m_connectionName(connectionName)
    , m_dayNum(dayNum) // Store the day number
    , leaderboardModel(new DailyLeaderboardModel(m_connectionName, m_dayNum, this)) // Create the model, pass dayNum
    , leaderboardView(new QTableView(this)) // Create the view
{
    // --- Setup Database Connection ---
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
         qDebug() << QString("DailyLeaderboardWidget (Day %1): ERROR: Invalid or closed database connection passed to constructor.").arg(m_dayNum);
         // Handle this error appropriately
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
DailyLeaderboardWidget::~DailyLeaderboardWidget()
{
    // Model and view are parented to 'this', so they will be deleted automatically.
}

// Helper to get database connection
QSqlDatabase DailyLeaderboardWidget::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// Helper to configure the table view settings
void DailyLeaderboardWidget::configureTableView()
{
    leaderboardView->verticalHeader()->setVisible(false); // Hide vertical header (row numbers)
    leaderboardView->setSelectionBehavior(QAbstractItemView::SelectRows); // Select entire rows
    leaderboardView->setSelectionMode(QAbstractItemView::NoSelection);    // Make view read-only (no selection)
    leaderboardView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make view non-editable
    leaderboardView->horizontalHeader()->setStretchLastSection(true); // Stretch the last section

    // Configure specific column resize modes (matching the model's expected structure)
    // DailyLeaderboardModel columns: Rank (0), Player (1), Daily Total (2), Daily Net (3)
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForRank(), QHeaderView::ResizeToContents);
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForPlayerName(), QHeaderView::Stretch);
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForDailyTotalPoints(), QHeaderView::ResizeToContents);
    leaderboardView->horizontalHeader()->setSectionResizeMode(leaderboardModel->getColumnForDailyNetPoints(), QHeaderView::ResizeToContents);
}


// Public method to refresh the leaderboard data
void DailyLeaderboardWidget::refreshData()
{
    qDebug() << QString("DailyLeaderboardWidget (Day %1): Refreshing data...").arg(m_dayNum);
    leaderboardModel->refreshData(); // Call the model's refresh method
    // No column visibility updates needed here as the model has fixed columns for its day
    qDebug() << QString("DailyLeaderboardWidget (Day %1): Data refreshed.").arg(m_dayNum);
}


// Public method to export the leaderboard as an image
QImage DailyLeaderboardWidget::exportToImage() const
{
    // --- Determine Image Dimensions ---
    int rowCount = leaderboardModel->rowCount();
    int colCount = leaderboardModel->columnCount(); // Total columns in this model (should be 4)

    if (rowCount == 0 || colCount == 0) {
        qDebug() << QString("DailyLeaderboardWidget (Day %1): exportToImage: No data available to export.").arg(m_dayNum);
        return QImage(); // Return null image
    }

    // Estimate sizes (adjust these values based on desired appearance)
    int titleHeight = 60;
    int headerHeight = 40;
    int rowHeight = 30;
    int padding = 20; // Padding around content

    // Estimate column widths (matching the model's columns)
    QVector<int> columnWidths;
    columnWidths << 60;  // Rank
    columnWidths << 250; // Player Name
    columnWidths << 100; // Daily Total
    columnWidths << 100; // Daily Net


    // Calculate total width based on column widths
    int totalWidth = padding * 2; // Left and right padding
    for (int width : columnWidths) {
        totalWidth += width;
    }

    // Calculate total height
    int totalHeight = padding * 2; // Top and bottom padding
    totalHeight += titleHeight;    // Title height
    totalHeight += headerHeight;   // Header height
    totalHeight += rowCount * rowHeight; // Height for all data rows

    // Ensure minimum size
    if (totalWidth < 400) totalWidth = 400;
    if (totalHeight < 200) totalHeight = 200;


    // --- Create the Paint Device (QImage) ---
    QImage image(totalWidth, totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white); // Fill with a background color

    // --- Create a QPainter ---
    QPainter painter(&image);
    if (!painter.isActive()) {
        qDebug() << QString("DailyLeaderboardWidget (Day %1): exportToImage: QPainter failed to start.").arg(m_dayNum);
        return QImage(); // Return null image
    }

    // --- Set Drawing Properties ---
    painter.setRenderHint(QPainter::Antialiasing); // Optional: for smoother text/lines
    painter.setPen(Qt::white); // Default pen color

    // --- Draw Content ---

    // Draw Title
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    QRect titleRect(padding, padding, totalWidth - padding * 2, titleHeight);
    painter.fillRect(titleRect, Qt::black); // Fill with black background
    painter.drawText(titleRect, Qt::AlignCenter, QString("Day %1 Leaderboard").arg(m_dayNum)); // Title includes day number


    // Draw Headers
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    int currentX = padding;
    int currentY = padding + titleHeight;

    for (int i = 0; i < colCount; ++i) { // Iterate through all model columns (should be 4)
        QRect headerRect(currentX, currentY, columnWidths.at(i), headerHeight); // Use estimated width
        painter.fillRect(headerRect, Qt::black); // Fill with black background
        painter.drawText(headerRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->headerData(i, Qt::Horizontal, Qt::TextAlignmentRole).toInt()),
                         leaderboardModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
        currentX += columnWidths.at(i); // Move X position by the width of this column
    }


    // Draw Data Rows
    painter.setFont(QFont("Arial", 10)); // Font for data
    painter.setPen(Qt::black);
    currentY += headerHeight; // Start drawing rows below headers

    for (int row = 0; row < rowCount; ++row) {
        currentX = padding; // Reset X for each row

        // --- Determine row background color ---
        QColor rowColor = ((row % 2) == 0) ? Qt::white : QColor(240, 240, 240); // Alternating gray/white

        // Check rank for top 3 (Assuming rank is column 0)
        QVariant rankData = leaderboardModel->data(leaderboardModel->index(row, leaderboardModel->getColumnForRank()), Qt::DisplayRole);
        bool ok;
        int rank = rankData.toInt(&ok);
        if (ok && rank >= 1 && rank <= 3)
            rowColor = QColor(255, 165, 0, 255); // Top 3 are yellow

        // Fill row background (covers the entire width)
        painter.fillRect(padding, currentY, totalWidth - padding * 2, rowHeight, rowColor);


        // Draw data columns
        for (int i = 0; i < colCount; ++i) { // Iterate through all model columns (should be 4)
            QRect dataRect(currentX, currentY, columnWidths.at(i), rowHeight); // Use estimated width
            painter.drawText(dataRect, static_cast<Qt::AlignmentFlag>(leaderboardModel->data(leaderboardModel->index(row, i), Qt::TextAlignmentRole).toInt()),
                             leaderboardModel->data(leaderboardModel->index(row, i), Qt::DisplayRole).toString());
            currentX += columnWidths.at(i); // Move X position by the width of this column
        }

        currentY += rowHeight; // Move to the next row position
    }

    // Optional: Draw grid lines or borders
    painter.setPen(Qt::black); // Set a lighter pen for grid lines
    // Draw horizontal lines
    currentY = padding + titleHeight + headerHeight;
    for (int row = 0; row <= rowCount; ++row) { // Draw line above headers and below each row
         painter.drawLine(padding, currentY, totalWidth - padding, currentY);
         currentY += rowHeight;
    }
    // Draw vertical lines
    // Calculate the x-positions of the left edge of each column and the right edge of the last column.
    QVector<int> verticalLineXPositions;
    verticalLineXPositions << padding; // Left border

    currentX = padding;
    for (int width : columnWidths) {
         currentX += width;
         verticalLineXPositions << currentX; // Add the right edge of each column
    }

    currentY = padding + titleHeight; // Start drawing vertical lines below the title
    for (int xPos : verticalLineXPositions) {
         painter.drawLine(xPos, currentY, xPos, totalHeight - padding);
    }


    // --- End Painting ---
    painter.end();

    // Return the created image
    return image;
}
