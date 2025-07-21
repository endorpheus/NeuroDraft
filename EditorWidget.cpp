/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "EditorWidget.h"
#include <QTextCursor>
#include <QTextDocument>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QToolBar>
#include <QColorDialog>
#include <QTextList>
#include <QTextTable>
#include <QTextDocumentFragment>

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_toolbar(nullptr)
    , m_textEditor(nullptr)
    , m_statusLayout(nullptr)
    , m_wordCountLabel(nullptr)
    , m_characterCountLabel(nullptr)
    , m_targetLabel(nullptr)
    , m_filePathLabel(nullptr)
    , m_contextMenu(nullptr)
    , m_lookupAction(nullptr)
    , m_translateAction(nullptr)
    , m_hashtagAction(nullptr)
    , m_hasUnsavedChanges(false)
    , m_wordTarget(0)
    , m_updateTimer(new QTimer(this))
    , m_currentWordCount(0)
    , m_currentCharCount(0)
    , m_currentParagraphCount(0)
{
    setupUI();
    setupEditor();
    setupToolbar();
    setupStatusBar();
    
    // Setup update timer for statistics
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(500); // Update every 500ms after typing stops
    connect(m_updateTimer, &QTimer::timeout, this, &EditorWidget::updateWordCount);
}

EditorWidget::~EditorWidget() = default;

void EditorWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(1);
    
    // Create toolbar first
    m_toolbar = new QToolBar(this);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolbar->setMaximumHeight(32);
    m_mainLayout->addWidget(m_toolbar);
    
    // Create text editor
    m_textEditor = new QTextEdit(this);
    m_mainLayout->addWidget(m_textEditor);
    
    // Set the layout on this widget
    this->setLayout(m_mainLayout);
}

void EditorWidget::setupEditor()
{
    // Configure text editor
    m_textEditor->setAcceptRichText(true);
    m_textEditor->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Set default font
    QFont font("Liberation Serif", 12);
    font.setPointSize(12);
    m_textEditor->setFont(font);
    
    // Enable word wrap
    m_textEditor->setLineWrapMode(QTextEdit::WidgetWidth);
    
    // Connect signals
    connect(m_textEditor, &QTextEdit::textChanged, this, &EditorWidget::onTextChanged);
    connect(m_textEditor, &QTextEdit::cursorPositionChanged, this, &EditorWidget::onCursorPositionChanged);
    connect(m_textEditor, &QTextEdit::customContextMenuRequested, 
            this, &EditorWidget::showContextMenu);
    
    // Create context menu
    m_contextMenu = new QMenu(this);
    
    m_lookupAction = new QAction("Look up word", this);
    connect(m_lookupAction, &QAction::triggered, this, &EditorWidget::lookupWord);
    m_contextMenu->addAction(m_lookupAction);
    
    m_translateAction = new QAction("Translate word", this);
    connect(m_translateAction, &QAction::triggered, this, &EditorWidget::translateWord);
    m_contextMenu->addAction(m_translateAction);
    
    m_contextMenu->addSeparator();
    
    m_hashtagAction = new QAction("Add hashtag", this);
    connect(m_hashtagAction, &QAction::triggered, this, &EditorWidget::addHashtag);
    m_contextMenu->addAction(m_hashtagAction);
}

