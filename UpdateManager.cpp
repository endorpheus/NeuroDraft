/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "UpdateManager.h"
#include "ProjectTreeWidget.h"
#include "ProjectManager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

// Initialize static constants
const QString UpdateManager::BACKUP_SUFFIX = ".neurodraft_backup";
const int UpdateManager::MAX_BACKUPS = 5;
const QRegularExpression UpdateManager::CHAPTER_REGEX(R"(^#\s+(.+)$)", QRegularExpression::MultilineOption);
const QRegularExpression UpdateManager::SUBSECTION_REGEX(R"(^##\s+(.+)$)", QRegularExpression::MultilineOption);

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
    , m_projectTree(nullptr)
    , m_projectManager(nullptr)
{
}

UpdateManager::~UpdateManager() = default;

void UpdateManager::setProjectTree(ProjectTreeWidget* tree)
{
    m_projectTree = tree;
}

void UpdateManager::setProjectManager(ProjectManager* manager)
{
    m_projectManager = manager;
}

bool UpdateManager::renumberChapters(const QString& projectPath)
{
    if (projectPath.isEmpty()) {
        emit updateError("Project path is empty");
        return false;
    }
    
    qDebug() << "Renumbering chapters for project:" << projectPath;
    
    // Analyze current project structure
    analyzeProject(projectPath);
    
    QList<ChapterInfo>& chapters = m_projectChapters[projectPath];
    if (chapters.isEmpty()) {
        qDebug() << "No chapters found to renumber";
        return true;
    }
    
    // Create backup before making changes
    for (const ChapterInfo& chapter : chapters) {
        if (!createBackup(chapter.filePath)) {
            emit updateError("Failed to create backup for: " + chapter.filePath);
            return false;
        }
    }
    
    // Renumber chapters and update files
    for (int i = 0; i < chapters.size(); ++i) {
        ChapterInfo& chapter = chapters[i];
        int newChapterNumber = i + 1;
        
        if (chapter.chapterNumber != newChapterNumber) {
            qDebug() << "Renumbering chapter" << chapter.chapterNumber << "to" << newChapterNumber;
            
            // Update chapter number
            int oldChapterNumber = chapter.chapterNumber;
            chapter.chapterNumber = newChapterNumber;
            
            // Generate new file name
            QString newFileName = generateChapterFileName(newChapterNumber, chapter.name);
            QString newFilePath = QDir(projectPath).filePath("chapters/" + newFileName);
            
            // Rename file if necessary
            if (chapter.filePath != newFilePath) {
                if (!renameProjectFile(chapter.filePath, newFilePath)) {
                    emit updateError("Failed to rename chapter file: " + chapter.fileName);
                    return false;
                }
                chapter.filePath = newFilePath;
                chapter.fileName = newFileName;
            }
            
            // Update chapter content and subsection numbering
            if (!updateChapterFile(chapter.filePath, chapter)) {
                emit updateError("Failed to update chapter content: " + chapter.fileName);
                return false;
            }
            
            // Renumber all subsections in this chapter
            if (!renumberSubsections(projectPath, newChapterNumber)) {
                emit updateError("Failed to renumber subsections for chapter: " + QString::number(newChapterNumber));
                return false;
            }
            
            emit chapterMoved(projectPath, oldChapterNumber - 1, newChapterNumber - 1);
        }
    }
    
    emit numberingUpdated(projectPath);
    qDebug() << "Chapter renumbering completed successfully";
    return true;
}

bool UpdateManager::renumberSubsections(const QString& projectPath, int chapterNumber)
{
    qDebug() << "Renumbering subsections for chapter" << chapterNumber;
    
    const QList<ChapterInfo>& chapters = m_projectChapters[projectPath];
    
    // Find the chapter
    auto chapterIt = std::find_if(chapters.begin(), chapters.end(),
        [chapterNumber](const ChapterInfo& ch) { return ch.chapterNumber == chapterNumber; });
    
    if (chapterIt == chapters.end()) {
        emit updateError("Chapter not found: " + QString::number(chapterNumber));
        return false;
    }
    
    const ChapterInfo& chapter = *chapterIt;
    
    // Parse existing subsections
    QList<SubsectionInfo> subsections = parseSubsections(chapter.filePath, chapterNumber);
    
    // Renumber subsections
    for (int i = 0; i < subsections.size(); ++i) {
        subsections[i].subsectionNumber = i + 1;
        subsections[i].anchor = generateSubsectionAnchor(chapterNumber, i + 1, subsections[i].title);
    }
    
    // Update the file with new subsection numbering
    if (!updateSubsectionsInFile(chapter.filePath, subsections)) {
        emit updateError("Failed to update subsections in file: " + chapter.fileName);
        return false;
    }
    
    qDebug() << "Subsection renumbering completed for chapter" << chapterNumber;
    return true;
}

