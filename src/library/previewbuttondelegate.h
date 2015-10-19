_Pragma("once")
#include <QStyledItemDelegate>
#include <QPushButton>
#include <QTableView>

#include "trackinfoobject.h"

class ControlObject;
class PreviewButtonDelegate : public QStyledItemDelegate {
  Q_OBJECT
  public:
    explicit PreviewButtonDelegate(QObject* parent = nullptr, int column = 0);
    virtual ~PreviewButtonDelegate();
    QWidget* createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option,const QModelIndex &index) const;
  signals:
    void loadTrackToPlayer(TrackPointer Track, QString group, bool play);
    void buttonSetChecked(bool);
  public slots:
    void cellEntered(const QModelIndex &index);
    void buttonClicked();
    void previewDeckPlayChanged(double v);
  private:
    QTableView* m_pTableView;
    ControlObject* m_pPreviewDeckPlay;
    QPushButton* m_pButton;
    bool m_isOneCellInEditMode;
    QPersistentModelIndex m_currentEditedCellIndex;
    int m_column;
};
