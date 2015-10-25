#include "form.h"

#include <QApplication>

#include <Windows.h>


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    OleInitialize(nullptr);

    Form w;

    w.show();
    return app.exec();
}
