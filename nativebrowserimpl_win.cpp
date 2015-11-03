#include "nativebrowserimpl.h"

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <atlbase.h> // for CComPtr<>
#include <comdef.h> // for variant_t
#include <Exdisp.h>
#include <ExDispid.h>
#include <MsHtmHst.h>
#include <MsHTML.h>
#include <strsafe.h>
#include <Windows.h>

#include <string>

using std::wstring;

#include <QDebug>
#include <QString>
#include <QTimer>
#include <QUrl>

namespace {

static bool SetBrowserFeatureControlKey(wstring feature, const wchar_t *appName, DWORD value) {
  HKEY key;
  bool success = true;
  wstring featuresPath(L"Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\");
  wstring path(featuresPath + feature);

  LONG nError = RegCreateKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &key, NULL);
  if (nError != ERROR_SUCCESS) {
    success = false;

  } else {
    nError = RegSetValueExW(key, appName, 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
    if (nError != ERROR_SUCCESS) {
      success = false;
    }

    nError = RegCloseKey(key);
    if (nError != ERROR_SUCCESS) {
      success = false;
    }
  }

  return success;
}

static LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue) {
  nValue = nDefaultValue;
  DWORD dwBufferSize(sizeof(DWORD));
  DWORD nResult(0);
  LONG nError = ::RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
  if (ERROR_SUCCESS == nError) {
    nValue = nResult;
  }

  return nError;
}

static LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue) {
  strValue = strDefaultValue;
  WCHAR szBuffer[512];
  DWORD dwBufferSize = sizeof(szBuffer);
  ULONG nError;
  nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
  if (ERROR_SUCCESS == nError) {
    strValue = szBuffer;
  }
  return nError;
}

static DWORD GetBrowserEmulationMode() {
  int browserVersion = 7;
  wstring sBrowserVersion;
  HKEY key;
  bool success = true;
  wstring path(L"SOFTWARE\\Microsoft\\Internet Explorer");
  LONG nError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_QUERY_VALUE, &key);
  DWORD mode = 11001; // IE11 edge mode

  if (nError != ERROR_SUCCESS) {
    success = false;
  } else {
    nError = GetStringRegKey(key, L"svcVersion", sBrowserVersion, L"7");
    if (nError != ERROR_SUCCESS) {
      nError = GetStringRegKey(key, L"version", sBrowserVersion, L"7");
      if (nError != ERROR_SUCCESS) {
        success = false;
      }
    }

   if (RegCloseKey(key) != ERROR_SUCCESS) {
      success = false;
    }
  }

  if (success) {
    browserVersion = QString::fromStdWString(sBrowserVersion).split('.', QString::SkipEmptyParts).at(0).toInt(); // convert base 16 number in s to int

    switch (browserVersion) {
    case 7:
      mode = 7000;
      break;
    case 8:
      mode = 8000;
      break;
    case 9:
      mode = 9000;
      break;
    case 10:
      mode = 10000;
      break;
    case 11:
      mode = 11000;
      break;
    default:
      // use IE11 edge mode
      break;
    }

  } else {
    mode = -1;
  }

  return mode;
}

