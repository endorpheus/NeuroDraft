/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "MainWindow.h"
#include "ProjectManager.h"
#include "PaneManager.h"
#include "ProjectDialog.h"
#include "EditorWidget.h"
#include "ProjectTreeWidget.h"
#include "AutoSaveManager.h"
#include "DraggableTabWidget.h"
#include "UpdateManager.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include <QFontDialog>
#include <QInputDialog>
#include <QRegularExpression>
#include <QDir>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftPane(nullptr)
    , m_centerPane(nullptr)
    , m_rightPane(nullptr)
    , m_projectManager(std::make_unique<ProjectManager>(this))
    , m_paneManager(std::make_unique<PaneManager>(this))
    , m_autoSaveManager(std::make_unique<AutoSaveManager>(this))
    , m_updateManager(std::make_unique<UpdateManager>(this))
    , m_projectTree(nullptr)
    , m_currentProjectPath("")
    , m_projectModified(false)
    , m_currentEditor(nullptr)
{
    setupUI();
    setupMenus();
    setupStatusBar();
    setupShortcuts();
    updateWindowTitle();
    
    // Configure window properties for proper resizing
    setMinimumSize(800, 600);         // Smaller minimum for flexibility
    resize(1400, 900);                // Good default size
    
    // Enable window resizing and all window controls
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | 
                   Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    
    // Make splitters handle resizing properly
    m_mainSplitter->setStretchFactor(0, 0);  // Left pane - fixed proportion
    m_mainSplitter->setStretchFactor(1, 1);  // Center pane - takes most space
    m_mainSplitter->setStretchFactor(2, 0);  // Right pane - fixed proportion
}

