/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tabwidget.h"
#include "webpage.h"
#include "webview.h"
#include <QMenu>
#include <QTabBar>
#include <QWebEngineProfile>
#include <QCoreApplication>
#include <QDir>

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    QTabBar *tabBar = this->tabBar();
    tabBar->setTabsClosable(true);
    tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    tabBar->setMovable(true);
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabBar, &QTabBar::customContextMenuRequested, this, &TabWidget::handleContextMenuRequested);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &TabWidget::closeTab);
    connect(tabBar, &QTabBar::tabBarDoubleClicked, [this](int index) {
        if (index != -1)
            return;
        createTab();
    });

    setDocumentMode(true);
    setElideMode(Qt::ElideRight);

    connect(this, &QTabWidget::currentChanged, this, &TabWidget::handleCurrentChanged);

    //
    currentProfile = QWebEngineProfile::defaultProfile();
    profileName_map_profilePoint["default"] = currentProfile;
    connect(currentProfile, SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
            this, SLOT(downloadRequested(QWebEngineDownloadItem*)));
    //验证码OCR
    QFile file;
    file.setFileName(QCoreApplication::applicationDirPath()+"/oneChar.dat");
    file.open(QIODevice::ReadOnly);
    if(file.isOpen()){
        QDataStream in(&file);
        in>>oneCharDataMap;
        file.close();
    }
}

TabWidget::~TabWidget()
{
}

void TabWidget::handleCurrentChanged(int index)
{
    if (index != -1) {
        WebView *view = webView(index);
        if (!view->url().isEmpty())
            view->setFocus();
        emit titleChanged(view->title());
        emit loadProgress(view->loadProgress());
        emit urlChanged(view->url());
        QIcon pageIcon = view->page()->icon();
        if (!pageIcon.isNull())
            emit iconChanged(pageIcon);
        else
            emit iconChanged(QIcon(QStringLiteral(":defaulticon.png")));
        emit webActionEnabledChanged(QWebEnginePage::Back, view->isWebActionEnabled(QWebEnginePage::Back));
        emit webActionEnabledChanged(QWebEnginePage::Forward, view->isWebActionEnabled(QWebEnginePage::Forward));
        emit webActionEnabledChanged(QWebEnginePage::Stop, view->isWebActionEnabled(QWebEnginePage::Stop));
        emit webActionEnabledChanged(QWebEnginePage::Reload,view->isWebActionEnabled(QWebEnginePage::Reload));
    } else {
        emit titleChanged(QString());
        emit loadProgress(0);
        emit urlChanged(QUrl());
        emit iconChanged(QIcon(QStringLiteral(":defaulticon.png")));
        emit webActionEnabledChanged(QWebEnginePage::Back, false);
        emit webActionEnabledChanged(QWebEnginePage::Forward, false);
        emit webActionEnabledChanged(QWebEnginePage::Stop, false);
        emit webActionEnabledChanged(QWebEnginePage::Reload, true);
    }
}

void TabWidget::handleContextMenuRequested(const QPoint &pos)
{
    QMenu menu;
    menu.addAction(tr("New &Tab"), this, &TabWidget::createTab, QKeySequence::AddTab);
    int index = tabBar()->tabAt(pos);
    if (index != -1) {
        QAction *action = menu.addAction(tr("Clone Tab"));
        connect(action, &QAction::triggered, this, [this,index]() {
            cloneTab(index);
        });
        menu.addSeparator();
        action = menu.addAction(tr("&Close Tab"));
        action->setShortcut(QKeySequence::Close);
        connect(action, &QAction::triggered, this, [this,index]() {
            closeTab(index);
        });
        action = menu.addAction(tr("Close &Other Tabs"));
        connect(action, &QAction::triggered, this, [this,index]() {
            closeOtherTabs(index);
        });
        menu.addSeparator();
        action = menu.addAction(tr("Reload Tab"));
        action->setShortcut(QKeySequence::Refresh);
        connect(action, &QAction::triggered, this, [this,index]() {
            reloadTab(index);
        });
    } else {
        menu.addSeparator();
    }
    menu.addAction(tr("Reload All Tabs"), this, &TabWidget::reloadAllTabs);
    menu.exec(QCursor::pos());
}

WebView *TabWidget::currentWebView() const
{
    return webView(currentIndex());
}

WebView *TabWidget::webView(int index) const
{
    return qobject_cast<WebView*>(widget(index));
}

