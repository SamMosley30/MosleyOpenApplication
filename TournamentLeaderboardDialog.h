/**
 * @file TournamentLeaderboardDialog.h
 * @brief Contains the declaration of the TournamentLeaderboardDialog class.
 */

#ifndef TOURNAMENTLEADERBOARDDIALOG_H
#define TOURNAMENTLEADERBOARDDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QSpinBox>

#include "TournamentLeaderboardWidget.h"
#include "DailyLeaderboardWidget.h"
#include "TeamLeaderboardWidget.h"

/**
 * @class TournamentLeaderboardDialog
 * @brief A dialog for displaying various tournament leaderboards.
 *
 * This dialog provides a tabbed interface for viewing different leaderboards,
 * including combined overall, Mosley Open, Twisted Creek, daily leaderboards,
 * and the team leaderboard. It also includes functionality for applying a cut line.
 */
class TournamentLeaderboardDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a TournamentLeaderboardDialog object.
     * @param connectionName The name of the database connection to use.
     * @param parent The parent widget.
     */
    explicit TournamentLeaderboardDialog(const QString &connectionName, QWidget *parent = nullptr);

    /**
     * @brief Destroys the TournamentLeaderboardDialog object.
     */
    ~TournamentLeaderboardDialog();

public slots:
    /**
     * @brief Refreshes all leaderboards.
     */
    void refreshLeaderboards();

private slots:
    void exportCurrentImage();
    void applyCutClicked();
    void clearCutClicked();
    void cutLineScoreChanged(int value);

private:
    QString m_connectionName;

    QTabWidget *tabWidget;

    // Leaderboard Widgets
    TournamentLeaderboardWidget *combinedOverallWidget;
    TournamentLeaderboardWidget *mosleyOpenWidget;
    TournamentLeaderboardWidget *twistedCreekWidget;
    
    DailyLeaderboardWidget *day1LeaderboardWidget;            
    DailyLeaderboardWidget *day2LeaderboardWidget;            
    DailyLeaderboardWidget *day3LeaderboardWidget;            
    TeamLeaderboardWidget *teamLeaderboardWidget;             

    // UI for Cut Line
    QLabel *cutLineLabel;
    QSpinBox *cutLineSpinBox;
    QPushButton *applyCutButton;
    QPushButton *clearCutButton;

    QPushButton *refreshButton; 
    QPushButton *closeButton;   
    QPushButton *exportImageButton; 

    // State for cut
    int m_cutLineScore;
    bool m_isCutApplied;

    QSqlDatabase database() const;
    void setupCutLineUI(QVBoxLayout* mainLayout);
    void loadCutSettings();
    void saveCutSettings();
};

#endif // TOURNAMENTLEADERBOARDDIALOG_H
