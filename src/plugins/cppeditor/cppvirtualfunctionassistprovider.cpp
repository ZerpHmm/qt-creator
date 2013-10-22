/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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


#include "cppvirtualfunctionassistprovider.h"

#include "cppeditorconstants.h"
#include "cppelementevaluator.h"
#include "cppvirtualfunctionproposalitem.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistproposal.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace CppEditor::Internal;
using namespace TextEditor;

/// Activate current item with the same shortcut that is configured for Follow Symbol Under Cursor.
/// This is limited to single-key shortcuts without modifiers.
class VirtualFunctionProposalWidget : public GenericProposalWidget
{
public:
    VirtualFunctionProposalWidget(bool openInSplit)
    {
        const char *id = openInSplit
            ? TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT
            : TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR;
        if (Core::Command *command = Core::ActionManager::command(id))
            m_sequence = command->keySequence();
    }

protected:
    bool eventFilter(QObject *o, QEvent *e)
    {
        if (e->type() == QEvent::ShortcutOverride && m_sequence.count() == 1) {
            QKeyEvent *ke = static_cast<QKeyEvent *>(e);
            const QKeySequence seq(ke->key());
            if (seq == m_sequence) {
                activateCurrentProposalItem();
                e->accept();
                return true;
            }
        }
        return GenericProposalWidget::eventFilter(o, e);
    }

private:
    QKeySequence m_sequence;
};

class VirtualFunctionProposal : public GenericProposal
{
public:
    VirtualFunctionProposal(int cursorPos, IGenericProposalModel *model, bool openInSplit)
        : GenericProposal(cursorPos, model)
        , m_openInSplit(openInSplit)
    {}

    bool isFragile() const
    { return true; }

    IAssistProposalWidget *createWidget() const
    { return new VirtualFunctionProposalWidget(m_openInSplit); }

private:
    bool m_openInSplit;
};

class VirtualFunctionsAssistProcessor : public IAssistProcessor
{
public:
    VirtualFunctionsAssistProcessor(const VirtualFunctionAssistProvider *provider)
        : m_startClass(provider->startClass())
        , m_function(provider->function())
        , m_snapshot(provider->snapshot())
        , m_openInNextSplit(provider->openInNextSplit())
    {}

    IAssistProposal *immediateProposal(const TextEditor::IAssistInterface *interface)
    {
        QTC_ASSERT(m_function, return 0);

        BasicProposalItem *hintItem = new VirtualFunctionProposalItem(CPPEditorWidget::Link());
        hintItem->setText(QCoreApplication::translate("VirtualFunctionsAssistProcessor",
                                                      "...searching overrides"));
        hintItem->setOrder(-1000);

        QList<BasicProposalItem *> items;
        items << itemFromSymbol(m_function, m_function);
        items << hintItem;
        return new VirtualFunctionProposal(interface->position(),
                                           new BasicProposalItemListModel(items),
                                           m_openInNextSplit);
    }

    IAssistProposal *perform(const IAssistInterface *interface)
    {
        if (!interface)
            return 0;

        QTC_ASSERT(m_startClass, return 0);
        QTC_ASSERT(m_function, return 0);
        QTC_ASSERT(!m_snapshot.isEmpty(), return 0);

        const QList<Symbol *> overrides = FunctionHelper::overrides(m_startClass, m_function,
                                                                    m_snapshot);
        QList<BasicProposalItem *> items;
        foreach (Symbol *symbol, overrides)
            items << itemFromSymbol(symbol, m_function);

        return new VirtualFunctionProposal(interface->position(),
                                           new BasicProposalItemListModel(items),
                                           m_openInNextSplit);
    }

