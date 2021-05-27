#include "desktop-view.h"
#include <QRect>
#include "private/qabstractitemview_p.h"
#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include <QApplication>
#include <QPaintEvent>
#include <QPainter>

#include <QDropEvent>

#include <QDebug>

#define SCREEN_ID 1000
#define RELATED_GRID_POSITION 1001

#define ICONVIEW_PADDING 5
#define INVALID_POS QPoint(-1, -1)

DesktopView::DesktopView(QWidget *parent) : QAbstractItemView(parent)
{
    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // init grid size
    setIconSize(QSize(64, 64));

    setWindowFlag(Qt::FramelessWindowHint);

    QIcon::setThemeName("ukui-icon-theme-default");

    for (auto qscreen : qApp->screens()) {
        auto screen = new Screen(qscreen, m_gridSize, this);
        addScreen(screen);
    }
}

Screen *DesktopView::getScreen(int screenId)
{
    if (m_screens.count() > screenId) {
        return m_screens.at(screenId);
    } else {
        return nullptr;
    }
}

void DesktopView::addScreen(Screen *screen)
{
    connect(screen, &Screen::screenVisibleChanged, this, [=](bool visible){
        if (!visible) {
            removeScreen(screen);
        }
    });
    m_screens<<screen;
}

void DesktopView::swapScreen(Screen *screen1, Screen *screen2)
{
    // primary screen should alwayls be the header of list.
    if (!screen1 || !screen2) {
        qCritical()<<"invalide screen arguments";
        return;
    }

    auto s1 = screen1->getScreen();
    auto s2 = screen2->getScreen();

    screen1->rebindScreen(s2);
    screen2->rebindScreen(s1);

    // list操作
    int index1 = m_screens.indexOf(screen1);
    int index2 = m_screens.indexOf(screen2);
    m_screens.replace(index1, screen2);
    m_screens.replace(index2, screen1);

    this->handleScreenChanged(screen1);
    this->handleScreenChanged(screen2);
}

void DesktopView::removeScreen(Screen *screen)
{
    if (!screen) {
        qCritical()<<"invalid screen id";
        return;
    }

    this->handleScreenChanged(screen);
}

void DesktopView::setGridSize(QSize size)
{
    m_gridSize = size;
    for (auto screen : m_screens) {
        screen->onScreenGridSizeChanged(size);
    }

    for (auto screen : m_screens) {
        //越界图标重排
        handleScreenChanged(screen);
    }
}

QRect DesktopView::visualRect(const QModelIndex &index) const
{
    auto rect = QRect(0, 0, m_gridSize.width(), m_gridSize.height());
    // FIXME: uri
    auto uri = getIndexUri(index);
    auto pos = m_itemsPosesCached.value(uri);
    rect.translate(pos);
    return rect.adjusted(ICONVIEW_PADDING, ICONVIEW_PADDING, -ICONVIEW_PADDING, -ICONVIEW_PADDING);
}

QModelIndex DesktopView::indexAt(const QPoint &point) const
{
    QList<QRect> visualRects;
    for (int row = 0; row < model()->rowCount(); row++) {
        auto index = model()->index(row, 0);
        visualRects<<visualRect(index);
    }
    for (auto rect : visualRects) {
        if (rect.contains(point)) {
            int row = visualRects.indexOf(rect);
            return model()->index(row, 0);
        }
    }
    return QModelIndex();
}

QModelIndex DesktopView::findIndexByUri(const QString &uri) const
{
    if (model()) {
        for (int row = 0; row < model()->rowCount(); row++) {
            //FIXME:
            auto index = model()->index(row, 0);
            auto indexUri = getIndexUri(index);
            if (indexUri == uri) {
                return index;
            }
        }
    }

    return QModelIndex();
}

QString DesktopView::getIndexUri(const QModelIndex &index) const
{
    return index.data(Qt::UserRole).toString();
}