static void SetBrowserFeatureControl() {
  // http://msdn.microsoft.com/en-us/library/ee330720(v=vs.85).aspx
  DWORD emulationMode = GetBrowserEmulationMode();

  if (emulationMode > 0) {
    // FeatureControl settings are per-process

    wchar_t fileName[MAX_PATH + 1];
    {   DWORD size;
        if ((size = GetModuleFileNameW(NULL, fileName, MAX_PATH)) == 0)
        {
            qCritical() << "SetBrowserFeatureControl: cannot get current module path.";
            return;
        }
        fileName[size] = L'\0';
        wstring filename(fileName);
        size_t sep_pos = filename.find_last_of('\\');
        if (sep_pos != wstring::npos)
        {
            memcpy(fileName, filename.c_str()+sep_pos+1, sizeof(filename[0]) * (filename.length() - sep_pos + 1));
        }
    }

    // Windows Internet Explorer 8 and later. The FEATURE_BROWSER_EMULATION feature defines the default emulation mode for Internet
    // Explorer and supports the following values.
    // Webpages containing standards-based !DOCTYPE directives are displayed in IE10 Standards mode.
    SetBrowserFeatureControlKey(L"FEATURE_BROWSER_EMULATION", fileName, emulationMode);

    // Internet Explorer 8 or later. The FEATURE_AJAX_CONNECTIONEVENTS feature enables events that occur when the value of the online
    // property of the navigator object changes, such as when the user chooses to work offline. For more information, see the ononline
    // and onoffline events.
    // Default: DISABLED
//    SetBrowserFeatureControlKey(L"FEATURE_AJAX_CONNECTIONEVENTS", fileName, 1);

    // Internet Explorer 9. Internet Explorer 9 optimized the performance of window-drawing routines that involve clipping regions associated
    // with child windows. This helped improve the performance of certain window drawing operations. However, certain applications hosting the
    // WebBrowser Control rely on the previous behavior and do not function correctly when these optimizations are enabled. The
    // FEATURE_ENABLE_CLIPCHILDREN_OPTIMIZATION feature can disable these optimizations.
    // Default: ENABLED
    // SetBrowserFeatureControlKey(L"FEATURE_ENABLE_CLIPCHILDREN_OPTIMIZATION", fileName, 1);

    // Internet Explorer 8 and later. By default, Internet Explorer reduces memory leaks caused by circular references between Internet Explorer
    // and the Microsoft JScript engine, especially in scenarios where a webpage defines an expando and the page is refreshed. If a legacy
    // application no longer functions with these changes, the FEATURE_MANAGE_SCRIPT_CIRCULAR_REFS feature can disable these improvements.
    // Default: ENABLED
    // SetBrowserFeatureControlKey(L"FEATURE_MANAGE_SCRIPT_CIRCULAR_REFS", fileName, 1);

    // Windows Internet Explorer 8. When enabled, the FEATURE_DOMSTORAGE feature allows Internet Explorer and applications hosting the WebBrowser
    // Control to use the Web Storage API. For more information, see Introduction to Web Storage.
    // Default: ENABLED
    // SetBrowserFeatureControlKey(L"FEATURE_DOMSTORAGE ", fileName, 1);

    // Internet Explorer 9. The FEATURE_GPU_RENDERING feature enables Internet Explorer to use a graphics processing unit (GPU) to render content.
    // This dramatically improves performance for webpages that are rich in graphics.
    // Default: DISABLED
//    SetBrowserFeatureControlKey(L"FEATURE_GPU_RENDERING ", fileName, 1);

    // Internet Explorer 9. By default, the WebBrowser Control uses Microsoft DirectX to render webpages, which might cause problems for
    // applications that use the Draw method to create bitmaps from certain webpages. In Internet Explorer 9, this method returns a bitmap
    // (in a Windows Graphics Device Interface (GDI) wrapper) instead of a GDI metafile representation of the webpage. When the
    // FEATURE_IVIEWOBJECTDRAW_DMLT9_WITH_GDI feature is enabled, the following conditions cause the Draw method to use GDI instead of DirectX
    // to create the resulting representation. The GDI representation will contain text records and vector data, but is not guaranteed to be
    // similar to the same represenation returned in earlier versions of the browser:
    //    The device context passed to the Draw method points to an enhanced metafile.
    //    The webpage is not displayed in IE9 Standards mode.
    // By default, this feature is ENABLED for applications hosting the WebBrowser Control. This feature is ignored by Internet Explorer and
    // Windows Explorer. To enable this feature by using the registry, add the name of your executable file to the following setting.
//    SetBrowserFeatureControlKey(L"FEATURE_IVIEWOBJECTDRAW_DMLT9_WITH_GDI  ", fileName, 0);

    // Windows 8 introduces a new input model that is different from the Windows 7 input model. In order to provide the broadest compatibility
    // for legacy applications, the WebBrowser Control for Windows 8 emulates the Windows 7 mouse, touch, and pen input model (also known as the
    // legacy input model). When the legacy input model is in effect, the following conditions are true:
    //    Windows pointer messages are not processed by the Trident rendering engine (mshtml.dll).
    //    Document Object Model (DOM) pointer and gesture events do not fire.
    //    Mouse and touch messages are dispatched according to the Windows 7 input model.
    //    Touch selection follows the Windows 7 model ("drag to select") instead of the Windows 8 model ("tap to select").
    //    Hardware accelerated panning and zooming is disabled.
    //    The Zoom and Pan Cascading Style Sheets (CSS) properties are ignored.
    // The FEATURE_NINPUT_LEGACYMODE feature control determines whether the legacy input model is enabled
    // Default: ENABLED
//    SetBrowserFeatureControlKey(L"FEATURE_NINPUT_LEGACYMODE", fileName, 0);

    // Internet Explorer 7 consolidated HTTP compression and data manipulation into a centralized component in order to improve performance and
    // to provide greater consistency between transfer encodings (such as HTTP no-cache headers). For compatibility reasons, the original
    // implementation was left in place. When the FEATURE_DISABLE_LEGACY_COMPRESSION feature is disabled, the original compression implementation
    // is used.
    // Default: ENABLED
    // SetBrowserFeatureControlKey(L"FEATURE_DISABLE_LEGACY_COMPRESSION", fileName, 1);

    // When the FEATURE_LOCALMACHINE_LOCKDOWN feature is enabled, Internet Explorer applies security restrictions on content loaded from the
    // user's local machine, which helps prevent malicious behavior involving local files:
    //    Scripts, Microsoft ActiveX controls, and binary behaviors are not allowed to run.
    //    Object safety settings cannot be overridden.
    //    Cross-domain data actions require confirmation from the user.
    // Default: DISABLED
    // SetBrowserFeatureControlKey(L"FEATURE_LOCALMACHINE_LOCKDOWN", fileName, 0);

    // Internet Explorer 7 and later. When enabled, the FEATURE_BLOCK_LMZ_??? feature allows ??? stored in the Local Machine zone to be
    // loaded only by webpages loaded from the Local Machine zone or by webpages hosted by sites in the Trusted Sites list. For more information,
    // see Security and Compatibility in Internet Explorer 7.
    // Default: DISABLED
    //    FEATURE_BLOCK_LMZ_IMG can block images that try to load from the user's local file system. To opt in, add your process name and set
    //                          the value to 0x00000001.
    //    FEATURE_BLOCK_LMZ_OBJECT can block objects that try to load from the user's local file system. To opt in, add your process name and
    //                          set the value to 0x00000001.
    //    FEATURE_BLOCK_LMZ_SCRIPT can block script access from the user's local file system. To opt in, add your process name and set the value
    //                          to 0x00000001.
    // SetBrowserFeatureControlKey(L"FEATURE_BLOCK_LMZ_OBJECT", fileName, 0);
    // SetBrowserFeatureControlKey(L"FEATURE_BLOCK_LMZ_OBJECT", fileName, 0);
    // SetBrowserFeatureControlKey(L"FEATURE_BLOCK_LMZ_SCRIPT", fileName, 0);

    // Internet Explorer 8 and later. When enabled, the FEATURE_DISABLE_NAVIGATION_SOUNDS feature disables the sounds played when you open a
    // link in a webpage.
    // Default: DISABLED
    SetBrowserFeatureControlKey(L"FEATURE_DISABLE_NAVIGATION_SOUNDS", fileName, 1);

    // Windows Internet Explorer 7 and later. Prior to Internet Explorer 7, href attributes of a objects supported the javascript prototcol;
    // this allowed webpages to execute script when the user clicked a link. For security reasons, this support was disabled in Internet
    // Explorer 7. For more information, see Event 1034 - Cross-Domain Barrier and Script URL Mitigation.
    // When enabled, the FEATURE_SCRIPTURL_MITIGATION feature allows the href attribute of a objects to support the javascript prototcol.
    // Default: DISABLED
//    SetBrowserFeatureControlKey(L"FEATURE_SCRIPTURL_MITIGATION", fileName, 1);

    // For Windows 8 and later, the FEATURE_SPELLCHECKING feature controls this behavior for Internet Explorer and for applications hosting
    // the web browser control (WebOC). When fully enabled, this feature automatically corrects grammar issues and identifies misspelled words
    // for the conditions described earlier.
    //    (DWORD) 00000000 - Features are disabled.
    //    (DWORD) 00000001 - Features are enabled for the conditions described earlier. (This is the default value.)
    //    (DWORD) 00000002 - Features are enabled, but only for elements that specifically set the spellcheck attribute to true.
//    SetBrowserFeatureControlKey(L"FEATURE_SPELLCHECKING", fileName, 0);

    // When enabled, the FEATURE_STATUS_BAR_THROTTLING feature limits the frequency of status bar updates to one update every 200 milliseconds.
    // Default: DISABLED
//    SetBrowserFeatureControlKey(L"FEATURE_STATUS_BAR_THROTTLING", fileName, 1);

    // Internet Explorer 7 or later. When enabled, the FEATURE_TABBED_BROWSING feature enables tabbed browsing navigation shortcuts and
    // notifications. For more information, see Tabbed Browsing for Developers.
    // Default: DISABLED
    // SetBrowserFeatureControlKey(L"FEATURE_TABBED_BROWSING", fileName, 1);

    // When enabled, the FEATURE_VALIDATE_NAVIGATE_URL feature control prevents Windows Internet Explorer from navigating to a badly formed URL.
    // Default: DISABLED
//    SetBrowserFeatureControlKey(L"FEATURE_VALIDATE_NAVIGATE_URL", fileName, 1);

    // When enabled,the FEATURE_WEBOC_DOCUMENT_ZOOM feature allows HTML dialog boxes to inherit the zoom state of the parent window.
    // Default: DISABLED
//    SetBrowserFeatureControlKey(L"FEATURE_WEBOC_DOCUMENT_ZOOM", fileName, 1);

    // The FEATURE_WEBOC_POPUPMANAGEMENT feature allows applications hosting the WebBrowser Control to receive the default Internet Explorer
    // pop-up window management behavior.
    // Default: ENABLED
//    SetBrowserFeatureControlKey(L"FEATURE_WEBOC_POPUPMANAGEMENT", fileName, 0);

    // Applications hosting the WebBrowser Control should ensure that window resizing and movement events are handled appropriately for the
    // needs of the application. By default, these events are ignored if the WebBrowser Control is not hosted in a proper container. When enabled,
    // the FEATURE_WEBOC_MOVESIZECHILD feature allows these events to affect the parent window of the application hosting the WebBrowser Control.
    // Because this can lead to unpredictable results, it is not considered desirable behavior.
    // Default: DISABLED
    // SetBrowserFeatureControlKey(L"FEATURE_WEBOC_MOVESIZECHILD", fileName, 0);

    // The FEATURE_ADDON_MANAGEMENT feature enables applications hosting the WebBrowser Control
    // to respect add-on management selections made using the Add-on Manager feature of Internet Explorer.
    // Add-ons disabled by the user or by administrative group policy will also be disabled in applications that enable this feature.
//    SetBrowserFeatureControlKey(L"FEATURE_ADDON_MANAGEMENT", fileName, 0);

    // Internet Explorer 10. When enabled, the FEATURE_WEBSOCKET feature allows script to create and use WebSocket objects.
    // The WebSocketobject allows websites to request data across domains from your browser by using the WebSocket protocol.
    // Default: ENABLED
//    SetBrowserFeatureControlKey(L"FEATURE_WEBSOCKET", fileName, 1);

    // When enabled, the FEATURE_WINDOW_RESTRICTIONS feature adds several restrictions to the size and behavior of popup windows:
    //    Popup windows must appear in the visible display area.
    //    Popup windows are forced to have status and address bars.
    //    Popup windows must have minimum sizes.
    //    Popup windows cannot cover important areas of the parent window.
    // When enabled, this feature can be configured differently for each security zone by using the URLACTION_FEATURE_WINDOW_RESTRICTIONS URL
    // action flag.
    // Default: ENABLED
//    SetBrowserFeatureControlKey(L"FEATURE_WINDOW_RESTRICTIONS", fileName, 0);

    // Internet Explorer 7 and later. The FEATURE_XMLHTTP feature enables or disables the native XMLHttpRequest object.
    // Default: ENABLED
    // SetBrowserFeatureControlKey(L"FEATURE_XMLHTTP", fileName, 1);
  }
}

} // anonymous