void EditorWidget::setupToolbar()
{
    // Bold, Italic, Underline
    m_boldAction = new QAction("B", this);
    m_boldAction->setCheckable(true);
    m_boldAction->setShortcut(QKeySequence::Bold);
    m_boldAction->setToolTip("Bold (Ctrl+B)");
    connect(m_boldAction, &QAction::triggered, this, [this](bool checked) { setBold(checked); });
    m_toolbar->addAction(m_boldAction);
    
    m_italicAction = new QAction("I", this);
    m_italicAction->setCheckable(true);
    m_italicAction->setShortcut(QKeySequence::Italic);
    m_italicAction->setToolTip("Italic (Ctrl+I)");
    connect(m_italicAction, &QAction::triggered, this, [this](bool checked) { setItalic(checked); });
    m_toolbar->addAction(m_italicAction);
    
    m_underlineAction = new QAction("U", this);
    m_underlineAction->setCheckable(true);
    m_underlineAction->setShortcut(QKeySequence::Underline);
    m_underlineAction->setToolTip("Underline (Ctrl+U)");
    connect(m_underlineAction, &QAction::triggered, this, [this](bool checked) { setUnderline(checked); });
    m_toolbar->addAction(m_underlineAction);
    
    m_toolbar->addSeparator();
    
    // Text and background colors
    m_textColorAction = new QAction("A", this);
    m_textColorAction->setToolTip("Text Color");
    connect(m_textColorAction, &QAction::triggered, this, [this]() {
        QColor color = QColorDialog::getColor(textColor(), this, "Select Text Color");
        if (color.isValid()) setTextColor(color);
    });
    m_toolbar->addAction(m_textColorAction);
    
    m_backgroundColorAction = new QAction("H", this);
    m_backgroundColorAction->setToolTip("Highlight Color");
    connect(m_backgroundColorAction, &QAction::triggered, this, [this]() {
        QColor color = QColorDialog::getColor(Qt::yellow, this, "Select Highlight Color");
        if (color.isValid()) setBackgroundColor(color);
    });
    m_toolbar->addAction(m_backgroundColorAction);
    
    m_toolbar->addSeparator();
    
    // Alignment
    m_alignLeftAction = new QAction("Left", this);
    m_alignLeftAction->setCheckable(true);
    m_alignLeftAction->setToolTip("Align Left");
    connect(m_alignLeftAction, &QAction::triggered, this, [this]() { setAlignment(Qt::AlignLeft); });
    m_toolbar->addAction(m_alignLeftAction);
    
    m_alignCenterAction = new QAction("Center", this);
    m_alignCenterAction->setCheckable(true);
    m_alignCenterAction->setToolTip("Align Center");
    connect(m_alignCenterAction, &QAction::triggered, this, [this]() { setAlignment(Qt::AlignCenter); });
    m_toolbar->addAction(m_alignCenterAction);
    
    m_alignRightAction = new QAction("Right", this);
    m_alignRightAction->setCheckable(true);
    m_alignRightAction->setToolTip("Align Right");
    connect(m_alignRightAction, &QAction::triggered, this, [this]() { setAlignment(Qt::AlignRight); });
    m_toolbar->addAction(m_alignRightAction);
    
    m_alignJustifyAction = new QAction("Justify", this);
    m_alignJustifyAction->setCheckable(true);
    m_alignJustifyAction->setToolTip("Justify");
    connect(m_alignJustifyAction, &QAction::triggered, this, [this]() { setAlignment(Qt::AlignJustify); });
    m_toolbar->addAction(m_alignJustifyAction);
    
    m_toolbar->addSeparator();
    
    // Lists and tables
    m_bulletListAction = new QAction("• List", this);
    m_bulletListAction->setToolTip("Bullet List");
    connect(m_bulletListAction, &QAction::triggered, this, &EditorWidget::insertBulletList);
    m_toolbar->addAction(m_bulletListAction);
    
    m_numberedListAction = new QAction("1. List", this);
    m_numberedListAction->setToolTip("Numbered List");
    connect(m_numberedListAction, &QAction::triggered, this, &EditorWidget::insertNumberedList);
    m_toolbar->addAction(m_numberedListAction);
    
    m_insertTableAction = new QAction("Table", this);
    m_insertTableAction->setToolTip("Insert Table");
    connect(m_insertTableAction, &QAction::triggered, this, [this]() { insertTable(3, 3); });
    m_toolbar->addAction(m_insertTableAction);
    
    m_insertRuleAction = new QAction("—", this);
    m_insertRuleAction->setToolTip("Horizontal Rule");
    connect(m_insertRuleAction, &QAction::triggered, this, &EditorWidget::insertHorizontalRule);
    m_toolbar->addAction(m_insertRuleAction);
}

