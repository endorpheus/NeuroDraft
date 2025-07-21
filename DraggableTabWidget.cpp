/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "DraggableTabWidget.h"
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QMoveEvent>
#include <QDebug>
#include <QDataStream>
#include <QPixmap>

// Initialize static counter
int DraggableTabWidget::s_tabCounter = 0;

// DraggableTabBar Implementation
DraggableTabBar::DraggableTabBar(QWidget* parent)
    : QTabBar(parent)
    , m_dragStarted(false)
    , m_dragIndex(-1)
{
    setAcceptDrops(true);
    setElideMode(Qt::ElideRight);
    setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    setMovable(false); // We handle movement ourselves
}

void DraggableTabBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_dragIndex = tabAt(event->pos());
        m_dragStarted = false;
    }
    
    QTabBar::mousePressEvent(event);
}

void DraggableTabBar::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    
    if (!m_dragStarted && (event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    
    if (m_dragIndex >= 0 && m_dragIndex < count()) {
        m_dragStarted = true;
        
        qDebug() << "Starting drag for tab" << m_dragIndex << ":" << tabText(m_dragIndex);
        
        // Get the parent DraggableTabWidget
        DraggableTabWidget* parentTabWidget = qobject_cast<DraggableTabWidget*>(parent());
        if (!parentTabWidget) {
            m_dragStarted = false;
            m_dragIndex = -1;
            return;
        }
        
        // Store reference to source widget and tab data
        QWidget* sourceWidget = parentTabWidget->widget(m_dragIndex);
        if (!sourceWidget) {
            m_dragStarted = false;
            m_dragIndex = -1;
            return;
        }
        
        // Create drag operation
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData();
        
        // Store tab information with widget pointer for reliable identification
        QByteArray tabData;
        QDataStream stream(&tabData, QIODevice::WriteOnly);
        stream << reinterpret_cast<quintptr>(parentTabWidget) << m_dragIndex << tabText(m_dragIndex);
        
        mimeData->setData("application/x-neurodraft-tab", tabData);
        mimeData->setText(tabText(m_dragIndex));
        
        drag->setMimeData(mimeData);
        
        // Create a simple pixmap for visual feedback
        QPixmap pixmap(120, 25);
        pixmap.fill(QColor(200, 200, 255, 180));
        drag->setPixmap(pixmap);
        
        qDebug() << "Executing drag operation for widget pointer:" << reinterpret_cast<void*>(parentTabWidget);
        
        // Start drag operation
        Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
        
        qDebug() << "Drag completed with action:" << dropAction;
        
        if (dropAction == Qt::IgnoreAction) {
            // Tab was dragged outside - emit detach signal
            qDebug() << "Tab dragged outside, emitting detach signal";
            emit tabDetachRequested(m_dragIndex, event->globalPosition().toPoint());
        }
        
        m_dragStarted = false;
        m_dragIndex = -1;
    }
    
    // Don't call base implementation during drag
    if (!m_dragStarted) {
        QTabBar::mouseMoveEvent(event);
    }
}

void DraggableTabBar::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-neurodraft-tab")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DraggableTabBar::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-neurodraft-tab")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DraggableTabBar::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-neurodraft-tab")) {
        int fromIndex = event->mimeData()->data("application/x-neurodraft-tab").toInt();
        int toIndex = tabAt(event->position().toPoint());
        
        if (toIndex == -1) {
            toIndex = count();
        }
        
        if (fromIndex != toIndex) {
            emit tabMoveRequested(fromIndex, toIndex);
        }
        
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

// DraggableTabWidget Implementation
DraggableTabWidget::DraggableTabWidget(QWidget* parent)
    : QTabWidget(parent)
    , m_dragEnabled(true)
{
    // Create custom tab bar
    m_tabBar = new DraggableTabBar(this);
    setTabBar(m_tabBar);
    
    setupDragAndDrop();
    
    // Connect signals
    connect(m_tabBar, &DraggableTabBar::tabDetachRequested, 
            this, &DraggableTabWidget::onTabDetachRequested);
    connect(m_tabBar, &DraggableTabBar::tabMoveRequested,
            this, &DraggableTabWidget::onTabMoveRequested);
}

DraggableTabWidget::~DraggableTabWidget() = default;

void DraggableTabWidget::setTabDragEnabled(bool enabled)
{
    m_dragEnabled = enabled;
    setAcceptDrops(enabled);
}

bool DraggableTabWidget::isTabDragEnabled() const
{
    return m_dragEnabled;
}

void DraggableTabWidget::attachTab(QWidget* widget, const QString& label, int index)
{
    if (index == -1) {
        index = count();
    }
    
    insertTab(index, widget, label);
    setCurrentIndex(index);
    
    emit tabAttachRequested(widget, label);
}

QWidget* DraggableTabWidget::detachTab(int index)
{
    if (index < 0 || index >= count()) {
        return nullptr;
    }
    
    QWidget* widget = this->widget(index);
    QString label = tabText(index);
    
    removeTab(index);
    
    return widget;
}

