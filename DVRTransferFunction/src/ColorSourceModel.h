#pragma once

#include "Dataset.h"

#include <QStandardItemModel>

using namespace mv;

/**
 * Color source model class
 *
 * Model which defines the sources to color scatter plot points (by constant or by dataset)
 *
 * @author Thomas Kroes
 */
class ColorSourceModel : public QStandardItemModel
{

    Q_OBJECT

protected:

    /** (Default) constructor */
    ColorSourceModel(QObject* parent = nullptr);

    /** Standard model item class for interacting with the dataset name */
    class ConstantColorItem final : public QStandardItem {
    public:

        /**
         * Get model data for \p role
         * @return Data for \p role in variant form
         */
        QVariant data(int role = Qt::UserRole + 1) const override;
    };

    /** Standard model item class for interacting with the dataset name */
    class ScatterLayoutItem final : public QStandardItem {
    public:

        /**
         * Get model data for \p role
         * @return Data for \p role in variant form
         */
        QVariant data(int role = Qt::UserRole + 1) const override;
    };

    /** Base standard model item class for dataset */
    class DatasetItem : public QStandardItem, public QObject {
    public:

        /**
         * Construct with \p dataset
         * @param dataset Pointer to dataset to display item for
         * @param colorSourceModel Pointer to owning color source model
         */
        DatasetItem(Dataset<DatasetImpl> dataset, ColorSourceModel* colorSourceModel);

        /**
         * Get model data for \p role
         * @return Data for \p role in variant form
         */
        QVariant data(int role = Qt::UserRole + 1) const override;

        /**
         * Get dataset
         * return Dataset to display item for
         */
        Dataset<DatasetImpl>& getDataset();

    private:
        Dataset<DatasetImpl>    _dataset;           /** Pointer to dataset to display item for */
        ColorSourceModel*       _colorSourceModel;  /** Pointer to owning color source model */
    };

public:

    /**
     * Add \p dataset to the color model
     * @param dataset Smart pointer to dataset
     */
    void addDataset(Dataset<DatasetImpl> dataset);

    /**
     * Remove \p dataset from the color model
     * @param dataset Smart pointer to dataset
     */
    void removeDataset(Dataset<DatasetImpl> dataset);

    /** Remove all datasets from the model */
    void removeAllDatasets();

    /**
     * Get datasets
     * @return Vector of smart pointers to datasets
     */
    Datasets getDatasets() const;

    /**
     * Get dataset at \p rowIndex
     * @param rowIndex Index of the row
     * @return Smart pointer to dataset
     */
    Dataset<DatasetImpl> getDataset(std::int32_t rowIndex) const;

    /**
     * Get the row index of \p dataset
     * @param parent Parent model index
     * @return Row index of the dataset
     */
    int rowIndex(Dataset<DatasetImpl> dataset) const;

public: // Full path name vs name

    /** Get whether to show the full path name in the GUI */
    bool getShowFullPathName() const;

    /**
     * Set whether to show the full path name in the GUI
     * @param showFullPathName Whether to show the full path name in the GUI
     */
    void setShowFullPathName(const bool& showFullPathName);

signals:

    /**
     * Signals that show full path name changed to \p showFullPathName
     * @param showFullPathName Current show full path name
     */
    void showFullPathNameChanged(bool showFullPathName);

    void dataChanged(const Dataset<DatasetImpl>& dataset);

protected:
    bool        _showFullPathName;      /** Whether to show the full path name in the GUI */

    friend class ColoringAction;
};