bool DesktopView::trySetIndexToPos(const QModelIndex &index, const QPoint &pos)
{
    for (auto screen : m_screens) {
        if (!screen->isValidScreen()) {
            continue;
        }
        if (screen->setItemWithGlobalPos(getIndexUri(index), pos)) {
            //清空其它屏幕关于此index的gridPos?
            for (auto screen : m_screens) {
                screen->makeItemGridPosInvalid(getIndexUri(index));
            }
            //效率？
            screen->setItemWithGlobalPos(getIndexUri(index), pos);
            m_itemsPosesCached.insert(getIndexUri(index), screen->getItemGlobalPosition(getIndexUri(index)));
            return true;
        } else {
            //不改变位置
        }
    }
    return false;
}

bool DesktopView::isIndexOverlapped(const QModelIndex &index)
{
    return isItemOverlapped(getIndexUri(index));
}

bool DesktopView::isItemOverlapped(const QString &uri)
{
    auto itemPos = m_itemsPosesCached.value(uri);
    QStringList overrlappedItems;
    for (auto item : m_items) {
        if (m_itemsPosesCached.value(item) == itemPos) {
            overrlappedItems<<item;
            if (overrlappedItems.count() > 2) {
                break;
            }
        }
    }

    overrlappedItems.removeOne(uri);
    if (!overrlappedItems.isEmpty()) {
        return true;
    } else {
        return false;
    }
}

void DesktopView::_saveItemsPoses()
{
    this->saveItemsPositions();
}

void DesktopView::paintEvent(QPaintEvent *event)
{
    qDebug()<<"paint evnet";
    QPainter p(viewport());
    if (!event->rect().isEmpty()) {
        // update one index only, used for dataChanged
    } else {
        // update all indexes, fixme: more effective?
    }

    // test
    for (auto item : m_items) {
        auto index = findIndexByUri(item);
        QStyleOptionViewItem opt = viewOptions();
        opt.text = index.data().toString();
        opt.icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        opt.rect = visualRect(index);
        opt.state |= QStyle::State_Enabled;
        if (selectedIndexes().contains(index)) {
            opt.state |= QStyle::State_Selected;
        }
        qApp->style()->drawControl(QStyle::CE_ItemViewItem, &opt, &p, this);
    }
}

void DesktopView::dropEvent(QDropEvent *event)
{
    // 有bug
    //计算全体偏移量，验证越界和重叠图标，重新排序
    if (event->source() == this) {
        QPoint offset = event->pos() - m_dragStartPos;
        QHash<QModelIndex, QPoint> indexesPoses;
        auto indexes = selectedIndexes();
        QStringList itemsNeedBeRelayouted;
        for (auto index : indexes) {
            //清空屏幕关于此index的gridPos
            for (auto screen : m_screens) {
                screen->makeItemGridPosInvalid(getIndexUri(index));
            }
        }

        for (auto index : indexes) {
            bool successed = false;
            auto sourceRect = visualRect(index);
            sourceRect.adjust(-ICONVIEW_PADDING, -ICONVIEW_PADDING, ICONVIEW_PADDING, ICONVIEW_PADDING);
            sourceRect.translate(offset);
            indexesPoses.insert(index, sourceRect.center());

            for (auto screen : m_screens) {
                if (!screen->isValidScreen()) {
                    continue;
                }
                if (screen->setItemWithGlobalPos(getIndexUri(index), sourceRect.center())) {
                    successed = true;

                    //效率？
                    screen->setItemWithGlobalPos(getIndexUri(index), sourceRect.center());
                    screen->setItemMetaInfoGridPos(getIndexUri(index), screen->itemGridPos(getIndexUri(index)));
                    setItemPosMetaInfo(getIndexUri(index), screen->itemGridPos(getIndexUri(index)), m_screens.indexOf(screen));
                    //覆盖原有index的位置
                    m_itemsPosesCached.insert(getIndexUri(index), screen->getItemGlobalPosition(getIndexUri(index)));

                    //TODO: 设置metainfo的位置
                } else {
                    //不改变位置
                }
            }
            if (!successed) {
                //排列失败，这个元素被列为浮动元素
                itemsNeedBeRelayouted<<getIndexUri(index);
            }
        }
        relayoutItems(itemsNeedBeRelayouted);
    } else {

    }

    viewport()->update();
}