void TabWidget::setupView(WebView *webView)
{
    QWebEnginePage *webPage = webView->page();

    connect(webView, &QWebEngineView::titleChanged, [this, webView](const QString &title) {
        int index = indexOf(webView);
        if (index != -1)
            setTabText(index, webView->page()->profile()->storageName()+" "+title);//让tab标题显示当前配置文件名字
        if (currentIndex() == index)
            emit titleChanged(title);
    });
    connect(webView, &QWebEngineView::urlChanged, [this, webView](const QUrl &url) {
        int index = indexOf(webView);
        if (index != -1)
            tabBar()->setTabData(index, url);
        if (currentIndex() == index)
            emit urlChanged(url);
    });
    connect(webView, &QWebEngineView::loadProgress, [this, webView](int progress) {
        if (currentIndex() == indexOf(webView))
            emit loadProgress(progress);
    });
    connect(webView,&WebView::loadFinished,[this,webView](bool b) {
        if(b){
            qDebug()<<"webView loadFinished";
            webView->page()->action(QWebEnginePage::SavePage)->activate(QAction::Trigger);
        }else
            qDebug()<<"loadFinished fail";

    });
    connect(webPage, &QWebEnginePage::linkHovered, [this, webView](const QString &url) {
        if (currentIndex() == indexOf(webView))
            emit linkHovered(url);
    });
    connect(webPage, &WebPage::iconChanged, [this, webView](const QIcon &icon) {
        int index = indexOf(webView);
        QIcon ico = icon.isNull() ? QIcon(QStringLiteral(":defaulticon.png")) : icon;

        if (index != -1)
            setTabIcon(index, ico);
        if (currentIndex() == index)
            emit iconChanged(ico);
    });
    connect(webView, &WebView::webActionEnabledChanged, [this, webView](QWebEnginePage::WebAction action, bool enabled) {
        if (currentIndex() ==  indexOf(webView))
            emit webActionEnabledChanged(action,enabled);
    });
    connect(webView, &QWebEngineView::loadStarted, [this, webView]() {
        int index = indexOf(webView);
        if (index != -1) {
            QIcon icon(QLatin1String(":view-refresh.png"));
            setTabIcon(index, icon);
        }
    });
    connect(webPage, &QWebEnginePage::windowCloseRequested, [this, webView]() {
        int index = indexOf(webView);
        if (index >= 0)
            closeTab(index);
    });
}

void TabWidget::downloadRequested(QWebEngineDownloadItem *dItem)
{
    QString profileName = qobject_cast<QWebEngineProfile*> (QObject::sender())->storageName();
    QString savePath = QCoreApplication::applicationDirPath()+"/cache/"+profileName;
    QDir(savePath+"/mail_files/").removeRecursively();

    qDebug()<<"downloadRequested mimeType:"<<dItem->mimeType()<<" profile:"<<profileName<<" id:"<<dItem->id();

    dItem->setSavePageFormat(QWebEngineDownloadItem::CompleteHtmlSaveFormat);//single HTML page and the resources
    dItem->setPath(savePath+"/main.html");
    connect(dItem,SIGNAL(finished()),this,SLOT(htmlDownloadFinished()));
    dItem->accept();
}
QStringList getStrLFromStrL(QStringList list, QRect rect)
{
    int startW = rect.x();
    int endW = startW+rect.width();
    int startH = rect.y();

    QStringList rList;
    for(int i=startW;i<endW;i++){
        QString lineH = list.at(i);
        rList.append(lineH.mid(startH,rect.height()));
    }
    return rList;
}

QRect PixelList2(QStringList list)
{
    int max = list.size()-1;
    int widStart(0),widEnd(0),heightStart(max),heightEnd(0);
    bool findS(false);
    for(int i=0;i<list.size();i++){
        QString hLine = list.at(i);
        if(!findS){
            findS = hLine.contains('1');
            widStart = i;
        }
        if(findS){
            if(!hLine.contains('1')){
                widEnd = i-1;
                break;
            }else
                widEnd = i;
            heightStart = qMin(heightStart,hLine.indexOf('1'));
            heightEnd = qMax(heightEnd,hLine.lastIndexOf('1'));
        }
    }

    return QRect(widStart,heightStart,widEnd-widStart+1,heightEnd-heightStart+1);
}
QString TabWidget::autoOCR(const QList<QStringList> &ll)
{
    QString rt;
    foreach (QStringList lr, ll) {
        QMapIterator<QString,QStringList> i(oneCharDataMap);
        QString key;
        while(i.hasNext()){
            i.next();
            if(lr==i.value()){
                key = i.key();
                break;
            }
        }
        if(!key.isEmpty()){
            rt.append(key.mid(0,key.indexOf('-')));
        }else
            rt.append('*');
    }
    return rt;
}
void TabWidget::htmlDownloadFinished()
{
    qDebug()<<"htmlDownloadFinished";

    QString downPath = qobject_cast<QWebEngineDownloadItem *> (QObject::sender())->path();
    downPath.resize(downPath.lastIndexOf("/"));

    QImage img;
    bool b = img.load(downPath+"/main_files/index!imgcode.action");
    if(!b){
        //htmlDownloadFinished load fail  "D:/+program/build-autoSppet-Desktop_Qt_5_9_1_MSVC2017_64bit-Release/release/cache/Default/main.html"
        qDebug()<<"htmlDownloadFinished load fail "<<downPath;
        return;
    }

    int RGB(380),R(123),G(128),B(123);
    int imgH = img.height();
    int imgW = img.width();

    QImage img_t(imgW,imgH,QImage::Format_RGB888);

    for(int w=0;w<imgW;w++){
        for(int h=0;h<imgH;h++){
            QColor c = img.pixelColor(w,h);
            if((c.red()+c.green()+c.blue())<RGB && (c.red()<R || c.green()<G || c.blue()<B) ){
                img_t.setPixel(w,h,QColor(0,0,0).rgb());
            }else
                img_t.setPixel(w,h,QColor(255,255,255).rgb());
        }
    }

    QStringList t_list;//将打开图片转换成字符串列表
    QImage t_img = img_t.convertToFormat(QImage::Format_Mono);
    for(int w=0;w<t_img.width();w++){
        QString s;
        for(int h=0;h<t_img.height();h++){
            s.append(t_img.pixelColor(w,h).red()==255?"0":"1");//1为有，0为无
        }
        t_list.append(s);
    }

    QList<QStringList> fourCharListList;
    while(1){
        QRect rect = PixelList2(t_list);//一个个的获取t_list中单独字符区域
        //qDebug()<<rect<<" "<<rect.topLeft()<<" "<<rect.topRight()<<" "<<rect.bottomLeft()<<" "<<rect.bottomRight();
        if(rect.isValid()){
            QStringList oneCharList = getStrLFromStrL(t_list,rect);//将指定区域转换成字符串列表，即一个字母的图形字符串列表
            fourCharListList.append(oneCharList);
            for(int i=rect.x();i<rect.x()+rect.width();i++){//将已获取区域清零，以显示下一个
                if(i<t_list.size())
                    t_list[i].fill('0');
            }
        }else
            break;
    }
    qDebug()<<"fourCharListList"<<fourCharListList.size();
    qDebug()<<"OCR:"<<autoOCR(fourCharListList);
}

