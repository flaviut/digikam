/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2017-05-15
 * Description : Managemenet dialogs for GPS bookmarks
 *
 * Copyright (C) 2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "bookmarksdlg.h"

// Qt includes

#include <QMenu>
#include <QBuffer>
#include <QFile>
#include <QMimeData>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QIcon>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QToolButton>
#include <QDebug>
#include <QAction>
#include <QObject>
#include <QUndoCommand>
#include <QVariant>
#include <QApplication>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QLabel>

// KDE includes

#include <klocalizedstring.h>

// Local includes

#include "bookmarksmngr.h"
#include "bookmarknode.h"
#include "imagepropertiesgpstab.h"
#include "gpsimageinfo.h"

using namespace GeoIface;

namespace Digikam
{

class AddBookmarkDialog::Private
{
public:

    Private() :
        manager(0),
        proxyModel(0),
        location(0),
        name(0)
    {
    }

    QString                url;
    BookmarksManager*      manager;
    AddBookmarkProxyModel* proxyModel;
    QComboBox*             location;
    QLineEdit*             name;
};

AddBookmarkDialog::AddBookmarkDialog(const QString& url,
                                     const QString& title,
                                     QWidget* const parent,
                                     BookmarksManager* const mngr)
    : QDialog(parent),
      d(new Private)
{
    d->url     = url;
    d->manager = mngr;

    setWindowFlags(Qt::Sheet);
    setWindowTitle(tr2i18n("Add Bookmark", 0));
    setObjectName(QStringLiteral("AddBookmarkDialog"));
    resize(300, 200);

    QLabel* const label = new QLabel(this);
    label->setText(tr2i18n("Type a name for the bookmark, and choose where to keep it.", 0));
    label->setObjectName(QStringLiteral("label"));
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);

    d->name = new QLineEdit(this);
    d->name->setObjectName(QStringLiteral("name"));
    d->name->setText(title);

    d->location = new QComboBox(this);
    d->location->setObjectName(QStringLiteral("location"));

    QSpacerItem* const verticalSpacer = new QSpacerItem(20, 2, QSizePolicy::Minimum,
                                                        QSizePolicy::Expanding);

    QDialogButtonBox* const buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->setCenterButtons(false);

    QVBoxLayout* const vbox = new QVBoxLayout(this);
    vbox->setObjectName(QStringLiteral("vbox"));
    vbox->addWidget(label);
    vbox->addWidget(d->name);
    vbox->addWidget(d->location);
    vbox->addItem(verticalSpacer);
    vbox->addWidget(buttonBox);

    QTreeView* const view       = new QTreeView(this);
    d->proxyModel               = new AddBookmarkProxyModel(this);
    BookmarksModel* const model = d->manager->bookmarksModel();
    d->proxyModel->setSourceModel(model);
    view->setModel(d->proxyModel);
    view->expandAll();
    view->header()->setStretchLastSection(true);
    view->header()->hide();
    view->setItemsExpandable(false);
    view->setRootIsDecorated(false);
    view->setIndentation(10);
    view->show();

    BookmarkNode* const menu = d->manager->bookmarks();
    QModelIndex idx          = d->proxyModel->mapFromSource(model->index(menu));
    view->setCurrentIndex(idx);

    d->location->setModel(d->proxyModel);
    d->location->setView(view);
    d->location->setCurrentIndex(idx.row());

    connect(buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));

    connect(buttonBox, SIGNAL(rejected()),
            this, SLOT(reject()));
}

AddBookmarkDialog::~AddBookmarkDialog()
{
    delete d;
}

void AddBookmarkDialog::accept()
{
    QModelIndex index = d->location->view()->currentIndex();
    index             = d->proxyModel->mapToSource(index);

    if (!index.isValid())
        index = d->manager->bookmarksModel()->index(0, 0);

    BookmarkNode* const parent   = d->manager->bookmarksModel()->node(index);
    BookmarkNode* const bookmark = new BookmarkNode(BookmarkNode::Bookmark);
    bookmark->url                = d->url;
    bookmark->title              = d->name->text();
    d->manager->addBookmark(parent, bookmark);
    QDialog::accept();
}

