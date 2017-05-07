/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-04-04
 * Description : a tool to generate HTML image galleries
 *
 * Copyright (C) 2012-2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "htmlselectionpage.h"

// Qt includes

#include <QIcon>
#include <QPixmap>
#include <QStackedWidget>

// Local includes

#include "htmlwizard.h"
#include "galleryinfo.h"
#include "dimageslist.h"

namespace Digikam
{

class HTMLSelectionPage::Private
{
public:

    Private(QWizard* const dialog)
      : albumSelector(0),
        imageList(0),
        stack(0),
        wizard(0),
        info(0),
        iface(0)
    {
        wizard = dynamic_cast<HTMLWizard*>(dialog);

        if (wizard)
        {
            info  = wizard->galleryInfo();
            iface = info->mIface;
        }
    }

    QWidget*         albumSelector;
    DImagesList*     imageList;
    QStackedWidget*  stack;
    HTMLWizard*      wizard;
    GalleryInfo*     info;
    DInfoInterface*  iface;
};

HTMLSelectionPage::HTMLSelectionPage(QWizard* const dialog, const QString& title)
    : DWizardPage(dialog, title),
      d(new Private(dialog))
{
    setObjectName(QLatin1String("AlbumSelectorPage"));

    d->stack              = new QStackedWidget(this);

    d->albumSelector = d->iface ? d->iface->albumChooser(this)
                                : new QWidget(this);

    d->stack->insertWidget(GalleryInfo::ALBUMS, d->albumSelector);

    d->imageList          = new DImagesList(this);
    d->imageList->setControlButtonsPlacement(DImagesList::ControlButtonsBelow);
    d->stack->insertWidget(GalleryInfo::IMAGES, d->imageList);

    setPageWidget(d->stack);
    setLeftBottomPix(QIcon::fromTheme(QLatin1String("folder-pictures")));

    connect(d->iface, SIGNAL(signalAlbumChooserSelectionChanged()),
            this, SIGNAL(completeChanged()));

    connect(d->imageList, SIGNAL(signalImageListChanged()),
            this, SIGNAL(completeChanged()));
}

HTMLSelectionPage::~HTMLSelectionPage()
{
    delete d;
}

void HTMLSelectionPage::initializePage()
{
    d->imageList->setIface(d->iface);
    d->imageList->loadImagesFromCurrentSelection();
    d->stack->setCurrentIndex(d->info->mGetOption);
}

bool HTMLSelectionPage::validatePage()
{
    if (d->stack->currentIndex() == GalleryInfo::ALBUMS)
    {
        if (d->iface)
        {
            if (d->iface->albumChooserItems().empty())
                return false;

            d->info->mAlbumList = d->iface->albumChooserItems();
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (d->imageList->imageUrls().empty())
            return false;

        d->info->mImageList = d->imageList->imageUrls();
    }

    return true;
}

bool HTMLSelectionPage::isComplete() const
{
    if (d->stack->currentIndex() == GalleryInfo::ALBUMS)
    {
        if (!d->iface)
            return false;

        return (!d->iface->albumChooserItems().empty());
    }

    return (!d->imageList->imageUrls().empty());
}

} // namespace Digikam