bool UpdateManager::moveChapter(const QString& projectPath, int fromIndex, int toIndex)
{
    qDebug() << "Moving chapter from index" << fromIndex << "to" << toIndex;
    
    QList<ChapterInfo>& chapters = m_projectChapters[projectPath];
    
    if (fromIndex < 0 || fromIndex >= chapters.size() || toIndex < 0 || toIndex >= chapters.size()) {
        emit updateError("Invalid chapter indices for move operation");
        return false;
    }
    
    if (fromIndex == toIndex) {
        return true; // No change needed
    }
    
    // Move the chapter in our list
    ChapterInfo chapter = chapters.takeAt(fromIndex);
    chapters.insert(toIndex, chapter);
    
    // Renumber all chapters to reflect new order
    if (!renumberChapters(projectPath)) {
        emit updateError("Failed to renumber chapters after move");
        return false;
    }
    
    emit chapterMoved(projectPath, fromIndex, toIndex);
    return true;
}

bool UpdateManager::isNameValid(const QString& projectPath, const QString& name, const QString& type) const
{
    if (name.trimmed().isEmpty()) {
        return false;
    }
    
    QStringList existingNames = getExistingNames(projectPath, type);
    QString normalizedName = normalizeName(name);
    
    for (const QString& existing : existingNames) {
        if (normalizeName(existing) == normalizedName) {
            return false;
        }
    }
    
    return true;
}

QString UpdateManager::suggestAlternativeName(const QString& projectPath, const QString& baseName, const QString& type) const
{
    QString suggestion = baseName.trimmed();
    if (suggestion.isEmpty()) {
        suggestion = "Untitled";
    }
    
    if (isNameValid(projectPath, suggestion, type)) {
        return suggestion;
    }
    
    // Try numbered variations
    for (int i = 2; i <= 100; ++i) {
        QString numberedName = QString("%1 (%2)").arg(suggestion).arg(i);
        if (isNameValid(projectPath, numberedName, type)) {
            return numberedName;
        }
    }
    
    // Fallback with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_%2").arg(suggestion).arg(timestamp);
}

QStringList UpdateManager::getExistingNames(const QString& projectPath, const QString& type) const
{
    QStringList names;
    
    if (type == "chapter") {
        const QList<ChapterInfo>& chapters = m_projectChapters.value(projectPath);
        for (const ChapterInfo& chapter : chapters) {
            names.append(chapter.name);
        }
    }
    // Add other types as needed (subsections, characters, etc.)
    
    return names;
}

void UpdateManager::analyzeProject(const QString& projectPath)
{
    qDebug() << "Analyzing project structure:" << projectPath;
    
    QList<ChapterInfo>& chapters = m_projectChapters[projectPath];
    chapters.clear();
    
    QString chaptersPath = QDir(projectPath).filePath("chapters");
    QDir chaptersDir(chaptersPath);
    
    if (!chaptersDir.exists()) {
        qDebug() << "Chapters directory does not exist";
        return;
    }
    
    // Get all chapter files
    QStringList filters;
    filters << "*.md" << "*.txt";
    QFileInfoList files = chaptersDir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    for (const QFileInfo& fileInfo : files) {
        ChapterInfo chapter = parseChapterFile(fileInfo.absoluteFilePath());
        if (!chapter.name.isEmpty()) {
            chapters.append(chapter);
        }
    }
    
    // Sort chapters by number
    std::sort(chapters.begin(), chapters.end(),
        [](const ChapterInfo& a, const ChapterInfo& b) {
            return a.chapterNumber < b.chapterNumber;
        });
    
    qDebug() << "Found" << chapters.size() << "chapters";
}