MainWindow::~MainWindow() 
{
    // Save all files on exit to prevent data loss
    if (m_autoSaveManager) {
        m_autoSaveManager->saveAllOnExit();
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    // Create main horizontal splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create the three main panes as draggable tab widgets
    m_leftPane = new DraggableTabWidget();
    m_centerPane = new DraggableTabWidget();
    m_rightPane = new DraggableTabWidget();
    
    // Configure panes - move all tabs to top
    m_leftPane->setTabPosition(QTabWidget::North);
    m_centerPane->setTabPosition(QTabWidget::North);
    m_rightPane->setTabPosition(QTabWidget::North);
    
    // Enable tab features for center pane
    m_centerPane->setTabsClosable(true);
    
    // Connect draggable tab signals for all panes
    connect(m_leftPane, &DraggableTabWidget::tabDetached, this, &MainWindow::onTabDetached);
    connect(m_leftPane, &DraggableTabWidget::tabAttachRequested, this, &MainWindow::onTabAttachRequested);
    
    connect(m_centerPane, &DraggableTabWidget::tabDetached, this, &MainWindow::onTabDetached);
    connect(m_centerPane, &DraggableTabWidget::tabAttachRequested, this, &MainWindow::onTabAttachRequested);
    connect(m_centerPane, &QTabWidget::currentChanged, this, &MainWindow::onChapterTabChanged);
    connect(m_centerPane, &QTabWidget::tabCloseRequested, this, &MainWindow::onChapterTabCloseRequested);
    
    connect(m_rightPane, &DraggableTabWidget::tabDetached, this, &MainWindow::onTabDetached);
    connect(m_rightPane, &DraggableTabWidget::tabAttachRequested, this, &MainWindow::onTabAttachRequested);
    
    // Create and add project tree
    m_projectTree = new ProjectTreeWidget(this);
    connect(m_projectTree, &ProjectTreeWidget::itemOpenRequested, this, &MainWindow::openChapterFile);
    connect(m_projectTree, &ProjectTreeWidget::chapterCreated, this, &MainWindow::onChapterCreatedFromTree);
    connect(m_projectTree, &ProjectTreeWidget::itemMoved, this, &MainWindow::onTreeItemMoved);
    
    // Connect itemRenamed with lambda to handle the signature properly
    connect(m_projectTree, &ProjectTreeWidget::itemRenamed, this, 
            [this](const QString& oldName, const QString& newName, ProjectTreeWidget::ItemType type, const QString& filePath) {
                onTreeItemRenamed(oldName, newName, static_cast<int>(type), filePath);
            });
    
    m_leftPane->addTab(m_projectTree, "Projects");
    
    // Setup UpdateManager dependencies
    m_updateManager->setProjectTree(m_projectTree);
    m_updateManager->setProjectManager(m_projectManager.get());
    
    // Connect update manager signals
    connect(m_updateManager.get(), &UpdateManager::updateError, this, [this](const QString& error) {
        QMessageBox::warning(this, "Update Error", error);
    });
    connect(m_updateManager.get(), &UpdateManager::numberingUpdated, this, [this](const QString& projectPath) {
        statusBar()->showMessage("Project numbering updated", 2000);
        m_projectTree->refreshProject(projectPath);
    });
    
    // Connect auto-save manager signals for change indicators
    connect(m_autoSaveManager.get(), &AutoSaveManager::autoSaveCompleted, 
            this, [this](int filesSaved) {
                if (filesSaved > 0) {
                    updateAllTabIndicators();
                    statusBar()->showMessage(QString("Auto-saved %1 file(s)").arg(filesSaved), 2000);
                }
            });
    
    m_leftPane->addTab(new QWidget(), "Navigator");
    m_leftPane->addTab(new QWidget(), "Characters");
    m_leftPane->addTab(new QWidget(), "Research");
    
    // Start with welcome tab in center
    QWidget* welcomeWidget = new QWidget();
    m_centerPane->addTab(welcomeWidget, "Welcome");
    
    m_rightPane->addTab(new QWidget(), "References");
    m_rightPane->addTab(new QWidget(), "Statistics");
    m_rightPane->addTab(new QWidget(), "Corkboard");
    
    // Add panes to splitter
    m_mainSplitter->addWidget(m_leftPane);
    m_mainSplitter->addWidget(m_centerPane);
    m_mainSplitter->addWidget(m_rightPane);
    
    // Set initial splitter sizes with better proportions for resizing
    m_mainSplitter->setSizes({250, 800, 250});  // More space for center editor
    
    // Configure splitter for better resizing behavior
    m_mainSplitter->setHandleWidth(4);
    m_mainSplitter->setChildrenCollapsible(false);  // Prevent panes from collapsing completely
    
    // Layout with proper stretch and margin handling
    QHBoxLayout* layout = new QHBoxLayout();
    layout->addWidget(m_mainSplitter);
    layout->setContentsMargins(2, 2, 2, 2);  // Small margins for clean look
    layout->setSpacing(0);
    m_centralWidget->setLayout(layout);
    
    // Ensure central widget expands properly
    m_centralWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MainWindow::setupMenus()
{
    // File Menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    m_newProjectAction = new QAction("&New Project...", this);
    m_newProjectAction->setShortcut(QKeySequence::New);
    connect(m_newProjectAction, &QAction::triggered, this, &MainWindow::newProject);
    fileMenu->addAction(m_newProjectAction);
    
    m_openProjectAction = new QAction("&Open Project...", this);
    m_openProjectAction->setShortcut(QKeySequence::Open);
    connect(m_openProjectAction, &QAction::triggered, this, &MainWindow::openProject);
    fileMenu->addAction(m_openProjectAction);
    
    fileMenu->addSeparator();
    
    m_saveProjectAction = new QAction("&Save Project", this);
    m_saveProjectAction->setShortcut(QKeySequence::Save);
    connect(m_saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);
    fileMenu->addAction(m_saveProjectAction);
    
    m_closeProjectAction = new QAction("&Close Project", this);
    connect(m_closeProjectAction, &QAction::triggered, this, &MainWindow::closeProject);
    fileMenu->addAction(m_closeProjectAction);
    
    fileMenu->addSeparator();
    
    m_newChapterAction = new QAction("New &Chapter...", this);
    m_newChapterAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    connect(m_newChapterAction, &QAction::triggered, this, &MainWindow::newChapter);
    fileMenu->addAction(m_newChapterAction);
    
    m_openChapterAction = new QAction("&Open Chapter...", this);
    m_openChapterAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(m_openChapterAction, &QAction::triggered, this, &MainWindow::openChapter);
    fileMenu->addAction(m_openChapterAction);
    
    m_saveChapterAction = new QAction("&Save Chapter", this);
    m_saveChapterAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(m_saveChapterAction, &QAction::triggered, this, &MainWindow::saveCurrentChapter);
    fileMenu->addAction(m_saveChapterAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    // Edit Menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    
    m_findReplaceAction = new QAction("&Find && Replace...", this);
    m_findReplaceAction->setShortcut(QKeySequence::Find);
    connect(m_findReplaceAction, &QAction::triggered, this, &MainWindow::findReplace);
    editMenu->addAction(m_findReplaceAction);
    
    m_projectSearchAction = new QAction("&Project Search...", this);
    m_projectSearchAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(m_projectSearchAction, &QAction::triggered, this, &MainWindow::projectSearch);
    editMenu->addAction(m_projectSearchAction);
    
    // View Menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    
    m_splitHorizontalAction = new QAction("Split &Horizontal", this);
    connect(m_splitHorizontalAction, &QAction::triggered, this, &MainWindow::splitHorizontal);
    viewMenu->addAction(m_splitHorizontalAction);
    
    m_splitVerticalAction = new QAction("Split &Vertical", this);
    connect(m_splitVerticalAction, &QAction::triggered, this, &MainWindow::splitVertical);
    viewMenu->addAction(m_splitVerticalAction);
    
    // Format Menu
    QMenu* formatMenu = menuBar()->addMenu("&Format");
    
    m_selectFontAction = new QAction("Select &Font...", this);
    connect(m_selectFontAction, &QAction::triggered, this, &MainWindow::selectFont);
    formatMenu->addAction(m_selectFontAction);
    
    // Tools Menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction("Word Count &Targets...");
    toolsMenu->addAction("&Statistics...");
    toolsMenu->addAction("&Export...");
    
    // Help Menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About NeuroDraft");
}

void MainWindow::setupStatusBar()
{
    // Create status bar with permanent widgets
    QLabel* readyLabel = new QLabel("Ready");
    statusBar()->addWidget(readyLabel);
    
    // Add permanent project status
    m_projectStatusLabel = new QLabel("No project loaded");
    m_projectStatusLabel->setStyleSheet("color: #666; font-style: italic;");
    statusBar()->addPermanentWidget(m_projectStatusLabel);
    
    // Ensure status bar is always visible with minimum height
    statusBar()->setMinimumHeight(20);
    statusBar()->setSizeGripEnabled(true);
}

void MainWindow::setupShortcuts()
{
    // Additional shortcuts can be added here
}

void MainWindow::updateWindowTitle(const QString& projectName)
{
    QString title = "NeuroDraft";
    if (!projectName.isEmpty()) {
        title += " - " + projectName;
        if (m_projectModified) {
            title += " •"; // Use bullet to indicate unsaved project changes
        }
    }
    setWindowTitle(title);
}

void MainWindow::updateTabIndicator(EditorWidget* editor, int tabIndex)
{
    if (!editor || tabIndex < 0 || tabIndex >= m_centerPane->count()) {
        return;
    }
    
    QString baseText = QFileInfo(editor->getFilePath()).baseName();
    if (baseText.isEmpty()) {
        baseText = "Untitled";
    }
    
    QString tabText;
    if (editor->hasUnsavedChanges()) {
        tabText = baseText + " •"; // Add bullet for unsaved changes
    } else {
        tabText = baseText;
    }
    
    // Only update if the text actually changed to avoid unnecessary updates
    if (m_centerPane->tabText(tabIndex) != tabText) {
        m_centerPane->setTabText(tabIndex, tabText);
    }
}

void MainWindow::updateAllTabIndicators()
{
    for (int i = 0; i < m_centerPane->count(); ++i) {
        QWidget* widget = m_centerPane->widget(i);
        EditorWidget* editor = qobject_cast<EditorWidget*>(widget);
        if (editor) {
            updateTabIndicator(editor, i);
        }
    }
}

QString MainWindow::createSafeFileName(const QString& name) const
{
    QString safeName = name;
    // Remove or replace characters that are invalid in filenames
    safeName.replace(QRegularExpression("[<>:\"/\\|?*]"), "_");
    safeName.replace(QRegularExpression("\\s+"), "_");
    safeName = safeName.trimmed();
    
    if (safeName.isEmpty()) {
        safeName = "Untitled";
    }
    
    return safeName;
}

bool MainWindow::renameProjectFile(const QString& oldPath, const QString& newPath)
{
    if (oldPath == newPath) {
        return true; // No change needed
    }
    
    // Check if the new path already exists
    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, "Rename Error", 
            QString("Cannot rename file: '%1' already exists.").arg(QFileInfo(newPath).fileName()));
        return false;
    }
    
    // Try to rename the file
    QFile file(oldPath);
    if (!file.rename(newPath)) {
        QMessageBox::warning(this, "Rename Error", 
            QString("Failed to rename file from '%1' to '%2'.\nError: %3")
            .arg(QFileInfo(oldPath).fileName())
            .arg(QFileInfo(newPath).fileName())
            .arg(file.errorString()));
        return false;
    }
    
    qDebug() << "Successfully renamed file:" << oldPath << "→" << newPath;
    return true;
}

void MainWindow::updateOpenEditorPath(const QString& oldPath, const QString& newPath)
{
    // Update the editor's file path if it's currently open
    if (m_openEditors.contains(oldPath)) {
        EditorWidget* editor = m_openEditors[oldPath];
        
        // Remove from old path mapping
        m_openEditors.remove(oldPath);
        
        // Update editor's internal file path
        editor->setFilePath(newPath);
        
        // Update auto-save manager
        m_autoSaveManager->updateFilePath(editor, newPath);
        
        // Add to new path mapping
        m_openEditors[newPath] = editor;
        
        // Update tab indicator
        for (int i = 0; i < m_centerPane->count(); ++i) {
            if (m_centerPane->widget(i) == editor) {
                updateTabIndicator(editor, i);
                break;
            }
        }
        
        qDebug() << "Updated open editor path:" << oldPath << "→" << newPath;
    }
}

// Slot implementations
void MainWindow::newProject()
{
    ProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString projectPath = dialog.getProjectPath();
        QString projectName = dialog.getProjectName();
        
        if (m_projectManager->createProject(projectPath, projectName)) {
            m_currentProjectPath = projectPath;
            updateWindowTitle(projectName);
            m_projectStatusLabel->setText(QString("Project: %1").arg(projectName));
            statusBar()->showMessage("Project created successfully", 2000);
            
            // Connect project manager signals
            connect(m_projectManager.get(), &ProjectManager::projectOpened, 
                    this, &MainWindow::onProjectOpened);
            
            // Add to project tree
            m_projectTree->addProject(projectPath, projectName);
            
            // Load existing chapters if any
            loadProjectChapters();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create project.");
        }
    }
}

