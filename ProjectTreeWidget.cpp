/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "ProjectTreeWidget.h"
#include "ProjectManager.h"
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QMimeData>
#include <QDebug>
#include <QDateTime>

ProjectTreeWidget::ProjectTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
    , m_currentContextItem(nullptr)
    , m_dragDropEnabled(true)
{
    setupContextMenus();
    setupDragDrop();
    
    // Configure tree widget
    setHeaderLabel("Project Structure");
    
    // Set context menu policy
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &ProjectTreeWidget::showCustomContextMenu);
    
    // Enable drag and drop with visual indicators
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    
    // Enable editing
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Make sure selection is visible
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Set alternating row colors for better visibility
    setAlternatingRowColors(true);
    setRootIsDecorated(true);
    setItemsExpandable(true);
    
    // Connect signals
    connect(this, &QTreeWidget::itemDoubleClicked, this, &ProjectTreeWidget::onItemDoubleClicked);
    connect(this, &QTreeWidget::itemSelectionChanged, this, &ProjectTreeWidget::onItemSelectionChanged);
    connect(this, &QTreeWidget::itemChanged, this, &ProjectTreeWidget::onItemChanged);
    
    // Load previous state
    restoreTreeState();
}

ProjectTreeWidget::~ProjectTreeWidget()
{
    saveTreeState();
}

void ProjectTreeWidget::addProject(const QString& projectPath, const QString& projectName)
{
    // Check if project already exists
    if (m_projectItems.contains(projectPath)) {
        return;
    }
    
    // Create project root item
    QTreeWidgetItem* projectItem = new QTreeWidgetItem(this, ProjectItem);
    projectItem->setText(0, projectName);
    projectItem->setData(0, Qt::UserRole, projectPath);
    projectItem->setExpanded(true);
    
    // Store reference
    m_projectItems[projectPath] = projectItem;
    
    // Populate project structure
    populateProjectTree(projectItem, projectPath);
    
    // Add to tree
    addTopLevelItem(projectItem);
    
    qDebug() << "Added project to tree:" << projectName;
}

void ProjectTreeWidget::removeProject(const QString& projectPath)
{
    if (m_projectItems.contains(projectPath)) {
        QTreeWidgetItem* item = m_projectItems[projectPath];
        delete item;
        m_projectItems.remove(projectPath);
        
        // Clean up project manager if exists
        if (m_projectManagers.contains(projectPath)) {
            delete m_projectManagers[projectPath];
            m_projectManagers.remove(projectPath);
        }
        
        qDebug() << "Removed project from tree:" << projectPath;
    }
}

void ProjectTreeWidget::refreshProject(const QString& projectPath)
{
    if (m_projectItems.contains(projectPath)) {
        QTreeWidgetItem* projectItem = m_projectItems[projectPath];
        
        // Clear existing children
        while (projectItem->childCount() > 0) {
            delete projectItem->takeChild(0);
        }
        
        // Repopulate
        populateProjectTree(projectItem, projectPath);
        
        qDebug() << "Refreshed project tree:" << projectPath;
    }
}

void ProjectTreeWidget::refreshAllProjects()
{
    for (const QString& projectPath : m_projectItems.keys()) {
        refreshProject(projectPath);
    }
}

void ProjectTreeWidget::expandProject(const QString& projectPath)
{
    if (m_projectItems.contains(projectPath)) {
        m_projectItems[projectPath]->setExpanded(true);
    }
}

void ProjectTreeWidget::collapseProject(const QString& projectPath)
{
    if (m_projectItems.contains(projectPath)) {
        m_projectItems[projectPath]->setExpanded(false);
    }
}

QTreeWidgetItem* ProjectTreeWidget::createChapterItem(const QString& chapterName, const QString& filePath)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ChapterItem);
    item->setText(0, chapterName);
    item->setData(0, Qt::UserRole, filePath);
    
    return item;
}

QTreeWidgetItem* ProjectTreeWidget::createSubsectionItem(const QString& subsectionTitle, int position)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(SubsectionItem);
    item->setText(0, subsectionTitle.isEmpty() ? QString("Section %1").arg(position) : subsectionTitle);
    item->setData(0, Qt::UserRole + 1, position);
    
    return item;
}