ChapterInfo UpdateManager::parseChapterFile(const QString& filePath) const
{
    ChapterInfo info;
    info.filePath = filePath;
    info.fileName = QFileInfo(filePath).fileName();
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << filePath;
        return info;
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    
    // Extract chapter number from filename (chapter_01.md -> 1)
    QRegularExpression filenameRegex(R"(chapter_(\d+)\.)");
    QRegularExpressionMatch match = filenameRegex.match(info.fileName);
    if (match.hasMatch()) {
        info.chapterNumber = match.captured(1).toInt();
    } else {
        info.chapterNumber = 1; // Default
    }
    
    // Extract chapter title from first # header
    QRegularExpressionMatch titleMatch = CHAPTER_REGEX.match(content);
    if (titleMatch.hasMatch()) {
        info.name = titleMatch.captured(1).trimmed();
    } else {
        info.name = QFileInfo(filePath).baseName();
    }
    
    // Parse subsections
    QRegularExpressionMatchIterator subsectionMatches = SUBSECTION_REGEX.globalMatch(content);
    while (subsectionMatches.hasNext()) {
        QRegularExpressionMatch subsectionMatch = subsectionMatches.next();
        info.subsections.append(subsectionMatch.captured(1).trimmed());
    }
    
    return info;
}

QList<SubsectionInfo> UpdateManager::parseSubsections(const QString& filePath, int chapterNumber) const
{
    QList<SubsectionInfo> subsections;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return subsections;
    }
    
    QTextStream in(&file);
    QStringList lines = in.readAll().split('\n');
    
    for (int lineNum = 0; lineNum < lines.size(); ++lineNum) {
        const QString& line = lines[lineNum];
        QRegularExpressionMatch match = SUBSECTION_REGEX.match(line);
        
        if (match.hasMatch()) {
            SubsectionInfo subsection;
            subsection.title = match.captured(1).trimmed();
            subsection.chapterNumber = chapterNumber;
            subsection.subsectionNumber = subsections.size() + 1;
            subsection.lineNumber = lineNum;
            subsection.anchor = generateSubsectionAnchor(chapterNumber, subsection.subsectionNumber, subsection.title);
            
            subsections.append(subsection);
        }
    }
    
    return subsections;
}

QString UpdateManager::generateChapterFileName(int chapterNumber, const QString& chapterName) const
{
    Q_UNUSED(chapterName) // For now, use standard numbering
    return QString("chapter_%1.md").arg(chapterNumber, 2, 10, QChar('0'));
}

QString UpdateManager::generateSubsectionAnchor(int chapterNumber, int subsectionNumber, const QString& title) const
{
    QString cleanTitle = title.toLower();
    cleanTitle.replace(QRegularExpression("[^a-z0-9]+"), "-");
    cleanTitle.remove(QRegularExpression("^-+|-+$"));
    
    return QString("%1-%2-%3").arg(chapterNumber).arg(subsectionNumber).arg(cleanTitle);
}

bool UpdateManager::createBackup(const QString& filePath) const
{
    QString backupPath = filePath + BACKUP_SUFFIX;
    
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    
    return QFile::copy(filePath, backupPath);
}

bool UpdateManager::updateChapterFile(const QString& filePath, const ChapterInfo& info) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QString content = file.readAll();
    file.close();
    
    // Update chapter header
    QString newHeader = QString("# Chapter %1: %2").arg(info.chapterNumber).arg(info.name);
    content.replace(CHAPTER_REGEX, newHeader);
    
    // Write back to file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out << content;
    
    return true;
}

bool UpdateManager::updateSubsectionsInFile(const QString& filePath, const QList<SubsectionInfo>& subsections) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    QStringList lines = content.split('\n');
    file.close();
    
    // Update subsection headers
    int subsectionIndex = 0;
    for (int i = 0; i < lines.size() && subsectionIndex < subsections.size(); ++i) {
        QRegularExpressionMatch match = SUBSECTION_REGEX.match(lines[i]);
        if (match.hasMatch()) {
            const SubsectionInfo& subsection = subsections[subsectionIndex];
            lines[i] = QString("## %1.%2: %3")
                      .arg(subsection.chapterNumber)
                      .arg(subsection.subsectionNumber)
                      .arg(subsection.title);
            subsectionIndex++;
        }
    }
    
    // Write back to file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out << lines.join('\n');
    
    return true;
}

bool UpdateManager::renameProjectFile(const QString& oldPath, const QString& newPath)
{
    if (oldPath == newPath) {
        return true;
    }
    
    if (QFile::exists(newPath)) {
        emit updateError("Target file already exists: " + newPath);
        return false;
    }
    
    return QFile::rename(oldPath, newPath);
}

QString UpdateManager::normalizeName(const QString& name) const
{
    return name.trimmed().toLower();
}