void MainWindow::openProject()
{
    QString projectFile = QFileDialog::getOpenFileName(
        this,
        "Open NeuroDraft Project",
        QDir::homePath(),
        "NeuroDraft Projects (*.json)"
    );
    
    if (!projectFile.isEmpty() && m_projectManager->openProject(projectFile)) {
        m_currentProjectPath = QFileInfo(projectFile).absolutePath();
        QString projectName = QFileInfo(m_currentProjectPath).baseName();
        updateWindowTitle(projectName);
        m_projectStatusLabel->setText(QString("Project: %1").arg(projectName));
        statusBar()->showMessage("Project opened successfully", 2000);
        
        // Connect project manager signals
        connect(m_projectManager.get(), &ProjectManager::projectOpened, 
                this, &MainWindow::onProjectOpened);
        
        // Add to project tree
        m_projectTree->addProject(m_currentProjectPath, projectName);
        
        // Load project chapters
        loadProjectChapters();
    }
}

void MainWindow::saveProject()
{
    if (m_currentProjectPath.isEmpty()) {
        QMessageBox::information(this, "No Project", "No project is currently open.");
        return;
    }
    
    if (m_projectManager->saveProject()) {
        m_projectModified = false;
        updateWindowTitle(QFileInfo(m_currentProjectPath).baseName());
        statusBar()->showMessage("Project saved", 2000);
    }
}