void EditorWidget::setupStatusBar()
{
    m_wordCountLabel = new QLabel("Words: 0", this);
    m_characterCountLabel = new QLabel("Characters: 0", this);
    m_targetLabel = new QLabel("Target: Not set", this);
    m_filePathLabel = new QLabel("Untitled", this);
    
    // Improved styling with better visibility
    QString labelStyle = "QLabel { "
                        "padding: 4px 8px; "
                        "border-right: 1px solid #bbb; "
                        "background-color: #f8f8f8; "
                        "color: #333; "
                        "font-size: 11px; "
                        "font-weight: bold; "
                        "}";
    
    QString filePathStyle = "QLabel { "
                           "padding: 4px 8px; "
                           "background-color: #f8f8f8; "
                           "color: #666; "
                           "font-size: 11px; "
                           "font-style: italic; "
                           "}";
    
    m_wordCountLabel->setStyleSheet(labelStyle);
    m_characterCountLabel->setStyleSheet(labelStyle);
    m_targetLabel->setStyleSheet(labelStyle);
    m_filePathLabel->setStyleSheet(filePathStyle);
    
    // Set minimum height for better visibility
    m_wordCountLabel->setMinimumHeight(24);
    m_characterCountLabel->setMinimumHeight(24);
    m_targetLabel->setMinimumHeight(24);
    m_filePathLabel->setMinimumHeight(24);
    
    // Create fresh status layout with better spacing
    m_statusLayout = new QHBoxLayout();
    m_statusLayout->setContentsMargins(0, 0, 0, 0);
    m_statusLayout->setSpacing(0);
    m_statusLayout->addWidget(m_wordCountLabel);
    m_statusLayout->addWidget(m_characterCountLabel);
    m_statusLayout->addWidget(m_targetLabel);
    m_statusLayout->addStretch();
    m_statusLayout->addWidget(m_filePathLabel);
    
    // Create status widget container with improved styling
    QWidget* statusWidget = new QWidget(this);
    statusWidget->setStyleSheet("QWidget { "
                               "background-color: #f0f0f0; "
                               "border-top: 2px solid #ddd; "
                               "border-bottom: 1px solid #ccc; "
                               "}");
    statusWidget->setFixedHeight(28);  // Increased height for better visibility
    statusWidget->setLayout(m_statusLayout);
    
    m_mainLayout->addWidget(statusWidget);
}

void EditorWidget::setContent(const QString& content)
{
    m_textEditor->setPlainText(content);
    m_hasUnsavedChanges = false;
    updateWordCount();
}

QString EditorWidget::getContent() const
{
    return m_textEditor->toPlainText();
}

bool EditorWidget::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open file: " + filePath);
        return false;
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    
    setContent(content);
    setFilePath(filePath);
    
    return true;
}

bool EditorWidget::saveToFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot save file: " + filePath);
        return false;
    }
    
    QTextStream out(&file);
    out << getContent();
    
    m_hasUnsavedChanges = false;
    setFilePath(filePath);
    
    return true;
}

void EditorWidget::setFilePath(const QString& filePath)
{
    m_filePath = filePath;
    
    if (filePath.isEmpty()) {
        m_filePathLabel->setText("Untitled");
    } else {
        QFileInfo info(filePath);
        m_filePathLabel->setText(info.fileName());
    }
}

int EditorWidget::getWordCount() const
{
    QString text = m_textEditor->toPlainText();
    if (text.isEmpty()) {
        return 0;
    }
    
    // Remove extra whitespace and split by whitespace
    text = text.simplified();
    QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    return words.count();
}

int EditorWidget::getCharacterCount() const
{
    return m_textEditor->toPlainText().length();
}

int EditorWidget::getParagraphCount() const
{
    return m_textEditor->document()->blockCount();
}

void EditorWidget::setWordTarget(int target)
{
    m_wordTarget = target;
    updateStatusBar();
}

void EditorWidget::setFontSize(int size)
{
    QFont font = m_textEditor->font();
    font.setPointSize(size);
    m_textEditor->setFont(font);
}

void EditorWidget::setFont(const QFont& font)
{
    m_textEditor->setFont(font);
}

void EditorWidget::setLineSpacing(double spacing)
{
    QTextCursor cursor = m_textEditor->textCursor();
    cursor.select(QTextCursor::Document);
    
    QTextBlockFormat blockFormat = cursor.blockFormat();
    blockFormat.setLineHeight(spacing, QTextBlockFormat::ProportionalHeight);
    cursor.setBlockFormat(blockFormat);
}

