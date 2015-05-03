// sidebarmodel.h
// Created 8/21/09 by RJ Ryan (rryan@mit.edu)

#ifndef SIDEBARMODEL_H
#define SIDEBARMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QVariant>

class LibraryFeature;

class SidebarModel : public QAbstractItemModel {
    Q_OBJECT
  public:
    explicit SidebarModel(QObject* parent = 0);
    virtual ~SidebarModel();

    void addLibraryFeature(LibraryFeature* feature);
    QModelIndex getDefaultSelection();
    void setDefaultSelection(unsigned int index);
    void activateDefaultSelection();

    // Required for QAbstractItemModel
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const;
    bool dropAccept(const QModelIndex& index, QList<QUrl> urls, QObject* pSource);
    bool dragMoveAccept(const QModelIndex& index, QUrl url);
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

  public slots:
    void clicked(const QModelIndex& index);
    void doubleClicked(const QModelIndex& index);
    void rightClicked(const QPoint& globalPos, const QModelIndex& index);
    void onFeatureSelect(LibraryFeature* pFeature, const QModelIndex& index = QModelIndex());

    // Slots for every single QAbstractItemModel signal
    // void onColumnsAboutToBeInserted(const QModelIndex& parent, int start, int end);
    // void onColumnsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    // void onColumnsInserted(const QModelIndex& parent, int start, int end);
    // void onColumnsRemoved(const QModelIndex& parent, int start, int end);
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex & bottomRight);
    //void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);
    // void onLayoutAboutToBeChanged();
    // void onLayoutChanged();
    // void onModelAboutToBeReset();
    // void onModelReset();
    void onRowsAboutToBeInserted(const QModelIndex& parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void onRowsInserted(const QModelIndex& parent, int start, int end);
    void onRowsRemoved(const QModelIndex& parent, int start, int end);
    void onModelReset();
    void onFeatureIsLoading(LibraryFeature*, bool selectFeature);
    void onFeatureLoadingFinished(LibraryFeature*);

  signals:
    void selectIndex(const QModelIndex& index);

  private:
    QModelIndex translateSourceIndex(const QModelIndex& parent);
    void featureRenamed(LibraryFeature*);
    QList<LibraryFeature*> m_sFeatures;
    unsigned int m_iDefaultSelectedIndex; /** Index of the item in the sidebar model to select at startup. */
};

#endif /* SIDEBARMODEL_H */