QTreeWidgetItem* ProjectTreeWidget::createCharacterItem(const QString& characterName)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(CharacterItem);
    item->setText(0, characterName);
    
    return item;
}

QTreeWidgetItem* ProjectTreeWidget::createResearchItem(const QString& researchName)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ResearchItem);
    item->setText(0, researchName);
    
    return item;
}

void ProjectTreeWidget::showCustomContextMenu(const QPoint& pos)
{
    qDebug() << "showCustomContextMenu called at position:" << pos;
    
    QTreeWidgetItem* item = itemAt(pos);
    if (!item) {
        qDebug() << "No item at context menu position";
        return;
    }
    
    m_currentContextItem = item;
    qDebug() << "Context menu on item:" << item->text(0) << "Type:" << item->type();
    
    QMenu* menu = nullptr;
    
    switch (item->type()) {
        case ProjectItem:
            menu = m_projectMenu;
            break;
        case ChaptersFolderItem:
        case CharactersFolderItem:
        case ResearchFolderItem:
        case CorkboardFolderItem:
            menu = m_folderMenu;
            break;
        case ChapterItem:
            menu = m_chapterMenu;
            break;
        case SubsectionItem:
            menu = m_subsectionMenu;
            break;
        case CharacterItem:
            menu = m_characterMenu;
            break;
        case ResearchItem:
            menu = m_researchMenu;
            break;
        default:
            qDebug() << "Unknown item type:" << item->type();
            break;
    }
    
    if (menu) {
        qDebug() << "Showing context menu with" << menu->actions().size() << "actions";
        menu->exec(mapToGlobal(pos));
    }
}

void ProjectTreeWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QTreeWidgetItem* item = itemAt(event->pos());
    if (!item) {
        return;
    }
    
    m_currentContextItem = item;
    
    QMenu* menu = nullptr;
    
    switch (item->type()) {
        case ProjectItem:
            menu = m_projectMenu;
            break;
        case ChaptersFolderItem:
        case CharactersFolderItem:
        case ResearchFolderItem:
        case CorkboardFolderItem:
            menu = m_folderMenu;
            break;
        case ChapterItem:
            menu = m_chapterMenu;
            break;
        case SubsectionItem:
            menu = m_subsectionMenu;
            break;
        case CharacterItem:
            menu = m_characterMenu;
            break;
        case ResearchItem:
            menu = m_researchMenu;
            break;
    }
    
    if (menu) {
        menu->exec(event->globalPos());
    }
}

void ProjectTreeWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    
    if (!item) {
        return;
    }
    
    QString filePath = item->data(0, Qt::UserRole).toString();
    
    switch (item->type()) {
        case ChapterItem:
            emit itemOpenRequested(filePath);
            break;
        case SubsectionItem:
            if (item->parent() && item->parent()->type() == ChapterItem) {
                QString chapterPath = item->parent()->data(0, Qt::UserRole).toString();
                QString subsectionTitle = item->text(0);
                emit itemOpenRequested(chapterPath, subsectionTitle);
            }
            break;
        case CharacterItem:
        case ResearchItem:
            emit itemOpenRequested(filePath);
            break;
    }
}

void ProjectTreeWidget::onItemSelectionChanged()
{
    QTreeWidgetItem* current = currentItem();
    bool hasSelection = (current != nullptr);
    
    m_renameAction->setEnabled(hasSelection);
    m_deleteAction->setEnabled(hasSelection);
    
    if (current) {
        bool canMoveUp = (current->parent() && 
                         current->parent()->indexOfChild(current) > 0);
        bool canMoveDown = (current->parent() && 
                           current->parent()->indexOfChild(current) < current->parent()->childCount() - 1);
        
        m_moveUpAction->setEnabled(canMoveUp);
        m_moveDownAction->setEnabled(canMoveDown);
    }
}

void ProjectTreeWidget::onItemChanged(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(item)
    Q_UNUSED(column)
    // Using dialog-based renaming instead of inline editing
}