// ----------------------------------------------------------------

class BookmarksDialog::Private
{
public:

    Private() :
        manager(0),
        bookmarksModel(0),
        proxyModel(0),
        search(0),
        tree(0),
        mapView(0)
    {
    }

    BookmarksManager*      manager;
    BookmarksModel*        bookmarksModel;
    TreeProxyModel*        proxyModel;
    SearchTextBar*         search;
    QTreeView*             tree;
    ImagePropertiesGPSTab* mapView;
};

BookmarksDialog::BookmarksDialog(QWidget* const parent, BookmarksManager* const mngr)
    : QDialog(parent),
      d(new Private)
{
    d->manager = mngr;

    setObjectName(QStringLiteral("GeolocationBookmarksEditDialog"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(i18n("Edit Geolocation Bookmarks"));
    resize(750, 450);

    d->search = new SearchTextBar(this, QLatin1String("DigikamBookmarksGeolocationSearchBar"));
    d->search->setObjectName(QStringLiteral("search"));

    d->tree   = new QTreeView(this);
    d->tree->setObjectName(QStringLiteral("tree"));
    d->tree->setUniformRowHeights(true);
    d->tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    d->tree->setTextElideMode(Qt::ElideMiddle);
    d->tree->setDragDropMode(QAbstractItemView::InternalMove);
    d->tree->setAlternatingRowColors(true);
    d->tree->setContextMenuPolicy(Qt::CustomContextMenu);

    d->mapView = new ImagePropertiesGPSTab(this);

    QPushButton* const removeButton    = new QPushButton(this);
    removeButton->setObjectName(QStringLiteral("removeButton"));
    removeButton->setText(i18n("&Remove"));

    QPushButton* const addFolderButton = new QPushButton(this);
    addFolderButton->setObjectName(QStringLiteral("addFolderButton"));
    addFolderButton->setText(i18n("Add Folder"));

    QSpacerItem* const spacerItem1     = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    QDialogButtonBox* const buttonBox  = new QDialogButtonBox(this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    QHBoxLayout* const hbox = new QHBoxLayout();
    hbox->setObjectName(QStringLiteral("hbox"));
    hbox->addWidget(removeButton);
    hbox->addWidget(addFolderButton);
    hbox->addItem(spacerItem1);
    hbox->addWidget(buttonBox);

    QGridLayout* const grid = new QGridLayout(this);
    grid->setObjectName(QStringLiteral("grid"));
    grid->addWidget(d->search,  0, 0, 1, 2);
    grid->addWidget(d->tree,    1, 0, 1, 2);
    grid->addLayout(hbox,       2, 0, 1, 3);
    grid->addWidget(d->mapView, 0, 2, 2, 1);
    grid->setColumnStretch(1, 10);

    d->bookmarksModel       = d->manager->bookmarksModel();
    d->proxyModel           = new TreeProxyModel(this);
    d->proxyModel->setSourceModel(d->bookmarksModel);
    d->tree->setModel(d->proxyModel);
    d->tree->setExpanded(d->proxyModel->index(0, 0), true);
    d->tree->header()->setSectionResizeMode(QHeaderView::Stretch);

    connect(buttonBox, SIGNAL(accepted()),
            this, SLOT(accept()));

    connect(d->search, SIGNAL(textChanged(QString)),
            d->proxyModel, SLOT(setFilterFixedString(QString)));

    connect(d->proxyModel, SIGNAL(signalFilterAccepts(bool)),
            d->search, SLOT(slotSearchResult(bool)));

    connect(removeButton, SIGNAL(clicked()),
            this, SLOT(slotRemoveOne()));

    connect(d->tree, SIGNAL(clicked(QModelIndex)),
            this, SLOT(slotOpenInMap(QModelIndex)));

    connect(d->tree, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotCustomContextMenuRequested(QPoint)));

    connect(addFolderButton, SIGNAL(clicked()),
            this, SLOT(slotNewFolder()));

    expandNodes(d->manager->bookmarks());
}

BookmarksDialog::~BookmarksDialog()
{
    if (saveExpandedNodes(d->tree->rootIndex()))
        d->manager->changeExpanded();

    delete d;
}

void BookmarksDialog::accept()
{
    d->manager->save();
    QDialog::accept();
}

bool BookmarksDialog::saveExpandedNodes(const QModelIndex& parent)
{
    bool changed = false;

    for (int i = 0 ; i < d->proxyModel->rowCount(parent) ; i++)
    {
        QModelIndex child             = d->proxyModel->index(i, 0, parent);
        QModelIndex sourceIndex       = d->proxyModel->mapToSource(child);
        BookmarkNode* const childNode = d->bookmarksModel->node(sourceIndex);
        bool wasExpanded              = childNode->expanded;

        if (d->tree->isExpanded(child))
        {
            childNode->expanded = true;
            changed            |= saveExpandedNodes(child);
        }
        else
        {
            childNode->expanded = false;
        }

        changed |= (wasExpanded != childNode->expanded);
    }

    return changed;
}

void BookmarksDialog::expandNodes(BookmarkNode* const node)
{
    for (int i = 0 ; i < node->children().count() ; i++)
    {
        BookmarkNode* const childNode = node->children()[i];

        if (childNode->expanded)
        {
            QModelIndex idx = d->bookmarksModel->index(childNode);
            idx             = d->proxyModel->mapFromSource(idx);
            d->tree->setExpanded(idx, true);
            expandNodes(childNode);
        }
    }
}

void BookmarksDialog::slotCustomContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = d->tree->indexAt(pos);
    index             = index.sibling(index.row(), 0);

    if (index.isValid())
    {
        QMenu menu;
        menu.addAction(i18n("Remove"), this, SLOT(slotRemoveOne()));
        menu.exec(QCursor::pos());
    }
}

void BookmarksDialog::slotOpenInMap(const QModelIndex& index)
{
    if (!index.parent().isValid())
    {
        d->mapView->setGPSInfoList(GPSImageInfo::List());
        d->mapView->setActive(false);
        return;
    }

    QModelIndexList list = d->tree->selectionModel()->selectedIndexes();
    GPSImageInfo::List ilst;

    foreach (const QModelIndex& item, list)
    {
        QUrl url                  = item.sibling(index.row(), 1).data(BookmarksModel::UrlRole).toUrl();
        bool ok                   = false;
        GeoCoordinates coordinate = GeoCoordinates::fromGeoUrl(url.toString(), &ok);

        if (ok)
        {
            GPSImageInfo gpsInfo;
            gpsInfo.coordinates = coordinate;
            gpsInfo.dateTime    = QDateTime();
            gpsInfo.rating      = -1;
            gpsInfo.url         = url;
            ilst << gpsInfo;
        }
    }

    d->mapView->setGPSInfoList(GPSImageInfo::List() << ilst);
    d->mapView->setActive(!ilst.empty());
}

void BookmarksDialog::slotNewFolder()
{
    QModelIndex currentIndex = d->tree->currentIndex();
    QModelIndex idx          = currentIndex;

    if (idx.isValid() && !idx.model()->hasChildren(idx))
        idx = idx.parent();

    if (!idx.isValid())
        idx = d->tree->rootIndex();

    idx                        = d->proxyModel->mapToSource(idx);
    BookmarkNode* const parent = d->manager->bookmarksModel()->node(idx);
    BookmarkNode* const node   = new BookmarkNode(BookmarkNode::Folder);
    node->title                = i18n("New Folder");
    d->manager->addBookmark(parent, node, currentIndex.row() + 1);
    d->manager->save();
}

void BookmarksDialog::slotRemoveOne()
{
    QModelIndex index = d->tree->currentIndex();

    if (index.isValid())
    {
        index                    = d->proxyModel->mapToSource(index);
        BookmarkNode* const node = d->manager->bookmarksModel()->node(index);
        d->manager->removeBookmark(node);
        d->manager->save();
    }
}

} // namespace Digikam