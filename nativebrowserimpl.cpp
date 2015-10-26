#include "nativebrowserimpl.h"

#include <QMetaObject>
#ifdef Q_OS_WIN
#include <QTimer>
#endif

#include "nativebrowser.h"

NativeBrowserImpl::NativeBrowserImpl()
    : parent_wnd(0)
{

}

NativeBrowserImpl::~NativeBrowserImpl()
{

}

NativeBrowserImpl *NativeBrowserImpl::createNewInstance(NativeBrowser *browserwindow)
{
    NativeBrowserImpl *result = createNewInstance(browserwindow->winId());
    result->setParent(browserwindow);
    result->parent_wnd = browserwindow;
    return result;
}

void NativeBrowserImpl::onProgress(int current_progress, int max_progress)
{
    int progress;
    if (current_progress < 0 || current_progress > max_progress || max_progress < 1)
    {
        progress = 100;
    }
    else
    {
        progress = int(double(current_progress)/max_progress);
    }
    if (!parent_wnd) return;
    parent_wnd->updateGeometry();
    emit parent_wnd->loadProgress(progress);
}

void NativeBrowserImpl::onLoadStart()
{
    if (!parent_wnd) return;
    emit parent_wnd->loadStarted();
}

void NativeBrowserImpl::onLoadFinish(bool success)
{
    if (!parent_wnd) return;
    parent_wnd->updateGeometry();
    emit parent_wnd->loadFinished(success);
}

void NativeBrowserImpl::onExternalNavigate(const QString &external_url)
{
    if (!parent_wnd) return;
    emit parent_wnd->externalNavigate(external_url);
}

void NativeBrowserImpl::queuedNavigate(const QString &url)
{
    if (!parent_wnd) return;
    QMetaObject::invokeMethod(parent_wnd, "load", Qt::QueuedConnection, Q_ARG(const QString&, url));
}
