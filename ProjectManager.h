/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QStringList>

class ProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectManager(QObject *parent = nullptr);
    ~ProjectManager();

    // Project operations
    bool createProject(const QString& projectPath, const QString& projectName);
    bool openProject(const QString& projectFilePath);
    bool saveProject();
    bool closeProject();
    
    // Project validation
    bool isValidProject(const QString& projectPath) const;
    bool projectExists(const QString& projectPath) const;
    
    // Project information
    QString getCurrentProjectPath() const { return m_currentProjectPath; }
    QString getCurrentProjectName() const { return m_currentProjectName; }
    QJsonObject getProjectMetadata() const { return m_projectMetadata; }
    
    // Project structure
    QStringList getChapterList() const;
    QStringList getCharacterList() const;
    
    // Word count targets
    void setChapterWordTarget(const QString& chapter, int target);
    int getChapterWordTarget(const QString& chapter) const;
    void setProjectWordTarget(int target);
    int getProjectWordTarget() const;
    
    // Global hashtags
    QStringList getAllHashtags() const;
    void addHashtag(const QString& hashtag);
    void removeHashtag(const QString& hashtag);

signals:
    void projectOpened(const QString& projectName);
    void projectClosed();
    void projectModified();
    void chapterAdded(const QString& chapterName);
    void chapterRemoved(const QString& chapterName);

private:
    void createProjectStructure(const QString& projectPath);
    void createDefaultProjectFile(const QString& projectPath, const QString& projectName);
    bool loadProjectMetadata(const QString& projectFilePath);
    bool saveProjectMetadata();
    void initializeHashtagIndex();
    void saveHashtagIndex();
    
    QString m_currentProjectPath;
    QString m_currentProjectName;
    QJsonObject m_projectMetadata;
    QStringList m_globalHashtags;
    bool m_projectModified;
    
    // Project structure paths
    QString m_chaptersPath;
    QString m_charactersPath;
    QString m_researchPath;
    QString m_corkboardPath;
    QString m_hashtagsPath;
};

#endif // PROJECTMANAGER_H