void MainWindow::closeProject()
{
    if (m_projectModified) {
        int ret = QMessageBox::question(this, "Unsaved Changes", 
            "Save changes before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            saveProject();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }
    
    m_currentProjectPath.clear();
    m_projectModified = false;
    updateWindowTitle();
    m_projectStatusLabel->setText("No project loaded");
    statusBar()->showMessage("Project closed", 2000);
}

void MainWindow::findReplace()
{
    // TODO: Implement find/replace dialog
    statusBar()->showMessage("Find/Replace - Coming soon", 2000);
}

void MainWindow::projectSearch()
{
    // TODO: Implement project-wide search
    statusBar()->showMessage("Project Search - Coming soon", 2000);
}

void MainWindow::convertTabToPane()
{
    // TODO: Implement tab to pane conversion
}

void MainWindow::convertPaneToTab()
{
    // TODO: Implement pane to tab conversion
}

void MainWindow::splitHorizontal()
{
    // TODO: Implement horizontal split
    statusBar()->showMessage("Horizontal Split - Coming soon", 2000);
}

void MainWindow::splitVertical()
{
    // TODO: Implement vertical split
    statusBar()->showMessage("Vertical Split - Coming soon", 2000);
}

void MainWindow::newChapter()
{
    if (m_currentProjectPath.isEmpty()) {
        QMessageBox::information(this, "No Project", "Please open or create a project first.");
        return;
    }
    
    bool ok;
    QString chapterName = QInputDialog::getText(this, "New Chapter", 
                                               "Chapter name:", QLineEdit::Normal, 
                                               "", &ok);
    
    if (ok && !chapterName.isEmpty()) {
        // Clean chapter name for filename
        QString cleanName = createSafeFileName(chapterName);
        QString chapterPath = QDir(m_currentProjectPath).filePath("chapters/" + cleanName + ".md");
        
        // Create new editor
        EditorWidget* editor = new EditorWidget(this);
        editor->setFilePath(chapterPath);
        
        // Add initial content
        editor->setContent("# " + chapterName + "\n\nBegin writing here...\n");
        
        // Connect content change signal for tab indicators
        connect(editor, &EditorWidget::contentChanged, this, [this, editor]() {
            // Find the tab index for this editor
            for (int i = 0; i < m_centerPane->count(); ++i) {
                if (m_centerPane->widget(i) == editor) {
                    updateTabIndicator(editor, i);
                    break;
                }
            }
        });
        
        // Add to center pane
        int tabIndex = m_centerPane->addTab(editor, chapterName);
        m_centerPane->setCurrentIndex(tabIndex);
        
        // Track the editor with auto-save
        m_openEditors[chapterPath] = editor;
        m_currentEditor = editor;
        m_autoSaveManager->registerEditor(editor, chapterPath);
        
        // Save immediately
        editor->saveToFile(chapterPath);
        
        // Update tab indicator after save
        updateTabIndicator(editor, tabIndex);
        
        // Refresh the project tree to show the new chapter
        m_projectTree->refreshProject(m_currentProjectPath);
        
        statusBar()->showMessage("Chapter created: " + chapterName, 2000);
    }
}

void MainWindow::openChapter()
{
    if (m_currentProjectPath.isEmpty()) {
        QMessageBox::information(this, "No Project", "Please open or create a project first.");
        return;
    }
    
    QString chapterPath = QFileDialog::getOpenFileName(
        this,
        "Open Chapter",
        QDir(m_currentProjectPath).filePath("chapters"),
        "Markdown Files (*.md);;Text Files (*.txt);;All Files (*)"
    );
    
    if (!chapterPath.isEmpty()) {
        openChapterFile(chapterPath);
    }
}

void MainWindow::saveCurrentChapter()
{
    if (m_currentEditor) {
        if (m_currentEditor->saveToFile(m_currentEditor->getFilePath())) {
            // Update tab indicator after save
            for (int i = 0; i < m_centerPane->count(); ++i) {
                if (m_centerPane->widget(i) == m_currentEditor) {
                    updateTabIndicator(m_currentEditor, i);
                    break;
                }
            }
            statusBar()->showMessage("Chapter saved", 2000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save chapter.");
        }
    } else {
        statusBar()->showMessage("No chapter open to save", 2000);
    }
}

void MainWindow::selectFont()
{
    bool ok;
    QFont currentFont("Liberation Serif", 12);
    if (m_currentEditor) {
        currentFont = m_currentEditor->font();
    }
    
    QFont font = QFontDialog::getFont(&ok, currentFont, this, "Select Font");
    if (ok) {
        // Apply to current editor
        if (m_currentEditor) {
            m_currentEditor->setFont(font);
        }
        
        // Apply to all open editors
        for (auto* editor : m_openEditors) {
            editor->setFont(font);
        }
        
        statusBar()->showMessage("Font changed to " + font.family(), 2000);
    }
}

void MainWindow::onProjectOpened(const QString& projectName)
{
    updateWindowTitle(projectName);
    m_projectStatusLabel->setText(QString("Project: %1").arg(projectName));
    loadProjectChapters();
}

void MainWindow::onChapterTabChanged(int index)
{
    if (index >= 0 && index < m_centerPane->count()) {
        QWidget* widget = m_centerPane->widget(index);
        m_currentEditor = qobject_cast<EditorWidget*>(widget);
        
        if (m_currentEditor) {
            QString tabText = m_centerPane->tabText(index);
            // Remove the change indicator for status display
            QString cleanTabText = tabText;
            cleanTabText.remove(" •");
            statusBar()->showMessage("Editing: " + cleanTabText, 2000);
        }
    }
}

void MainWindow::onChapterTabCloseRequested(int index)
{
    if (index >= 0 && index < m_centerPane->count()) {
        QWidget* widget = m_centerPane->widget(index);
        EditorWidget* editor = qobject_cast<EditorWidget*>(widget);
        
        if (editor && editor->hasUnsavedChanges()) {
            int ret = QMessageBox::question(this, "Unsaved Changes", 
                "Save changes before closing?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            
            if (ret == QMessageBox::Save) {
                editor->saveToFile(editor->getFilePath());
            } else if (ret == QMessageBox::Cancel) {
                return;
            }
        }
        
        // Remove from tracking
        if (editor) {
            m_openEditors.remove(editor->getFilePath());
            if (m_currentEditor == editor) {
                m_currentEditor = nullptr;
            }
        }
        
        // Remove tab
        m_centerPane->removeTab(index);
        
        // Update current editor
        if (m_centerPane->count() > 0) {
            onChapterTabChanged(m_centerPane->currentIndex());
        }
    }
}

void MainWindow::loadProjectChapters()
{
    if (m_currentProjectPath.isEmpty()) {
        return;
    }
    
    // Clear welcome tab if it exists
    for (int i = 0; i < m_centerPane->count(); ++i) {
        if (m_centerPane->tabText(i) == "Welcome") {
            m_centerPane->removeTab(i);
            break;
        }
    }
    
    // Refresh the project tree first
    m_projectTree->refreshProject(m_currentProjectPath);
    
    // Load existing chapters
    QStringList chapters = m_projectManager->getChapterList();
    QString chaptersPath = QDir(m_currentProjectPath).filePath("chapters");
    
    for (const QString& chapter : chapters) {
        QString chapterPath = QDir(chaptersPath).filePath(chapter + ".md");
        if (QFile::exists(chapterPath)) {
            openChapterFile(chapterPath);
        }
    }
    
    statusBar()->showMessage(QString("Loaded %1 chapters").arg(chapters.size()), 2000);
}

void MainWindow::openChapterFile(const QString& filePath)
{
    // Check if already open
    if (m_openEditors.contains(filePath)) {
        // Switch to existing tab
        EditorWidget* existingEditor = m_openEditors[filePath];
        for (int i = 0; i < m_centerPane->count(); ++i) {
            if (m_centerPane->widget(i) == existingEditor) {
                m_centerPane->setCurrentIndex(i);
                break;
            }
        }
        return;
    }
    
    // Create new editor
    EditorWidget* editor = new EditorWidget(this);
    if (editor->loadFromFile(filePath)) {
        QFileInfo fileInfo(filePath);
        QString tabName = fileInfo.baseName();
        
        // Connect content change signal for tab indicators
        connect(editor, &EditorWidget::contentChanged, this, [this, editor]() {
            // Find the tab index for this editor
            for (int i = 0; i < m_centerPane->count(); ++i) {
                if (m_centerPane->widget(i) == editor) {
                    updateTabIndicator(editor, i);
                    break;
                }
            }
        });
        
        // Add to center pane
        int tabIndex = m_centerPane->addTab(editor, tabName);
        m_centerPane->setCurrentIndex(tabIndex);
        
        // Track the editor with auto-save
        m_openEditors[filePath] = editor;
        m_currentEditor = editor;
        m_autoSaveManager->registerEditor(editor, filePath);
        
        // Update tab indicator
        updateTabIndicator(editor, tabIndex);
        
        statusBar()->showMessage("Opened: " + tabName, 2000);
    } else {
        delete editor;
        QMessageBox::warning(this, "Error", "Failed to open chapter file.");
    }
}

void MainWindow::onChapterCreatedFromTree(const QString& projectPath, const QString& chapterName)
{
    // Set the project as current if it's not already
    if (m_currentProjectPath != projectPath) {
        m_currentProjectPath = projectPath;
        QString projectName = QFileInfo(projectPath).baseName();
        updateWindowTitle(projectName);
        m_projectStatusLabel->setText(QString("Project: %1").arg(projectName));
    }
    
    // Create the chapter using existing logic
    QString cleanName = createSafeFileName(chapterName);
    QString chapterPath = QDir(projectPath).filePath("chapters/" + cleanName + ".md");
    
    // Create new editor
    EditorWidget* editor = new EditorWidget(this);
    editor->setFilePath(chapterPath);
    
    // Add initial content
    editor->setContent("# " + chapterName + "\n\nBegin writing here...\n");
    
    // Connect content change signal for tab indicators
    connect(editor, &EditorWidget::contentChanged, this, [this, editor]() {
        // Find the tab index for this editor
        for (int i = 0; i < m_centerPane->count(); ++i) {
            if (m_centerPane->widget(i) == editor) {
                updateTabIndicator(editor, i);
                break;
            }
        }
    });
    
    // Add to center pane
    int tabIndex = m_centerPane->addTab(editor, chapterName);
    m_centerPane->setCurrentIndex(tabIndex);
    
    // Track the editor with auto-save
    m_openEditors[chapterPath] = editor;
    m_currentEditor = editor;
    m_autoSaveManager->registerEditor(editor, chapterPath);
    
    // Save immediately
    editor->saveToFile(chapterPath);
    
    // Update tab indicator after save
    updateTabIndicator(editor, tabIndex);
    
    // Refresh the project tree
    m_projectTree->refreshProject(projectPath);
    
    statusBar()->showMessage("Chapter created: " + chapterName, 2000);
}

void MainWindow::onTabDetached(QWidget* widget, const QString& label, const QPoint& globalPos)
{
    // For now, just show a message about the detachment
    // In a future implementation, we'll create a floating window
    QMessageBox::information(this, "Tab Detached", 
                            QString("Tab '%1' was detached at position (%2, %3)")
                            .arg(label).arg(globalPos.x()).arg(globalPos.y()));
    
    // For now, reattach to center pane
    m_centerPane->attachTab(widget, label);
    
    statusBar()->showMessage("Tab detached: " + label, 2000);
}

void MainWindow::onTabAttachRequested(QWidget* widget, const QString& label)
{
    statusBar()->showMessage("Tab reattached: " + label, 2000);
}

void MainWindow::onTreeItemMoved(const QString& fromPath, const QString& toPath, int itemType)
{
    // Handle different item types
    if (itemType == ProjectTreeWidget::ChapterItem) {
        int fromIndex = fromPath.toInt();
        int toIndex = toPath.toInt();
        
        qDebug() << "MainWindow: Chapter moved from index" << fromIndex << "to" << toIndex;
        
        if (!m_currentProjectPath.isEmpty()) {
            // Trigger the UpdateManager to handle the move
            if (m_updateManager->moveChapter(m_currentProjectPath, fromIndex, toIndex)) {
                statusBar()->showMessage("Chapters reordered successfully", 2000);
            } else {
                statusBar()->showMessage("Failed to reorder chapters", 2000);
            }
        }
    } else {
        statusBar()->showMessage("Item moved in project tree", 2000);
    }
}

void MainWindow::onTreeItemRenamed(const QString& oldName, const QString& newName, int itemType, const QString& filePath)
{
    qDebug() << "MainWindow: Item renamed from" << oldName << "to" << newName << "Type:" << itemType;
    
    // Handle file renaming based on item type
    if (itemType == ProjectTreeWidget::ChapterItem) {
        // Rename the actual chapter file
        if (!filePath.isEmpty() && QFile::exists(filePath)) {
            QFileInfo oldFileInfo(filePath);
            QString newFileName = createSafeFileName(newName) + ".md";
            QString newFilePath = QDir(oldFileInfo.absolutePath()).filePath(newFileName);
            
            qDebug() << "Attempting to rename chapter file:" << filePath << "→" << newFilePath;
            
            if (renameProjectFile(filePath, newFilePath)) {
                // Update any open editor
                updateOpenEditorPath(filePath, newFilePath);
                
                // Refresh the project tree to reflect the file change
                m_projectTree->refreshProject(m_currentProjectPath);
                
                statusBar()->showMessage("Chapter renamed: " + oldName + " → " + newName, 2000);
            } else {
                // Rename failed, refresh tree to revert the display name
                m_projectTree->refreshProject(m_currentProjectPath);
                statusBar()->showMessage("Failed to rename chapter file", 2000);
            }
        } else {
            qDebug() << "Chapter file path is empty or doesn't exist:" << filePath;
            statusBar()->showMessage("Chapter file not found for renaming", 2000);
        }
    } 
    else if (itemType == ProjectTreeWidget::CharacterItem) {
        // Handle character file renaming when implemented
        statusBar()->showMessage("Character renamed: " + oldName + " → " + newName, 2000);
    }
    else if (itemType == ProjectTreeWidget::ResearchItem) {
        // Handle research file renaming
        if (!filePath.isEmpty() && QFile::exists(filePath)) {
            QFileInfo oldFileInfo(filePath);
            QString extension = oldFileInfo.suffix();
            QString newFileName = createSafeFileName(newName);
            if (!extension.isEmpty()) {
                newFileName += "." + extension;
            }
            QString newFilePath = QDir(oldFileInfo.absolutePath()).filePath(newFileName);
            
            if (renameProjectFile(filePath, newFilePath)) {
                updateOpenEditorPath(filePath, newFilePath);
                m_projectTree->refreshProject(m_currentProjectPath);
                statusBar()->showMessage("Research file renamed: " + oldName + " → " + newName, 2000);
            } else {
                m_projectTree->refreshProject(m_currentProjectPath);
                statusBar()->showMessage("Failed to rename research file", 2000);
            }
        }
    }
    else {
        statusBar()->showMessage("Item renamed: " + oldName + " → " + newName, 2000);
    }
    
    // Update any open tabs with the old name (for display purposes)
    for (int i = 0; i < m_centerPane->count(); ++i) {
        QWidget* widget = m_centerPane->widget(i);
        EditorWidget* editor = qobject_cast<EditorWidget*>(widget);
        
        if (editor) {
            QString currentTabText = m_centerPane->tabText(i);
            // Remove change indicator to get base name
            QString baseTabName = currentTabText;
            baseTabName.remove(" •");
            
            if (baseTabName == oldName) {
                // Update the tab with new name, preserving change indicator if present
                if (editor->hasUnsavedChanges()) {
                    m_centerPane->setTabText(i, newName + " •");
                } else {
                    m_centerPane->setTabText(i, newName);
                }
                qDebug() << "Updated tab text from" << oldName << "to" << newName;
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save all files before closing to prevent data loss
    if (m_autoSaveManager) {
        m_autoSaveManager->saveAllOnExit();
    }
    
    // Check for unsaved project changes
    if (m_projectModified) {
        int ret = QMessageBox::question(this, "Unsaved Project Changes", 
            "Save project changes before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            saveProject();
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    
    // Accept the close event
    event->accept();
}