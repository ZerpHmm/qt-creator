/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakestep.h"
#include "ui_qmakestep.h"

#include "qmakeparser.h"
#include "qmakebuildconfiguration.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakekitinformation.h"
#include "qmakenodes.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <qtsupport/debugginghelperbuildtask.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QMessageBox>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

namespace {
const char QMAKE_BS_ID[] = "QtProjectManager.QMakeBuildStep";

const char QMAKE_ARGUMENTS_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeArguments";
const char QMAKE_FORCED_KEY[] = "QtProjectManager.QMakeBuildStep.QMakeForced";
const char QMAKE_QMLDEBUGLIBAUTO_KEY[] = "QtProjectManager.QMakeBuildStep.LinkQmlDebuggingLibraryAuto";
const char QMAKE_QMLDEBUGLIB_KEY[] = "QtProjectManager.QMakeBuildStep.LinkQmlDebuggingLibrary";
}

QMakeStep::QMakeStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(QMAKE_BS_ID)),
    m_forced(false),
    m_needToRunQMake(false),
    m_linkQmlDebuggingLibrary(DebugLink)
{
    ctor();
}

QMakeStep::QMakeStep(BuildStepList *bsl, const Core::Id id) :
    AbstractProcessStep(bsl, id),
    m_forced(false),
    m_linkQmlDebuggingLibrary(DebugLink)
{
    ctor();
}

QMakeStep::QMakeStep(BuildStepList *bsl, QMakeStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_forced(bs->m_forced),
    m_userArgs(bs->m_userArgs),
    m_linkQmlDebuggingLibrary(bs->m_linkQmlDebuggingLibrary)
{
    ctor();
}

void QMakeStep::ctor()
{
    //: QMakeStep default display name
    setDefaultDisplayName(tr("qmake"));
}

QMakeStep::~QMakeStep()
{
}

QmakeBuildConfiguration *QMakeStep::qmakeBuildConfiguration() const
{
    return static_cast<QmakeBuildConfiguration *>(buildConfiguration());
}

///
/// Returns all arguments
/// That is: possbile subpath
/// spec
/// config arguemnts
/// moreArguments
/// user arguments
QString QMakeStep::allArguments(bool shorted)
{
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    QStringList arguments;
    if (bc->subNodeBuild())
        arguments << QDir::toNativeSeparators(bc->subNodeBuild()->path());
    else if (shorted)
        arguments << QDir::toNativeSeparators(QFileInfo(project()->projectFilePath()).fileName());
    else
        arguments << QDir::toNativeSeparators(project()->projectFilePath());

    arguments << QLatin1String("-r");
    bool userProvidedMkspec = false;
    for (QtcProcess::ConstArgIterator ait(m_userArgs); ait.next(); ) {
        if (ait.value() == QLatin1String("-spec")) {
            if (ait.next()) {
                userProvidedMkspec = true;
                break;
            }
        }
    }
    FileName specArg = mkspec();
    if (!userProvidedMkspec && !specArg.isEmpty())
        arguments << QLatin1String("-spec") << specArg.toUserOutput();

    // Find out what flags we pass on to qmake
    arguments << bc->configCommandLineArguments();

    arguments << deducedArguments();

    QString args = QtcProcess::joinArgs(arguments);
    // User arguments
    QtcProcess::addArgs(&args, m_userArgs);
    // moreArgumentsAfter
    foreach (const QString &arg, deducedArgumentsAfter())
        QtcProcess::addArg(&args, arg);
    return args;
}

///
/// moreArguments,
/// iphoneos/iphonesimulator for ios
/// QMAKE_VAR_QMLJSDEBUGGER_PATH
QStringList QMakeStep::deducedArguments()
{
    QStringList arguments;
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    ProjectExplorer::Abi targetAbi;
    if (tc)
        targetAbi = tc->targetAbi();

    // explicitly add architecture to CONFIG
    if ((targetAbi.os() == ProjectExplorer::Abi::MacOS)
            && (targetAbi.binaryFormat() == ProjectExplorer::Abi::MachOFormat)) {
        if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture) {
            if (targetAbi.wordWidth() == 32)
                arguments << QLatin1String("CONFIG+=x86");
            else if (targetAbi.wordWidth() == 64)
                arguments << QLatin1String("CONFIG+=x86_64");

            const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // from Ios::Constants (include header?)
            QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
            if (version && version->type() == QLatin1String(IOSQT))
                arguments << QLatin1String("CONFIG+=iphonesimulator");
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::PowerPCArchitecture) {
            if (targetAbi.wordWidth() == 32)
                arguments << QLatin1String("CONFIG+=ppc");
            else if (targetAbi.wordWidth() == 64)
                arguments << QLatin1String("CONFIG+=ppc64");
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::ArmArchitecture) {
            arguments << QLatin1String("CONFIG+=iphoneos");
        }
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (linkQmlDebuggingLibrary() && version) {
        arguments << QLatin1String(Constants::QMAKEVAR_QUICK1_DEBUG);
        if (version->qtVersion().majorVersion >= 5)
            arguments << QLatin1String(Constants::QMAKEVAR_QUICK2_DEBUG);
    }


    return arguments;
}

/// -after OBJECTS_DIR, MOC_DIR, UI_DIR, RCC_DIR
QStringList QMakeStep::deducedArgumentsAfter()
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (version && !version->supportsShadowBuilds()) {
        // We have a target which does not allow shadow building.
        // But we really don't want to have the build artefacts in the source dir
        // so we try to hack around it, to make the common cases work.
        // This is a HACK, remove once all make generators support
        // shadow building
        return QStringList() << QLatin1String("-after")
                             << QLatin1String("OBJECTS_DIR=obj")
                             << QLatin1String("MOC_DIR=moc")
                             << QLatin1String("UI_DIR=ui")
                             << QLatin1String("RCC_DIR=rcc");
    }
    return QStringList();
}

