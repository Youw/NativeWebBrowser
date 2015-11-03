#include "form.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Form w;

    w.show();
    return app.exec();
}
