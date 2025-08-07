/**
 * @file HolesTransposedModel.cpp
 * @brief Implements the HolesTransposedModel class.
 */

#include "HolesTransposedModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

HolesTransposedModel::HolesTransposedModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent)
    , m_connectionName(connectionName)
    , m_currentCourseId(-1)
{
    m_holeData.resize(18);
    for(int i = 0; i < 18; ++i) {
        m_holeData[i].holeNum = i + 1;
        m_holeData[i].par = 0;
        m_holeData[i].handicap = 0;
    }
}

HolesTransposedModel::~HolesTransposedModel()
{
}

QSqlDatabase HolesTransposedModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

int HolesTransposedModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2; // Par and Handicap
}

int HolesTransposedModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 18; // 18 holes
}

QVariant HolesTransposedModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount()) {
        qDebug() << "HolesTransposedModel::data(): Invalid index requested.";
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        int holeNum = index.column() + 1;
        const HoleData* hole = getHoleByNumber(holeNum);

        if (hole) {
            if (index.row() == 0) {
                return hole->par;
            } else if (index.row() == 1) {
                return hole->handicap;
            }
            qDebug() << "HolesTransposedModel::data(): Requested row" << index.row() << "does not map to a known attribute.";
        } else {
            qDebug() << "HolesTransposedModel::data(): ERROR: Could not find data for Hole #" << holeNum << " in internal storage.";
        }
    }

    return QVariant();
}

QVariant HolesTransposedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            return QString("Hole %1").arg(section + 1);
        } else {
            if (section == 0) {
                return "Par";
            } else if (section == 1) {
                return "Handicap";
            }
        }
    }

    return QVariant();
}

const HoleData* HolesTransposedModel::getHoleByNumber(int holeNum) const
{
    if (holeNum >= 1 && holeNum <= 18) {
        return &m_holeData[holeNum - 1];
    }
    return nullptr;
}

void HolesTransposedModel::setCourseId(int courseId)
{
    bool needToLoad = false;
    bool needToClear = false;

    if (courseId <= 0) {
        if (m_currentCourseId > 0 || !m_holeData.isEmpty()) {
            needToClear = true;
            qDebug() << "HolesTransposedModel: Invalid ID received, need to clear.";
        } else {
             qDebug() << "HolesTransposedModel: Invalid ID received, but already cleared/empty.";
        }
    } else {
        if (m_currentCourseId != courseId) {
            needToLoad = true;
        }
    }

    if (needToClear) {
        beginResetModel();
        m_holeData.clear();
        m_holeData.resize(18);
        for(int i = 0; i < 18; ++i) {
           m_holeData[i].holeNum = i + 1;
           m_holeData[i].par = 0;
           m_holeData[i].handicap = 0;
       }
        m_currentCourseId = courseId;
        endResetModel();
        return;
    }

    if (needToLoad) {
        m_currentCourseId = courseId;

        QSqlDatabase db = database();
        if (!db.isValid() || !db.isOpen()) {
            qDebug() << "HolesTransposedModel: ERROR: Invalid or closed database connection when loading data.";
            beginResetModel();
            m_holeData.clear();
            m_holeData.resize(18);
             for(int i = 0; i < 18; ++i) {
                m_holeData[i].holeNum = i + 1;
                m_holeData[i].par = 0;
                m_holeData[i].handicap = 0;
            }
            endResetModel();
            return;
        }

        beginResetModel();
        m_holeData.clear();
        m_holeData.resize(18);
        for(int i = 0; i < 18; ++i) {
           m_holeData[i].holeNum = i + 1;
           m_holeData[i].par = 0;
           m_holeData[i].handicap = 0;
       }

        QSqlQuery query(db);
        query.prepare("SELECT hole_num, par, handicap FROM holes WHERE course_id = :cid ORDER BY hole_num;");
        query.bindValue(":cid", m_currentCourseId);

        if (query.exec()) {
            while (query.next()) {
                int fetchedHoleNum = query.value("hole_num").toInt();
                int fetchedPar = query.value("par").toInt();
                int fetchedHandicap = query.value("handicap").toInt();

                if (fetchedHoleNum >= 1 && fetchedHoleNum <= 18) {
                     m_holeData[fetchedHoleNum - 1].par = fetchedPar;
                     m_holeData[fetchedHoleNum - 1].handicap = fetchedHandicap;
                 } else {
                      qDebug() << "HolesTransposedModel: Warning: Unexpected hole number" << fetchedHoleNum << "fetched from database.";
                 }
            }
        } else {
            qDebug() << "HolesTransposedModel: ERROR executing query to fetch holes:" << query.lastError().text();
        }

        endResetModel();
    }
}

Qt::ItemFlags HolesTransposedModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);
    if (index.isValid() && index.row() >= 0 && index.row() < rowCount() &&
        index.column() >= 0 && index.column() < columnCount()) {
        if (index.row() == 0 || index.row() == 1) {
            return defaultFlags | Qt::ItemIsEditable;
        }
    }
    return defaultFlags;
}

bool HolesTransposedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount()) {
        return false;
    }

    bool ok;
    int intValue = value.toInt(&ok);
    if (!ok) {
        qDebug() << "HolesTransposedModel::setData: Value is not a valid integer.";
        return false;
    }

    int holeNum = index.column() + 1;
    HoleData* hole = nullptr;
    if (holeNum >= 1 && holeNum <= 18) {
        hole = &m_holeData[holeNum - 1];
    }

    if (hole) {
        bool dataUpdated = false;
        if (index.row() == 0) { // Par
            if (hole->par != intValue) {
                 hole->par = intValue;
                 dataUpdated = true;
            }
        } else if (index.row() == 1) { // Handicap
             if (hole->handicap != intValue) {
                hole->handicap = intValue;
                dataUpdated = true;
            }
        }

        if (dataUpdated) {
            emit dataChanged(index, index, {role});

            QSqlDatabase db = database();
            if (db.isValid() && db.isOpen()) {
                QSqlQuery query(db);
                query.prepare("UPDATE holes SET par = :par, handicap = :hc WHERE course_id = :cid AND hole_num = :hnum;");
                query.bindValue(":par", hole->par);
                query.bindValue(":hc", hole->handicap);
                query.bindValue(":cid", m_currentCourseId);
                query.bindValue(":hnum", hole->holeNum);

                if (!query.exec()) {
                     qDebug() << "HolesTransposedModel::setData: ERROR updating database:" << query.lastError().text();
                     return false;
                }
            } else {
                 qDebug() << "HolesTransposedModel::setData: ERROR: Database connection invalid or closed for update.";
                 return false;
            }
            return true;
        }
    }
    return false;
}