void TabWidget::setCurrentProfile(QWebEngineProfile *value)
{
    currentProfile = value;
}

QWebEngineProfile *TabWidget::getCurrentProfile() const
{
    return currentIndex()==-1 ? QWebEngineProfile::defaultProfile() : currentWebView()->page()->profile();
}

WebView *TabWidget::createTab(bool makeCurrent)
{
    WebView *webView = new WebView;
    WebPage *webPage = new WebPage(getCurrentProfile(), webView);
    webView->setPage(webPage);
    setupView(webView);
    //addTab(webView, tr("(Untitled)"));
    insertTab(currentIndex()+1,webView, tr("(Untitled)"));//在当前窗口后面增加tab
    if (makeCurrent)
        setCurrentWidget(webView);
    return webView;
}

WebView *TabWidget::createTabById(const QString& index, bool makeCurrent)
{
    WebView *webView = new WebView;

    if(!profileName_map_profilePoint.contains(index))
        profileName_map_profilePoint[index] = new QWebEngineProfile(index);
    profileName_map_WebViewMain[index] = webView;

    WebPage *webPage = new WebPage(profileName_map_profilePoint[index], webView);
    webView->setPage(webPage);
    setupView(webView);
    addTab(webView, tr("(Untitled)"));//在最后面增加tab
    if (makeCurrent){
        setCurrentWidget(webView);
        currentProfile = webPage->profile();
    }
    return webView;
}

void TabWidget::reloadAllTabs()
{
    for (int i = 0; i < count(); ++i)
        webView(i)->reload();
}

void TabWidget::closeOtherTabs(int index)
{
    for (int i = count() - 1; i > index; --i)
        closeTab(i);
    for (int i = index - 1; i >= 0; --i)
        closeTab(i);
}

void TabWidget::closeTab(int index)
{
    if (WebView *view = webView(index)) {
        bool hasFocus = view->hasFocus();
        removeTab(index);
        if (hasFocus && count() > 0)
            currentWebView()->setFocus();
        if (count() == 0)
            createTab();
        view->deleteLater();
    }
}

void TabWidget::cloneTab(int index)
{
    if (WebView *view = webView(index)) {
        WebView *tab = createTab(false);
        tab->setUrl(view->url());
    }
}

void TabWidget::setUrl(const QUrl &url)
{
    if (WebView *view = currentWebView()) {
        view->setUrl(url);
        view->setFocus();
    }
}

void TabWidget::triggerWebPageAction(QWebEnginePage::WebAction action)
{
    if (WebView *webView = currentWebView()) {
        webView->triggerPageAction(action);
        webView->setFocus();
    }
}

void TabWidget::nextTab()
{
    int next = currentIndex() + 1;
    if (next == count())
        next = 0;
    setCurrentIndex(next);
}

void TabWidget::previousTab()
{
    int next = currentIndex() - 1;
    if (next < 0)
        next = count() - 1;
    setCurrentIndex(next);
}

void TabWidget::reloadTab(int index)
{
    if (WebView *view = webView(index))
        view->reload();
}
