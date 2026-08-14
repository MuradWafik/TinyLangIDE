#pragma once

#include <QAction>
#include <QApplication>
#include <QHash>
#include <QKeySequence>
#include <QMenuBar>
#include <QObject>
#include <QString>



inline QAction* MakeAction(
    const QString& text,
    const QKeySequence& shortcut = {},
    QObject* parent = nullptr)
{
    auto* action = new QAction(text, parent);
    action->setShortcut(shortcut);
    return action;
}


class MenuRegistry
{
public:
    explicit MenuRegistry(QMenuBar* menu_bar)
        : menu_bar(menu_bar)
    {}

    void AddMenu(const QString& id, const QString& title)
    {
        if(menus.contains(id))
        {
            return;
        }

        QMenu* menu = menu_bar->addMenu(title);
        menus.insert(id, menu);
    }

    void RemoveMenu(const QString& id)
    {
        const auto menu_iterator = menus.find(id);
        if(menu_iterator == menus.end())
        {
            return;
        }

        QMenu* menu = menu_iterator.value();

        menu_bar->removeAction(menu->menuAction());
        menu->deleteLater();

        menus.erase(menu_iterator);
        actions.remove(id);
    }

    void AddAction(
        const QString& menu_id,
        const QString& id,
        QAction* action)
    {
        QMenu* menu = menus.value(menu_id);

        if(menu == nullptr)
        {
            return;
        }

        if(actions[menu_id].contains(id))
        {
            return;
        }

        menu->addAction(action);
        actions[menu_id].insert(id, action);
    }

    void RemoveAction(
        const QString& menu_id,
        const QString& id)
    {
        const auto menu_iterator = actions.find(menu_id);
        if(menu_iterator == actions.end())
        {
            return;
        }

        const auto action_iterator = menu_iterator->find(id);
        if(action_iterator == menu_iterator->end())
        {
            return;
        }

        QAction* action = action_iterator.value();

        if(QMenu* menu = menus.value(menu_id);
            menu != nullptr)
        {
            menu->removeAction(action);
        }

        menu_iterator->erase(action_iterator);
    }

    void ReplaceAction(
        const QString& menu_id,
        const QString& id,
        QAction* action)
    {
        RemoveAction(menu_id, id);
        AddAction(menu_id, id, action);
    }

    void Clear()
    {
        menu_bar->clear();
        menus.clear();
        actions.clear();
    }

private:
    QMenuBar* menu_bar;
    QHash<QString, QMenu*> menus;
    QHash<QString, QHash<QString, QAction*>> actions;
};


class IMenuProvider
{
public:
    virtual ~IMenuProvider() = default;
    virtual void ContributeMenus(MenuRegistry& registry) = 0;
};


class DefaultMenuProvider final : public QObject, public IMenuProvider
{
public:
    explicit DefaultMenuProvider(QObject* parent = nullptr)
        : QObject(parent)
    {
        CreateActions();
        ConnectActions();
    }

    void ContributeMenus(MenuRegistry& registry) override
    {
        registry.AddMenu("file", "&File");
        registry.AddMenu("edit", "&Edit");
        registry.AddMenu("view", "&View");

        registry.AddAction("file", "new", new_action);
        registry.AddAction("file", "open", open_action);
        registry.AddAction("file", "exit", exit_action);

        registry.AddAction("edit", "undo", undo_action);
        registry.AddAction("edit", "redo", redo_action);
        registry.AddAction("edit", "cut", cut_action);
        registry.AddAction("edit", "copy", copy_action);
        registry.AddAction("edit", "paste", paste_action);
    }

protected:
    QAction* new_action;
    QAction* open_action;
    QAction* exit_action;

    QAction* undo_action;
    QAction* redo_action;
    QAction* cut_action;
    QAction* copy_action;
    QAction* paste_action;

    void CreateActions()
    {
        new_action = MakeAction("&New", QKeySequence::New,this);
        open_action = MakeAction("&Open...", QKeySequence::Open,this);

        exit_action = MakeAction("E&xit", QKeySequence::Quit, this);

        undo_action = MakeAction("&Undo", QKeySequence::Undo, this);
        redo_action = MakeAction("&Redo", QKeySequence::Redo,this);

        cut_action = MakeAction("Cu&t", QKeySequence::Cut, this);
        copy_action = MakeAction("&Copy", QKeySequence::Copy,this);
        paste_action = MakeAction("&Paste", QKeySequence::Paste,this);

    }

    void ConnectActions()
    {
        connect(exit_action, &QAction::triggered, qApp, &QApplication::quit);
    }
};