void DesktopView::startDrag(Qt::DropActions supportedActions)
{
    QAbstractItemView::startDrag(supportedActions);
}

void DesktopView::mousePressEvent(QMouseEvent *event)
{
    QAbstractItemView::mousePressEvent(event);
    m_dragStartPos = event->pos();
    qDebug()<<m_dragStartPos;
}

void DesktopView::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractItemView::mouseMoveEvent(event);

    if (!indexAt(m_dragStartPos).isValid() && event->buttons() & Qt::LeftButton) {
        if (m_rubberBand->size().width() > 100 && m_rubberBand->height() > 100) {
            m_rubberBand->setVisible(true);
        }
        setSelection(m_rubberBand->geometry(), QItemSelectionModel::Select|QItemSelectionModel::Deselect);
    } else {
        m_rubberBand->setVisible(false);
    }

    m_rubberBand->setGeometry(QRect(m_dragStartPos, event->pos()).normalized());
}

void DesktopView::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractItemView::mouseReleaseEvent(event);
    m_rubberBand->hide();
}

QStyleOptionViewItem DesktopView::viewOptions() const
{
    QStyleOptionViewItem item;
    item.decorationAlignment = Qt::AlignHCenter|Qt::AlignBottom;
    item.decorationSize = iconSize();
    item.decorationPosition = QStyleOptionViewItem::Position::Top;
    item.displayAlignment = Qt::AlignHCenter|Qt::AlignTop;
    item.features = QStyleOptionViewItem::HasDecoration|QStyleOptionViewItem::HasDisplay|QStyleOptionViewItem::WrapText;
    item.font = qApp->font();
    item.fontMetrics = qApp->fontMetrics();
    return item;
}

void DesktopView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    qDebug()<<"set selection";
    // FIXME:
    if (m_rubberBand->isVisible()) {
        for (int row = 0; row < model()->rowCount(); row++) {
            auto index = model()->index(row, 0);
            auto indexRect = visualRect(index);
            if (rect.intersects(indexRect)) {
                selectionModel()->select(index, QItemSelectionModel::Select);
            } else {
                selectionModel()->select(index, QItemSelectionModel::Deselect);
            }
        }
    } else {
        auto index = indexAt(rect.topLeft());
        selectionModel()->select(index, command);
    }
}

QRegion DesktopView::visualRegionForSelection(const QItemSelection &selection) const
{
    QRegion visualRegion;

    for (auto index : selection.indexes()) {
        visualRegion += visualRect(index);
    }

    return visualRegion;
}

void DesktopView::setItemPosMetaInfo(const QString &uri, const QPoint &gridPos, int screenId)
{
    // FIXME:
}

void DesktopView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    auto string = topLeft.data().toString();
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)
    auto demageRect = visualRect(topLeft);
    viewport()->update(demageRect);
}

void DesktopView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i <= end ; i++) {
        auto index = model()->index(i, 0);
        m_items.append(getIndexUri(index));
        // FIXME: check if index has metainfo postion
        if (false) {

        } else {
            // add index to float items.
            m_floatItems<<getIndexUri(index);
            for (auto screen : m_screens) {
                // fixme: improve layout speed with cached position
                auto gridPos = screen->placeItem(getIndexUri(index));
                if (gridPos.x() >= 0) {
                    m_itemsPosesCached.insert(getIndexUri(index), screen->getItemGlobalPosition(getIndexUri(index)));
                    break;
                }
            }
        }
    }

    viewport()->update();
}

void DesktopView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(end)
    auto indexAboutToBeRemoved = model()->index(start, 0);

    m_itemsPosesCached.remove(getIndexUri(indexAboutToBeRemoved));
    m_items.removeOne(getIndexUri(indexAboutToBeRemoved));
    m_floatItems.removeOne(getIndexUri(indexAboutToBeRemoved));
    for (auto screen : m_screens) {
        screen->makeItemGridPosInvalid(getIndexUri(indexAboutToBeRemoved));
    }

    // 重排浮动元素
    relayoutItems(m_floatItems);

    viewport()->update();
}

