#ifndef NATIVEBROWSER_H
#define NATIVEBROWSER_H

#include <QWidget>

class NativeBrowserImpl;

class NativeBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit NativeBrowser(QWidget *parent = 0);
    virtual ~NativeBrowser();

    Q_PROPERTY(QString url READ url WRITE load RESET loadBlank)

    QString url() const;

    QSize sizeHint() const override;
    void updateGeometry();

signals:
    void loadStarted();
    void loadProgress(int progress);
    void loadFinished(bool ok);

    void externalNavigate(const QString &url);

public slots:
    void load(const QString &url);

protected slots:
    void loadBlank();

protected:
    virtual void resizeEvent(QResizeEvent *);

private:
    friend class NativeBrowserImpl;
    NativeBrowserImpl *browser;
};

#endif // NATIVEBROWSER_H