bool QMakeStep::init()
{
    QmakeBuildConfiguration *qt4bc = qmakeBuildConfiguration();
    const QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());

    if (!qtVersion)
        return false;

    QString args = allArguments();
    QString workingDirectory;

    if (qt4bc->subNodeBuild())
        workingDirectory = qt4bc->subNodeBuild()->buildDir();
    else
        workingDirectory = qt4bc->buildDirectory().toString();

    FileName program = qtVersion->qmakeCommand();

    QString makefile = workingDirectory;

    if (qt4bc->subNodeBuild()) {
        if (!qt4bc->subNodeBuild()->makefile().isEmpty())
            makefile.append(qt4bc->subNodeBuild()->makefile());
        else
            makefile.append(QLatin1String("/Makefile"));
    } else if (!qt4bc->makefile().isEmpty()) {
        makefile.append(QLatin1Char('/'));
        makefile.append(qt4bc->makefile());
    } else {
        makefile.append(QLatin1String("/Makefile"));
    }

    // Check whether we need to run qmake
    bool makefileOutDated = (qt4bc->compareToImportFrom(makefile) != QmakeBuildConfiguration::MakefileMatches);
    if (m_forced || makefileOutDated)
        m_needToRunQMake = true;
    m_forced = false;

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(qt4bc->macroExpander());
    pp->setWorkingDirectory(workingDirectory);
    pp->setCommand(program.toString());
    pp->setArguments(args);
    pp->setEnvironment(qt4bc->environment());
    pp->resolveAll();

    setOutputParser(new QMakeParser);

    QmakeProFileNode *node = static_cast<QmakeProject *>(qt4bc->target()->project())->rootQmakeProjectNode();
    if (qt4bc->subNodeBuild())
        node = qt4bc->subNodeBuild();
    QString proFile = node->path();

    QList<ProjectExplorer::Task> tasks = qtVersion->reportIssues(proFile, workingDirectory);
    qSort(tasks);

    if (!tasks.isEmpty()) {
        bool canContinue = true;
        foreach (const ProjectExplorer::Task &t, tasks) {
            addTask(t);
            if (t.type == Task::Error)
                canContinue = false;
        }
        if (!canContinue) {
            emit addOutput(tr("Configuration is faulty, please check the Issues view for details."), BuildStep::MessageOutput);
            return false;
        }
    }

    m_scriptTemplate = node->projectType() == ScriptTemplate;

    return AbstractProcessStep::init();
}

