/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "AutoSaveManager.h"
#include "EditorWidget.h"
#include <QDebug>
#include <QStandardPaths>

AutoSaveManager::AutoSaveManager(QObject *parent)
    : QObject(parent)
    , m_autoSaveTimer(new QTimer(this))
    , m_typingPauseTimer(new QTimer(this))
    , m_intervalSeconds(DEFAULT_INTERVAL)
    , m_typingPauseSeconds(TYPING_PAUSE_INTERVAL)
    , m_enabled(true)
{
    // Setup regular auto-save timer (fallback)
    m_autoSaveTimer->setSingleShot(false);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &AutoSaveManager::performAutoSave);
    
    // Setup typing pause detection timer
    m_typingPauseTimer->setSingleShot(true);
    connect(m_typingPauseTimer, &QTimer::timeout, this, &AutoSaveManager::onTypingPaused);
    
    // Load settings
    loadSettings();
    
    // Start regular timer if enabled (as backup)
    if (m_enabled) {
        m_autoSaveTimer->start(m_intervalSeconds * 1000);
    }
}

AutoSaveManager::~AutoSaveManager()
{
    saveSettings();
}

void AutoSaveManager::setAutoSaveInterval(int seconds)
{
    if (seconds < MIN_INTERVAL || seconds > MAX_INTERVAL) {
        qWarning() << "Auto-save interval out of range:" << seconds;
        return;
    }
    
    m_intervalSeconds = seconds;
    
    if (m_enabled) {
        m_autoSaveTimer->stop();
        m_autoSaveTimer->start(m_intervalSeconds * 1000);
    }
    
    saveSettings();
    emit statusChanged(QString("Auto-save interval set to %1 seconds").arg(seconds));
}

int AutoSaveManager::getAutoSaveInterval() const
{
    return m_intervalSeconds;
}

void AutoSaveManager::setTypingPauseInterval(int seconds)
{
    if (seconds < MIN_TYPING_PAUSE || seconds > MAX_TYPING_PAUSE) {
        qWarning() << "Typing pause interval out of range:" << seconds;
        return;
    }
    
    m_typingPauseSeconds = seconds;
    saveSettings();
    emit statusChanged(QString("Typing pause auto-save set to %1 seconds").arg(seconds));
}

int AutoSaveManager::getTypingPauseInterval() const
{
    return m_typingPauseSeconds;
}

void AutoSaveManager::setEnabled(bool enabled)
{
    m_enabled = enabled;
    
    if (enabled) {
        m_autoSaveTimer->start(m_intervalSeconds * 1000);
        emit statusChanged("Auto-save enabled");
    } else {
        m_autoSaveTimer->stop();
        m_typingPauseTimer->stop();
        emit statusChanged("Auto-save disabled");
    }
    
    saveSettings();
}

bool AutoSaveManager::isEnabled() const
{
    return m_enabled;
}

void AutoSaveManager::registerEditor(EditorWidget* editor, const QString& filePath)
{
    if (!editor) {
        return;
    }
    
    EditorInfo info;
    info.editor = editor;
    info.filePath = filePath;
    info.lastSaved = QDateTime::currentDateTime();
    info.hasUnsavedChanges = false;
    
    m_trackedEditors[editor] = info;
    
    // Connect to editor signals for typing detection
    connect(editor, &EditorWidget::contentChanged, this, &AutoSaveManager::onEditorModified);
    connect(editor, &QObject::destroyed, this, &AutoSaveManager::onEditorDestroyed);
    
    qDebug() << "Registered editor for auto-save:" << filePath;
}

void AutoSaveManager::unregisterEditor(EditorWidget* editor)
{
    if (m_trackedEditors.contains(editor)) {
        disconnect(editor, nullptr, this, nullptr);
        m_trackedEditors.remove(editor);
        qDebug() << "Unregistered editor from auto-save";
    }
}

void AutoSaveManager::updateFilePath(EditorWidget* editor, const QString& newPath)
{
    if (m_trackedEditors.contains(editor)) {
        m_trackedEditors[editor].filePath = newPath;
        qDebug() << "Updated editor file path:" << newPath;
    }
}

void AutoSaveManager::saveAll()
{
    int savedCount = 0;
    
    for (auto it = m_trackedEditors.begin(); it != m_trackedEditors.end(); ++it) {
        if (needsSaving(it.key())) {
            if (saveEditor(it.key())) {
                savedCount++;
            }
        }
    }
    
    if (savedCount > 0) {
        m_lastAutoSave = QDateTime::currentDateTime();
        emit autoSaveCompleted(savedCount);
        emit statusChanged(QString("Auto-saved %1 files after typing pause").arg(savedCount));
        qDebug() << "Auto-saved" << savedCount << "files after typing pause";
    }
}

