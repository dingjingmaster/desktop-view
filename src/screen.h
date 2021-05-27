#ifndef SCREEN_H
#define SCREEN_H
#include <QHash>
#include <QRect>
#include <QSize>
#include <QScreen>
#include <QObject>
#include <QModelIndex>

class DesktopView;

class Screen : public QObject
{
    friend class DesktopView;
    Q_OBJECT
public:
    explicit Screen(QScreen *screen, QSize gridSize, QObject *parent = nullptr);

    bool isValidScreen();

    int maxRow() const;
    int maxColumn() const;

    QPoint placeItem(const QString &uri, QPoint lastPos = QPoint()); // try place item into an empty point, if failed return (-1, -1)
    QPoint itemGridPos(const QString &uri);
    bool setItemGridPos(const QString &uri, const QPoint &pos);
    bool setItemWithGlobalPos(const QString &uri, const QPoint &pos);
    void makeItemGridPosInvalid(const QString &uri);

    bool isItemOutOfGrid(const QString &uri);

    QPoint gridPosFromRelatedPosition(const QPoint &pos);
    QPoint gridPosFromGlobalPosition(const QPoint &pos);
    QPoint relatedPositionFromGridPos(const QPoint &pos);
    QPoint globalPositionFromGridPos(const QPoint &pos);

    QPoint getItemRelatedPosition(const QString &uri);
    QPoint getItemGlobalPosition(const QString &uri);
    QString getItemFromRelatedPosition(const QPoint &pos);
    QString getItemFromGlobalPosition(const QPoint &pos);

    QScreen *getScreen() const;
    QRect getGeometry() const;

    QStringList getAllItemsOnScreen();
    QStringList getItemsOutOfScreen();
    QStringList getItemsVisibleOnScreen();

    void setItemMetaInfoGridPos(const QString &uri, const QPoint &pos);
    QPoint getItemMetaInfoGridPos(const QString &uri);
    QStringList getItemsMetaGridPosOutOfScreen();
    QStringList getItemMetaGridPosVisibleOnScreen();

signals:
    void screenVisibleChanged(bool visible);

public slots:
    void rebindScreen(QScreen *screen);
    void onScreenGeometryChanged(const QRect &geometry);
    void onScreenGridSizeChanged(const QSize &gridSize);
    void setPanelMargins(const QMargins &margins);

protected:
    void recalculateGrid();

    DesktopView *getView();
    QString getIndexUri(const QModelIndex &index);

private:
    void clearItems();

private:
    QRect m_geometry;
    QSize m_gridSize;
    QMargins m_panelMargins;
    int m_maxRow = 0;
    int m_maxColumn = 0;
    QHash<QString, QPoint> m_items;
    QHash<QString, QPoint> m_itemsMetaPoses;

    QScreen *m_screen = nullptr;
};

#endif // SCREEN_H
