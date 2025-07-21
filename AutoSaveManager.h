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
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Editor management
    void registerEditor(EditorWidget* editor, const QString& filePath);
    void unregisterEditor(EditorWidget* editor);
    void updateFilePath(EditorWidget* editor, const QString& newPath);
    
    // Manual operations
    void saveAll();
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
    void performAutoSave();
    void onEditorModified();
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
    
    QTimer* m_autoSaveTimer;
    QHash<EditorWidget*, EditorInfo> m_trackedEditors;
    
    // Settings
    int m_intervalSeconds;
    bool m_enabled;
    QDateTime m_lastAutoSave;
    
    // Constants
    static const int DEFAULT_INTERVAL = 300; // 5 minutes
    static const int MIN_INTERVAL = 30;      // 30 seconds
    static const int MAX_INTERVAL = 3600;    // 1 hour
};

#endif // AUTOSAVEMANAGER_H