void ProjectTreeWidget::setupContextMenus()
{
    // Create actions
    m_newChapterAction = new QAction("New Chapter", this);
    connect(m_newChapterAction, &QAction::triggered, this, &ProjectTreeWidget::createNewChapter);
    
    m_newSubsectionAction = new QAction("New Subsection", this);
    connect(m_newSubsectionAction, &QAction::triggered, this, &ProjectTreeWidget::createNewSubsection);
    
    m_newCharacterAction = new QAction("New Character", this);
    connect(m_newCharacterAction, &QAction::triggered, this, &ProjectTreeWidget::createNewCharacter);
    
    m_newResearchAction = new QAction("New Research", this);
    connect(m_newResearchAction, &QAction::triggered, this, &ProjectTreeWidget::createNewResearch);
    
    m_renameAction = new QAction("Rename", this);
    connect(m_renameAction, &QAction::triggered, this, &ProjectTreeWidget::renameItem);
    
    m_deleteAction = new QAction("Delete", this);
    connect(m_deleteAction, &QAction::triggered, this, &ProjectTreeWidget::deleteItem);
    
    m_moveUpAction = new QAction("Move Up", this);
    connect(m_moveUpAction, &QAction::triggered, this, &ProjectTreeWidget::moveItemUp);
    
    m_moveDownAction = new QAction("Move Down", this);
    connect(m_moveDownAction, &QAction::triggered, this, &ProjectTreeWidget::moveItemDown);
    
    // Create menus
    m_projectMenu = new QMenu(this);
    m_projectMenu->addAction(m_newChapterAction);
    m_projectMenu->addAction(m_newCharacterAction);
    m_projectMenu->addAction(m_newResearchAction);
    
    m_folderMenu = new QMenu(this);
    m_folderMenu->addAction(m_newChapterAction);
    m_folderMenu->addAction(m_newCharacterAction);
    m_folderMenu->addAction(m_newResearchAction);
    
    m_chapterMenu = new QMenu(this);
    m_chapterMenu->addAction(m_newSubsectionAction);
    m_chapterMenu->addSeparator();
    m_chapterMenu->addAction(m_renameAction);
    m_chapterMenu->addAction(m_deleteAction);
    m_chapterMenu->addSeparator();
    m_chapterMenu->addAction(m_moveUpAction);
    m_chapterMenu->addAction(m_moveDownAction);
    
    m_subsectionMenu = new QMenu(this);
    m_subsectionMenu->addAction(m_renameAction);
    m_subsectionMenu->addAction(m_deleteAction);
    m_subsectionMenu->addSeparator();
    m_subsectionMenu->addAction(m_moveUpAction);
    m_subsectionMenu->addAction(m_moveDownAction);
    
    m_characterMenu = new QMenu(this);
    m_characterMenu->addAction(m_renameAction);
    m_characterMenu->addAction(m_deleteAction);
    
    m_researchMenu = new QMenu(this);
    m_researchMenu->addAction(m_renameAction);
    m_researchMenu->addAction(m_deleteAction);
}

void ProjectTreeWidget::setupDragDrop()
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropOverwriteMode(false);
    setAutoScroll(true);
    setAutoScrollMargin(16);
}

void ProjectTreeWidget::populateProjectTree(QTreeWidgetItem* projectItem, const QString& projectPath)
{
    // Create main folders
    QTreeWidgetItem* chaptersFolder = new QTreeWidgetItem(projectItem, ChaptersFolderItem);
    chaptersFolder->setText(0, "Chapters");
    chaptersFolder->setExpanded(true);
    
    QTreeWidgetItem* charactersFolder = new QTreeWidgetItem(projectItem, CharactersFolderItem);
    charactersFolder->setText(0, "Characters");
    
    QTreeWidgetItem* researchFolder = new QTreeWidgetItem(projectItem, ResearchFolderItem);
    researchFolder->setText(0, "Research");
    
    QTreeWidgetItem* corkboardFolder = new QTreeWidgetItem(projectItem, CorkboardFolderItem);
    corkboardFolder->setText(0, "Corkboard");
    
    // Populate folders
    populateChapters(chaptersFolder, projectPath);
    populateCharacters(charactersFolder, projectPath);
    populateResearch(researchFolder, projectPath);
    populateCorkboard(corkboardFolder, projectPath);
}

