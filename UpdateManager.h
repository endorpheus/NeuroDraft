/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QFileInfo>
#include <QTreeWidgetItem>
#include <QRegularExpression>

class ProjectTreeWidget;
class ProjectManager;

struct ChapterInfo {
    QString name;
    QString fileName;
    QString filePath;
    int chapterNumber;
    QStringList subsections;
    QTreeWidgetItem* treeItem;
};

struct SubsectionInfo {
    QString title;
    int chapterNumber;
    int subsectionNumber;
    int lineNumber;  // Line in file where subsection starts
    QString anchor;  // Generated anchor for cross-references
};

class UpdateManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager();

    // Set dependencies
    void setProjectTree(ProjectTreeWidget* tree);
    void setProjectManager(ProjectManager* manager);
    
    // Chapter operations
    bool renumberChapters(const QString& projectPath);
    bool renameChapter(const QString& projectPath, int oldChapterNum, const QString& newName);
    bool moveChapter(const QString& projectPath, int fromIndex, int toIndex);
    
    // Subsection operations
    bool renumberSubsections(const QString& projectPath, int chapterNumber);
    bool moveSubsection(const QString& projectPath, int chapterNum, int fromIndex, int toIndex);
    bool renameSubsection(const QString& projectPath, int chapterNum, int subsectionNum, const QString& newTitle);
    
    // Validation
    bool isNameValid(const QString& projectPath, const QString& name, const QString& type) const;
    QString suggestAlternativeName(const QString& projectPath, const QString& baseName, const QString& type) const;
    QStringList getExistingNames(const QString& projectPath, const QString& type) const;
    
    // File operations
    bool updateFileReferences(const QString& filePath, const QHash<QString, QString>& referenceMap);
    bool renameProjectFile(const QString& oldPath, const QString& newPath);
    
    // Cross-reference tracking
    QStringList findCrossReferences(const QString& projectPath, const QString& targetReference) const;
    bool updateCrossReferences(const QString& projectPath, const QString& oldReference, const QString& newReference);

signals:
    void chapterRenamed(const QString& projectPath, int chapterNumber, const QString& newName);
    void chapterMoved(const QString& projectPath, int fromIndex, int toIndex);
    void subsectionRenamed(const QString& projectPath, int chapterNumber, int subsectionNumber, const QString& newTitle);
    void subsectionMoved(const QString& projectPath, int chapterNumber, int fromIndex, int toIndex);
    void numberingUpdated(const QString& projectPath);
    void updateError(const QString& error);

private:
    // Internal helpers
    void analyzeProject(const QString& projectPath);
    ChapterInfo parseChapterFile(const QString& filePath) const;
    QList<SubsectionInfo> parseSubsections(const QString& filePath, int chapterNumber) const;
    bool updateChapterFile(const QString& filePath, const ChapterInfo& info) const;
    bool updateSubsectionsInFile(const QString& filePath, const QList<SubsectionInfo>& subsections) const;
    
    QString generateChapterFileName(int chapterNumber, const QString& chapterName) const;
    QString generateSubsectionAnchor(int chapterNumber, int subsectionNumber, const QString& title) const;
    
    // Cross-reference utilities
    QRegularExpression getCrossReferenceRegex() const;
    QString normalizeName(const QString& name) const;
    
    // File backup and safety
    bool createBackup(const QString& filePath) const;
    bool restoreBackup(const QString& filePath) const;
    void cleanupBackups(const QString& projectPath) const;
    
    // Data storage
    ProjectTreeWidget* m_projectTree;
    ProjectManager* m_projectManager;
    QHash<QString, QList<ChapterInfo>> m_projectChapters;  // projectPath -> chapters
    QHash<QString, QStringList> m_existingNames;  // projectPath -> names by type
    
    // Constants
    static const QString BACKUP_SUFFIX;
    static const int MAX_BACKUPS;
    static const QRegularExpression CHAPTER_REGEX;
    static const QRegularExpression SUBSECTION_REGEX;
};

#endif // UPDATEMANAGER_H