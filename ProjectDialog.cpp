/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include "ProjectDialog.h"
#include <QMessageBox>
#include <QStandardPaths>
#include <QRegularExpression>

ProjectDialog::ProjectDialog(QWidget *parent)
    : QDialog(parent)
    , m_projectNameEdit(nullptr)
    , m_authorEdit(nullptr)
    , m_descriptionEdit(nullptr)
    , m_locationEdit(nullptr)
    , m_browseButton(nullptr)
    , m_fullPathLabel(nullptr)
    , m_createButton(nullptr)
    , m_cancelButton(nullptr)
    , m_selectedLocation(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
{
    setupUI();
    setWindowTitle("New NeuroDraft Project");
    setModal(true);
    resize(500, 400);
}

ProjectDialog::~ProjectDialog() = default;

void ProjectDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel* titleLabel = new QLabel("Create New Project", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    m_mainLayout->addWidget(titleLabel);
    
    // Form layout
    m_formLayout = new QFormLayout();
    
    // Project name
    m_projectNameEdit = new QLineEdit(this);
    m_projectNameEdit->setPlaceholderText("Enter project name...");
    connect(m_projectNameEdit, &QLineEdit::textChanged, this, &ProjectDialog::updateProjectPath);
    connect(m_projectNameEdit, &QLineEdit::textChanged, this, &ProjectDialog::validateInput);
    m_formLayout->addRow("Project Name:", m_projectNameEdit);
    
    // Author name
    m_authorEdit = new QLineEdit(this);
    m_authorEdit->setPlaceholderText("Your name...");
    m_formLayout->addRow("Author:", m_authorEdit);
    
    // Description
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText("Brief description of your project...");
    m_descriptionEdit->setMaximumHeight(80);
    m_formLayout->addRow("Description:", m_descriptionEdit);
    
    // Location selection
    m_locationLayout = new QHBoxLayout();
    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setText(m_selectedLocation);
    m_locationEdit->setReadOnly(true);
    
    m_browseButton = new QPushButton("Browse...", this);
    connect(m_browseButton, &QPushButton::clicked, this, &ProjectDialog::browseLocation);
    
    m_locationLayout->addWidget(m_locationEdit);
    m_locationLayout->addWidget(m_browseButton);
    
    QWidget* locationWidget = new QWidget(this);
    locationWidget->setLayout(m_locationLayout);
    m_formLayout->addRow("Location:", locationWidget);
    
    m_mainLayout->addLayout(m_formLayout);
    
    // Full path display
    QLabel* pathLabel = new QLabel("Project will be created at:", this);
    pathLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    m_mainLayout->addWidget(pathLabel);
    
    m_fullPathLabel = new QLabel(this);
    m_fullPathLabel->setStyleSheet("color: #666; font-family: monospace; background: #f5f5f5; padding: 5px; border: 1px solid #ddd; border-radius: 3px;");
    m_fullPathLabel->setWordWrap(true);
    updateFullPath();
    m_mainLayout->addWidget(m_fullPathLabel);
    
    // Spacer
    m_mainLayout->addStretch();
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    m_createButton = new QPushButton("Create Project", this);
    m_createButton->setDefault(true);
    m_createButton->setEnabled(false);
    connect(m_createButton, &QPushButton::clicked, this, &QDialog::accept);
    
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_createButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Focus on project name
    m_projectNameEdit->setFocus();
}

void ProjectDialog::browseLocation()
{
    QString directory = QFileDialog::getExistingDirectory(
        this,
        "Select Project Location",
        m_selectedLocation,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!directory.isEmpty()) {
        m_selectedLocation = directory;
        m_locationEdit->setText(directory);
        updateFullPath();
        validateInput();
    }
}

void ProjectDialog::updateProjectPath()
{
    updateFullPath();
}

void ProjectDialog::updateFullPath()
{
    QString projectName = m_projectNameEdit->text().trimmed();
    if (projectName.isEmpty()) {
        m_fullPathLabel->setText("Enter a project name to see the full path");
        return;
    }
    
    // Clean the project name for use as a directory name
    QString cleanName = projectName;
    cleanName.replace(QRegularExpression("[^a-zA-Z0-9_\\-\\s]"), "");
    cleanName.replace(QRegularExpression("\\s+"), "_");
    
    QString fullPath = QDir(m_selectedLocation).filePath(cleanName);
    m_fullPathLabel->setText(fullPath);
}

void ProjectDialog::validateInput()
{
    QString projectName = m_projectNameEdit->text().trimmed();
    bool isValid = !projectName.isEmpty() && !m_selectedLocation.isEmpty();
    
    // Check if directory already exists
    if (isValid) {
        QString cleanName = projectName;
        cleanName.replace(QRegularExpression("[^a-zA-Z0-9_\\-\\s]"), "");
        cleanName.replace(QRegularExpression("\\s+"), "_");
        
        QString fullPath = QDir(m_selectedLocation).filePath(cleanName);
        if (QDir(fullPath).exists()) {
            m_fullPathLabel->setStyleSheet("color: #d32f2f; font-family: monospace; background: #ffebee; padding: 5px; border: 1px solid #f44336; border-radius: 3px;");
            m_fullPathLabel->setText(fullPath + " (Directory already exists!)");
            isValid = false;
        } else {
            m_fullPathLabel->setStyleSheet("color: #666; font-family: monospace; background: #f5f5f5; padding: 5px; border: 1px solid #ddd; border-radius: 3px;");
        }
    }
    
    m_createButton->setEnabled(isValid);
}

QString ProjectDialog::getProjectName() const
{
    return m_projectNameEdit->text().trimmed();
}

QString ProjectDialog::getProjectPath() const
{
    QString projectName = getProjectName();
    QString cleanName = projectName;
    cleanName.replace(QRegularExpression("[^a-zA-Z0-9_\\-\\s]"), "");
    cleanName.replace(QRegularExpression("\\s+"), "_");
    
    return QDir(m_selectedLocation).filePath(cleanName);
}

QString ProjectDialog::getAuthorName() const
{
    return m_authorEdit->text().trimmed();
}

QString ProjectDialog::getDescription() const
{
    return m_descriptionEdit->toPlainText().trimmed();
}