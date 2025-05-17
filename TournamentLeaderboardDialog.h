#ifndef TOURNAMENTLEADERBOARDDIALOG_H
#define TOURNAMENTLEADERBOARDDIALOG_H

#include <QDialog>
#include <QSqlDatabase> 
#include <QPushButton>
#include <QVBoxLayout> 
#include <QHBoxLayout> 
#include <QTabWidget>  
#include <QLabel>      // For labels
#include <QSpinBox>    // For cut line input

// Include the custom widget headers
#include "tournamentleaderboardwidget.h" // This will be used for Combined, Mosley, and Twisted views
#include "dailyleaderboardwidget.h"
#include "TeamLeaderboardWidget.h" 


class TournamentLeaderboardDialog : public QDialog
{
    Q_OBJECT 

public:
    explicit TournamentLeaderboardDialog(const QString &connectionName, QWidget *parent = nullptr);
    ~TournamentLeaderboardDialog();

public slots:
    void refreshLeaderboards();

private slots:
    void exportCurrentImage();
    void applyCutClicked();
    void clearCutClicked();
    void cutLineScoreChanged(int value); // Optional: for immediate save on spinbox change

private:
    QString m_connectionName; 

    QTabWidget *tabWidget; 

    // Leaderboard Widgets
    TournamentLeaderboardWidget *combinedOverallWidget; // For pre-cut or combined view
    TournamentLeaderboardWidget *mosleyOpenWidget;    // For Mosley Open (post-cut)
    TournamentLeaderboardWidget *twistedCreekWidget;  // For Twisted Creek (post-cut)
    
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
    void setupCutLineUI(QVBoxLayout* mainLayout); // Helper to add cut line UI
    void loadCutSettings();
    void saveCutSettings();
};

#endif // TOURNAMENTLEADERBOARDDIALOG_H
