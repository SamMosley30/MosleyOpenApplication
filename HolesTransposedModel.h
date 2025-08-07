/**
 * @file HolesTransposedModel.h
 * @brief Contains the declaration of the HolesTransposedModel class.
 */

#ifndef HOLESTRANSPOSEDMODEL_H
#define HOLESTRANSPOSEDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>

/**
 * @struct HoleData
 * @brief Holds data for a single hole.
 */
struct HoleData {
    int holeNum;   ///< The hole number.
    int par;       ///< The par for the hole.
    int handicap;  ///< The handicap for the hole.
};

/**
 * @class HolesTransposedModel
 * @brief A model for displaying hole data in a transposed view.
 *
 * This model displays hole data with holes as columns and properties (par, handicap)
 * as rows. It is designed to be used with a QTableView to show the 18 holes of a
 * selected course.
 */
class HolesTransposedModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a HolesTransposedModel object.
     * @param connectionName The name of the database connection to use.
     * @param parent The parent object.
     */
    explicit HolesTransposedModel(const QString &connectionName, QObject *parent = nullptr);

    /**
     * @brief Destroys the HolesTransposedModel object.
     */
    ~HolesTransposedModel();

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @brief Sets the course ID to load data for.
     *
     * This method clears the current data and fetches the hole data for the
     * specified course ID from the database.
     *
     * @param courseId The ID of the course to load.
     */
    void setCourseId(int courseId);

    /**
     * @brief Submits all pending changes to the database.
     * @return True if the submission is successful, false otherwise.
     */
    bool submitAll();

private:
    QString m_connectionName; ///< The name of the database connection.
    int m_currentCourseId;    ///< The ID of the currently selected course.
    QVector<HoleData> m_holeData; ///< The fetched hole data for the current course.

    /**
     * @brief Gets the database connection by name.
     * @return The QSqlDatabase object.
     */
    QSqlDatabase database() const;

    /**
     * @brief Gets the hole data for a given hole number.
     * @param holeNum The hole number.
     * @return A pointer to the HoleData, or nullptr if not found.
     */
    const HoleData* getHoleByNumber(int holeNum) const;
};

#endif // HOLESTRANSPOSEDMODEL_H