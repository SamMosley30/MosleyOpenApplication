/**
 * @file Exporter.h
 * @brief Contains the declaration of the Exporter interface.
 */

#ifndef EXPORTER_H
#define EXPORTER_H

#include <QAbstractItemModel>

/**
 * @class Exporter
 * @brief An interface for exporting data from a model.
 *
 * This class defines the interface for exporting data from a QAbstractItemModel
 * to a specific format.
 */
class Exporter {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~Exporter() = default;

    /**
     * @brief Exports data from the given model.
     * @param model The model to export data from.
     * @return True if the export was successful, false otherwise.
     */
    virtual bool exportData(QAbstractItemModel *model) = 0;
};

#endif // EXPORTER_H
