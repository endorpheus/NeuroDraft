/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef PANEMANAGER_H
#define PANEMANAGER_H

#include <QObject>
#include <QTabWidget>
#include <QSplitter>
#include <QWidget>
#include <QHash>
#include <QUuid>
#include <memory>

class PaneManager : public QObject
{
    Q_OBJECT

public:
    enum class PaneType {
        TabWidget,
        HorizontalSplit,
        VerticalSplit
    };

    struct PaneInfo {
        QUuid id;
        PaneType type;
        QWidget* widget;
        QSplitter* splitter;
        QTabWidget* tabWidget;
        QWidget* parentPane;
        QString title;
        bool isDetached;
    };

    explicit PaneManager(QObject *parent = nullptr);
    ~PaneManager();

    // Tab to pane conversion
    QUuid convertTabToPane(QTabWidget* sourceTabWidget, int tabIndex, 
                          PaneType splitType = PaneType::HorizontalSplit);
    bool convertPaneToTab(const QUuid& paneId, QTabWidget* targetTabWidget);
    
    // Pane operations
    QUuid createPane(PaneType type, QWidget* parent = nullptr);
    bool closePane(const QUuid& paneId);
    bool splitPane(const QUuid& paneId, PaneType splitType);
    
    // Pane management
    PaneInfo* getPaneInfo(const QUuid& paneId);
    QList<QUuid> getAllPanes() const;
    QTabWidget* createTabWidget(QWidget* parent = nullptr);
    
    // Detached windows
    bool detachPane(const QUuid& paneId);
    bool attachPane(const QUuid& paneId, QWidget* parent);
    
    // State management
    void savePaneLayout();
    void restorePaneLayout();

signals:
    void paneCreated(const QUuid& paneId);
    void paneDestroyed(const QUuid& paneId);
    void paneDetached(const QUuid& paneId);
    void paneAttached(const QUuid& paneId);

private slots:
    void onTabCloseRequested(int index);
    void onTabDetachRequested(int index);

private:
    void setupTabWidget(QTabWidget* tabWidget);
    void cleanupPane(const QUuid& paneId);
    QSplitter* createSplitter(Qt::Orientation orientation, QWidget* parent = nullptr);
    
    QHash<QUuid, PaneInfo*> m_panes;
    QWidget* m_mainParent;
};

#endif // PANEMANAGER_H