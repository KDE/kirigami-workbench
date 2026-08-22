// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>

class ProjectGenerator;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTextBrowser;

class CreateProjectDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param exampleName    Display name of the example
     * @param exampleUiDir   Absolute path to the example
     * @param parent         Parent widget
     */
    explicit CreateProjectDialog(const QString &exampleName,
                                 const QString &exampleUiDir,
                                 QWidget *parent = nullptr);

Q_SIGNALS:
    /**
     * Emitted when the user clicks "Open in Workbench" after a successful
     * generation. @p filePaths is the list of QML files that were copied into
     * the generated project's src/ directory, in the order they should be
     * opened (main.qml first).
     */
    void openInWorkbench(const QStringList &filePaths);

private Q_SLOTS:
    void startGeneration();
    void onGenerationDone(const QString &summary);
    void onGenerationError(const QString &errorMessage);
    void onProgress(int progress);
    void browseLocation();
    void validateInput();

private:
    void setupFormPage();
    void setupProgressPage();
    void setupResultPage();
    QStringList copyExampleQml(const QString &destDir);

    // Form page widgets
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_locationEdit = nullptr;
    QLineEdit *m_authorNameEdit = nullptr;
    QLineEdit *m_authorEmailEdit = nullptr;
    QLineEdit *m_versionEdit = nullptr;
    QComboBox *m_templateCombo = nullptr;
    QLabel *m_noTemplatesLabel = nullptr;

    // Progress page widgets
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_progressLabel = nullptr;

    // Result page widgets
    QTextBrowser *m_resultBrowser = nullptr;
    QLabel *m_resultIcon = nullptr;

    QStackedWidget *m_stack = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QPushButton *m_generateButton = nullptr;
    QPushButton *m_openFolderButton = nullptr;

    ProjectGenerator *m_generator = nullptr;
    QString m_exampleName;
    QString m_exampleUiDir;
    QString m_generatedDest;
    QStringList m_copiedQmlFiles;
};