// Rich text formatting methods
void EditorWidget::setBold(bool bold)
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    cursor.setCharFormat(format);
    m_textEditor->setTextCursor(cursor);
    m_textEditor->setFocus();
}

void EditorWidget::setItalic(bool italic)
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontItalic(italic);
    cursor.setCharFormat(format);
    m_textEditor->setTextCursor(cursor);
    m_textEditor->setFocus();
}

void EditorWidget::setUnderline(bool underline)
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontUnderline(underline);
    cursor.setCharFormat(format);
    m_textEditor->setTextCursor(cursor);
    m_textEditor->setFocus();
}

void EditorWidget::setTextColor(const QColor& color)
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setForeground(color);
    cursor.setCharFormat(format);
    m_textEditor->setTextCursor(cursor);
    m_textEditor->setFocus();
}

void EditorWidget::setBackgroundColor(const QColor& color)
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setBackground(color);
    cursor.setCharFormat(format);
    m_textEditor->setTextCursor(cursor);
    m_textEditor->setFocus();
}

void EditorWidget::setAlignment(Qt::Alignment alignment)
{
    m_textEditor->setAlignment(alignment);
    updateFormattingButtons();
}

void EditorWidget::insertBulletList()
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextList* list = cursor.currentList();
    
    if (list) {
        // If already in a list, remove from list
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setIndent(0);
        cursor.setBlockFormat(blockFormat);
    } else {
        // Create new bullet list
        QTextListFormat listFormat;
        listFormat.setStyle(QTextListFormat::ListDisc);
        cursor.createList(listFormat);
    }
    
    m_textEditor->setFocus();
}

void EditorWidget::insertNumberedList()
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextList* list = cursor.currentList();
    
    if (list) {
        // If already in a list, remove from list
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setIndent(0);
        cursor.setBlockFormat(blockFormat);
    } else {
        // Create new numbered list
        QTextListFormat listFormat;
        listFormat.setStyle(QTextListFormat::ListDecimal);
        cursor.createList(listFormat);
    }
    
    m_textEditor->setFocus();
}

void EditorWidget::insertTable(int rows, int columns)
{
    QTextCursor cursor = m_textEditor->textCursor();
    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(4);
    tableFormat.setCellSpacing(0);
    
    cursor.insertTable(rows, columns, tableFormat);
    m_textEditor->setFocus();
}

void EditorWidget::insertHorizontalRule()
{
    QTextCursor cursor = m_textEditor->textCursor();
    
    // Insert a horizontal line using dashes
    cursor.insertText("\n" + QString(50, QChar('-')) + "\n");
    m_textEditor->setFocus();
}

// Rich text formatting queries
bool EditorWidget::isBold() const
{
    return m_textEditor->fontWeight() == QFont::Bold;
}

bool EditorWidget::isItalic() const
{
    return m_textEditor->fontItalic();
}

bool EditorWidget::isUnderline() const
{
    return m_textEditor->fontUnderline();
}

QColor EditorWidget::textColor() const
{
    return m_textEditor->textColor();
}

Qt::Alignment EditorWidget::alignment() const
{
    return m_textEditor->alignment();
}

void EditorWidget::onTextChanged()
{
    m_hasUnsavedChanges = true;
    m_updateTimer->start(); // Restart timer for delayed update
    emit contentChanged();
}

void EditorWidget::updateWordCount()
{
    m_currentWordCount = getWordCount();
    m_currentCharCount = getCharacterCount();
    m_currentParagraphCount = getParagraphCount();
    
    updateStatusBar();
    emit wordCountChanged(m_currentWordCount);
}

