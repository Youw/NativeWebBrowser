#ifndef NATIVEBROWSERIMPL_H
#define NATIVEBROWSERIMPL_H

#include <QObject>

#include "nativebrowser.h"

class QPoint;
class QString;
class QTimer;

class NativeBrowserImpl: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(NativeBrowserImpl)
public:
    virtual ~NativeBrowserImpl();

    virtual void navigate(const QString &url) = 0;
    virtual QString location() const = 0;
    virtual void stop() = 0;

    virtual void setSize(const QSize& size) = 0;

    virtual QSize sizeHint() const = 0;

    static NativeBrowserImpl* createNewInstance(NativeBrowser *browserwindow);
protected:
    static NativeBrowserImpl* createNewInstance(WId browserwindow);
    NativeBrowserImpl();

    void onProgress(int current_progress, int max_progress);
    void onLoadStart();
    void onLoadFinish(bool success);

    void queuedNavigate(const QString &url);

protected slots:
    void onExternalNavigate(const QString &external_url);

private:
    NativeBrowser *parent_wnd;
#ifdef Q_OS_WIN
protected slots:
    virtual void navigateNotStarted() = 0;
    virtual void navigateFinished() = 0;
#endif
};

#endif // NATIVEBROWSERIMPL_H

