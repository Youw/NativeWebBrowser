#include "nativebrowserimpl.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include <QEvent>
#include <QUrl>
#include <QResizeEvent>
#include <QString>

class MacNativeBrowserImpl;

@interface WebViewNotificationListener : NSObject <WebFrameLoadDelegate, WebUIDelegate, WebPolicyDelegate>
{
    bool download_success;
    int current_porgress;
    MacNativeBrowserImpl *web_view_impl;
}

- (void) initNativeWebView:(MacNativeBrowserImpl *)view_impl;

@end

class MacNativeBrowserImpl : public NativeBrowserImpl
{
public:
    MacNativeBrowserImpl(NSView *native_window)
    {
        Q_ASSERT_X(native_window, "MacNativeBrowserImpl", "Cannot init MacNativeBrowserImpl without native NSView");
        web = [[WebView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
        notification_listener = [[WebViewNotificationListener alloc] init];
        [notification_listener initNativeWebView:this];
        [web setFrameLoadDelegate:notification_listener];
        [web setUIDelegate:notification_listener];
        [web setPolicyDelegate:notification_listener];

        [[NSNotificationCenter defaultCenter] addObserver:notification_listener selector:@selector(_webViewProgressStarted:) name:WebViewProgressStartedNotification object:web];
        [[NSNotificationCenter defaultCenter] addObserver:notification_listener selector:@selector(_webViewProgressFinished:) name:WebViewProgressFinishedNotification object:web];
        [[NSNotificationCenter defaultCenter] addObserver:notification_listener selector:@selector(_webViewProgressEstimateChanged:) name:WebViewProgressEstimateChangedNotification object:web];
        [native_window addSubview:web];
    }

    ~MacNativeBrowserImpl()
    {
        [web retain];
        [web removeFromSuperview];
        [web setFrameLoadDelegate:NULL];
        [web release];
        [notification_listener release];
    }

    void navigate(const QString &url_str) override
    {
        QUrl url = QUrl::fromUserInput(url_str);
        current_url_host = url.host();
        web.mainFrameURL = url.toString(QUrl::EncodeUnicode).toNSString();
    }

    QString location() const override
    {
        return QString::fromNSString(web.mainFrameURL);
    }

    bool locationIsSame(const QUrl &url)
    {
        return current_url_host == url.host();
    }

    void externalNatigate(const QUrl &url)
    {
        onExternalNavigate(url.toString());
    }

    void externalNatigateQueued(const QUrl &url)
    {
        QMetaObject::invokeMethod(this, "onExternalNavigate", Qt::QueuedConnection, Q_ARG(const QUrl&, url.toString()));
    }

    void stop() override
    {
        [web stopLoading:web];
    }

    void setSize(const QSize& size) override
    {
        [web setFrameSize:NSMakeSize(size.width(), size.height())];
        [web setNeedsDisplay:YES];
    }

    QSize sizeHint() const override
    {
        NSRect webFrameRect = [[[web.mainFrame frameView] documentView] frame];
        return QSize(webFrameRect.size.width, webFrameRect.size.height);
    }

    inline void pageLoadStarted()
    {
        onLoadStart();
    }

    inline void pageLoadFinished(bool success)
    {
        if (success)
        {
            QUrl cur_url =  QUrl::fromUserInput(location());
            current_url_host = cur_url.host();
        }
        onLoadFinish(success);
    }

    inline void pageLoadProgress(int percentage)
    {
        onProgress(percentage, 100);
    }

    inline double loadEstimate()
    {
        return [web estimatedProgress];
    }

private:
    WebView *web;
    WebViewNotificationListener *notification_listener;
    QString current_url_host;
};

@implementation WebViewNotificationListener

- (void) initNativeWebView:(MacNativeBrowserImpl *)view_impl
{
    web_view_impl = view_impl;
}

// ------- WebPolicyDelegate --------

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation
                                                           request:(NSURLRequest *)request
                                                             frame:(WebFrame *)frame
                                                  decisionListener:(id<WebPolicyDecisionListener>)listener
{
    Q_UNUSED(webView)
    Q_UNUSED(frame)
    if (WebNavigationTypeLinkClicked == [[actionInformation objectForKey:WebActionNavigationTypeKey] intValue])
    {
        if (web_view_impl->locationIsSame(QUrl::fromNSURL(request.URL)))
        {
            [listener use];
        }
        else
        {
            web_view_impl->externalNatigateQueued(QUrl::fromNSURL(request.URL));
            [listener ignore];
        }
    }
    else
    {
        [listener use];
    }
}


-(void)webView:(WebView *)webView decidePolicyForNewWindowAction:(NSDictionary *)actionInformation
                                                         request:(NSURLRequest *)request
                                                    newFrameName:(NSString *)frameName
                                                decisionListener:(id <WebPolicyDecisionListener>)listener
{
    Q_UNUSED(webView)
    Q_UNUSED(frameName)
    if (WebNavigationTypeLinkClicked == [[actionInformation objectForKey:WebActionNavigationTypeKey] intValue])
    {
        web_view_impl->externalNatigate(QUrl::fromNSURL(request.URL));
    }
    [listener ignore];
}

// ------- WebUIDelegate --------

- (NSArray *)webView:(WebView *)sender contextMenuItemsForElement:(NSDictionary *)element defaultMenuItems:(NSArray *)defaultMenuItems
{
    Q_UNUSED(sender)
    Q_UNUSED(element)
    Q_UNUSED(defaultMenuItems)
    // disable right-click context menu
    return nil;
}

// ------- WebFrameLoadDelegate --------

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    Q_UNUSED(error)
    if (frame == [sender mainFrame])
    {
        download_success = false;
    }
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    Q_UNUSED(error)
    if (frame == [sender mainFrame])
    {
        download_success = false;
    }
}

// -------- Notifications ------

- (void)_webViewProgressStarted:(NSNotification *)notification
{
    Q_UNUSED(notification)
    download_success = true;
    current_porgress = 0;
    web_view_impl->pageLoadStarted();
}

- (void)_webViewProgressFinished:(NSNotification *)notification
{
    Q_UNUSED(notification)
    web_view_impl->pageLoadFinished(download_success);
}

- (void)_webViewProgressEstimateChanged:(NSNotification *)notification
{
    Q_UNUSED(notification)
    current_porgress = int(100 * web_view_impl->loadEstimate());
    web_view_impl->pageLoadProgress(current_porgress);
}

@end

NativeBrowserImpl* NativeBrowserImpl::createNewInstance(WId browserwindow)
{
    return new MacNativeBrowserImpl(reinterpret_cast<NSView *>(browserwindow));
}
