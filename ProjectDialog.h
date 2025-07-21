/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#ifndef PROJECTDIALOG_H
#define PROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QDir>

class ProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(QWidget *parent = nullptr);
    ~ProjectDialog();
    
    QString getProjectName() const;
    QString getProjectPath() const;
    QString getAuthorName() const;
    QString getDescription() const;

private slots:
    void browseLocation();
    void updateProjectPath();
    void validateInput();

private:
    void setupUI();
    void updateFullPath();
    
    // UI components
    QLineEdit* m_projectNameEdit;
    QLineEdit* m_authorEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_locationEdit;
    QPushButton* m_browseButton;
    QLabel* m_fullPathLabel;
    QPushButton* m_createButton;
    QPushButton* m_cancelButton;
    
    // Layouts
    QVBoxLayout* m_mainLayout;
    QFormLayout* m_formLayout;
    QHBoxLayout* m_locationLayout;
    QHBoxLayout* m_buttonLayout;
    
    QString m_selectedLocation;
};

#endif // PROJECTDIALOG_H