void DesktopView::saveItemsPositions()
{
    //非越界元素的确认，越界元素不应该保存位置
    QStringList itemOnAllScreen;
    for (auto screen : m_screens) {
        itemOnAllScreen<<screen->getItemsVisibleOnScreen();
    }

    for (auto item : itemOnAllScreen) {
        //检查当前位置是否有重叠，如果有，则不确认
        bool isOverlapped = isItemOverlapped(item);
        if (!isOverlapped) {
            //从浮动元素中排除
            m_floatItems.removeOne(item);
            //设置metainfo
            auto screen = getItemScreen(item);
            if (screen) {
                int screenId = m_screens.indexOf(screen, 0);
                QPoint gridPos = screen->itemGridPos(item);
                screen->setItemMetaInfoGridPos(item, gridPos);
                setItemPosMetaInfo(item, gridPos, screenId);
            }
        }
    }
}

void DesktopView::handleScreenChanged(Screen *screen)
{
    QStringList itemsNeedBeRelayouted = screen->getAllItemsOnScreen();
    // 优先排列界内的有metainfo的图标
    auto itemsMetaGridPosOnScreen = screen->getItemMetaGridPosVisibleOnScreen();
    for (auto uri : itemsMetaGridPosOnScreen) {
        itemsNeedBeRelayouted.removeOne(uri);
        auto gridPos = screen->getItemMetaInfoGridPos(uri);
        m_itemsPosesCached.insert(uri, screen->globalPositionFromGridPos(gridPos));
    }
    // sort?
    relayoutItems(itemsNeedBeRelayouted);

//    if (!screen->isValidScreen()) {
//        // 对越界图标进行重排，但是不记录位置
//        auto items = screen->getAllItemsOnScreen();
//        itemsNeedBeRelayouted<<items;
//    } else {
//        // 更新屏幕内图标的位置
//        auto items = screen->getItemsVisibleOnScreen();
//        for (auto item : items) {
//            auto pos = screen->getItemGlobalPosition(item);
//            m_itemsPosesCached.insert(item, pos);
//        }

//        // 对越界图标进行重排，但是不记录位置
//        items = screen->getItemsOutOfScreen();
//        itemsNeedBeRelayouted<<items;
//        relayoutItems(items);
//    }

    viewport()->update();
}

void DesktopView::handleGridSizeChanged()
{
    // 保持index相对的grid位置不变，对越界图标进行处理
    for (auto screen : m_screens) {
        screen->onScreenGridSizeChanged(m_gridSize);
        // 更新屏幕内图标的位置

        // 记录越界图标
        handleScreenChanged(screen);
    }
    // 重排越界图标

    viewport()->update();
}

void DesktopView::relayoutItems(const QStringList &uris)
{
    for (auto uri : uris) {
        for (auto screen : m_screens) {
            screen->makeItemGridPosInvalid(uri);
        }
    }

    for (auto uri : uris) {
        for (auto screen : m_screens) {
            QPoint currentGridPos = QPoint();
            // fixme: improve layout speed with cached position
            currentGridPos = screen->placeItem(uri, currentGridPos);
            if (currentGridPos != INVALID_POS) {
                m_itemsPosesCached.insert(uri, screen->getItemGlobalPosition(uri));
                break;
            } else {
                // FIXME:
                // no place to place items
                m_itemsPosesCached.remove(uri);
            }
        }
    }
}

Screen *DesktopView::getItemScreen(const QString &uri)
{
    auto itemPos = m_itemsPosesCached.value(uri);
    for (auto screen : m_screens) {
        if (screen->isValidScreen()) {
            if (screen->getGeometry().contains(itemPos))
                return screen;
        }
    }

    // should not happend
    return nullptr;
}
