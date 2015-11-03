#include "form.h"
#include "ui_form.h"

#include <QDebug>
#include <QDesktopServices>
#include <QUrl>

Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);
    connect(ui->browser, SIGNAL(externalNavigate(QString)), this, SLOT(onExternalLink(QString)));
    connect(ui->browser, SIGNAL(loadProgress(int)), ui->progressBar, SLOT(setValue(int)));

    connect(ui->browser, SIGNAL(loadStarted()), this, SLOT(onStart()));
    connect(ui->browser, SIGNAL(loadFinished(bool)), this, SLOT(onFinish(bool)));
}

Form::~Form()
{
    delete ui;
}

void Form::on_button_clicked()
{
    ui->browser->load(ui->edit->text());
}

void Form::onExternalLink(const QString &url)
{
    QDesktopServices::openUrl(QUrl::fromUserInput(url));
}

void Form::onStart()
{
    qDebug() << "start";
}

void Form::onFinish(bool ok)
{
    qDebug() << "finish:" << ok;
}

void Form::on_pushButton_clicked()
{
    adjustSize();
}
