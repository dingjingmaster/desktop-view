#include "screen.h"
#include "desktop-view.h"
#include <QDebug>

#define INVALID_POS QPoint(-1, -1)
#define ICONVIEW_PADDING 5

Screen::Screen(QScreen *screen, QSize gridSize, QObject *parent) : QObject(parent)
{
    if (!screen) {
        qCritical()<<"invalid screen";
        return;
    }

    m_screen = screen;
    m_geometry = screen->geometry();
    m_geometry.adjust(m_panelMargins.left(), m_panelMargins.top(), -m_panelMargins.right(), -m_panelMargins.bottom());
    m_gridSize = gridSize;
    recalculateGrid();
    connect(screen, &QScreen::geometryChanged, this, &Screen::onScreenGeometryChanged);
    connect(screen, &QScreen::destroyed, this, [=](){
        m_screen = nullptr;
        Q_EMIT screenVisibleChanged(false);
    });
}

bool Screen::isValidScreen()
{
    return m_screen;
}

int Screen::maxRow() const
{
    return m_maxRow;
}

int Screen::maxColumn() const
{
    return m_maxColumn;
}

void Screen::onScreenGeometryChanged(const QRect &geometry)
{
    if (!geometry.isEmpty()) {
        m_geometry = geometry;
        m_geometry.adjust(m_panelMargins.left(), m_panelMargins.top(), -m_panelMargins.right(), -m_panelMargins.bottom());
        recalculateGrid();
        getView()->handleScreenChanged(this);
    }
}

void Screen::onScreenGridSizeChanged(const QSize &gridSize)
{
    if (gridSize.isEmpty()) {
        qCritical()<<"invalid grid size";
        return;
    }

    m_gridSize = gridSize;
    recalculateGrid();
}

