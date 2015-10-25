#include "nativebrowser.h"
#include "nativebrowserimpl.h"

#include <QResizeEvent>

NativeBrowser::NativeBrowser(QWidget *parent)
    : QWidget(parent)
    , browser(NativeBrowserImpl::createNewInstance(this))
{
}

NativeBrowser::~NativeBrowser()
{
    delete browser;
}

QString NativeBrowser::url() const
{
    return browser->location();
}

QSize NativeBrowser::sizeHint() const
{
    return browser->sizeHint();
}

void NativeBrowser::updateGeometry()
{
    QSize result = browser->sizeHint();
    setMinimumSize(result);
    QWidget::updateGeometry();
    setMinimumSize(0,0);
    QWidget::updateGeometry();
}

void NativeBrowser::load(const QString &url)
{
    browser->navigate(url);
}

void NativeBrowser::loadBlank()
{
    load("about:blank");
}

void NativeBrowser::resizeEvent(QResizeEvent *e)
{
    browser->setSize(e->size());
}