class WinNativeBrowserImpl
        : public NativeBrowserImpl
        , public IOleClientSite
        , public IOleInPlaceSite
        , public IStorage
        , public IDocHostUIHandler
        , public DWebBrowserEvents2
{
public:
    WinNativeBrowserImpl(HWND _mainWindow)
        : m_DWebBrowserEvents2_conn_id(0)
        , document_start_emited(false)
        , load_progress_crunch(new QTimer(this))
    {
        load_progress_crunch->setSingleShot(true);
        m_comRefCount = 0;
        m_mainWindow = _mainWindow;

        // enable current IE core use
        SetBrowserFeatureControl();

        CreateBrowserObject();

        // disable script error messages
        m_webBrowser->put_Silent(VARIANT_TRUE);
    }

    virtual ~WinNativeBrowserImpl()
    {
        CloseBrowserObject();
        delete load_progress_crunch;
    }

    virtual void navigate(const QString &_url) override
    {
        QString new_url(_url.isEmpty() ? "about:blank" : _url);
        current_url_host = QUrl::fromUserInput(new_url).host();
        bstr_t url(new_url.toStdWString().c_str());
        variant_t flags(navNoHistory);
        document_start_emited = false;
        m_webBrowser->Navigate(url, &flags, NULL, NULL, NULL);
    }

    QString location() const
    {
        bstr_t url;
        if (m_webBrowser->get_LocationURL(url.GetAddress()) == S_OK)
        {
            QString result = QString::fromWCharArray(url);
            return result;
        }
        else
        {
            return QString();
        }
    }

    virtual void stop() override
    {
        if (m_webBrowser != 0)
            m_webBrowser->Stop();
    }

    virtual QSize sizeHint() const override
    {
        QSize result;
        if (m_webBrowser == 0)
        {
            return result;
        }
        CComPtr<IDispatch> disp;
        m_webBrowser->get_Document(&disp);
        if (disp == 0)
        {
            return result;
        }
        CComPtr<IHTMLDocument2> html;
        disp.QueryInterface(&html);
        if (html == 0)
        {
            return result;
        }
        CComPtr<IHTMLElement> body;
        html->get_body(&body);
        if (body == 0)
        {
            return result;
        }
        long height, width;
        if (body->get_offsetWidth(&width) == S_OK)
            result.setWidth(width);
        if (body->get_offsetHeight(&height) == S_OK)
            result.setHeight(height);
        return result;
    }

    virtual void setSize(const QSize& size) override
    {
        ::SetRect(&m_objectRect, 0, 0, size.width(), size.height());

        {
            RECT hiMetricRect = PixelToHiMetric(m_objectRect);
            SIZEL sz;
            sz.cx = hiMetricRect.right - hiMetricRect.left;
            sz.cy = hiMetricRect.bottom - hiMetricRect.top;
            m_oleObject->SetExtent(DVASPECT_CONTENT, &sz);
        }

        if(m_oleInPlaceObject != NULL)
        {
            m_oleInPlaceObject->SetObjectRects(&m_objectRect, &m_objectRect);
        }
    }

private:


    void CreateBrowserObject()
    {
        HRESULT hr = ::OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW, 0, this, this, (void**)&m_oleObject);
        if(FAILED(hr))
            qCritical() << "WinNativeBrowserImpl: OleCreate() failed";

        hr = m_oleObject->SetClientSite(this);
        hr = OleSetContainedObject(m_oleObject, TRUE);

        RECT posRect;
        ::SetRect(&posRect, -300, -300, 300, 300);
        hr = m_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE,NULL, this, -1, m_mainWindow, &posRect);
        if(FAILED(hr))
            qCritical() << "WinNativeBrowserImpl: DoVerb(OLEIVERB_INPLACEACTIVATE) failed";

        hr = m_oleObject.QueryInterface(&m_webBrowser);
        if(FAILED(hr))
            qCritical() << "WinNativeBrowserImpl: QueryInterface(IWebBrowser) failed";

        AdviseWebBrowser(__uuidof(DWebBrowserEvents2), &m_DWebBrowserEvents2_conn_id);
    }

    void CloseBrowserObject()
    {
        UnadviseWebBrowser(__uuidof(DWebBrowserEvents2), m_DWebBrowserEvents2_conn_id);
        m_webBrowser->Stop();
        m_webBrowser->Quit();
        m_webBrowser->ExecWB(OLECMDID_CLOSE, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
        m_webBrowser->put_Visible(VARIANT_FALSE);
        m_oleObject->DoVerb(OLEIVERB_HIDE, NULL, this, 0, m_mainWindow, NULL);
        m_oleObject->Close(OLECLOSE_NOSAVE);
        OleSetContainedObject(m_oleObject, FALSE);
        m_oleObject->SetClientSite(NULL);
    }

    void AdviseWebBrowser(const IID& iid, DWORD *connection_id)
    {
        if(m_webBrowser == NULL) {
            return;
        }

        IConnectionPointContainer* pCPC =NULL;
        HRESULT hr = m_webBrowser->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC));
        if (SUCCEEDED(hr)) {
            IConnectionPoint* pCP = NULL;
            hr = pCPC->FindConnectionPoint(iid, &pCP);
            if (SUCCEEDED(hr)) {
                hr = pCP->Advise(static_cast<DWebBrowserEvents2*>(this), connection_id);
                pCP->Release();
            }
            else
            {
                qCritical() << "WinNativeBrowserImpl: Advise iWebBrowser2->FindConnectionPoint(IID, IConnectionPoint) failed";
            }
            pCPC->Release();
        }
        else
        {
            qCritical() << "WinNativeBrowserImpl: Advise iWebBrowser2->QueryInterface(IID_IConnectionPointContainer, IConnectionPointContainer) failed";
        }
    }

    void UnadviseWebBrowser(const IID& iid, DWORD connection_id)
    {
        if(m_webBrowser == NULL) {
            return;
        }

        IConnectionPointContainer* pCPC =NULL;
        HRESULT hr = m_webBrowser->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC));
        if (SUCCEEDED(hr)) {
            IConnectionPoint* pCP = NULL;
            hr = pCPC->FindConnectionPoint(iid, &pCP);
            if (SUCCEEDED(hr)) {
                hr = pCP->Unadvise(connection_id);
                pCP->Release();
            }
            else
            {
                qCritical() << "WinNativeBrowserImpl: Unadvise iWebBrowser2->FindConnectionPoint(IID, IConnectionPoint) failed";
            }
            pCPC->Release();
        }
        else
        {
            qCritical() << "WinNativeBrowserImpl: Unadvise iWebBrowser2->QueryInterface(IID_IConnectionPointContainer, IConnectionPointContainer) failed";
        }
    }

    RECT PixelToHiMetric(const RECT& _rc)
    {
        static bool s_initialized = false;
        static int s_pixelsPerInchX, s_pixelsPerInchY;
        if(!s_initialized)
        {
            HDC hdc = ::GetDC(NULL);
            s_pixelsPerInchX = ::GetDeviceCaps(hdc, LOGPIXELSX);
            s_pixelsPerInchY = ::GetDeviceCaps(hdc, LOGPIXELSY);
            ::ReleaseDC(NULL, hdc);
            s_initialized = true;
        }

        RECT rc;
        rc.left = MulDiv(2540, _rc.left, s_pixelsPerInchX);
        rc.top = MulDiv(2540, _rc.top, s_pixelsPerInchY);
        rc.right = MulDiv(2540, _rc.right, s_pixelsPerInchX);
        rc.bottom = MulDiv(2540, _rc.bottom, s_pixelsPerInchY);
        return rc;
    }

    // ----- IUnknown -----

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void**ppvObject) override
    {
        if(riid == __uuidof(IUnknown)) {
            (*ppvObject) = static_cast<IOleClientSite*>(this);
        } else if(riid == __uuidof(IOleInPlaceSite)) {
            (*ppvObject) = static_cast<IOleInPlaceSite*>(this);
        } else if(riid == __uuidof(IDocHostUIHandler)) {
            (*ppvObject) = static_cast<IDocHostUIHandler*>(this);
        } else if(riid == __uuidof(DWebBrowserEvents2)) {
            (*ppvObject) = static_cast<DWebBrowserEvents2*>(this);
        } else if(riid == __uuidof(IDispatch)) {
            (*ppvObject) = static_cast<IDispatch*>(this);
        } else {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void) override
    {
        m_comRefCount++;
        return m_comRefCount;
    }


    virtual ULONG STDMETHODCALLTYPE Release(void) override
    {
        m_comRefCount--;
        return m_comRefCount;
    }

    // ---------- IOleWindow ----------

    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd) override
    {
        (*phwnd) = m_mainWindow;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override
    {
        return E_NOTIMPL;
    }

    // ---------- IOleInPlaceSite ----------

    virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void) override
    {
        OleLockRunning(m_oleObject, TRUE, FALSE);
        m_oleObject.QueryInterface(&m_oleInPlaceObject);
        m_oleInPlaceObject->SetObjectRects(&m_objectRect, &m_objectRect);

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnUIActivate(void) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetWindowContext(
        IOleInPlaceFrame **ppFrame,
        IOleInPlaceUIWindow **ppDoc,
        LPRECT lprcPosRect,
        LPRECT lprcClipRect,
        LPOLEINPLACEFRAMEINFO lpFrameInfo) override
    {
        HWND hwnd = m_mainWindow;

        (*ppFrame) = NULL;
        (*ppDoc) = NULL;
        (*lprcPosRect).left = m_objectRect.left;
        (*lprcPosRect).top = m_objectRect.top;
        (*lprcPosRect).right = m_objectRect.right;
        (*lprcPosRect).bottom = m_objectRect.bottom;
        *lprcClipRect = *lprcPosRect;

        lpFrameInfo->fMDIApp = false;
        lpFrameInfo->hwndFrame = hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Scroll(SIZE) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL) override
    {
        return S_OK;
    }

    virtual HWND GetControlWindow()
    {
        if(m_controlWindow != NULL)
            return m_controlWindow;

        if(m_oleInPlaceObject == NULL)
            return NULL;

        m_oleInPlaceObject->GetWindow(&m_controlWindow);
        return m_controlWindow;
    }

    virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void) override
    {
        m_controlWindow = NULL;
        m_oleInPlaceObject = NULL;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE DiscardUndoState(void) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT) override
    {
        return E_NOTIMPL;
    }

    // ---------- IOleClientSite ----------

    virtual HRESULT STDMETHODCALLTYPE SaveObject(void) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetMoniker(
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        IMoniker **) override
    {
        if((dwAssign == OLEGETMONIKER_ONLYIFTHERE) && (dwWhichMoniker == OLEWHICHMK_CONTAINER))
            return E_FAIL;

        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer **) override
    {
        return E_NOINTERFACE;
    }

    virtual HRESULT STDMETHODCALLTYPE ShowObject(void) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout(void) override
    {
        return E_NOTIMPL;
    }

    // ----- IStorage -----

    virtual HRESULT STDMETHODCALLTYPE CreateStream(
        const OLECHAR *,
        DWORD ,
        DWORD ,
        DWORD ,
        IStream **) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OpenStream(
        const OLECHAR *,
        void *,
        DWORD ,
        DWORD ,
        IStream **) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CreateStorage(
        const OLECHAR *,
        DWORD ,
        DWORD ,
        DWORD ,
        IStorage **) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OpenStorage(
        const OLECHAR *,
        IStorage *,
        DWORD ,
        SNB ,
        DWORD ,
        IStorage **) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo(
        DWORD ,
        const IID *,
        SNB ,
        IStorage *) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE MoveElementTo(
        const OLECHAR *,
        IStorage *,
        const OLECHAR *,
        DWORD) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Revert(void) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE EnumElements(
        DWORD ,
        void *,
        DWORD ,
        IEnumSTATSTG **) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE DestroyElement(const OLECHAR *) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE RenameElement(const OLECHAR *, const OLECHAR *) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetElementTimes(
        const OLECHAR *,
        const FILETIME *,
        const FILETIME *,
        const FILETIME *) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetClass(REFCLSID) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetStateBits(DWORD, DWORD) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *, DWORD) override
    {
        return E_NOTIMPL;
    }

    // ---------- IDocHostUIHandler ----------

    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(
        DWORD /*dwID*/,
        POINT * /*ppt*/,
        IUnknown * /*pcmdtReserved*/,
        IDispatch * /*pdispReserved*/) override
    {
        // disable standard context menu
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo) override
    {
        const OLECHAR* szCSS = L"a:link{ color:blue; } a:visited{ color:blue; }";

        #define CCHMAX 256
        size_t cchLengthCSS;

        HRESULT hr = StringCchLengthW(szCSS, CCHMAX, &cchLengthCSS);
        OLECHAR* pCSSBuffer = (OLECHAR*)CoTaskMemAlloc((cchLengthCSS + 1) * sizeof(OLECHAR));
        hr = StringCchCopyW(pCSSBuffer, cchLengthCSS + 1, szCSS);

        pInfo->cbSize = sizeof(DOCHOSTUIINFO);
        pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_NO3DOUTERBORDER;
        pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
        pInfo->pchHostCss = pCSSBuffer;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ShowUI(
        DWORD /*dwID*/,
        IOleInPlaceActiveObject * /*pActiveObject*/,
        IOleCommandTarget * /*pCommandTarget*/,
        IOleInPlaceFrame * /*pFrame*/,
        IOleInPlaceUIWindow * /*pDoc*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE HideUI(void) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UpdateUI(void) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL /*fEnable*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL /*fActivate*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL /*fActivate*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE ResizeBorder(
        LPCRECT /*prcBorder*/,
        IOleInPlaceUIWindow * /*pUIWindow*/,
        BOOL /*fRameWindow*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(
        LPMSG /*lpMsg*/,
        const GUID * /*pguidCmdGroup*/,
        DWORD /*nCmdID*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR * /*pchKey*/, DWORD /*dw*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget * /*pDropTarget*/, IDropTarget ** /*ppDropTarget*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetExternal(IDispatch ** /*ppDispatch*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE TranslateUrl(
        DWORD /*dwTranslate*/,
        OLECHAR * /*pchURLIn*/,
        OLECHAR ** /*ppchURLOut*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject * /*pDO*/, IDataObject ** /*ppDORet*/) override
    {
        return E_NOTIMPL;
    }

    // ---------- IDispatch ----------

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT * /*pctinfo*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(
        UINT /*iTInfo*/,
        LCID /*lcid*/,
        ITypeInfo ** /*ppTInfo*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        REFIID /*riid*/,
        LPOLESTR * /*rgszNames*/,
        UINT /*cNames*/,
        LCID /*lcid*/,
        DISPID * /*rgDispId*/) override
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID /*lcid*/,
        WORD /*wFlags*/,
        DISPPARAMS * pDispParams,
        VARIANT * pVarResult,
        EXCEPINFO * /*pExcepInfo*/,
        UINT * /*puArgErr*/) override
    {
        if (pVarResult)
        {
            pVarResult->boolVal = VARIANT_TRUE;
        }
        if (riid != IID_NULL)
        {
            return DISP_E_UNKNOWNINTERFACE;
        }

        switch (dispIdMember)
        {
        case DISPID_BEFORENAVIGATE2:
        {
            QString navigate_url = QString::fromWCharArray(pDispParams->rgvarg[5].pvarVal->bstrVal);
            QString navigate_url_host = QUrl::fromUserInput(navigate_url).host();
            if (current_url_host != navigate_url_host)
            {
                if (!document_start_emited && navigate_url != "about:blank")
                {
                    // skip navigate
                    *pDispParams->rgvarg[0].pboolVal = VARIANT_TRUE;
                    if (navigate_url_host != "ieframe.dll")
                    {
                        onExternalNavigate(navigate_url);
                    }
                }
            }
            else
            {
                if (!document_start_emited)
                {
                    document_start_emited = true;
                    load_progress_crunch->disconnect(this);
                    connect(load_progress_crunch, SIGNAL(timeout()), this, SLOT(navigateNotStarted()));
                    load_progress_crunch->start(5000);
                    onLoadStart();
                }
            }
            break;
        }
        case DISPID_NEWWINDOW3:
            *pDispParams->rgvarg[3].pboolVal = VARIANT_TRUE;
            onExternalNavigate(QString::fromWCharArray(pDispParams->rgvarg[0].bstrVal));
            break;
        case DISPID_PROGRESSCHANGE:
        {
            LONG nProgressMax = pDispParams->rgvarg[0].lVal;
            LONG nProgress = pDispParams->rgvarg[1].lVal;
            if (load_progress_crunch->isActive())
            {
                load_progress_crunch->disconnect(this);
                connect(load_progress_crunch, SIGNAL(timeout()), this, SLOT(navigateFinished()));
                load_progress_crunch->start(600);
            }
            onProgress(nProgress, nProgressMax);
            break;
        }
        }

        return S_OK;
    }

    void navigateNotStarted() override
    {
        stop();
        current_url_host = QUrl::fromUserInput(location()).host();
        document_start_emited = false;
        onLoadFinish(false);
    }

    void navigateFinished() override
    {
        current_url_host = QUrl::fromUserInput(location()).host();
        document_start_emited = false;
        onLoadFinish(true);
    }

private:
    CComPtr<IOleObject> m_oleObject;
    LONG m_comRefCount;
    HWND m_mainWindow;
    RECT m_objectRect;
    CComPtr<IWebBrowser2> m_webBrowser;
    CComPtr<IOleInPlaceObject> m_oleInPlaceObject;
    HWND m_controlWindow;
    DWORD m_DWebBrowserEvents2_conn_id;
    QString current_url_host;
    bool document_start_emited;
    QTimer *load_progress_crunch;
};

NativeBrowserImpl* NativeBrowserImpl::createNewInstance(WId browserwindow)
{
    return new WinNativeBrowserImpl(reinterpret_cast<HWND>(browserwindow));
}