    BasicProposalItem *itemFromSymbol(Symbol *symbol, Symbol *firstSymbol) const
    {
        const QString text = m_overview.prettyName(LookupContext::fullyQualifiedName(symbol));
        const CPPEditorWidget::Link link = CPPEditorWidget::linkToSymbol(symbol);

        BasicProposalItem *item = new VirtualFunctionProposalItem(link, m_openInNextSplit);
        item->setText(text);
        item->setIcon(m_icons.iconForSymbol(symbol));
        if (symbol == firstSymbol)
            item->setOrder(1000); // Ensure top position for function of static type

        return item;
    }

private:
    Class *m_startClass;
    Function *m_function;
    Snapshot m_snapshot;
    bool m_openInNextSplit;
    Overview m_overview;
    Icons m_icons;
};

VirtualFunctionAssistProvider::VirtualFunctionAssistProvider()
    : m_function(0)
    , m_openInNextSplit(false)
{
}

bool VirtualFunctionAssistProvider::configure(Class *startClass, Function *function,
                                              const Snapshot &snapshot, bool openInNextSplit)
{
    m_startClass = startClass;
    m_function = function;
    m_snapshot = snapshot;
    m_openInNextSplit = openInNextSplit;
    return true;
}

bool VirtualFunctionAssistProvider::isAsynchronous() const
{
    return true;
}

bool VirtualFunctionAssistProvider::supportsEditor(const Core::Id &editorId) const
{
    return editorId == CppEditor::Constants::CPPEDITOR_ID;
}

IAssistProcessor *VirtualFunctionAssistProvider::createProcessor() const
{
    return new VirtualFunctionsAssistProcessor(this);
}

enum VirtualType { Virtual, PureVirtual };

