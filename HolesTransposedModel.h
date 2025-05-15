#ifndef HOLESTRANSPOSEDMODEL_H
#define HOLESTRANSPOSEDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>

// Structure to hold data for a single hole
struct HoleData {
    int holeNum;
    int par;
    int handicap;
    // Add score later if needed
};

class HolesTransposedModel : public QAbstractTableModel
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name
    explicit HolesTransposedModel(const QString &connectionName, QObject *parent = nullptr);
    ~HolesTransposedModel();

    // === Required QAbstractTableModel methods ===
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // === Public method to load data for a specific course ===
    void setCourseId(int courseId);

    // Optional: Make editable
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool submitAll(); // If implementing editing


private:
    QString m_connectionName; // Stores the name of the database connection
    int m_currentCourseId;    // Stores the ID of the currently selected course
    QVector<HoleData> m_holeData; // Stores the fetched hole data for the current course

    // Helper to get database connection by name
    QSqlDatabase database() const;

    // Helper to find hole data by hole number
    const HoleData* getHoleByNumber(int holeNum) const;
};

#endif // HOLESTRANSPOSEDMODEL_H