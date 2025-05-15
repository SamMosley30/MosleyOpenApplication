#ifndef PLAYERDIALOG_H
#define PLAYERDIALOG_H

#include <QDialog>
#include <QSqlTableModel>

class QTableView;
class QPushButton;

class PlayerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlayerDialog(QSqlDatabase &db, QWidget *parent = nullptr);

private slots:
    void addPlayer();
    void removeSelected();
    void exportToCsv();

private:
    QSqlTableModel *model;
    QTableView *tableView;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *closeButton;
    QPushButton *exportButton;
};

#endif // PLAYERDIALOG_H