void ProjectTreeWidget::populateChapters(QTreeWidgetItem* chaptersFolder, const QString& projectPath)
{
    QString chaptersPath = QDir(projectPath).filePath("chapters");
    QDir chaptersDir(chaptersPath);
    
    if (!chaptersDir.exists()) {
        return;
    }
    
    QStringList filters;
    filters << "*.md" << "*.txt";
    QFileInfoList files = chaptersDir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    for (const QFileInfo& fileInfo : files) {
        QTreeWidgetItem* chapterItem = createChapterItem(fileInfo.baseName(), fileInfo.absoluteFilePath());
        chaptersFolder->addChild(chapterItem);
        
        chapterItem->addChild(createSubsectionItem("Beginning", 1));
        chapterItem->addChild(createSubsectionItem("Middle", 2));
        chapterItem->addChild(createSubsectionItem("End", 3));
    }
}

void ProjectTreeWidget::populateCharacters(QTreeWidgetItem* charactersFolder, const QString& projectPath)
{
    Q_UNUSED(projectPath)
    charactersFolder->addChild(createCharacterItem("Main Character"));
    charactersFolder->addChild(createCharacterItem("Supporting Character"));
}

void ProjectTreeWidget::populateResearch(QTreeWidgetItem* researchFolder, const QString& projectPath)
{
    QString researchPath = QDir(projectPath).filePath("research");
    QDir researchDir(researchPath);
    
    if (!researchDir.exists()) {
        return;
    }
    
    QStringList filters;
    filters << "*.md" << "*.txt" << "*.pdf" << "*.doc" << "*.docx";
    QFileInfoList files = researchDir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    for (const QFileInfo& fileInfo : files) {
        QTreeWidgetItem* researchItem = createResearchItem(fileInfo.baseName());
        researchItem->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
        researchFolder->addChild(researchItem);
    }
}

void ProjectTreeWidget::populateCorkboard(QTreeWidgetItem* corkboardFolder, const QString& projectPath)
{
    Q_UNUSED(projectPath)
    
    QTreeWidgetItem* plotIdeas = new QTreeWidgetItem(CorkboardItem);
    plotIdeas->setText(0, "Plot Ideas");
    corkboardFolder->addChild(plotIdeas);
    
    QTreeWidgetItem* sceneCards = new QTreeWidgetItem(CorkboardItem);
    sceneCards->setText(0, "Scene Cards");
    corkboardFolder->addChild(sceneCards);
}

QString ProjectTreeWidget::getItemPath(QTreeWidgetItem* item) const
{
    if (!item) {
        return QString();
    }
    
    return item->data(0, Qt::UserRole).toString();
}

QString ProjectTreeWidget::getProjectPath(QTreeWidgetItem* item) const
{
    if (!item) {
        return QString();
    }
    
    QTreeWidgetItem* current = item;
    while (current && current->type() != ProjectItem) {
        current = current->parent();
    }
    
    if (current) {
        return current->data(0, Qt::UserRole).toString();
    }
    
    return QString();
}

QTreeWidgetItem* ProjectTreeWidget::findProjectItem(const QString& projectPath) const
{
    return m_projectItems.value(projectPath, nullptr);
}

QTreeWidgetItem* ProjectTreeWidget::findChaptersFolder(QTreeWidgetItem* projectItem) const
{
    if (!projectItem) {
        return nullptr;
    }
    
    for (int i = 0; i < projectItem->childCount(); ++i) {
        QTreeWidgetItem* child = projectItem->child(i);
        if (child->type() == ChaptersFolderItem) {
            return child;
        }
    }
    
    return nullptr;
}

bool ProjectTreeWidget::canDropOn(QTreeWidgetItem* target, ItemType sourceType) const
{
    if (!target) {
        return false;
    }
    
    switch (target->type()) {
        case ProjectItem:
            return false;
        case ChaptersFolderItem:
            return (sourceType == ChapterItem);
        case ChapterItem:
            return (sourceType == SubsectionItem || sourceType == ChapterItem);
        case CharactersFolderItem:
            return (sourceType == CharacterItem);
        case ResearchFolderItem:
            return (sourceType == ResearchItem);
        case CorkboardFolderItem:
            return (sourceType == CorkboardItem);
        default:
            return false;
    }
}

void ProjectTreeWidget::updateChapterNumbers(QTreeWidgetItem* chaptersFolder)
{
    if (!chaptersFolder || chaptersFolder->type() != ChaptersFolderItem) {
        return;
    }
    // TODO: Implement chapter renumbering logic
}