void QMakeStep::run(QFutureInterface<bool> &fi)
{
    if (m_scriptTemplate) {
        fi.reportResult(true);
        return;
    }

    if (!m_needToRunQMake) {
        emit addOutput(tr("Configuration unchanged, skipping qmake step."), BuildStep::MessageOutput);
        fi.reportResult(true);
        emit finished();
        return;
    }

    m_needToRunQMake = false;
    AbstractProcessStep::run(fi);
}

void QMakeStep::setForced(bool b)
{
    m_forced = b;
}

bool QMakeStep::forced()
{
    return m_forced;
}

ProjectExplorer::BuildStepConfigWidget *QMakeStep::createConfigWidget()
{
    return new QMakeStepConfigWidget(this);
}

bool QMakeStep::immutable() const
{
    return false;
}

void QMakeStep::processStartupFailed()
{
    m_needToRunQMake = true;
    AbstractProcessStep::processStartupFailed();
}

bool QMakeStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    bool result = AbstractProcessStep::processSucceeded(exitCode, status);
    if (!result)
        m_needToRunQMake = true;
    QmakeProject *project = static_cast<QmakeProject *>(qmakeBuildConfiguration()->target()->project());
    project->emitBuildDirectoryInitialized();
    return result;
}

void QMakeStep::setUserArguments(const QString &arguments)
{
    if (m_userArgs == arguments)
        return;
    m_userArgs = arguments;

    emit userArgumentsChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

bool QMakeStep::linkQmlDebuggingLibrary() const
{
    if (m_linkQmlDebuggingLibrary == DoLink)
        return true;
    if (m_linkQmlDebuggingLibrary == DoNotLink)
        return false;

    const Core::Context languages = project()->projectLanguages();
    if (!languages.contains(ProjectExplorer::Constants::LANG_QMLJS))
        return false;
    return (qmakeBuildConfiguration()->buildType() & BuildConfiguration::Debug);
}

void QMakeStep::setLinkQmlDebuggingLibrary(bool enable)
{
    if ((enable && (m_linkQmlDebuggingLibrary == DoLink))
            || (!enable && (m_linkQmlDebuggingLibrary == DoNotLink)))
        return;
    m_linkQmlDebuggingLibrary = enable ? DoLink : DoNotLink;

    emit linkQmlDebuggingLibraryChanged();

    qmakeBuildConfiguration()->emitQMakeBuildConfigurationChanged();
    qmakeBuildConfiguration()->emitProFileEvaluateNeeded();
}

QStringList QMakeStep::parserArguments()
{
    QStringList result;
    for (QtcProcess::ConstArgIterator ait(allArguments()); ait.next(); )
        if (ait.isSimple())
            result << ait.value();
    return result;
}

QString QMakeStep::userArguments()
{
    return m_userArgs;
}

FileName QMakeStep::mkspec()
{
    QString additionalArguments = m_userArgs;
    for (QtcProcess::ArgIterator ait(&additionalArguments); ait.next(); ) {
        if (ait.value() == QLatin1String("-spec")) {
            if (ait.next())
                return FileName::fromUserInput(ait.value());
        }
    }

    return QmakeProjectManager::QmakeKitInformation::effectiveMkspec(target()->kit());
}

QVariantMap QMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(QMAKE_ARGUMENTS_KEY), m_userArgs);
    map.insert(QLatin1String(QMAKE_QMLDEBUGLIBAUTO_KEY), m_linkQmlDebuggingLibrary == DebugLink);
    map.insert(QLatin1String(QMAKE_QMLDEBUGLIB_KEY), m_linkQmlDebuggingLibrary == DoLink);
    map.insert(QLatin1String(QMAKE_FORCED_KEY), m_forced);
    return map;
}