static bool isVirtualFunction_helper(const Function *function,
                                     const Snapshot &snapshot,
                                     VirtualType virtualType)
{
    if (!function)
        return false;

    if (virtualType == PureVirtual)
        return function->isPureVirtual();

    if (function->isVirtual())
        return true;

    const QString filePath = QString::fromUtf8(function->fileName(), function->fileNameLength());
    if (Document::Ptr document = snapshot.document(filePath)) {
        LookupContext context(document, snapshot);
        QList<LookupItem> results = context.lookup(function->name(), function->enclosingScope());
        if (!results.isEmpty()) {
            foreach (const LookupItem &item, results) {
                if (Symbol *symbol = item.declaration()) {
                    if (Function *functionType = symbol->type()->asFunctionType()) {
                        if (functionType == function) // already tested
                            continue;
                        if (functionType->isFinal())
                            return false;
                        if (functionType->isVirtual())
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

bool FunctionHelper::isVirtualFunction(const Function *function, const Snapshot &snapshot)
{
    return isVirtualFunction_helper(function, snapshot, Virtual);
}

bool FunctionHelper::isPureVirtualFunction(const Function *function, const Snapshot &snapshot)
{
    return isVirtualFunction_helper(function, snapshot, PureVirtual);
}

QList<Symbol *> FunctionHelper::overrides(Class *startClass, Function *function,
                                          const Snapshot &snapshot)
{
    QList<Symbol *> result;
    QTC_ASSERT(startClass && function, return result);

    FullySpecifiedType referenceType = function->type();
    const Name *referenceName = function->name();
    QTC_ASSERT(referenceName && referenceType.isValid(), return result);

    // Add itself
    result << function;

    // Find overrides
    CppEditor::Internal::CppClass cppClass = CppClass(startClass);
    cppClass.lookupDerived(startClass, snapshot);

    QList<CppClass> l;
    l << cppClass;

    while (!l.isEmpty()) {
        // Add derived
        CppClass clazz = l.takeFirst();
        foreach (const CppClass &d, clazz.derived) {
            if (!l.contains(d))
                l << d;
        }

        // Check member functions
        QTC_ASSERT(clazz.declaration, continue);
        Class *c = clazz.declaration->asClass();
        QTC_ASSERT(c, continue);
        for (int i = 0, total = c->memberCount(); i < total; ++i) {
            Symbol *candidate = c->memberAt(i);
            const Name *candidateName = candidate->name();
            const FullySpecifiedType candidateType = candidate->type();
            if (!candidateName || !candidateType.isValid())
                continue;
            if (candidateName->isEqualTo(referenceName) && candidateType.isEqualTo(referenceType))
                result << candidate;
        }
    }

    return result;
}

#ifdef WITH_TESTS
#include "cppeditorplugin.h"

#include <QList>
#include <QTest>

namespace CppEditor {
namespace Internal {

enum Virtuality
{
    NotVirtual,
    Virtual,
    PureVirtual
};
typedef QList<Virtuality> VirtualityList;
} // Internal namespace
} // CppEditor namespace

Q_DECLARE_METATYPE(CppEditor::Internal::Virtuality)
Q_DECLARE_METATYPE(CppEditor::Internal::VirtualityList)

namespace CppEditor {
namespace Internal {

void CppEditorPlugin::test_functionhelper_virtualFunctions()
{
    // Create and parse document
    QFETCH(QByteArray, source);
    QFETCH(VirtualityList, virtualityList);
    Document::Ptr document = Document::create(QLatin1String("virtuals"));
    document->setUtf8Source(source);
    document->check(); // calls parse();
    QCOMPARE(document->diagnosticMessages().size(), 0);
    QVERIFY(document->translationUnit()->ast());

    // Iterate through Function symbols
    Snapshot snapshot;
    snapshot.insert(document);
    Control *control = document->translationUnit()->control();
    Symbol **end = control->lastSymbol();
    for (Symbol **it = control->firstSymbol(); it != end; ++it) {
        const CPlusPlus::Symbol *symbol = *it;
        if (const Function *function = symbol->asFunction()) {
            QTC_ASSERT(!virtualityList.isEmpty(), return);
            Virtuality virtuality = virtualityList.takeFirst();
            if (FunctionHelper::isVirtualFunction(function, snapshot)) {
                if (FunctionHelper::isPureVirtualFunction(function, snapshot))
                    QCOMPARE(virtuality, PureVirtual);
                else
                    QCOMPARE(virtuality, Virtual);
            } else {
                QCOMPARE(virtuality, NotVirtual);
            }
        }
    }
    QVERIFY(virtualityList.isEmpty());
}

void CppEditorPlugin::test_functionhelper_virtualFunctions_data()
{
    typedef QByteArray _;
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<VirtualityList>("virtualityList");

    QTest::newRow("none")
            << _("struct None { void foo() {} };\n")
            << (VirtualityList() << NotVirtual);

    QTest::newRow("single-virtual")
            << _("struct V { virtual void foo() {} };\n")
            << (VirtualityList() << Virtual);

    QTest::newRow("single-pure-virtual")
            << _("struct PV { virtual void foo() = 0; };\n")
            << (VirtualityList() << PureVirtual);

    QTest::newRow("virtual-derived-with-specifier")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() {} };\n")
            << (VirtualityList() << Virtual << Virtual);

    QTest::newRow("virtual-derived-implicit")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { void foo() {} };\n")
            << (VirtualityList() << Virtual << Virtual);

    QTest::newRow("not-virtual-then-virtual")
            << _("struct Base { void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() {} };\n")
            << (VirtualityList() << NotVirtual << Virtual);

    QTest::newRow("virtual-final-not-virtual")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { void foo() final {} };\n"
                 "struct Derived2 : Derived { void foo() {} };")
            << (VirtualityList() << Virtual << Virtual << NotVirtual);

    QTest::newRow("virtual-then-pure")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() = 0; };\n"
                 "struct Derived2 : Derived { void foo() {} };")
            << (VirtualityList() << Virtual << PureVirtual << Virtual);

    QTest::newRow("virtual-virtual-final-not-virtual")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() final {} };\n"
                 "struct Derived2 : Derived { void foo() {} };")
            << (VirtualityList() << Virtual << Virtual << NotVirtual);
}

} // namespace Internal
} // namespace CppEditor

#endif