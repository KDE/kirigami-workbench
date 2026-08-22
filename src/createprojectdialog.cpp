// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "createprojectdialog.h"
#include "projectgenerator.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

static const QList<QPair<QString, QString>> s_knownTemplates = {
    {u"kirigami6"_s,               u"Kirigami Application (Qt6)"_s},
    {u"kirigamiaddons6"_s,         u"Advanced Kirigami Application (Qt6)"_s},
    {u"kde-frameworks6-simple"_s,  u"KDE Frameworks 6 (Simple)"_s},
    {u"kde-frameworks6"_s,         u"KDE Frameworks 6 (Full)"_s},
    {u"kioworker6"_s,              u"KIO Worker (KF6)"_s},
    {u"kparts6-app"_s,             u"KParts Application (KF6)"_s},
    {u"ktexteditor6-plugin"_s,     u"KTextEditor Plugin (KF6)"_s},
    {u"librarymanager6"_s,         u"Multimedia Manager with Kirigami (Qt6)"_s},
};

CreateProjectDialog::CreateProjectDialog(const QString &exampleName,
                                         const QString &exampleUiDir,
                                         QWidget *parent)
    : QDialog(parent)
    , m_exampleName(exampleName)
    , m_exampleUiDir(exampleUiDir)
{
    setWindowTitle(i18nc("@title:window", "Create Project from Example"));
    setMinimumWidth(480);

    m_generator = new ProjectGenerator(this);

    connect(m_generator, &ProjectGenerator::done, this, &CreateProjectDialog::onGenerationDone);
    connect(m_generator, &ProjectGenerator::errorOccurred, this, &CreateProjectDialog::onGenerationError);
    connect(m_generator, &ProjectGenerator::progressUpdated, this, &CreateProjectDialog::onProgress);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setSpacing(style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));

    m_stack = new QStackedWidget(this);
    outerLayout->addWidget(m_stack);

    setupFormPage();
    setupProgressPage();
    setupResultPage();

    m_buttonBox = new QDialogButtonBox(this);

    m_generateButton = m_buttonBox->addButton(i18nc("@action:button", "Create Project"), QDialogButtonBox::AcceptRole);
    m_generateButton->setIcon(QIcon::fromTheme(u"project-development-new-template"_s));
    m_generateButton->setDefault(true);

    m_openFolderButton = m_buttonBox->addButton(i18nc("@action:button", "Open in Workbench"), QDialogButtonBox::ActionRole);
    m_openFolderButton->setIcon(QIcon::fromTheme(u"document-open-folder-symbolic"_s));
    m_openFolderButton->setVisible(false);

    auto *closeButton = m_buttonBox->addButton(QDialogButtonBox::Close);

    outerLayout->addWidget(m_buttonBox);

    connect(m_generateButton, &QPushButton::clicked, this, &CreateProjectDialog::startGeneration);
    connect(m_openFolderButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT openInWorkbench(m_copiedQmlFiles);
        accept();
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    validateInput();
}

