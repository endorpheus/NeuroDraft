/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "PaneManager.h"
#include <QApplication>
#include <QMainWindow>
#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

PaneManager::PaneManager(QObject *parent)
    : QObject(parent)
    , m_mainParent(qobject_cast<QWidget*>(parent))
{
}

PaneManager::~PaneManager()
{
    // Clean up all panes
    for (auto it = m_panes.constBegin(); it != m_panes.constEnd(); ++it) {
        delete it.value();
    }
    m_panes.clear();
}

QUuid PaneManager::convertTabToPane(QTabWidget* sourceTabWidget, int tabIndex, PaneType splitType)
{
    if (!sourceTabWidget || tabIndex < 0 || tabIndex >= sourceTabWidget->count()) {
        return QUuid();
    }
    
    // Get the tab content
    QWidget* tabContent = sourceTabWidget->widget(tabIndex);
    QString tabText = sourceTabWidget->tabText(tabIndex);
    
    if (!tabContent) {
        return QUuid();
    }
    
    // Remove tab from source
    sourceTabWidget->removeTab(tabIndex);
    
    // Create new pane
    QUuid paneId = createPane(splitType, m_mainParent);
    PaneInfo* pane = getPaneInfo(paneId);
    
    if (!pane) {
        // If pane creation failed, add tab back
        sourceTabWidget->insertTab(tabIndex, tabContent, tabText);
        return QUuid();
    }
    
    // Add content to new pane
    if (pane->tabWidget) {
        pane->tabWidget->addTab(tabContent, tabText);
        pane->title = tabText;
    }
    
    emit paneCreated(paneId);
    return paneId;
}

bool PaneManager::convertPaneToTab(const QUuid& paneId, QTabWidget* targetTabWidget)
{
    if (!targetTabWidget) {
        return false;
    }
    
    PaneInfo* pane = getPaneInfo(paneId);
    if (!pane || !pane->tabWidget) {
        return false;
    }
    
    // Move all tabs from pane to target
    while (pane->tabWidget->count() > 0) {
        QWidget* tabContent = pane->tabWidget->widget(0);
        QString tabText = pane->tabWidget->tabText(0);
        pane->tabWidget->removeTab(0);
        targetTabWidget->addTab(tabContent, tabText);
    }
    
    // Clean up the pane
    closePane(paneId);
    return true;
}

QUuid PaneManager::createPane(PaneType type, QWidget* parent)
{
    QUuid paneId = QUuid::createUuid();
    PaneInfo* paneInfo = new PaneInfo();
    
    paneInfo->id = paneId;
    paneInfo->type = type;
    paneInfo->parentPane = parent;
    paneInfo->isDetached = false;
    
    switch (type) {
        case PaneType::TabWidget:
            paneInfo->tabWidget = createTabWidget(parent);
            paneInfo->widget = paneInfo->tabWidget;
            paneInfo->splitter = nullptr;
            break;
            
        case PaneType::HorizontalSplit:
            paneInfo->splitter = createSplitter(Qt::Horizontal, parent);
            paneInfo->widget = paneInfo->splitter;
            paneInfo->tabWidget = nullptr;
            break;
            
        case PaneType::VerticalSplit:
            paneInfo->splitter = createSplitter(Qt::Vertical, parent);
            paneInfo->widget = paneInfo->splitter;
            paneInfo->tabWidget = nullptr;
            break;
    }
    
    if (paneInfo->widget) {
        m_panes[paneId] = paneInfo;
        return paneId;
    }
    
    delete paneInfo;
    return QUuid();
}

bool PaneManager::closePane(const QUuid& paneId)
{
    auto it = m_panes.find(paneId);
    if (it == m_panes.end()) {
        return false;
    }
    
    PaneInfo* pane = it.value();
    
    // Close all child tabs if it's a tab widget
    if (pane->tabWidget) {
        while (pane->tabWidget->count() > 0) {
            QWidget* tab = pane->tabWidget->widget(0);
            pane->tabWidget->removeTab(0);
            if (tab) {
                tab->deleteLater();
            }
        }
    }
    
    // Delete the widget
    if (pane->widget) {
        pane->widget->deleteLater();
    }
    
    // Remove from map and delete
    m_panes.erase(it);
    delete pane;
    
    emit paneDestroyed(paneId);
    return true;
}