void ProjectTreeWidget::saveTreeState()
{
    // TODO: Save expansion state and project list to settings
}

void ProjectTreeWidget::restoreTreeState()
{
    // TODO: Restore expansion state and project list from settings
}

void ProjectTreeWidget::createNewChapter()
{
    if (!m_currentContextItem) {
        return;
    }
    
    QString projectPath = getProjectPath(m_currentContextItem);
    if (projectPath.isEmpty()) {
        return;
    }
    
    bool ok;
    QString chapterName = QInputDialog::getText(this, "New Chapter", 
                                               "Chapter name:", QLineEdit::Normal, 
                                               "", &ok);
    
    if (ok && !chapterName.isEmpty()) {
        emit chapterCreated(projectPath, chapterName);
    }
}

void ProjectTreeWidget::createNewSubsection()
{
    if (!m_currentContextItem || m_currentContextItem->type() != ChapterItem) {
        return;
    }
    
    QString chapterPath = getItemPath(m_currentContextItem);
    if (chapterPath.isEmpty()) {
        return;
    }
    
    bool ok;
    QString subsectionTitle = QInputDialog::getText(this, "New Subsection", 
                                                   "Subsection title (optional):", 
                                                   QLineEdit::Normal, "", &ok);
    
    if (ok) {
        emit subsectionCreated(chapterPath, subsectionTitle);
    }
}

void ProjectTreeWidget::createNewCharacter()
{
    if (!m_currentContextItem) {
        return;
    }
    
    QString projectPath = getProjectPath(m_currentContextItem);
    if (projectPath.isEmpty()) {
        return;
    }
    
    bool ok;
    QString characterName = QInputDialog::getText(this, "New Character", 
                                                 "Character name:", QLineEdit::Normal, 
                                                 "", &ok);
    
    if (ok && !characterName.isEmpty()) {
        emit characterCreated(projectPath, characterName);
    }
}

void ProjectTreeWidget::createNewResearch()
{
    // TODO: Implement research item creation
}

void ProjectTreeWidget::renameItem()
{
    if (!m_currentContextItem) {
        return;
    }
    
    // Only allow renaming of certain item types
    ItemType itemType = static_cast<ItemType>(m_currentContextItem->type());
    if (itemType == ProjectItem || itemType == ChaptersFolderItem || 
        itemType == CharactersFolderItem || itemType == ResearchFolderItem || 
        itemType == CorkboardFolderItem) {
        return;
    }
    
    QString currentName = m_currentContextItem->text(0);
    QString itemTypeName;
    
    switch (itemType) {
        case ChapterItem:
            itemTypeName = "Chapter";
            break;
        case SubsectionItem:
            itemTypeName = "Subsection";
            break;
        case CharacterItem:
            itemTypeName = "Character";
            break;
        case ResearchItem:
            itemTypeName = "Research Item";
            break;
        default:
            itemTypeName = "Item";
            break;
    }
    
    bool ok;
    QString newName = QInputDialog::getText(this, 
                                           QString("Rename %1").arg(itemTypeName),
                                           QString("Enter new name for %1:").arg(itemTypeName),
                                           QLineEdit::Normal,
                                           currentName,
                                           &ok);
    
    if (!ok || newName.trimmed().isEmpty()) {
        return;
    }
    
    newName = newName.trimmed();
    
    if (newName == currentName) {
        return;
    }
    
    if (!validateItemName(m_currentContextItem, newName)) {
        QString suggestion = suggestAlternativeName(m_currentContextItem, newName);
        
        int ret = QMessageBox::question(this, "Name Conflict", 
            QString("The name '%1' already exists. Use '%2' instead?")
            .arg(newName).arg(suggestion),
            QMessageBox::Yes | QMessageBox::No);
        
        if (ret != QMessageBox::Yes) {
            return;
        }
        
        newName = suggestion;
    }
    
    m_currentContextItem->setText(0, newName);
    
    QString filePath = getItemPath(m_currentContextItem);
    emit itemRenamed(currentName, newName, itemType, filePath);
}