void CreateProjectDialog::setupFormPage()
{
    auto *page = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *descLabel = new QLabel(i18n("Create a new KDE project scaffolded from the <b>%1</b> example.", m_exampleName), page);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);
    mainLayout->addSpacing(4);

    auto *projectGroup = new QGroupBox(i18nc("@title:group", "Project"), page);
    auto *formLayout = new QFormLayout(projectGroup);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_nameEdit = new QLineEdit(m_generator->name(), projectGroup);
    m_nameEdit->setPlaceholderText(i18nc("@info:placeholder", "e.g. MyKirigamiApp"));
    m_nameEdit->setToolTip(i18n("Application name used as directory name and in source files.\n"
                                 "Use CamelCase without spaces (e.g. MyApp)."));
    formLayout->addRow(i18nc("@label:textbox", "Name:"), m_nameEdit);

    auto *locationRow = new QWidget(projectGroup);
    auto *locationLayout = new QHBoxLayout(locationRow);
    locationLayout->setContentsMargins(0, 0, 0, 0);
    locationLayout->setSpacing(4);

    m_locationEdit = new QLineEdit(m_generator->location(), locationRow);
    m_locationEdit->setPlaceholderText(QDir::homePath() + u"/src"_s);
    m_locationEdit->setToolTip(i18n("Parent directory where the project folder will be created."));

    auto *browseButton = new QPushButton(i18nc("@action:button", "Browse…"), locationRow);
    browseButton->setToolTip(i18n("Choose output directory"));
    connect(browseButton, &QPushButton::clicked, this, &CreateProjectDialog::browseLocation);

    locationLayout->addWidget(m_locationEdit, 1);
    locationLayout->addWidget(browseButton);
    formLayout->addRow(i18nc("@label:textbox", "Location:"), locationRow);

    m_versionEdit = new QLineEdit(m_generator->version(), projectGroup);
    m_versionEdit->setPlaceholderText(u"0.1"_s);
    formLayout->addRow(i18nc("@label:textbox", "Version:"), m_versionEdit);

    mainLayout->addWidget(projectGroup);

    auto *authorGroup = new QGroupBox(i18nc("@title:group", "Author"), page);
    auto *authorLayout = new QFormLayout(authorGroup);
    authorLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_authorNameEdit = new QLineEdit(m_generator->authorName(), authorGroup);
    m_authorNameEdit->setPlaceholderText(i18nc("@info:placeholder", "Your Name"));
    authorLayout->addRow(i18nc("@label:textbox", "Name:"), m_authorNameEdit);

    m_authorEmailEdit = new QLineEdit(m_generator->authorEmail(), authorGroup);
    m_authorEmailEdit->setPlaceholderText(i18nc("@info:placeholder", "you@example.com"));
    authorLayout->addRow(i18nc("@label:textbox", "Email:"), m_authorEmailEdit);

    mainLayout->addWidget(authorGroup);

    auto *templateGroup = new QGroupBox(i18nc("@title:group", "Template"), page);
    auto *templateLayout = new QFormLayout(templateGroup);
    templateLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_templateCombo = new QComboBox(templateGroup);
    m_templateCombo->setToolTip(i18n("C++ KDE project template used for scaffolding.\n"
                                      "Requires kapptemplate to be installed."));

    const QStringList available = ProjectGenerator::availableTemplates();
    bool anyAdded = false;
    for (const auto &[id, label] : s_knownTemplates) {
        if (available.contains(id)) {
            m_templateCombo->addItem(label, id);
            anyAdded = true;
        }
    }

    for (const QString &tmpl : available) {
        bool known = false;
        for (const auto &[id, label] : s_knownTemplates) {
            if (id == tmpl) { known = true; break; }
        }
        if (!known)
            m_templateCombo->addItem(tmpl, tmpl);
    }

    m_noTemplatesLabel = new QLabel(
        i18n("<b>No templates found.</b> Install <a href=\"https://apps.kde.org/kapptemplate\">kapptemplate</a> to enable project creation."),
        templateGroup);
    m_noTemplatesLabel->setOpenExternalLinks(true);
    m_noTemplatesLabel->setWordWrap(true);
    m_noTemplatesLabel->setVisible(!anyAdded);
    m_templateCombo->setVisible(anyAdded);

    if (anyAdded)
        templateLayout->addRow(i18nc("@label:listbox", "Template:"), m_templateCombo);
    else
        templateLayout->addRow(m_noTemplatesLabel);

    mainLayout->addWidget(templateGroup);
    mainLayout->addStretch();

    connect(m_nameEdit, &QLineEdit::textChanged, this, &CreateProjectDialog::validateInput);
    connect(m_locationEdit, &QLineEdit::textChanged, this, &CreateProjectDialog::validateInput);
    connect(m_authorNameEdit, &QLineEdit::textChanged, this, &CreateProjectDialog::validateInput);

    m_stack->addWidget(page);
}

void CreateProjectDialog::setupProgressPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    m_progressLabel = new QLabel(i18n("Creating project…"), page);
    m_progressLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_progressLabel);
    layout->addSpacing(8);

    m_progressBar = new QProgressBar(page);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    m_stack->addWidget(page);
}

void CreateProjectDialog::setupResultPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *headerRow = new QHBoxLayout();

    m_resultIcon = new QLabel(page);
    m_resultIcon->setFixedSize(32, 32);
    headerRow->addWidget(m_resultIcon);
    headerRow->addStretch();

    layout->addLayout(headerRow);

    m_resultBrowser = new QTextBrowser(page);
    m_resultBrowser->setOpenExternalLinks(true);
    m_resultBrowser->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_resultBrowser, 1);

    m_stack->addWidget(page);
}