void EditorWidget::updateStatusBar()
{
    m_wordCountLabel->setText(QString("Words: %1").arg(m_currentWordCount));
    m_characterCountLabel->setText(QString("Characters: %1").arg(m_currentCharCount));
    
    if (m_wordTarget > 0) {
        double percentage = (double)m_currentWordCount / m_wordTarget * 100.0;
        m_targetLabel->setText(QString("Target: %1/%2 (%3%)")
                               .arg(m_currentWordCount)
                               .arg(m_wordTarget)
                               .arg(QString::number(percentage, 'f', 1)));
        
        // Color-code based on progress
        if (percentage >= 100.0) {
            m_targetLabel->setStyleSheet("QLabel { padding: 4px 8px; border-right: 1px solid #bbb; background-color: #f8f8f8; color: #4caf50; font-size: 11px; font-weight: bold; }");
        } else if (percentage >= 75.0) {
            m_targetLabel->setStyleSheet("QLabel { padding: 4px 8px; border-right: 1px solid #bbb; background-color: #f8f8f8; color: #ff9800; font-size: 11px; font-weight: bold; }");
        } else {
            m_targetLabel->setStyleSheet("QLabel { padding: 4px 8px; border-right: 1px solid #bbb; background-color: #f8f8f8; color: #f44336; font-size: 11px; font-weight: bold; }");
        }
    } else {
        m_targetLabel->setText("Target: Not set");
        m_targetLabel->setStyleSheet("QLabel { padding: 4px 8px; border-right: 1px solid #bbb; background-color: #f8f8f8; color: #333; font-size: 11px; font-weight: bold; }");
    }
}

void EditorWidget::showContextMenu(const QPoint& pos)
{
    QString selectedWord = getSelectedWord();
    
    // Enable/disable actions based on selection
    bool hasSelection = !selectedWord.isEmpty();
    m_lookupAction->setEnabled(hasSelection);
    m_translateAction->setEnabled(hasSelection);
    
    // Update action text
    if (hasSelection) {
        m_lookupAction->setText(QString("Look up \"%1\"").arg(selectedWord));
        m_translateAction->setText(QString("Translate \"%1\"").arg(selectedWord));
    } else {
        m_lookupAction->setText("Look up word");
        m_translateAction->setText("Translate word");
    }
    
    m_contextMenu->exec(m_textEditor->mapToGlobal(pos));
}

void EditorWidget::lookupWord()
{
    QString word = getSelectedWord();
    if (!word.isEmpty()) {
        emit wordSelected(word);
        // TODO: Integrate with word reference system
    }
}

void EditorWidget::translateWord()
{
    QString word = getSelectedWord();
    if (!word.isEmpty()) {
        // TODO: Integrate with translation service
        QMessageBox::information(this, "Translation", 
                                QString("Translation for \"%1\" - Coming soon!").arg(word));
    }
}

void EditorWidget::addHashtag()
{
    bool ok;
    QString hashtag = QInputDialog::getText(this, "Add Hashtag", 
                                          "Enter hashtag (without #):", 
                                          QLineEdit::Normal, "", &ok);
    
    if (ok && !hashtag.isEmpty()) {
        if (!hashtag.startsWith('#')) {
            hashtag = '#' + hashtag;
        }
        
        QTextCursor cursor = m_textEditor->textCursor();
        cursor.insertText(hashtag + " ");
        
        emit hashtagClicked(hashtag);
    }
}

void EditorWidget::onCursorPositionChanged()
{
    updateFormattingButtons();
    emit formattingChanged();
}

void EditorWidget::updateFormattingButtons()
{
    // Update toolbar button states based on current formatting
    QTextCursor cursor = m_textEditor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    
    m_boldAction->setChecked(format.fontWeight() == QFont::Bold);
    m_italicAction->setChecked(format.fontItalic());
    m_underlineAction->setChecked(format.fontUnderline());
    
    // Update alignment buttons
    Qt::Alignment align = m_textEditor->alignment();
    m_alignLeftAction->setChecked(align & Qt::AlignLeft);
    m_alignCenterAction->setChecked(align & Qt::AlignCenter);
    m_alignRightAction->setChecked(align & Qt::AlignRight);
    m_alignJustifyAction->setChecked(align & Qt::AlignJustify);
}

QString EditorWidget::getSelectedWord() const
{
    QTextCursor cursor = m_textEditor->textCursor();
    
    if (cursor.hasSelection()) {
        return cursor.selectedText();
    }
    
    // If no selection, try to select word under cursor
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

QStringList EditorWidget::extractHashtags(const QString& text) const
{
    QStringList hashtags;
    QRegularExpression hashtagRegex("#\\w+");
    QRegularExpressionMatchIterator matches = hashtagRegex.globalMatch(text);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        hashtags.append(match.captured(0));
    }
    
    return hashtags;
}