bool QMakeStep::fromMap(const QVariantMap &map)
{
    m_userArgs = map.value(QLatin1String(QMAKE_ARGUMENTS_KEY)).toString();
    m_forced = map.value(QLatin1String(QMAKE_FORCED_KEY), false).toBool();
    if (map.value(QLatin1String(QMAKE_QMLDEBUGLIBAUTO_KEY), false).toBool()) {
        m_linkQmlDebuggingLibrary = DebugLink;
    } else {
        if (map.value(QLatin1String(QMAKE_QMLDEBUGLIB_KEY), false).toBool())
            m_linkQmlDebuggingLibrary = DoLink;
        else
            m_linkQmlDebuggingLibrary = DoNotLink;
    }

    return BuildStep::fromMap(map);
}

////
// QMakeStepConfigWidget
////

QMakeStepConfigWidget::QMakeStepConfigWidget(QMakeStep *step)
    : BuildStepConfigWidget(), m_ui(new Internal::Ui::QMakeStep), m_step(step),
      m_ignoreChange(false)
{
    m_ui->setupUi(this);

    m_ui->qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());
    m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->linkQmlDebuggingLibrary());

    qmakeBuildConfigChanged();

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();

    connect(m_ui->qmakeAdditonalArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(qmakeArgumentsLineEdited()));
    connect(m_ui->buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(buildConfigurationSelected()));
    connect(m_ui->qmlDebuggingLibraryCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(linkQmlDebuggingLibraryChecked(bool)));
    connect(step, SIGNAL(userArgumentsChanged()),
            this, SLOT(userArgumentsChanged()));
    connect(step, SIGNAL(linkQmlDebuggingLibraryChanged()),
            this, SLOT(linkQmlDebuggingLibraryChanged()));
    connect(step->qmakeBuildConfiguration(), SIGNAL(qmakeBuildConfigurationChanged()),
            this, SLOT(qmakeBuildConfigChanged()));
    connect(step->target(), SIGNAL(kitChanged()), this, SLOT(qtVersionChanged()));
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(dumpUpdatedFor(Utils::FileName)),
            this, SLOT(qtVersionChanged()));
}

QMakeStepConfigWidget::~QMakeStepConfigWidget()
{
    delete m_ui;
}

QString QMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

QString QMakeStepConfigWidget::additionalSummaryText() const
{
    return m_additionalSummaryText;
}

QString QMakeStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void QMakeStepConfigWidget::qtVersionChanged()
{
    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
}

void QMakeStepConfigWidget::qmakeBuildConfigChanged()
{
    QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
    bool debug = bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild;
    m_ignoreChange = true;
    m_ui->buildConfigurationComboBox->setCurrentIndex(debug? 0 : 1);
    m_ignoreChange = false;
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::userArgumentsChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->qmakeAdditonalArgumentsLineEdit->setText(m_step->userArguments());
    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::linkQmlDebuggingLibraryChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->qmlDebuggingLibraryCheckBox->setChecked(m_step->linkQmlDebuggingLibrary());

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();
}

