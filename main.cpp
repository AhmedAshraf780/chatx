
#include <QtWidgets/QApplication>
#include "Chat__Application.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Chat__Application w;
    w.show();
    return a.exec();
}
