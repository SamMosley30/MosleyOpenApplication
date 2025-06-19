#include "tournamentleaderboarddialog.h"
#include "tournamentleaderboardmodel.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSet>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QAbstractTableModel>

const QString SETTING_CUT_LINE_SCORE = "cutLineScore";
const QString SETTING_IS_CUT_APPLIED = "isCutApplied";
const int DEFAULT_CUT_LINE_SCORE = 0;

TournamentLeaderboardDialog::TournamentLeaderboardDialog(const QString &connectionName, QWidget *parent)
    : QDialog(parent), m_connectionName(connectionName), tabWidget(new QTabWidget(this)),
      mosleyOpenWidget(new TournamentLeaderboardWidget(m_connectionName, this)), twistedCreekWidget(new TournamentLeaderboardWidget(m_connectionName, this)), day1LeaderboardWidget(new DailyLeaderboardWidget(m_connectionName, 1, this)), day2LeaderboardWidget(new DailyLeaderboardWidget(m_connectionName, 2, this)), day3LeaderboardWidget(new DailyLeaderboardWidget(m_connectionName, 3, this)), teamLeaderboardWidget(new TeamLeaderboardWidget(m_connectionName, this)), cutLineLabel(new QLabel(tr("Cut Line Score (2-Day Mosley Net Stableford):"), this)), cutLineSpinBox(new QSpinBox(this)), applyCutButton(new QPushButton(tr("Apply Cut"), this)), clearCutButton(new QPushButton(tr("Clear Cut"), this)), refreshButton(new QPushButton(tr("Refresh All"), this)), closeButton(new QPushButton(tr("Close"), this)), exportImageButton(new QPushButton(tr("Export Current Tab"), this)), m_cutLineScore(DEFAULT_CUT_LINE_SCORE), m_isCutApplied(false)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "TournamentLeaderboardDialog: ERROR: Invalid or closed database connection.";
        refreshButton->setEnabled(false);
        exportImageButton->setEnabled(false);
        applyCutButton->setEnabled(false);
        clearCutButton->setEnabled(false);
        cutLineSpinBox->setEnabled(false);
    }

    loadCutSettings();

    cutLineSpinBox->setRange(-100, 200);
    cutLineSpinBox->setValue(m_cutLineScore);
    applyCutButton->setEnabled(!m_isCutApplied);
    clearCutButton->setEnabled(m_isCutApplied);

    // --- Setup Tab Widget ---
    tabWidget->addTab(mosleyOpenWidget, tr("Mosley Open"));
    tabWidget->addTab(twistedCreekWidget, tr("Twisted Creek"));
    tabWidget->addTab(day1LeaderboardWidget, tr("Day 1 Scores"));
    tabWidget->addTab(day2LeaderboardWidget, tr("Day 2 Scores"));
    tabWidget->addTab(day3LeaderboardWidget, tr("Day 3 Scores"));
    tabWidget->addTab(teamLeaderboardWidget, tr("Team Leaderboard"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupCutLineUI(mainLayout);
    mainLayout->addWidget(tabWidget);

    QHBoxLayout *bottomButtonLayout = new QHBoxLayout();
    bottomButtonLayout->addStretch();
    bottomButtonLayout->addWidget(refreshButton);
    bottomButtonLayout->addWidget(exportImageButton);
    bottomButtonLayout->addWidget(closeButton);
    mainLayout->addLayout(bottomButtonLayout);

    setLayout(mainLayout);
    setWindowTitle(tr("Tournament Leaderboards"));
    resize(950, 700);

    connect(refreshButton, &QPushButton::clicked, this, &TournamentLeaderboardDialog::refreshLeaderboards);
    connect(exportImageButton, &QPushButton::clicked, this, &TournamentLeaderboardDialog::exportCurrentImage);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(applyCutButton, &QPushButton::clicked, this, &TournamentLeaderboardDialog::applyCutClicked);
    connect(clearCutButton, &QPushButton::clicked, this, &TournamentLeaderboardDialog::clearCutClicked);

    refreshLeaderboards();
}

TournamentLeaderboardDialog::~TournamentLeaderboardDialog() {}

void TournamentLeaderboardDialog::setupCutLineUI(QVBoxLayout *mainLayout)
{
    QHBoxLayout *cutLineLayout = new QHBoxLayout();
    cutLineLayout->addWidget(cutLineLabel);
    cutLineLayout->addWidget(cutLineSpinBox);
    cutLineLayout->addWidget(applyCutButton);
    cutLineLayout->addWidget(clearCutButton);
    cutLineLayout->addStretch();
    mainLayout->addLayout(cutLineLayout);
}

QSqlDatabase TournamentLeaderboardDialog::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

void TournamentLeaderboardDialog::loadCutSettings()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "TournamentLeaderboardDialog::loadCutSettings: Database not open.";
        m_cutLineScore = DEFAULT_CUT_LINE_SCORE;
        m_isCutApplied = false;
        return;
    }
    QSqlQuery query(db);
    query.prepare("SELECT value FROM settings WHERE key = :key");

    query.bindValue(":key", SETTING_CUT_LINE_SCORE);
    if (query.exec() && query.next())
    {
        bool ok;
        int score = query.value(0).toInt(&ok);
        m_cutLineScore = ok ? score : DEFAULT_CUT_LINE_SCORE;
    }
    else
        m_cutLineScore = DEFAULT_CUT_LINE_SCORE;

    query.bindValue(":key", SETTING_IS_CUT_APPLIED);
    if (query.exec() && query.next())
        m_isCutApplied = query.value(0).toBool();
    else
        m_isCutApplied = false;
}

