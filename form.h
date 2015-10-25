#ifndef FORM_H
#define FORM_H

#include <QWidget>

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = 0);
    ~Form();

private slots:
    void on_button_clicked();

    void onExternalLink(const QString &url);
    void onStart();
    void onFinish(bool ok);

    void on_pushButton_clicked();

private:
    Ui::Form *ui;
};

#endif // FORM_H