bool PaneManager::splitPane(const QUuid& paneId, PaneType splitType)
{
    PaneInfo* pane = getPaneInfo(paneId);
    if (!pane) {
        return false;
    }
    
    // Create new splitter
    Qt::Orientation orientation = (splitType == PaneType::HorizontalSplit) ? 
        Qt::Horizontal : Qt::Vertical;
    
    QSplitter* newSplitter = createSplitter(orientation, pane->parentPane);
    
    // Move existing widget to splitter
    QWidget* oldWidget = pane->widget;
    newSplitter->addWidget(oldWidget);
    
    // Create new tab widget for the split
    QTabWidget* newTabWidget = createTabWidget();
    newSplitter->addWidget(newTabWidget);
    
    // Update pane info
    pane->widget = newSplitter;
    pane->splitter = newSplitter;
    pane->type = splitType;
    
    return true;
}

PaneManager::PaneInfo* PaneManager::getPaneInfo(const QUuid& paneId)
{
    auto it = m_panes.find(paneId);
    return (it != m_panes.end()) ? it.value() : nullptr;
}

QList<QUuid> PaneManager::getAllPanes() const
{
    QList<QUuid> paneIds;
    for (auto it = m_panes.constBegin(); it != m_panes.constEnd(); ++it) {
        paneIds.append(it.key());
    }
    return paneIds;
}

QTabWidget* PaneManager::createTabWidget(QWidget* parent)
{
    QTabWidget* tabWidget = new QTabWidget(parent);
    setupTabWidget(tabWidget);
    return tabWidget;
}

bool PaneManager::detachPane(const QUuid& paneId)
{
    PaneInfo* pane = getPaneInfo(paneId);
    if (!pane || pane->isDetached) {
        return false;
    }
    
    // Create new window for detached pane
    QMainWindow* detachedWindow = new QMainWindow();
    detachedWindow->setWindowTitle(pane->title.isEmpty() ? "Detached Pane" : pane->title);
    detachedWindow->setCentralWidget(pane->widget);
    detachedWindow->show();
    
    pane->isDetached = true;
    pane->parentPane = detachedWindow;
    
    emit paneDetached(paneId);
    return true;
}

bool PaneManager::attachPane(const QUuid& paneId, QWidget* parent)
{
    PaneInfo* pane = getPaneInfo(paneId);
    if (!pane || !pane->isDetached || !parent) {
        return false;
    }
    
    // Remove from detached window
    if (pane->parentPane) {
        QMainWindow* window = qobject_cast<QMainWindow*>(pane->parentPane);
        if (window) {
            window->takeCentralWidget();
            window->deleteLater();
        }
    }
    
    // Attach to new parent
    pane->widget->setParent(parent);
    pane->parentPane = parent;
    pane->isDetached = false;
    
    emit paneAttached(paneId);
    return true;
}

void PaneManager::savePaneLayout()
{
    // TODO: Implement layout serialization
    qDebug() << "Save pane layout - TODO";
}

void PaneManager::restorePaneLayout()
{
    // TODO: Implement layout deserialization
    qDebug() << "Restore pane layout - TODO";
}

void PaneManager::onTabCloseRequested(int index)
{
    QTabWidget* tabWidget = qobject_cast<QTabWidget*>(sender());
    if (!tabWidget || index < 0 || index >= tabWidget->count()) {
        return;
    }
    
    QWidget* tab = tabWidget->widget(index);
    tabWidget->removeTab(index);
    
    if (tab) {
        tab->deleteLater();
    }
}

void PaneManager::onTabDetachRequested(int index)
{
    QTabWidget* tabWidget = qobject_cast<QTabWidget*>(sender());
    if (!tabWidget) {
        return;
    }
    
    // Convert tab to detached pane
    QUuid paneId = convertTabToPane(tabWidget, index, PaneType::TabWidget);
    if (!paneId.isNull()) {
        detachPane(paneId);
    }
}

void PaneManager::setupTabWidget(QTabWidget* tabWidget)
{
    if (!tabWidget) {
        return;
    }
    
    // Enable tab closing
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    
    // Connect signals
    connect(tabWidget, &QTabWidget::tabCloseRequested, 
            this, &PaneManager::onTabCloseRequested);
    
    // Add context menu for tab operations
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void PaneManager::cleanupPane(const QUuid& paneId)
{
    // Helper method for cleanup operations
    PaneInfo* pane = getPaneInfo(paneId);
    if (!pane) {
        return;
    }
    
    // Additional cleanup logic can be added here
}

QSplitter* PaneManager::createSplitter(Qt::Orientation orientation, QWidget* parent)
{
    QSplitter* splitter = new QSplitter(orientation, parent);
    splitter->setHandleWidth(4);
    splitter->setChildrenCollapsible(false);
    return splitter;
}