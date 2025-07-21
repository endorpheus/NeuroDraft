/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef AUTOSAVEMANAGER_H
#define AUTOSAVEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QSettings>

class EditorWidget;

class AutoSaveManager : public QObject
{
    Q_OBJECT

public:
    explicit AutoSaveManager(QObject *parent = nullptr);
    ~AutoSaveManager();

    // Configuration
    void setAutoSaveInterval(int seconds);
    int getAutoSaveInterval() const;
    void setTypingPauseInterval(int seconds);
    int getTypingPauseInterval() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Editor management
    void registerEditor(EditorWidget* editor, const QString& filePath);
    void unregisterEditor(EditorWidget* editor);
    void updateFilePath(EditorWidget* editor, const QString& newPath);
    
    // Manual operations
    void saveAll();
    void saveAllOnExit();
    bool saveEditor(EditorWidget* editor);
    
    // Status
    QDateTime getLastAutoSave() const;
    int getModifiedFileCount() const;
    QStringList getModifiedFiles() const;

signals:
    void autoSaveCompleted(int filesSaved);
    void autoSaveFailed(const QString& filePath, const QString& error);
    void statusChanged(const QString& status);

private slots:
    void performAutoSave();        // Regular interval-based save (fallback)
    void onTypingPaused();         // Triggered when user stops typing
    void onEditorModified();       // Triggered when user types
    void onEditorDestroyed();

private:
    void loadSettings();
    void saveSettings();
    bool needsSaving(EditorWidget* editor) const;
    void markAsSaved(EditorWidget* editor);
    
    struct EditorInfo {
        EditorWidget* editor;
        QString filePath;
        QDateTime lastSaved;
        bool hasUnsavedChanges;
    };
    
    QTimer* m_autoSaveTimer;       // Regular interval timer (fallback)
    QTimer* m_typingPauseTimer;    // Typing pause detection timer
    QHash<EditorWidget*, EditorInfo> m_trackedEditors;
    
    // Settings
    int m_intervalSeconds;         // Regular auto-save interval
    int m_typingPauseSeconds;      // Time to wait after typing stops
    bool m_enabled;
    QDateTime m_lastAutoSave;
    
    // Constants
    static const int DEFAULT_INTERVAL = 300;        // 5 minutes (fallback timer)
    static const int MIN_INTERVAL = 60;             // 1 minute minimum
    static const int MAX_INTERVAL = 3600;           // 1 hour maximum
    
    static const int TYPING_PAUSE_INTERVAL = 10;    // 10 seconds after typing stops
    static const int MIN_TYPING_PAUSE = 5;          // 5 seconds minimum
    static const int MAX_TYPING_PAUSE = 60;         // 1 minute maximum
};

#endif // AUTOSAVEMANAGER_H