/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef PROJECTTREEWIDGET_H
#define PROJECTTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QString>
#include <QStringList>
#include <QHash>

class ProjectManager;

class ProjectTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    enum ItemType {
        ProjectItem = QTreeWidgetItem::UserType + 1,
        ChaptersFolderItem,
        ChapterItem,
        SubsectionItem,
        CharactersFolderItem,
        CharacterItem,
        ResearchFolderItem,
        ResearchItem,
        CorkboardFolderItem,
        CorkboardItem
    };

    explicit ProjectTreeWidget(QWidget *parent = nullptr);
    ~ProjectTreeWidget();

    // Project management
    void addProject(const QString& projectPath, const QString& projectName);
    void removeProject(const QString& projectPath);
    void refreshProject(const QString& projectPath);
    void refreshAllProjects();
    
    // Tree operations
    void expandProject(const QString& projectPath);
    void collapseProject(const QString& projectPath);
    void startInlineEdit(QTreeWidgetItem* item);
    bool validateItemName(QTreeWidgetItem* item, const QString& newName);
    QString suggestAlternativeName(QTreeWidgetItem* item, const QString& baseName);
    
    // Item creation
    QTreeWidgetItem* createChapterItem(const QString& chapterName, const QString& filePath);
    QTreeWidgetItem* createSubsectionItem(const QString& subsectionTitle, int position);
    QTreeWidgetItem* createCharacterItem(const QString& characterName);
    QTreeWidgetItem* createResearchItem(const QString& researchName);

signals:
    void itemOpenRequested(const QString& filePath, const QString& subsection = QString());
    void chapterCreated(const QString& projectPath, const QString& chapterName);
    void subsectionCreated(const QString& chapterPath, const QString& subsectionTitle);
    void characterCreated(const QString& projectPath, const QString& characterName);
    void itemMoved(const QString& fromPath, const QString& toPath, ItemType type);
    void itemRenamed(const QString& oldName, const QString& newName, ItemType type, const QString& filePath);
    void itemDeleted(const QString& path, ItemType type);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemSelectionChanged();
    void onItemChanged(QTreeWidgetItem* item, int column);
    void showCustomContextMenu(const QPoint& pos);
    void createNewChapter();
    void createNewSubsection();
    void createNewCharacter();
    void createNewResearch();
    void renameItem();
    void deleteItem();
    void moveItemUp();
    void moveItemDown();

private:
    void setupContextMenus();
    void setupDragDrop();
    void populateProjectTree(QTreeWidgetItem* projectItem, const QString& projectPath);
    void populateChapters(QTreeWidgetItem* chaptersFolder, const QString& projectPath);
    void populateCharacters(QTreeWidgetItem* charactersFolder, const QString& projectPath);
    void populateResearch(QTreeWidgetItem* researchFolder, const QString& projectPath);
    void populateCorkboard(QTreeWidgetItem* corkboardFolder, const QString& projectPath);
    
    // Helper methods
    QString getItemPath(QTreeWidgetItem* item) const;
    QString getProjectPath(QTreeWidgetItem* item) const;
    QTreeWidgetItem* findProjectItem(const QString& projectPath) const;
    QTreeWidgetItem* findChaptersFolder(QTreeWidgetItem* projectItem) const;
    bool canDropOn(QTreeWidgetItem* target, ItemType sourceType) const;
    void updateChapterNumbers(QTreeWidgetItem* chaptersFolder);
    void saveTreeState();
    void restoreTreeState();
    
    // Context menus
    QMenu* m_projectMenu;
    QMenu* m_chapterMenu;
    QMenu* m_subsectionMenu;
    QMenu* m_characterMenu;
    QMenu* m_researchMenu;
    QMenu* m_folderMenu;
    
    // Actions
    QAction* m_newChapterAction;
    QAction* m_newSubsectionAction;
    QAction* m_newCharacterAction;
    QAction* m_newResearchAction;
    QAction* m_renameAction;
    QAction* m_deleteAction;
    QAction* m_moveUpAction;
    QAction* m_moveDownAction;
    
    // State tracking
    QHash<QString, QTreeWidgetItem*> m_projectItems;
    QTreeWidgetItem* m_currentContextItem;
    QTreeWidgetItem* m_editingItem;
    QString m_originalItemText;
    bool m_dragDropEnabled;
    
    // Project managers for each open project
    QHash<QString, ProjectManager*> m_projectManagers;
};

#endif // PROJECTTREEWIDGET_H