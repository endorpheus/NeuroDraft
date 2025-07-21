/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QTextEdit>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QString>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QToolBar>

class EditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWidget(QWidget *parent = nullptr);
    ~EditorWidget();
    
    // Content management
    void setContent(const QString& content);
    QString getContent() const;
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    
    // File operations
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    void setFilePath(const QString& filePath);
    QString getFilePath() const { return m_filePath; }
    
    // Word count and statistics
    int getWordCount() const;
    int getCharacterCount() const;
    int getParagraphCount() const;
    
    // Word count targets
    void setWordTarget(int target);
    int getWordTarget() const { return m_wordTarget; }
    
    // Editor settings
    void setFontSize(int size);
    void setFont(const QFont& font);
    void setLineSpacing(double spacing);
    
    // Rich text formatting
    void setBold(bool bold);
    void setItalic(bool italic);
    void setUnderline(bool underline);
    void setTextColor(const QColor& color);
    void setBackgroundColor(const QColor& color);
    void setAlignment(Qt::Alignment alignment);
    void insertBulletList();
    void insertNumberedList();
    void insertTable(int rows, int columns);
    void insertHorizontalRule();
    
    // Text formatting queries
    bool isBold() const;
    bool isItalic() const;
    bool isUnderline() const;
    QColor textColor() const;
    Qt::Alignment alignment() const;

signals:
    void contentChanged();
    void wordCountChanged(int wordCount);
    void wordSelected(const QString& word);
    void hashtagClicked(const QString& hashtag);
    void formattingChanged();  // New signal for rich text formatting changes

private slots:
    void onTextChanged();
    void updateWordCount();
    void showContextMenu(const QPoint& pos);
    void lookupWord();
    void translateWord();
    void addHashtag();
    void onCursorPositionChanged();  // New slot for rich text formatting updates

private:
    void setupUI();
    void setupEditor();
    void setupStatusBar();
    void setupToolbar();  // New method for rich text toolbar
    void updateStatusBar();
    void updateFormattingButtons();  // Update toolbar button states
    QString getSelectedWord() const;
    QStringList extractHashtags(const QString& text) const;
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QToolBar* m_toolbar;  // Rich text formatting toolbar
    QTextEdit* m_textEditor;
    QHBoxLayout* m_statusLayout;
    QLabel* m_wordCountLabel;
    QLabel* m_characterCountLabel;
    QLabel* m_targetLabel;
    QLabel* m_filePathLabel;
    
    // Toolbar actions
    QAction* m_boldAction;
    QAction* m_italicAction;
    QAction* m_underlineAction;
    QAction* m_textColorAction;
    QAction* m_backgroundColorAction;
    QAction* m_alignLeftAction;
    QAction* m_alignCenterAction;
    QAction* m_alignRightAction;
    QAction* m_alignJustifyAction;
    QAction* m_bulletListAction;
    QAction* m_numberedListAction;
    QAction* m_insertTableAction;
    QAction* m_insertRuleAction;
    
    // Context menu
    QMenu* m_contextMenu;
    QAction* m_lookupAction;
    QAction* m_translateAction;
    QAction* m_hashtagAction;
    
    // State
    QString m_filePath;
    bool m_hasUnsavedChanges;
    int m_wordTarget;
    QTimer* m_updateTimer;
    
    // Statistics
    int m_currentWordCount;
    int m_currentCharCount;
    int m_currentParagraphCount;
};

#endif // EDITORWIDGET_H