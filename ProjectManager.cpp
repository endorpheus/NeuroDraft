/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "ProjectManager.h"
#include <QFileInfo>
#include <QJsonArray>
#include <QDebug>
#include <QStandardPaths>

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
    , m_projectModified(false)
{
}

ProjectManager::~ProjectManager() = default;

bool ProjectManager::createProject(const QString& projectPath, const QString& projectName)
{
    if (projectPath.isEmpty() || projectName.isEmpty()) {
        return false;
    }
    
    QDir dir(projectPath);
    if (!dir.exists() && !dir.mkpath(projectPath)) {
        qDebug() << "Failed to create project directory:" << projectPath;
        return false;
    }
    
    // Check if project already exists
    if (projectExists(projectPath)) {
        qDebug() << "Project already exists at:" << projectPath;
        return false;
    }
    
    try {
        createProjectStructure(projectPath);
        createDefaultProjectFile(projectPath, projectName);
        
        m_currentProjectPath = projectPath;
        m_currentProjectName = projectName;
        
        // Initialize paths
        m_chaptersPath = QDir(projectPath).filePath("chapters");
        m_charactersPath = QDir(projectPath).filePath("characters");
        m_researchPath = QDir(projectPath).filePath("research");
        m_corkboardPath = QDir(projectPath).filePath("corkboard");
        m_hashtagsPath = QDir(projectPath).filePath(".hashtags");
        
        initializeHashtagIndex();
        
        emit projectOpened(projectName);
        return true;
        
    } catch (const std::exception& e) {
        qDebug() << "Exception creating project:" << e.what();
        return false;
    }
}

bool ProjectManager::openProject(const QString& projectFilePath)
{
    if (!QFileInfo::exists(projectFilePath)) {
        qDebug() << "Project file does not exist:" << projectFilePath;
        return false;
    }
    
    if (!loadProjectMetadata(projectFilePath)) {
        return false;
    }
    
    m_currentProjectPath = QFileInfo(projectFilePath).absolutePath();
    m_currentProjectName = m_projectMetadata["name"].toString();
    
    // Initialize paths
    m_chaptersPath = QDir(m_currentProjectPath).filePath("chapters");
    m_charactersPath = QDir(m_currentProjectPath).filePath("characters");
    m_researchPath = QDir(m_currentProjectPath).filePath("research");
    m_corkboardPath = QDir(m_currentProjectPath).filePath("corkboard");
    m_hashtagsPath = QDir(m_currentProjectPath).filePath(".hashtags");
    
    // Load hashtag index
    QDir hashtagDir(m_hashtagsPath);
    if (hashtagDir.exists("index.json")) {
        QFile hashtagFile(hashtagDir.filePath("index.json"));
        if (hashtagFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(hashtagFile.readAll());
            QJsonArray array = doc.array();
            m_globalHashtags.clear();
            for (const auto& value : array) {
                m_globalHashtags.append(value.toString());
            }
        }
    }
    
    emit projectOpened(m_currentProjectName);
    return true;
}

bool ProjectManager::saveProject()
{
    if (m_currentProjectPath.isEmpty()) {
        return false;
    }
    
    bool success = saveProjectMetadata();
    if (success) {
        saveHashtagIndex();
        m_projectModified = false;
    }
    
    return success;
}

bool ProjectManager::closeProject()
{
    if (m_projectModified) {
        saveProject();
    }
    
    m_currentProjectPath.clear();
    m_currentProjectName.clear();
    m_projectMetadata = QJsonObject();
    m_globalHashtags.clear();
    m_projectModified = false;
    
    emit projectClosed();
    return true;
}

bool ProjectManager::isValidProject(const QString& projectPath) const
{
    QDir dir(projectPath);
    return dir.exists("project.json") && 
           dir.exists("chapters") &&
           dir.exists("characters") &&
           dir.exists("corkboard");
}

bool ProjectManager::projectExists(const QString& projectPath) const
{
    return QDir(projectPath).exists("project.json");
}

QStringList ProjectManager::getChapterList() const
{
    QStringList chapters;
    QDir chaptersDir(m_chaptersPath);
    
    if (chaptersDir.exists()) {
        QStringList filters;
        filters << "*.md" << "*.txt";
        QFileInfoList files = chaptersDir.entryInfoList(filters, QDir::Files, QDir::Name);
        
        for (const QFileInfo& file : files) {
            chapters.append(file.baseName());
        }
    }
    
    return chapters;
}