void DraggableTabWidget::moveTab(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex || fromIndex < 0 || toIndex < 0 || 
        fromIndex >= count() || toIndex > count()) {
        return;
    }
    
    // Get tab information
    QWidget* widget = this->widget(fromIndex);
    QString label = tabText(fromIndex);
    QIcon icon = tabIcon(fromIndex);
    QString tooltip = tabToolTip(fromIndex);
    
    // Remove tab
    removeTab(fromIndex);
    
    // Adjust toIndex if necessary
    if (toIndex > fromIndex) {
        toIndex--;
    }
    
    // Insert at new position
    insertTab(toIndex, widget, icon, label);
    setTabToolTip(toIndex, tooltip);
    setCurrentIndex(toIndex);
    
    emit tabReordered(fromIndex, toIndex);
}

void DraggableTabWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (m_dragEnabled && event->mimeData()->hasFormat("application/x-neurodraft-tab")) {
        qDebug() << "DraggableTabWidget accepting drag enter";
        event->acceptProposedAction();
    } else {
        QTabWidget::dragEnterEvent(event);
    }
}

void DraggableTabWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (m_dragEnabled && event->mimeData()->hasFormat("application/x-neurodraft-tab")) {
        qDebug() << "DraggableTabWidget accepting drag move";
        event->acceptProposedAction();
    } else {
        QTabWidget::dragMoveEvent(event);
    }
}

void DraggableTabWidget::dropEvent(QDropEvent* event)
{
    if (m_dragEnabled && event->mimeData()->hasFormat("application/x-neurodraft-tab")) {
        qDebug() << "DraggableTabWidget" << this << "received drop event";
        
        // Parse the tab data
        QByteArray tabData = event->mimeData()->data("application/x-neurodraft-tab");
        QDataStream stream(&tabData, QIODevice::ReadOnly);
        quintptr sourceWidgetPtr;
        int sourceIndex;
        QString tabText;
        stream >> sourceWidgetPtr >> sourceIndex >> tabText;
        
        // Convert back to widget pointer
        DraggableTabWidget* sourceTabWidget = reinterpret_cast<DraggableTabWidget*>(sourceWidgetPtr);
        
        qDebug() << "Source widget:" << sourceTabWidget << "Index:" << sourceIndex << "Text:" << tabText;
        qDebug() << "Target widget:" << this;
        
        // Verify source widget is valid and not the same as target
        if (!sourceTabWidget || sourceTabWidget == this) {
            qDebug() << "Invalid source widget or same as target, ignoring drop";
            event->ignore();
            return;
        }
        
        // Verify source index is still valid
        if (sourceIndex < 0 || sourceIndex >= sourceTabWidget->count()) {
            qDebug() << "Source index out of range, ignoring drop";
            event->ignore();
            return;
        }
        
        // Double-check the tab text matches
        if (sourceTabWidget->tabText(sourceIndex) != tabText) {
            qDebug() << "Tab text mismatch, ignoring drop";
            event->ignore();
            return;
        }
        
        QWidget* sourceWidget = sourceTabWidget->widget(sourceIndex);
        if (!sourceWidget) {
            qDebug() << "Could not get source widget, ignoring drop";
            event->ignore();
            return;
        }
        
        qDebug() << "Moving tab from" << sourceTabWidget << "to" << this;
        
        // Get all tab properties before removing
        QString label = sourceTabWidget->tabText(sourceIndex);
        QIcon icon = sourceTabWidget->tabIcon(sourceIndex);
        QString tooltip = sourceTabWidget->tabToolTip(sourceIndex);
        
        // Remove from source (this will change indices, so do this carefully)
        sourceTabWidget->removeTab(sourceIndex);
        
        // Add to this widget
        int newIndex = addTab(sourceWidget, icon, label);
        setTabToolTip(newIndex, tooltip);
        setCurrentIndex(newIndex);
        
        event->acceptProposedAction();
        
        qDebug() << "Tab move completed successfully to index" << newIndex;
    } else {
        qDebug() << "Drop event not for our MIME type, passing to base class";
        QTabWidget::dropEvent(event);
    }
}

void DraggableTabWidget::onTabDetachRequested(int index, const QPoint& globalPos)
{
    if (!m_dragEnabled || index < 0 || index >= count()) {
        return;
    }
    
    QWidget* widget = this->widget(index);
    QString label = tabText(index);
    
    // Remove tab from this widget
    removeTab(index);
    
    // Emit signal for external handling
    emit tabDetached(widget, label, globalPos);
}

void DraggableTabWidget::onTabMoveRequested(int fromIndex, int toIndex)
{
    moveTab(fromIndex, toIndex);
}

void DraggableTabWidget::setupDragAndDrop()
{
    setAcceptDrops(m_dragEnabled);
    setMovable(false); // We handle movement ourselves
}

QString DraggableTabWidget::getTabMimeType() const
{
    return "application/x-neurodraft-tab";
}

// DetachedTabWindow Implementation
DetachedTabWindow::DetachedTabWindow(QWidget* contentWidget, const QString& title, QWidget* parent)
    : QWidget(parent, Qt::Window)
    , m_contentWidget(contentWidget)
    , m_title(title)
{
    setWindowTitle(title);
    setMinimumSize(400, 300);
    resize(800, 600);
    
    // Setup layout
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    
    if (contentWidget) {
        contentWidget->setParent(this);
        m_layout->addWidget(contentWidget);
    }
    
    // Set window flags for proper behavior
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | 
                   Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
}

DetachedTabWindow::~DetachedTabWindow() = default;

void DetachedTabWindow::closeEvent(QCloseEvent* event)
{
    emit windowClosed(m_contentWidget, m_title);
    event->accept();
}

void DetachedTabWindow::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    // Could emit position changed signal here if needed
}