void Screen::setPanelMargins(const QMargins &margins)
{
    m_panelMargins = margins;
    m_geometry.adjust(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    recalculateGrid();

    getView()->handleScreenChanged(this);
}

void Screen::recalculateGrid()
{
    m_maxColumn = m_geometry.width()/m_gridSize.width();
    m_maxRow = m_geometry.height()/m_gridSize.height();
    if (/*m_geometry.width() % m_gridSize.width() == 0 && */m_maxColumn > 0) {
        m_maxColumn--;
    }
    if (/*m_geometry.height() % m_gridSize.height() == 0 && */m_maxRow > 0) {
        m_maxRow--;
    }
}

DesktopView *Screen::getView()
{
    return qobject_cast<DesktopView *>(parent());
}

QString Screen::getIndexUri(const QModelIndex &index)
{
    return index.data(Qt::UserRole).toString();
}

void Screen::clearItems()
{
    m_items.clear();
}

QRect Screen::getGeometry() const
{
    return m_geometry;
}

QStringList Screen::getAllItemsOnScreen()
{
    return m_items.keys();
}

QStringList Screen::getItemsOutOfScreen()
{
    QStringList list;
    for (auto uri : m_items.keys()) {
        auto gridPos = m_items.value(uri);
        if (gridPos.x() > m_maxColumn || gridPos.y() > m_maxRow) {
            list<<uri;
        }
    }

    return list;
}

QStringList Screen::getItemsVisibleOnScreen()
{
    if (!isValidScreen())
        return QStringList();
    QStringList list;
    for (auto uri : m_items.keys()) {
        auto gridPos = m_items.value(uri);
        if (gridPos.x() <= m_maxColumn && gridPos.y() <= m_maxRow) {
            list<<uri;
        }
    }

    return list;
}

void Screen::setItemMetaInfoGridPos(const QString &uri, const QPoint &pos)
{
    m_itemsMetaPoses.remove(uri);
    m_itemsMetaPoses.insert(uri, pos);
}

QPoint Screen::getItemMetaInfoGridPos(const QString &uri)
{
    return m_itemsMetaPoses.value(uri, INVALID_POS);
}

QStringList Screen::getItemsMetaGridPosOutOfScreen()
{
    QStringList list;
    if (!m_screen)
        return m_itemsMetaPoses.keys();
    for (auto uri : m_itemsMetaPoses.keys()) {
        auto gridPos = m_itemsMetaPoses.value(uri);
        if (gridPos.x() > m_maxColumn || gridPos.y() > m_maxRow) {
            list<<uri;
        }
    }

    return list;
}

QStringList Screen::getItemMetaGridPosVisibleOnScreen()
{
    QStringList list;
    if (m_screen) {
        for (auto uri : m_itemsMetaPoses.keys()) {
            auto gridPos = m_itemsMetaPoses.value(uri);
            if (gridPos.x() <= m_maxColumn && gridPos.y() <= m_maxRow && gridPos != INVALID_POS) {
                list<<uri;
            }
        }
    }

    return list;
}

QScreen *Screen::getScreen() const
{
    return m_screen;
}

QPoint Screen::placeItem(const QString &uri, QPoint lastPos)
{
    // remove current pos
    if (m_items.value(uri, INVALID_POS) != INVALID_POS) {
        m_items.remove(uri);
    }

    QPoint pos = INVALID_POS;
    int x = lastPos.x();
    int y = lastPos.y();
    while (x <= m_maxColumn && y <= m_maxRow) {
        // check if there is an index in this grid pos.
        auto tmp = QPoint(x, y);
        if (m_items.key(tmp).isEmpty()) {
            pos.setX(x);
            pos.setY(y);
            // FIXME:
            m_items.insert(uri, pos);
            return pos;
        } else {
            if (y + 1 <= m_maxRow) {
                y++;
            } else {
                y = 0;
                if (x + 1 <= m_maxColumn) {
                    x++;
                } else {
                    // out of grid, break
                    break;
                }
            }
            continue;
        }
    }
    return pos;
}

QPoint Screen::itemGridPos(const QString &uri)
{
    return m_items.value(uri, INVALID_POS);
}

void Screen::makeItemGridPosInvalid(const QString &uri)
{
    m_items.remove(uri);
}

bool Screen::isItemOutOfGrid(const QString &uri)
{
    auto pos = m_items.value(uri);
    if (pos == INVALID_POS) {
        return true;
    } else if (pos.x() > m_maxColumn || pos.y() > m_maxRow) {
        return false;
    }
    return true;
}

QPoint Screen::gridPosFromRelatedPosition(const QPoint &pos)
{
    if (!m_screen->geometry().contains(pos)) {
        return INVALID_POS;
    }
    int x = pos.x()/m_gridSize.width();
    int y = pos.y()/m_gridSize.height();
    return QPoint(x, y);
}

QPoint Screen::gridPosFromGlobalPosition(const QPoint &pos)
{
    auto relatedPos = pos - m_screen->geometry().topLeft();
    return gridPosFromRelatedPosition(relatedPos);
}

QPoint Screen::relatedPositionFromGridPos(const QPoint &pos)
{
    return QPoint(pos.x() * m_gridSize.width(), pos.y() * m_gridSize.height());
}

QPoint Screen::globalPositionFromGridPos(const QPoint &pos)
{
    return relatedPositionFromGridPos(pos) + m_geometry.topLeft();
}

QPoint Screen::getItemRelatedPosition(const QString &uri)
{
    if (!m_screen)
        return INVALID_POS;

    auto gridPos = m_items.value(uri, INVALID_POS);
    if (gridPos == INVALID_POS)
        return INVALID_POS;
    return QPoint(gridPos.x() * m_gridSize.width(), gridPos.y() * m_gridSize.height());
}

QPoint Screen::getItemGlobalPosition(const QString &uri)
{
    if (!m_screen)
        return INVALID_POS;

    auto gridPos = getItemRelatedPosition(uri);
    if (gridPos == INVALID_POS) {
        return INVALID_POS;
    }

    return getItemRelatedPosition(uri) + m_geometry.topLeft();
}

QString Screen::getItemFromRelatedPosition(const QPoint &pos)
{
    if (!m_screen) {
        return nullptr;
    }
    // used at indexAt(). need margins
    int x = pos.x()/m_gridSize.width();
    int y = pos.y()/m_gridSize.height();
    if (x <= m_maxColumn && x >= 0 && y <= m_maxRow && y >= 0) {
        // check if pos is closed to grid border.
        QRect visualRect = QRect(x * m_gridSize.width(), y * m_gridSize.height(), m_gridSize.width(), m_gridSize.height());
        visualRect.adjust(ICONVIEW_PADDING, ICONVIEW_PADDING, -ICONVIEW_PADDING, -ICONVIEW_PADDING);
        if (!visualRect.contains(pos)) {
            return nullptr;
        }
        return m_items.key(QPoint(x, y));
    } else {
        return nullptr;
    }
}

QString Screen::getItemFromGlobalPosition(const QPoint &pos)
{
    if (!m_screen) {
        return nullptr;
    }
    return getItemFromRelatedPosition(pos - m_screen->geometry().topLeft());
}

bool Screen::setItemGridPos(const QString &uri, const QPoint &pos)
{
    auto currentGridPos = m_items.value(uri);
    if (currentGridPos == pos)
        return true;

    if (pos.x() > m_maxColumn || pos.y() > m_maxRow || pos.x() < 0 || pos.y() < 0) {
        qWarning()<<"invalid grid pos";
        return false;
    }

    auto itemOnTargetPos = m_items.key(pos);
    if (itemOnTargetPos.isEmpty()) {
        m_items.insert(uri, pos);
        return true;
    } else {
        return false;
    }
}

bool Screen::setItemWithGlobalPos(const QString &uri, const QPoint &pos)
{
    if (m_screen && m_screen->geometry().contains(pos)) {
        auto relatedPos = pos - m_screen->geometry().topLeft();
        auto gridPos = QPoint(relatedPos.x()/m_gridSize.width(), relatedPos.y()/m_gridSize.height());
        return setItemGridPos(uri, gridPos);
    }
    return false;
}

void Screen::rebindScreen(QScreen *screen)
{
    if (!screen) {
        qCritical()<<"invalid screen";
        return;
    }

    if (m_screen) {
        m_screen->disconnect(m_screen, &QScreen::geometryChanged, this, 0);
        m_screen->disconnect(m_screen, &QScreen::destroyed, this, 0);
    }

    m_geometry = screen->geometry();
    m_geometry.adjust(m_panelMargins.left(), m_panelMargins.top(), -m_panelMargins.right(), -m_panelMargins.bottom());
    connect(screen, &QScreen::geometryChanged, this, &Screen::onScreenGeometryChanged);
    connect(screen, &QScreen::destroyed, this, [=](){
        m_screen = nullptr;
        Q_EMIT screenVisibleChanged(false);
    });
    Q_EMIT screenVisibleChanged(true);
}
