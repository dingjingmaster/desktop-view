//#include "example-window.h"

#include <QApplication>

#include <QFileSystemModel>
#include <QApplication>

#include "desktop-view.h"
#include "filesystem-model.h"

#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DesktopView v;
    FileSystemModel m;
    v.setModel(&m);
    v.showMaximized();

    for (int i = 0; i < 50; i++) {
        m.appendRow(new QStandardItem(QIcon::fromTheme("folder"), QString::number(i)));
    }

//#define TEST_GEOMETRY_CHANGED
#ifdef TEST_GEOMETRY_CHANGED
    QTimer::singleShot(2000, [&]{
        v._saveItemsPoses();
        a.primaryScreen()->geometryChanged(QRect(0, 0, 800, 600));
        QTimer::singleShot(2000, [&]{
            a.primaryScreen()->geometryChanged(a.primaryScreen()->geometry());
        });
    });
#endif

#define TEST_FLOAT_ITEM_AND_SCREEN
#ifdef TEST_FLOAT_ITEM_AND_SCREEN
    QTimer::singleShot(2000, [&](){
        m.removeRow(3);
    });
    QTimer::singleShot(3000, [&](){
        v._saveItemsPoses();
        m.removeRow(0);
    });
//    QTimer::singleShot(4000, [&](){
//        m.appendRow(new QStandardItem(QIcon::fromTheme("folder"), QString::number(300)));
//    });

    QTimer::singleShot(4000, [&]{
        a.primaryScreen()->geometryChanged(QRect(100, 0, 800, 600));
        if (a.screens().count() > 1) {
            a.screens().at(1)->geometryChanged(QRect(900, 600, 400, 300));
        }
    });

    QTimer::singleShot(8000, [&]{
        v.swapScreen(v.getScreen(0), v.getScreen(1));
        v.setGridSize(QSize(200, 300));
        v.setIconSize(QSize(128, 128));
    });
#endif


    return a.exec();
}