QStringList ProjectManager::getCharacterList() const
{
    QStringList characters;
    // TODO: Implement character list extraction from characters.json
    return characters;
}

void ProjectManager::setChapterWordTarget(const QString& chapter, int target)
{
    QJsonObject targets = m_projectMetadata["wordTargets"].toObject();
    QJsonObject chapterTargets = targets["chapters"].toObject();
    chapterTargets[chapter] = target;
    targets["chapters"] = chapterTargets;
    m_projectMetadata["wordTargets"] = targets;
    m_projectModified = true;
    emit projectModified();
}

int ProjectManager::getChapterWordTarget(const QString& chapter) const
{
    QJsonObject targets = m_projectMetadata["wordTargets"].toObject();
    QJsonObject chapterTargets = targets["chapters"].toObject();
    return chapterTargets[chapter].toInt();
}

void ProjectManager::setProjectWordTarget(int target)
{
    QJsonObject targets = m_projectMetadata["wordTargets"].toObject();
    targets["project"] = target;
    m_projectMetadata["wordTargets"] = targets;
    m_projectModified = true;
    emit projectModified();
}

int ProjectManager::getProjectWordTarget() const
{
    QJsonObject targets = m_projectMetadata["wordTargets"].toObject();
    return targets["project"].toInt();
}

QStringList ProjectManager::getAllHashtags() const
{
    return m_globalHashtags;
}

void ProjectManager::addHashtag(const QString& hashtag)
{
    if (!m_globalHashtags.contains(hashtag)) {
        m_globalHashtags.append(hashtag);
        m_globalHashtags.sort();
        m_projectModified = true;
        emit projectModified();
    }
}

void ProjectManager::removeHashtag(const QString& hashtag)
{
    if (m_globalHashtags.removeAll(hashtag) > 0) {
        m_projectModified = true;
        emit projectModified();
    }
}

void ProjectManager::createProjectStructure(const QString& projectPath)
{
    QDir dir(projectPath);
    
    // Create subdirectories
    dir.mkpath("chapters");
    dir.mkpath("characters");
    dir.mkpath("research");
    dir.mkpath("corkboard");
    dir.mkpath(".hashtags");
    
    // Create initial chapter file
    QFile chapterFile(dir.filePath("chapters/chapter_01.md"));
    if (chapterFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&chapterFile);
        stream << "# Chapter 1\n\n";
        stream << "Begin your story here...\n";
    }
}

void ProjectManager::createDefaultProjectFile(const QString& projectPath, const QString& projectName)
{
    QJsonObject project;
    project["name"] = projectName;
    project["version"] = "1.0";
    project["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    project["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    project["author"] = "";
    project["description"] = "";
    
    // Word count targets
    QJsonObject wordTargets;
    wordTargets["project"] = 80000;  // Default novel target
    wordTargets["chapters"] = QJsonObject();
    project["wordTargets"] = wordTargets;
    
    // Settings
    QJsonObject settings;
    settings["autoSave"] = true;
    settings["backupCount"] = 5;
    project["settings"] = settings;
    
    m_projectMetadata = project;
    
    QJsonDocument doc(project);
    QFile file(QDir(projectPath).filePath("project.json"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

bool ProjectManager::loadProjectMetadata(const QString& projectFilePath)
{
    QFile file(projectFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open project file:" << projectFilePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Invalid project file format:" << projectFilePath;
        return false;
    }
    
    m_projectMetadata = doc.object();
    return true;
}

bool ProjectManager::saveProjectMetadata()
{
    if (m_currentProjectPath.isEmpty()) {
        return false;
    }
    
    // Update modification time
    m_projectMetadata["modified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(m_projectMetadata);
    QFile file(QDir(m_currentProjectPath).filePath("project.json"));
    
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot save project file";
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

void ProjectManager::initializeHashtagIndex()
{
    m_globalHashtags.clear();
    // Add some default hashtags
    m_globalHashtags << "#character" << "#plot" << "#scene" << "#research" << "#todo";
    saveHashtagIndex();
}

void ProjectManager::saveHashtagIndex()
{
    QJsonArray array;
    for (const QString& tag : m_globalHashtags) {
        array.append(tag);
    }
    
    QJsonDocument doc(array);
    QDir hashtagDir(m_hashtagsPath);
    QFile file(hashtagDir.filePath("index.json"));
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}