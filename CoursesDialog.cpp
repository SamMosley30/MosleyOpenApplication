#include "CoursesDialog.h"
#include "SpinBoxDelegate.h"
#include "CheckBoxDelegate.h"
#include <QtWidgets>
#include <QtSql>

CoursesDialog::CoursesDialog(QSqlDatabase &db, QWidget *parent)
    : QDialog(parent)
    , database(db)
    , courseModel(new QSqlTableModel(this, db))
    , holesTransposedModel(new HolesTransposedModel(db.connectionName(), this))
    , courseView(new QTableView(this))
    , holesView(new QTableView(this))
    , addButton(new QPushButton(tr("Add"), this))
    , removeButton(new QPushButton(tr("Remove"), this))
    , exportButton(new QPushButton(tr("Export"), this))
    , closeButton(new QPushButton(tr("Close"), this))
{
    // Models
    courseModel->setTable("courses");
    courseModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    courseModel->select();
    courseModel->setHeaderData(1, Qt::Horizontal, tr("Course Name"));

    // Views
    courseView->setModel(courseModel);
    courseView->hideColumn(0);
    courseView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    courseView->setSelectionBehavior(QAbstractItemView::SelectRows);
    courseView->setSelectionMode(QAbstractItemView::SingleSelection);

    holesView->setModel(holesTransposedModel);

    // --- Control Column Widths for the Holes View ---
    // Access the horizontal header of the holesView
    QHeaderView *header = holesView->horizontalHeader();

    // Set a default section size (optional, can also set fixed size per column)
    // header->setDefaultSectionSize(40); // Set a default size for all columns

    // Iterate through the 18 hole columns (logical index 0 to 17)
    // and set a fixed width for each.
    int fixedColumnWidth = 50; // Adjust this value based on desired width

    for (int i = 0; i < holesTransposedModel->columnCount(); ++i) {
        // Set the resize mode for this column to Fixed
        header->setSectionResizeMode(i, QHeaderView::Stretch);

        // Set the fixed size for this column
        header->resizeSection(i, fixedColumnWidth);

        // Optional: You might also set a minimum size if needed
        // header->setMinimumSectionSize(30);
    }

    // You might still want the vertical header (row headers "Par", "Handicap") to resize to content
    holesView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Buttons
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(courseView);
    mainLayout->addWidget(holesView);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
    setWindowTitle(tr("Course Database"));
    resize(1000,400);

    connect(addButton, &QPushButton::clicked, this, &CoursesDialog::addCourse);
    connect(removeButton, &QPushButton::clicked, this, &CoursesDialog::removeSelected);
    connect(exportButton, &QPushButton::clicked, this, &CoursesDialog::exportData);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *selModel = courseView->selectionModel();
    connect(selModel, &QItemSelectionModel::currentChanged,
            this, &CoursesDialog::onCourseSelectionChanged);
    connect(selModel, &QItemSelectionModel::selectionChanged,
        this, [this](const QItemSelection &sel, const QItemSelection&){
            auto idx = sel.indexes().value(0);
            if (idx.isValid()) onCourseSelectionChanged(idx);
        });

    // If you have at least one course already, select the first one:
    if (courseModel->rowCount() > 0) {
        courseView->selectRow(0);
        onCourseSelectionChanged(courseModel->index(0, 0));
}
}

void CoursesDialog::onCourseSelectionChanged(const QModelIndex &current) {
    int cid = courseModel->data(courseModel->index(current.row(), 0)).toInt();
    holesTransposedModel->setCourseId(cid);
}

void CoursesDialog::addCourse() {
    // 1) Insert the new course row
    int row = courseModel->rowCount();
    courseModel->insertRow(row);
    courseModel->setData(courseModel->index(row, 1), "New Course");
    courseModel->submitAll();  // commit to get an ID
    courseModel->select();

    // 2) Fetch its generated ID
    int courseId = courseModel->data(
        courseModel->index(row, /*id_column=*/0)
    ).toInt();

    // 3) Populate exactly 18 holes for that course
    QSqlQuery q(database);
    q.prepare("INSERT INTO holes (course_id, hole_num, par, handicap) "
              "VALUES (:cid, :hnum, :par, :hc)");
    for (int holeNum = 1; holeNum <= 18; ++holeNum) {
        q.bindValue(":cid", courseId);
        q.bindValue(":hnum", holeNum);
        q.bindValue(":par", 4);    // or whatever default par you choose
        q.bindValue(":hc", holeNum);
        q.exec();
    }
    // Refresh the model so the view picks up the new rows
    holesTransposedModel->setCourseId(courseId);

    // 4) Refresh the display
    courseModel->selectRow(row);
    onCourseSelectionChanged(courseModel->index(row, 0));
}

void CoursesDialog::removeSelected() {
    QModelIndexList selected = courseView->selectionModel()->selectedRows();
    if (selected.isEmpty()) 
    {
        return; 
    }

    // Mark rows for removal in the cache
    for (int i = selected.count() - 1; i >= 0; --i) 
    {
        int rowToRemove = selected.at(i).row();
        if (!courseModel->removeRow(rowToRemove)) 
        {
             return;
        }
    }

    bool success = courseModel->submitAll();
    courseModel->select(); 
}