void QMakeStepConfigWidget::qmakeArgumentsLineEdited()
{
    m_ignoreChange = true;
    m_step->setUserArguments(m_ui->qmakeAdditonalArgumentsLineEdit->text());
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::buildConfigurationSelected()
{
    if (m_ignoreChange)
        return;
    QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
    QtSupport::BaseQtVersion::QmakeBuildConfigs buildConfiguration = bc->qmakeBuildConfiguration();
    if (m_ui->buildConfigurationComboBox->currentIndex() == 0) { // debug
        buildConfiguration = buildConfiguration | QtSupport::BaseQtVersion::DebugBuild;
    } else {
        buildConfiguration = buildConfiguration & ~QtSupport::BaseQtVersion::DebugBuild;
    }
    m_ignoreChange = true;
    bc->setQMakeBuildConfiguration(buildConfiguration);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
}

void QMakeStepConfigWidget::linkQmlDebuggingLibraryChecked(bool checked)
{
    if (m_ignoreChange)
        return;

    m_ignoreChange = true;
    m_step->setLinkQmlDebuggingLibrary(checked);
    m_ignoreChange = false;

    updateSummaryLabel();
    updateEffectiveQMakeCall();
    updateQmlDebuggingOption();

    QMessageBox *question = new QMessageBox(Core::ICore::mainWindow());
    question->setWindowTitle(tr("QML Debugging"));
    question->setText(tr("The option will only take effect if the project is recompiled. Do you want to recompile now?"));
    question->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    question->setModal(true);
    connect(question, SIGNAL(finished(int)), this, SLOT(recompileMessageBoxFinished(int)));
    question->show();
}

void QMakeStepConfigWidget::updateSummaryLabel()
{
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(m_step->target()->kit());
    if (!qtVersion) {
        setSummaryText(tr("<b>qmake:</b> No Qt version set. Cannot run qmake."));
        return;
    }
    // We don't want the full path to the .pro file
    QString args = m_step->allArguments(true);
    // And we only use the .pro filename not the full path
    QString program = qtVersion->qmakeCommand().toFileInfo().fileName();
    setSummaryText(tr("<b>qmake:</b> %1 %2").arg(program, args));
}

void QMakeStepConfigWidget::updateQmlDebuggingOption()
{
    QString warningText;
    bool supported = QtSupport::BaseQtVersion::isQmlDebuggingSupported(m_step->target()->kit(),
                                                                     &warningText);
    m_ui->qmlDebuggingLibraryCheckBox->setEnabled(supported);
    m_ui->debuggingLibraryLabel->setText(tr("Enable QML debugging:"));

    if (supported && m_step->linkQmlDebuggingLibrary())
        warningText = tr("Might make your application vulnerable. Only use in a safe environment.");

    m_ui->qmlDebuggingWarningText->setText(warningText);
    m_ui->qmlDebuggingWarningIcon->setVisible(!warningText.isEmpty());
}

void QMakeStepConfigWidget::updateEffectiveQMakeCall()
{
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(m_step->target()->kit());
    QString program = tr("<No Qt version>");
    if (qtVersion)
        program = qtVersion->qmakeCommand().toFileInfo().fileName();
    m_ui->qmakeArgumentsEdit->setPlainText(program + QLatin1Char(' ') + m_step->allArguments());
}

void QMakeStepConfigWidget::recompileMessageBoxFinished(int button)
{
    if (button == QMessageBox::Yes) {
        QmakeBuildConfiguration *bc = m_step->qmakeBuildConfiguration();
        if (!bc)
            return;

        QList<ProjectExplorer::BuildStepList *> stepLists;
        const Core::Id clean = ProjectExplorer::Constants::BUILDSTEPS_CLEAN;
        const Core::Id build = ProjectExplorer::Constants::BUILDSTEPS_BUILD;
        stepLists << bc->stepList(clean) << bc->stepList(build);
        BuildManager::buildLists(stepLists, QStringList() << ProjectExplorerPlugin::displayNameForStepId(clean)
                       << ProjectExplorerPlugin::displayNameForStepId(build));
    }
}

void QMakeStepConfigWidget::setSummaryText(const QString &text)
{
    if (text == m_summaryText)
        return;
    m_summaryText = text;
    emit updateSummary();
}

////
// QMakeStepFactory
////

QMakeStepFactory::QMakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

QMakeStepFactory::~QMakeStepFactory()
{
}

bool QMakeStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    if (!qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
        return false;
    return id == QMAKE_BS_ID;
}

ProjectExplorer::BuildStep *QMakeStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    return new QMakeStep(parent);
}

bool QMakeStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep *QMakeStepFactory::clone(BuildStepList *parent, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new QMakeStep(parent, qobject_cast<QMakeStep *>(source));
}

bool QMakeStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QMakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QMakeStep *bs = new QMakeStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> QMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        if (QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
            if (!bc->qmakeStep())
                return QList<Core::Id>() << Core::Id(QMAKE_BS_ID);
    return QList<Core::Id>();
}

QString QMakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == QMAKE_BS_ID)
        return tr("qmake");
    return QString();
}
