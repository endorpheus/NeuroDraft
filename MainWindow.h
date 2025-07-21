/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QCloseEvent>
#include <memory>

class ProjectManager;
class EditorWidget;
class PaneManager;
class ProjectTreeWidget;
class AutoSaveManager;
class DraggableTabWidget;
class UpdateManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void closeProject();
    void newChapter();
    void openChapter();
    void saveCurrentChapter();
    void findReplace();
    void projectSearch();
    void selectFont();
    void convertTabToPane();
    void convertPaneToTab();
    void splitHorizontal();
    void splitVertical();
    void onProjectOpened(const QString& projectName);
    void onChapterTabChanged(int index);
    void onChapterTabCloseRequested(int index);
    void onChapterCreatedFromTree(const QString& projectPath, const QString& chapterName);
    void onTabDetached(QWidget* widget, const QString& label, const QPoint& globalPos);
    void onTabAttachRequested(QWidget* widget, const QString& label);
    void onTreeItemMoved(const QString& fromPath, const QString& toPath, int itemType);
    void onTreeItemRenamed(const QString& oldName, const QString& newName, int itemType, const QString& filePath);

private:
    void setupUI();
    void setupMenus();
    void setupStatusBar();
    void setupShortcuts();
    void updateWindowTitle(const QString& projectName = QString());
    void loadProjectChapters();
    void openChapterFile(const QString& filePath);
    
    // Change indicator methods
    void updateTabIndicator(EditorWidget* editor, int tabIndex);
    void updateAllTabIndicators();
    
    // File operations
    QString createSafeFileName(const QString& name) const;
    bool renameProjectFile(const QString& oldPath, const QString& newPath);
    void updateOpenEditorPath(const QString& oldPath, const QString& newPath);
    
    // Core UI components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    
    // Three main panes
    DraggableTabWidget* m_leftPane;     // Project navigator
    DraggableTabWidget* m_centerPane;   // Main editor
    DraggableTabWidget* m_rightPane;    // Word references
    
    // Managers
    std::unique_ptr<ProjectManager> m_projectManager;
    std::unique_ptr<PaneManager> m_paneManager;
    std::unique_ptr<AutoSaveManager> m_autoSaveManager;
    std::unique_ptr<UpdateManager> m_updateManager;
    
    // Custom widgets
    ProjectTreeWidget* m_projectTree;
    
    // Status bar components
    QLabel* m_projectStatusLabel;
    
    // Menu actions
    QAction* m_newProjectAction;
    QAction* m_openProjectAction;
    QAction* m_saveProjectAction;
    QAction* m_closeProjectAction;
    QAction* m_newChapterAction;
    QAction* m_openChapterAction;
    QAction* m_saveChapterAction;
    QAction* m_findReplaceAction;
    QAction* m_projectSearchAction;
    QAction* m_selectFontAction;
    QAction* m_splitHorizontalAction;
    QAction* m_splitVerticalAction;
    
    // Current project state
    QString m_currentProjectPath;
    bool m_projectModified;
    
    // Document management
    QHash<QString, EditorWidget*> m_openEditors;  // filename -> editor
    EditorWidget* m_currentEditor;
};

#endif // MAINWINDOW_H