void CoursesDialog::exportData() {
    // Use QFileDialog to get the base file path from the user
    QString baseFilePath = QFileDialog::getSaveFileName(this,
                                                       tr("Export Course Data"),
                                                       QDir::homePath(),
                                                       tr("CSV Files (*.csv);;All Files (*)")
                                                       );

    // Check if the user cancelled the dialog
    if (baseFilePath.isEmpty()) {
        return;
    }

    // Remove .csv extension if the user added it, as we'll add suffixes
    if (baseFilePath.toLower().endsWith(".csv")) {
        baseFilePath.chop(4); // Remove the last 4 characters (.csv)
    }

    QString courseFilePath = baseFilePath + "_Courses.csv";

    QStringList exportedFiles; // To list files successfully exported
    QStringList failedFiles; // To list files that failed to export

    // --- Export Course Data ---
    QFile courseFile(courseFilePath);
    if (courseFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&courseFile);
        out.setEncoding(QStringConverter::Utf8);

        // Write headers from courseModel
        QStringList courseHeaderLabels;
        for (int i = 0; i < courseModel->columnCount(); ++i) {
            // Get header data, handle quoting for CSV
            QString headerText = courseModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            courseHeaderLabels << QString("\"%1\"").arg(headerText.replace("\"", "\"\""));
        }
        out << courseHeaderLabels.join(",") << "\n";

        // Write data rows from courseModel
        for (int row = 0; row < courseModel->rowCount(); ++row) {
            QStringList rowData;
            for (int column = 0; column < courseModel->columnCount(); ++column) {
                QVariant cellData = courseModel->data(courseModel->index(row, column), Qt::DisplayRole);
                QString cellString = cellData.toString();
                // Handle CSV quoting
                if (cellString.contains(',') || cellString.contains('"') || cellString.contains('\n') || cellString.contains('\r')) {
                     rowData << QString("\"%1\"").arg(cellString.replace("\"", "\"\""));
                } else {
                     rowData << cellString;
                }
            }
            out << rowData.join(",") << "\n";
        }
        courseFile.close();
        exportedFiles << QDir::toNativeSeparators(courseFilePath);
        qDebug() << "Course data exported to:" << courseFilePath;

    } else {
        qCritical() << "Failed to open file for course data export:" << courseFilePath << "-" << courseFile.errorString();
         QMessageBox::critical(this, tr("File Error"),
                              tr("Could not open file for writing course data:\n%1\nError: %2")
                              .arg(QDir::toNativeSeparators(courseFilePath)).arg(courseFile.errorString()));
    }

    // --- Export Hole Data (for the currently selected course) ---
    // --- Export All Hole Data (Normalized Format) ---
    QString allHolesFilePath = baseFilePath + "_AllHoles_Normalized.csv";
    QFile allHolesFile(allHolesFilePath);
     if (allHolesFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&allHolesFile);
        out.setEncoding(QStringConverter::Utf8);

        QSqlQuery holesQuery(database);
        // Select all hole data, ordered by course and hole number for readability
        if (holesQuery.exec("SELECT course_id, id, hole_num, par, handicap FROM holes ORDER BY course_id, hole_num")) { // Added 'id' if you need the hole's primary key
            // Write header row
            QSqlRecord record = holesQuery.record();
            QStringList headerLabels;
            for (int i = 0; i < record.count(); ++i) {
                QString fieldName = record.fieldName(i);
                 headerLabels << QString("\"%1\"").arg(fieldName.replace("\"", "\"\""));
            }
            out << headerLabels.join(",") << "\n";

            // Write data rows
            while (holesQuery.next()) {
                QStringList rowData;
                for (int i = 0; i < record.count(); ++i) {
                     QVariant cellData = holesQuery.value(i);
                     QString cellString = cellData.toString();
                     // Handle CSV quoting
                    if (cellString.contains(',') || cellString.contains('"') || cellString.contains('\n') || cellString.contains('\r')) {
                         rowData << QString("\"%1\"").arg(cellString.replace("\"", "\"\""));
                    } else {
                         rowData << cellString;
                    }
                }
                out << rowData.join(",") << "\n";
            }
            allHolesFile.close();
            exportedFiles << QDir::toNativeSeparators(allHolesFilePath);
            qDebug() << "All Hole data (normalized) exported to:" << allHolesFilePath;
        } else {
            allHolesFile.close(); // Close file even on query error
            failedFiles << QDir::toNativeSeparators(allHolesFilePath);
            qCritical() << "Failed to query holes table:" << holesQuery.lastError().text();
        }

     } else {
        failedFiles << QDir::toNativeSeparators(allHolesFilePath);
        qCritical() << "Failed to open file for holes data export:" << allHolesFilePath << "-" << allHolesFile.errorString();
     }


    // Show a summary message if any files were exported
    if (!exportedFiles.isEmpty()) {
        QMessageBox::information(this, tr("Export Complete"),
                                 tr("Successfully exported the following files:\n%1").arg(exportedFiles.join("\n")));
    } else {
         QMessageBox::warning(this, tr("Export Complete"), tr("No data was exported. Check if courses and holes exist."));
    }
}