void TournamentLeaderboardDialog::saveCutSettings()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
        return;
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)");

    query.bindValue(":key", SETTING_CUT_LINE_SCORE);
    query.bindValue(":value", m_cutLineScore);
    if (!query.exec())
        qWarning() << "TournamentLeaderboardDialog::saveCutSettings: Failed to save cut_line_score:" << query.lastError().text();

    query.bindValue(":key", SETTING_IS_CUT_APPLIED);
    query.bindValue(":value", m_isCutApplied);
    if (!query.exec())
        qWarning() << "TournamentLeaderboardDialog::saveCutSettings: Failed to save is_cut_applied:" << query.lastError().text();
}

void TournamentLeaderboardDialog::applyCutClicked()
{
    m_isCutApplied = true;
    m_cutLineScore = cutLineSpinBox->value();
    applyCutButton->setEnabled(false);
    clearCutButton->setEnabled(true);
    saveCutSettings();
    refreshLeaderboards();
    QMessageBox::information(this, tr("Cut Applied"), tr("The cut has been applied with score: %1. Leaderboards refreshed.").arg(m_cutLineScore));
}

void TournamentLeaderboardDialog::clearCutClicked()
{
    m_isCutApplied = false;
    applyCutButton->setEnabled(true);
    clearCutButton->setEnabled(false);
    saveCutSettings();
    refreshLeaderboards();
    QMessageBox::information(this, tr("Cut Cleared"), tr("The cut has been cleared. Leaderboards refreshed."));
}

void TournamentLeaderboardDialog::cutLineScoreChanged(int value)
{
}

void TournamentLeaderboardDialog::refreshLeaderboards()
{
    // Configure and refresh Mosley Open Widget
    if (TournamentLeaderboardModel *model = qobject_cast<TournamentLeaderboardModel *>(mosleyOpenWidget->leaderboardModel))
    {
        model->setTournamentContext(TournamentLeaderboardModel::MosleyOpen);
        model->setCutLineScore(m_cutLineScore);
        model->setIsCutApplied(m_isCutApplied);
        mosleyOpenWidget->refreshData(); // Refresh data for this specific widget
    }
    else
        qWarning() << "Failed to cast model for Mosley Open Widget.";

    // Configure and refresh Twisted Creek Widget
    if (TournamentLeaderboardModel *model = qobject_cast<TournamentLeaderboardModel *>(twistedCreekWidget->leaderboardModel))
    {
        model->setTournamentContext(TournamentLeaderboardModel::TwistedCreek);
        model->setCutLineScore(m_cutLineScore); // It needs the cut score to know who *not* to include if cut is applied
        model->setIsCutApplied(m_isCutApplied);
        twistedCreekWidget->refreshData(); // Refresh data for this specific widget
    }
    else
        qWarning() << "Failed to cast model for Twisted Creek Widget.";

    day1LeaderboardWidget->refreshData();
    day2LeaderboardWidget->refreshData();
    day3LeaderboardWidget->refreshData();
    teamLeaderboardWidget->refreshData();
}

void TournamentLeaderboardDialog::exportCurrentImage()
{
    QWidget *currentWidget = tabWidget->currentWidget();
    QImage exportedImage;

    if (TournamentLeaderboardWidget *overallWidget = qobject_cast<TournamentLeaderboardWidget *>(currentWidget))
        exportedImage = overallWidget->exportToImage();
    else if (DailyLeaderboardWidget *dailyWidget = qobject_cast<DailyLeaderboardWidget *>(currentWidget))
        exportedImage = dailyWidget->exportToImage();
    else if (TeamLeaderboardWidget *teamWidget = qobject_cast<TeamLeaderboardWidget *>(currentWidget))
        exportedImage = teamWidget->exportToImage();
    else
    {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot export the current tab type."));
        return;
    }

    if (exportedImage.isNull())
    {
        qDebug() << "TournamentLeaderboardDialog::exportCurrentImage: ExportToImage returned null for widget:" << (currentWidget ? currentWidget->objectName() : "null");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Leaderboard Image"), QDir::homePath(), tr("PNG Files (*.png);;JPEG Files (*.jpg *.jpeg);;BMP Files (*.bmp)"));
    if (filePath.isEmpty())
        return;

    if (exportedImage.save(filePath))
        QMessageBox::information(this, tr("Export Successful"), tr("Leaderboard image saved to:\n%1").arg(QDir::toNativeSeparators(filePath)));
    else
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not save image to:\n%1").arg(QDir::toNativeSeparators(filePath)));
}