bool ProjectTreeWidget::validateItemName(QTreeWidgetItem* item, const QString& newName)
{
    if (newName.isEmpty()) {
        return false;
    }
    
    QTreeWidgetItem* parent = item->parent();
    if (!parent) {
        parent = invisibleRootItem();
    }
    
    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem* sibling = parent->child(i);
        if (sibling != item && sibling->text(0).trimmed() == newName) {
            return false;
        }
    }
    
    return true;
}

QString ProjectTreeWidget::suggestAlternativeName(QTreeWidgetItem* item, const QString& baseName)
{
    QString cleanBase = baseName.trimmed();
    if (cleanBase.isEmpty()) {
        cleanBase = "Untitled";
    }
    
    for (int i = 2; i <= 100; ++i) {
        QString suggestion = QString("%1 (%2)").arg(cleanBase).arg(i);
        if (validateItemName(item, suggestion)) {
            return suggestion;
        }
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("hhmmss");
    return QString("%1_%2").arg(cleanBase).arg(timestamp);
}

void ProjectTreeWidget::deleteItem()
{
    if (!m_currentContextItem) {
        return;
    }
    
    QString itemName = m_currentContextItem->text(0);
    int ret = QMessageBox::question(this, "Delete Item", 
        QString("Are you sure you want to delete '%1'?").arg(itemName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        QString itemPath = getItemPath(m_currentContextItem);
        ItemType itemType = static_cast<ItemType>(m_currentContextItem->type());
        
        emit itemDeleted(itemPath, itemType);
        
        delete m_currentContextItem;
        m_currentContextItem = nullptr;
    }
}

void ProjectTreeWidget::moveItemUp()
{
    if (!m_currentContextItem || !m_currentContextItem->parent()) {
        return;
    }
    
    QTreeWidgetItem* parent = m_currentContextItem->parent();
    int currentIndex = parent->indexOfChild(m_currentContextItem);
    
    if (currentIndex > 0) {
        QTreeWidgetItem* item = parent->takeChild(currentIndex);
        parent->insertChild(currentIndex - 1, item);
        setCurrentItem(item);
        
        if (parent->type() == ChaptersFolderItem) {
            updateChapterNumbers(parent);
        }
    }
}

void ProjectTreeWidget::moveItemDown()
{
    if (!m_currentContextItem || !m_currentContextItem->parent()) {
        return;
    }
    
    QTreeWidgetItem* parent = m_currentContextItem->parent();
    int currentIndex = parent->indexOfChild(m_currentContextItem);
    
    if (currentIndex < parent->childCount() - 1) {
        QTreeWidgetItem* item = parent->takeChild(currentIndex);
        parent->insertChild(currentIndex + 1, item);
        setCurrentItem(item);
        
        if (parent->type() == ChaptersFolderItem) {
            updateChapterNumbers(parent);
        }
    }
}

void ProjectTreeWidget::dropEvent(QDropEvent* event)
{
    QTreeWidgetItem* draggedItem = currentItem();
    if (!draggedItem) {
        event->ignore();
        return;
    }
    
    QTreeWidgetItem* originalParent = draggedItem->parent();
    int originalIndex = originalParent ? originalParent->indexOfChild(draggedItem) : indexOfTopLevelItem(draggedItem);
    
    QTreeWidget::dropEvent(event);
    
    QTreeWidgetItem* newParent = draggedItem->parent();
    int newIndex = newParent ? newParent->indexOfChild(draggedItem) : indexOfTopLevelItem(draggedItem);
    
    if (draggedItem->type() == ChapterItem && 
        newParent && newParent->type() == ChaptersFolderItem &&
        originalParent == newParent && originalIndex != newIndex) {
        
        emit itemMoved(QString::number(originalIndex), QString::number(newIndex), ChapterItem);
    }
    
    event->accept();
}

void ProjectTreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeWidgetItem* item = itemAt(event->position().toPoint());
    
    if (item) {
        setDropIndicatorShown(true);
        
        if (item->type() == ChaptersFolderItem || item->type() == ChapterItem) {
            event->acceptProposedAction();
            update();
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }
    
    QTreeWidget::dragMoveEvent(event);
}

void ProjectTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        setDropIndicatorShown(true);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ProjectTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    if (!m_dragDropEnabled) {
        return;
    }
    
    QTreeWidget::startDrag(supportedActions);
}