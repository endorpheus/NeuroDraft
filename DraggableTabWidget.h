/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef DRAGGABLETABWIDGET_H
#define DRAGGABLETABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QApplication>
#include <QDrag>
#include <QVBoxLayout>

class DraggableTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DraggableTabBar(QWidget* parent = nullptr);

signals:
    void tabDetachRequested(int index, const QPoint& globalPos);
    void tabMoveRequested(int fromIndex, int toIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    bool m_dragStarted;
    QPoint m_dragStartPos;
    int m_dragIndex;
};

class DraggableTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit DraggableTabWidget(QWidget* parent = nullptr);
    ~DraggableTabWidget();

    // Enable/disable tab dragging
    void setTabDragEnabled(bool enabled);
    bool isTabDragEnabled() const;
    
    // Detached tab management
    void attachTab(QWidget* widget, const QString& label, int index = -1);
    QWidget* detachTab(int index);
    
    // Tab reordering
    void moveTab(int fromIndex, int toIndex);

signals:
    void tabDetached(QWidget* widget, const QString& label, const QPoint& globalPos);
    void tabAttachRequested(QWidget* widget, const QString& label);
    void tabReordered(int fromIndex, int toIndex);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onTabDetachRequested(int index, const QPoint& globalPos);
    void onTabMoveRequested(int fromIndex, int toIndex);

private:
    void setupDragAndDrop();
    QString getTabMimeType() const;
    
    DraggableTabBar* m_tabBar;
    bool m_dragEnabled;
    
    // Static counter for unique tab identification
    static int s_tabCounter;
};

// Detached tab window for floating tabs
class DetachedTabWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DetachedTabWindow(QWidget* contentWidget, const QString& title, QWidget* parent = nullptr);
    ~DetachedTabWindow();
    
    QWidget* getContentWidget() const { return m_contentWidget; }
    QString getTitle() const { return m_title; }

signals:
    void windowClosed(QWidget* contentWidget, const QString& title);

protected:
    void closeEvent(QCloseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;

private:
    QWidget* m_contentWidget;
    QString m_title;
    QVBoxLayout* m_layout;
};

#endif // DRAGGABLETABWIDGET_H