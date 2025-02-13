/*
 * Copyright (C) 2015-2017 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "actionchain_p.h"
#include "request_p.h"

#include "context_p.h"

using namespace Cutelyst;

ActionChain::ActionChain(const ActionList &chain, QObject *parent) : Action(new ActionChainPrivate, parent)
{
    Q_D(ActionChain);
    d->chain = chain;

    const Action *final = d->chain.last();

    QVariantHash args;
    args.insert(QStringLiteral("namespace"), final->ns());
    setupAction(args, nullptr);

    setName(QLatin1Char('_') + final->name());
    setReverse(final->reverse());
    setAttributes(final->attributes());
    setController(final->controller());

    for (Action *action : chain) {
        // FINAL should not have captures?
        if (/*action != final && */action->numberOfCaptures() > 0) {
            d->captures += action->numberOfCaptures();
        }
    }
}

ActionList ActionChain::chain() const noexcept
{
    Q_D(const ActionChain);
    return d->chain;
}

qint8 ActionChain::numberOfCaptures() const noexcept
{
    Q_D(const ActionChain);
    return d->captures;
}

bool ActionChain::doExecute(Context *c)
{
    Q_D(const ActionChain);

    Request *request =  c->request();
    const QStringList captures = request->captures();
    const QStringList currentArgs = request->args();
    const ActionList chain = d->chain;

    int &asyncDetached = c->d_ptr->asyncDetached;
    int &captured = c->d_ptr->chainedCaptured;
    int &chainedIx = c->d_ptr->chainedIx;

    for (; chainedIx < chain.size(); ++chainedIx) {
        if (asyncDetached) {
            c->d_ptr->pendingAsync.prepend(this);
            request->setArguments(currentArgs);
            break;
        }

        Action *action = chain.at(chainedIx);

        QStringList args;
        while (args.size() < action->numberOfCaptures() && captured < captures.size()) {
            args.append(captures.at(captured++));
        }

        // Final action gets args instead of captures
        request->setArguments(action != chain.last() ? args : currentArgs);
        if (!action->dispatch(c)) {
            return false;
        }
    }

    return true;
}

#include "moc_actionchain.cpp"
