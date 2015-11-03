# NativeWebBrowser
Very simple web browser widget for Qt.

It is based on current system IE core on Windows (IWebBrowser2) and Netscape core on Mac OS X (WebView).

Tested with Qt5.5.1. Can be used with Qt4 but on MAC QUrl::fromNSURL and QString::fromNSString must be replaced with something other.
