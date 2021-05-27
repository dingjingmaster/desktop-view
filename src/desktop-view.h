#ifndef DESKTOPVIEW_H
#define DESKTOPVIEW_H

#include "screen.h"
#include <QAbstractItemView>

class DesktopViewPrivate;

class DesktopView : public QAbstractItemView
{
friend class Screen;
    Q_OBJECT
public:
    explicit DesktopView(QWidget *parent = nullptr);

    Screen *getScreen(int screenId);

    void addScreen(Screen *screen);
    void swapScreen(Screen *screen1, Screen *screen2);
    void removeScreen(Screen *screen);

    void setGridSize(QSize size);

    QRect visualRect(const QModelIndex &index) const override;
    QModelIndex indexAt(const QPoint &point) const override;
    QModelIndex findIndexByUri(const QString &uri) const;
    QString getIndexUri(const QModelIndex &index) const;

    bool trySetIndexToPos(const QModelIndex &index, const QPoint &pos);
    bool isIndexOverlapped(const QModelIndex &index);
    bool isItemOverlapped(const QString &uri);

    void scrollTo(const QModelIndex &index, ScrollHint hint) override {}

    void _saveItemsPoses(); //测试用

protected:
    void paintEvent(QPaintEvent *event) override;
    void dropEvent(QDropEvent *event) override; //可能改变metainfo
    void startDrag(Qt::DropActions supportedActions) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    QStyleOptionViewItem viewOptions() const override;

    int horizontalOffset() const override {return 0;}
    int verticalOffset() const override {return 0;}
    bool isIndexHidden(const QModelIndex &index) const override {return false;}
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion visualRegionForSelection(const QItemSelection &selection) const override;

    void setItemPosMetaInfo(const QString &uri, const QPoint &gridPos, int screenId = 0);

protected slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                     const QVector<int> &roles = QVector<int>()) override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override; //改变metainfo，浮动元素除外
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;

    void saveItemsPositions();

    void handleScreenChanged(Screen *screen); //不改变metainfo
    void handleGridSizeChanged(); //不改变metainfo

    void relayoutItems(const QStringList &uris);

    Screen *getItemScreen(const QString &uri);

private:
    QSize m_gridSize = QSize(100, 150);
    QList <Screen *> m_screens;

    QStringList m_items; //uris
    QStringList m_floatItems; //当有拖拽或者libpeony文件操作触发时，固定所有float元素并记录metaInfo
    QMap<QString, QPoint> m_itemsPosesCached;

    QPoint m_dragStartPos;

    QRubberBand *m_rubberBand = nullptr;
};

#endif // DESKTOPVIEW_H