void CreateProjectDialog::browseLocation()
{
    const QString dir = QFileDialog::getExistingDirectory(this,
                                                          i18nc("@title:window", "Choose Output Directory"),
                                                          m_locationEdit->text());
    if (!dir.isEmpty())
        m_locationEdit->setText(dir);
}

void CreateProjectDialog::validateInput()
{
    const bool hasName = !m_nameEdit->text().trimmed().isEmpty();
    const bool hasLocation = !m_locationEdit->text().trimmed().isEmpty();
    const bool hasAuthor = !m_authorNameEdit->text().trimmed().isEmpty();
    const bool hasTemplates = m_templateCombo->count() > 0;

    m_generateButton->setEnabled(hasName && hasLocation && hasAuthor && hasTemplates);
}

void CreateProjectDialog::startGeneration()
{
    m_generator->setName(m_nameEdit->text().trimmed());
    m_generator->setLocation(m_locationEdit->text().trimmed());
    m_generator->setAuthorName(m_authorNameEdit->text().trimmed());
    m_generator->setAuthorEmail(m_authorEmailEdit->text().trimmed());
    m_generator->setVersion(m_versionEdit->text().trimmed().isEmpty() ? u"0.1"_s : m_versionEdit->text().trimmed());

    m_stack->setCurrentIndex(1);
    m_generateButton->setEnabled(false);
    m_progressBar->setValue(0);
    m_progressLabel->setText(i18n("Creating project…"));

    const QString templateId = m_templateCombo->currentData().toString();
    const QString templateLabel = m_templateCombo->currentText();
    m_generator->startGeneration(templateId, templateLabel);
}

void CreateProjectDialog::onProgress(int progress)
{
    m_progressBar->setValue(progress);
}

void CreateProjectDialog::onGenerationDone(const QString &summary)
{
    const QString appName = m_generator->name().toLower();
    m_generatedDest = QDir(m_generator->location()).filePath(appName);

    m_copiedQmlFiles = copyExampleQml(m_generatedDest);

    m_resultIcon->setPixmap(
        style()->standardIcon(QStyle::SP_DialogApplyButton).pixmap(32, 32));

    QString fullSummary = summary;
    if (!m_copiedQmlFiles.isEmpty()) {
        fullSummary += u"<br /><br />"_s
            + i18n("The example QML files have been copied to <tt>%1/src/</tt>.", m_generatedDest)
            + u"<br />"_s
            + i18n("Click <b>Open in Workbench</b> to start editing them.");
    }

    m_resultBrowser->setHtml(fullSummary);
    m_stack->setCurrentIndex(2);
    m_openFolderButton->setVisible(true);
    m_openFolderButton->setDefault(true);
    m_openFolderButton->setFocus();
}

void CreateProjectDialog::onGenerationError(const QString &errorMessage)
{
    m_resultIcon->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(32, 32));
    m_resultBrowser->setHtml(u"<b>"_s + i18n("Error") + u"</b><br />"_s + errorMessage.toHtmlEscaped());
    m_stack->setCurrentIndex(2);
}

QStringList CreateProjectDialog::copyExampleQml(const QString &destProjectDir)
{
    QStringList copied;

    if (m_exampleUiDir.isEmpty())
        return copied;

    const QDir uiDir(m_exampleUiDir);
    if (!uiDir.exists())
        return copied;

    QString targetDir = destProjectDir + u"/src"_s;
    if (!QDir(targetDir).exists())
        targetDir = destProjectDir;

    QStringList qmlFiles = uiDir.entryList({u"*.qml"_s}, QDir::Files);

    const int mainIdx = qmlFiles.indexOf(u"main.qml"_s, Qt::CaseInsensitive);
    if (mainIdx > 0)
        qmlFiles.move(mainIdx, 0);

    for (const QString &fileName : qmlFiles) {
        const QString src = uiDir.filePath(fileName);
        const QString dst = QDir(targetDir).filePath(fileName);
        if (QFile::exists(dst))
            QFile::remove(dst);
        if (QFile::copy(src, dst))
            copied << dst;
    }
    return copied;
}