void AutoSaveManager::saveAllOnExit()
{
    qDebug() << "Saving all files on exit...";
    
    int savedCount = 0;
    int totalEditors = m_trackedEditors.size();
    
    // Save all tracked editors, regardless of whether they have changes
    // This ensures no data is lost on exit
    for (auto it = m_trackedEditors.begin(); it != m_trackedEditors.end(); ++it) {
        EditorWidget* editor = it.key();
        if (editor && saveEditor(editor)) {
            savedCount++;
        }
    }
    
    qDebug() << "Exit save completed:" << savedCount << "of" << totalEditors << "files saved";
    
    if (savedCount > 0) {
        emit statusChanged(QString("Exit: Saved %1 files").arg(savedCount));
    }
}

bool AutoSaveManager::saveEditor(EditorWidget* editor)
{
    if (!editor || !m_trackedEditors.contains(editor)) {
        return false;
    }
    
    const EditorInfo& info = m_trackedEditors[editor];
    
    if (editor->saveToFile(info.filePath)) {
        markAsSaved(editor);
        return true;
    } else {
        emit autoSaveFailed(info.filePath, "Failed to save file");
        return false;
    }
}

QDateTime AutoSaveManager::getLastAutoSave() const
{
    return m_lastAutoSave;
}

int AutoSaveManager::getModifiedFileCount() const
{
    int count = 0;
    for (const auto& info : m_trackedEditors) {
        if (info.hasUnsavedChanges) {
            count++;
        }
    }
    return count;
}

QStringList AutoSaveManager::getModifiedFiles() const
{
    QStringList files;
    for (const auto& info : m_trackedEditors) {
        if (info.hasUnsavedChanges) {
            files.append(info.filePath);
        }
    }
    return files;
}

void AutoSaveManager::performAutoSave()
{
    if (!m_enabled) {
        return;
    }
    
    // This is the regular interval-based auto-save (fallback)
    int savedCount = 0;
    for (auto it = m_trackedEditors.begin(); it != m_trackedEditors.end(); ++it) {
        if (needsSaving(it.key())) {
            if (saveEditor(it.key())) {
                savedCount++;
            }
        }
    }
    
    if (savedCount > 0) {
        m_lastAutoSave = QDateTime::currentDateTime();
        emit autoSaveCompleted(savedCount);
        emit statusChanged(QString("Auto-saved %1 files (interval)").arg(savedCount));
        qDebug() << "Interval auto-save completed:" << savedCount << "files";
    }
}

void AutoSaveManager::onEditorModified()
{
    if (!m_enabled) {
        return;
    }
    
    EditorWidget* editor = qobject_cast<EditorWidget*>(sender());
    if (editor && m_trackedEditors.contains(editor)) {
        m_trackedEditors[editor].hasUnsavedChanges = true;
        
        // Reset the typing pause timer every time the user types
        // This creates the "countdown after stopping typing" behavior
        m_typingPauseTimer->stop();
        m_typingPauseTimer->start(m_typingPauseSeconds * 1000);
        
        qDebug() << "User typing detected, restarting" << m_typingPauseSeconds << "second countdown";
    }
}

void AutoSaveManager::onTypingPaused()
{
    if (!m_enabled) {
        return;
    }
    
    qDebug() << "Typing pause detected, triggering auto-save...";
    
    // Save all files that have unsaved changes
    saveAll();
}

void AutoSaveManager::onEditorDestroyed()
{
    EditorWidget* editor = qobject_cast<EditorWidget*>(sender());
    if (editor) {
        unregisterEditor(editor);
    }
}

void AutoSaveManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("AutoSave");
    
    m_intervalSeconds = settings.value("interval", DEFAULT_INTERVAL).toInt();
    m_typingPauseSeconds = settings.value("typingPause", TYPING_PAUSE_INTERVAL).toInt();
    m_enabled = settings.value("enabled", true).toBool();
    
    // Validate intervals
    if (m_intervalSeconds < MIN_INTERVAL || m_intervalSeconds > MAX_INTERVAL) {
        m_intervalSeconds = DEFAULT_INTERVAL;
    }
    
    if (m_typingPauseSeconds < MIN_TYPING_PAUSE || m_typingPauseSeconds > MAX_TYPING_PAUSE) {
        m_typingPauseSeconds = TYPING_PAUSE_INTERVAL;
    }
    
    settings.endGroup();
}

void AutoSaveManager::saveSettings()
{
    QSettings settings;
    settings.beginGroup("AutoSave");
    
    settings.setValue("interval", m_intervalSeconds);
    settings.setValue("typingPause", m_typingPauseSeconds);
    settings.setValue("enabled", m_enabled);
    
    settings.endGroup();
}

bool AutoSaveManager::needsSaving(EditorWidget* editor) const
{
    if (!editor || !m_trackedEditors.contains(editor)) {
        return false;
    }
    
    return m_trackedEditors[editor].hasUnsavedChanges;
}

void AutoSaveManager::markAsSaved(EditorWidget* editor)
{
    if (editor && m_trackedEditors.contains(editor)) {
        m_trackedEditors[editor].hasUnsavedChanges = false;
        m_trackedEditors[editor].lastSaved = QDateTime::currentDateTime();
    }
}