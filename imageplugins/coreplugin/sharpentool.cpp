/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2004-07-09
 * Description : a tool to sharp an image
 *
 * Copyright (C) 2004-2010 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#include "sharpentool.moc"

// C++ includes

#include <cmath>

// Qt includes

#include <QGridLayout>
#include <QLabel>

// KDE includes

#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>

// Local includes

#include "sharpsettings.h"
#include "dimgrefocus.h"
#include "dimgsharpen.h"
#include "dimgunsharpmask.h"
#include "editortoolsettings.h"
#include "imageiface.h"
#include "imageregionwidget.h"

namespace DigikamImagesPluginCore
{

class SharpenToolPriv
{

public:

    SharpenToolPriv() :
        configGroupName("sharpen Tool"),
        sharpSettings(0),
        previewWidget(0),
        gboxSettings(0)
        {}

    const QString       configGroupName;

    SharpSettings*      sharpSettings;
    ImageRegionWidget*  previewWidget;
    EditorToolSettings* gboxSettings;
};

SharpenTool::SharpenTool(QObject* parent)
           : EditorToolThreaded(parent),
             d(new SharpenToolPriv)
{
    setObjectName("sharpen");
    setToolName(i18n("Sharpen"));
    setToolIcon(SmallIcon("sharpenimage"));
    setToolHelp("blursharpentool.anchor");

    // -------------------------------------------------------------

    d->gboxSettings = new EditorToolSettings;
    d->gboxSettings->setButtons(EditorToolSettings::Default|
                                EditorToolSettings::Ok|
                                EditorToolSettings::Cancel|
                                EditorToolSettings::Load|
                                EditorToolSettings::SaveAs|
                                EditorToolSettings::Try);


    d->previewWidget = new ImageRegionWidget;

    d->sharpSettings = new SharpSettings(d->gboxSettings->plainPage());
    setToolSettings(d->gboxSettings);
    setToolView(d->previewWidget);
    setPreviewModeMask(PreviewToolBar::AllPreviewModes);

    connect(d->sharpSettings, SIGNAL(signalSettingsChanged()),
            this, SLOT(slotSettingsChanged()));

    init();
}

SharpenTool::~SharpenTool()
{
    delete d;
}

void SharpenTool::slotSettingsChanged()
{
    switch (d->sharpSettings->settings().method)
    {
        case SharpContainer::SimpleSharp:
        {
            d->gboxSettings->enableButton(EditorToolSettings::Load, false);
            d->gboxSettings->enableButton(EditorToolSettings::SaveAs, false);
        }
        case SharpContainer::UnsharpMask:
        {
            d->gboxSettings->enableButton(EditorToolSettings::Load, false);
            d->gboxSettings->enableButton(EditorToolSettings::SaveAs, false);
        }
        case SharpContainer::Refocus:
        {
            break;
        }
    }
}

void SharpenTool::renderingFinished()
{
    toolView()->setEnabled(true);
    toolSettings()->setEnabled(true);
    slotSettingsChanged();
}

void SharpenTool::readSettings()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group(d->configGroupName);
    d->sharpSettings->readSettings(group);
}

void SharpenTool::writeSettings()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group        = config->group(d->configGroupName);
    d->sharpSettings->writeSettings(group);
    group.sync();
}

void SharpenTool::slotResetSettings()
{
    d->sharpSettings->resetToDefault();
}

void SharpenTool::prepareEffect()
{
    toolView()->setEnabled(false);
    toolSettings()->setEnabled(false);
    SharpContainer settings = d->sharpSettings->settings();

    switch (settings.method)
    {
        case SharpContainer::SimpleSharp:
        {
            DImg img      = d->previewWidget->getOriginalRegionImage();
            double radius = settings.ssRadius/10.0;
            double sigma;

            if (radius < 1.0) sigma = radius;
            else sigma = sqrt(radius);

            setFilter(new DImgSharpen(&img, this, radius, sigma));
            break;
        }

        case SharpContainer::UnsharpMask:
        {
            DImg img  = d->previewWidget->getOriginalRegionImage();
            int    r  = (int)settings.umRadius;
            double a  = settings.umAmount;
            double th = settings.umThreshold;

            setFilter(new DImgUnsharpMask(&img, this, r, a, th));
            break;
        }

        case SharpContainer::Refocus:
        {
            DImg   img = d->previewWidget->getOriginalRegionImage();
            double r   = settings.rfRadius;
            double c   = settings.rfCorrelation;
            double n   = settings.rfNoise;
            double g   = settings.rfGauss;
            int    ms  = settings.rfMatrix;

            setFilter(new DImgRefocus(&img, this, ms, r, g, c, n));
            break;
        }
    }
}

void SharpenTool::prepareFinal()
{
    ImageIface iface(0, 0);
    uchar *data     = iface.getOriginalImage();
    int w           = iface.originalWidth();
    int h           = iface.originalHeight();
    bool sixteenBit = iface.originalSixteenBit();
    bool hasAlpha   = iface.originalHasAlpha();
    DImg orgImage   = DImg(w, h, sixteenBit, hasAlpha ,data);
    delete [] data;

    toolView()->setEnabled(false);
    toolSettings()->setEnabled(false);
    SharpContainer settings = d->sharpSettings->settings();

    switch (settings.method)
    {
        case SharpContainer::SimpleSharp:
        {
            double radius = settings.ssRadius/10.0;
            double sigma;

            if (radius < 1.0) sigma = radius;
            else sigma = sqrt(radius);

            setFilter(new DImgSharpen(&orgImage, this, radius, sigma));
            break;
        }

        case SharpContainer::UnsharpMask:
        {
            int    r  = (int)settings.umRadius;
            double a  = settings.umAmount;
            double th = settings.umThreshold;

            setFilter(new DImgUnsharpMask(&orgImage, this, r, a, th));
            break;
        }

        case SharpContainer::Refocus:
        {
            double r   = settings.rfRadius;
            double c   = settings.rfCorrelation;
            double n   = settings.rfNoise;
            double g   = settings.rfGauss;
            int    ms  = settings.rfMatrix;

            setFilter(new DImgRefocus(&orgImage, this, ms, r, g, c, n));
            break;
        }
    }
}

void SharpenTool::putPreviewData()
{
    DImg imDest = filter()->getTargetImage();
    d->previewWidget->setPreviewImage(imDest);
}

void SharpenTool::putFinalData()
{
    ImageIface iface(0, 0);
    DImg imDest = filter()->getTargetImage();
    SharpContainer settings = d->sharpSettings->settings();

    switch (settings.method)
    {
        case SharpContainer::SimpleSharp:
        {
            iface.putOriginalImage(i18n("Sharpen"), imDest.bits());
            break;
        }

        case SharpContainer::UnsharpMask:
        {
            iface.putOriginalImage(i18n("Unsharp Mask"), imDest.bits());
            break;
        }

        case SharpContainer::Refocus:
        {
            iface.putOriginalImage(i18n("Refocus"), imDest.bits());
            break;
        }
    }
}

void SharpenTool::slotLoadSettings()
{
    d->sharpSettings->loadSettings();
}

void SharpenTool::slotSaveAsSettings()
{
    d->sharpSettings->saveAsSettings();
}

}  // namespace DigikamImagesPluginCore
