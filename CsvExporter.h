/**
 * @file CsvExporter.h
 * @brief Contains the declaration of the CsvExporter class.
 */

#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include "Exporter.h"

class QWidget;

/**
 * @class CsvExporter
 * @brief An implementation of the Exporter interface that exports data to a CSV file.
 */
class CsvExporter : public Exporter {
public:
    /**
     * @brief Constructs a CsvExporter object.
     * @param parent The parent widget for any dialogs.
     */
    explicit CsvExporter(QWidget *parent = nullptr);

    /**
     * @brief Exports data from the given model to a CSV file.
     * @param model The model to export data from.
     * @return True if the export was successful, false otherwise.
     */
    bool exportData(QAbstractItemModel *model) override;

private:
    QWidget *m_parent; ///< The parent widget.
};

#endif // CSVEXPORTER_H
