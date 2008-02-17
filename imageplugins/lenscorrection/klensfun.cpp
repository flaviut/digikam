/* ============================================================
 *
 * Date        : 2008-02-10
 * Description : KLensFun library, KDE interface for lensfun library
 *               http://lensfun.berlios.de
 *
 * Copyright (C) 2008 by Adrian Schroeter <adrian@suse.de>
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
 * ============================================================
 */

#include "klensfun.h"

#include <QByteArray>
#include <QGridLayout>
#include <QString>
#include <QCheckBox>
#include <KComboBox>
#include <KDebug>


Q_DECLARE_METATYPE( KLFDeviceSelector::DevicePtr );
Q_DECLARE_METATYPE( KLFDeviceSelector::LensPtr );

KLensFun::KLensFun()
{
    m_init = false;
    init();
};

KLensFun::~KLensFun()
{
    if ( m_init ){

    };
};

bool KLensFun::init()
{
    m_lfDb = lf_db_new ();
    m_lfDb->Load ();
    m_lfCameras = m_lfDb->GetCameras ();
    m_init = true;
    m_usedLens = NULL;

    m_filterCCA  = true;
    m_filterVig  = true;
    m_filterDist = true;

    return true;
};

void KLensFun::setCorrection( bool CCA, bool Vig, bool CCI, bool Dist, bool Geom )
{
    m_filterCCA  = CCA;
    m_filterVig  = Vig;
    m_filterCCI  = CCI;
    m_filterDist = Dist;
    m_filterGeom = Geom;
};

bool KLensFun::supportsDistortion()
{
  if ( m_usedLens == NULL ) return false;

  lfLensCalibDistortion res;
  return m_usedLens->InterpolateDistortion( m_focalLength, res );
};

bool KLensFun::supportsCCA()
{
  if ( m_usedLens == NULL ) return false;

  lfLensCalibTCA res;
  return m_usedLens->InterpolateTCA( m_focalLength, res );
};

bool KLensFun::supportsVig()
{
  if ( m_usedLens == NULL ) return false;

  lfLensCalibVignetting res;
  return m_usedLens->InterpolateVignetting( m_focalLength, m_aperature, m_subjectDistance, res );
};

#if 0
KLensFun::correctionData KLensFun::getCorrectionData()
{
};
#endif

#include <klocale.h>
#include <QWidget>

KLFDeviceSelector::KLFDeviceSelector( QWidget *parent ) : QWidget(parent)
{
    m_klf = new KLensFun();

    QGridLayout* gridSettings = new QGridLayout( this );

    m_ExifUsage = new QCheckBox( i18n("Use Exif Data"), parent);
    m_Maker = new KComboBox(parent);
    m_Model = new KComboBox(parent);
    m_Lens  = new KComboBox(parent);

    m_ExifUsage->setEnabled( false );
    m_ExifUsage->setCheckState( Qt::Unchecked );
    m_Maker->setInsertPolicy(QComboBox::InsertAlphabetically);
    m_Model->setInsertPolicy(QComboBox::InsertAlphabetically);
    m_Lens->setInsertPolicy(QComboBox::InsertAlphabetically);

    gridSettings->addWidget( m_ExifUsage );
    gridSettings->addWidget( m_Maker );
    gridSettings->addWidget( m_Model );
    gridSettings->addWidget( m_Lens );

    connect( m_ExifUsage, SIGNAL(stateChanged(int)), this, SLOT(exifUsageSlot(int)) );
    connect( m_Maker, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCombos()) );
    connect( m_Model, SIGNAL(currentIndexChanged(int)), this, SLOT(updateLensCombo(int)) );
    connect( m_Lens, SIGNAL(currentIndexChanged(int)), this, SLOT(selectLens()) );

    KLFDeviceSelector::Device firstDevice; // empty strings
//    setDevice( firstDevice );
};

KLFDeviceSelector::~KLFDeviceSelector()
{
    delete m_klf;
};

#if 0
KLFDeviceSelector::Device KLFDeviceSelector::getDevice()
{
};
#endif

void KLFDeviceSelector::findFromExif( KExiv2Iface::KExiv2 &meta )
{
    m_ExivMeta = meta;
    KLFDeviceSelector::findFromExif();
};

void KLFDeviceSelector::findFromExif()
{
//    KLFDeviceSelector::Device firstDevice; // empty strings
//    setDevice( firstDevice );
    m_ExifUsage->setCheckState( Qt::Checked );
    m_ExifUsage->setEnabled( true );

    QString Lens, Maker, Model;
    Maker = m_ExivMeta.getExifTagString("Exif.Image.Make");
    Model = m_ExivMeta.getExifTagString("Exif.Image.Model");

    // Not standarized, maybe such a thing should go to libexiv2 instead
    // please run "exiv2 -p t some_image_file.jpeg" and send me the data
    // of your camera, if this does not work for you, thanks. adrian@suse.de
    if ( Maker.toUpper() == "CANON" )
       Lens = m_ExivMeta.getExifTagString("Exif.Canon.0x0095");

    int makerIdx = m_Maker->findText( Maker );
    if ( makerIdx >= 0 ) {
      m_Maker->setCurrentIndex( makerIdx );
      m_Maker->setEnabled( false );
    };

    updateCombos();
    int modelIdx = m_Model->findText( Model );
    if ( modelIdx >= 0 ) {
      m_Model->setCurrentIndex( modelIdx );
      m_Model->setEnabled( false );
      updateLensCombo();
    };

    // The LensFun DB has the Maker in front of the Lens model name.
    // We use here the Camera Maker, because the Lens Maker seems not to be
    // part of the Exif data. This is of course bad for 3rd party lenses, but
    // they seem anyway not to have Exif entrys ususally :/
    int lensIdx = m_Lens->findText( Lens ); 
    if ( lensIdx < 0 )
       lensIdx = m_Lens->findText( Maker + " " + Lens ); 
    if ( lensIdx >= 0 ) {
       // found lens model directly, best case :)
       m_Lens->setCurrentIndex( lensIdx );
       m_Lens->setEnabled( false );
    } else {
       // Lens not found, try to reduce the list according to the values we have
       // FIXME: Implement removal of not matching lenses ...
       m_Lens->setEnabled( true );
    };
    kDebug() << "  >>>>>>search for Lens:" << Maker << " " << Lens << "< and found: >" << m_Lens->itemText(0) + "<";
    QString temp = m_ExivMeta.getExifTagString("Exif.Photo.FocalLength");
    m_klf->m_focalLength = temp.mid(0, temp.length() -3 ).toFloat(); // HACK: strip the " mm" at the end ... 
    m_klf->m_aperature = m_ExivMeta.getExifTagString("Exif.Photo.ApertureValue").mid(1).toFloat();
    m_klf->m_subjectDistance = m_ExivMeta.getExifTagString("Exif.CanonSi.SubjectDistance").toFloat();
    kDebug() << "  >>>>>>focalLength" << temp << "|" << m_klf->m_focalLength << "Aperatur" << m_klf->m_aperature << "Distance" << m_klf->m_subjectDistance;
};

void KLFDeviceSelector::exifUsageSlot(int mode)
{
    if ( mode == Qt::Checked )
      findFromExif();
    else {
      m_Maker->setEnabled( true );
      m_Model->setEnabled( true );
      m_Lens->setEnabled( true );
    };
};

void KLFDeviceSelector::updateCombos()
{
    const lfCamera* const *it = m_klf->m_lfCameras;

    // reset box
    m_Model->clear();
    
    bool firstRun = false;
    if ( m_Maker->count() == 0 )
       firstRun = true;

    while ( *it ) {
       if ( firstRun )
       {
           // Maker DB does not change, so we fill it only once.
           if ( (*it)->Maker ) {
              QString t( (*it)->Maker );
              if ( m_Maker->findText( t, Qt::MatchExactly ) < 0 )
                   m_Maker->addItem( t );
           };
       };
       // Fill models for current selected maker
       if ( (*it)->Model && (*it)->Maker == m_Maker->currentText() ) {
          KLFDeviceSelector::DevicePtr dev;
          dev = *it;
          QVariant b = qVariantFromValue(dev);
          m_Model->addItem( (*it)->Model, b );
       };
       it++;
    };

    // Fill Lens list for current Maker & Model
    updateLensCombo();
};

void KLFDeviceSelector::updateLensCombo()
{
    m_Lens->clear();
    const lfLens **lenses;

    QVariant v = m_Model->itemData( m_Model->currentIndex() );
    DevicePtr dev = v.value<KLFDeviceSelector::DevicePtr>();
    lenses = m_klf->m_lfDb->FindLenses( dev, NULL, NULL );

    m_klf->m_cropFactor = dev->CropFactor;

    while ( lenses && *lenses ) {
        KLFDeviceSelector::LensPtr lens;
        lens = *lenses;
        QVariant b = qVariantFromValue(lens);
        m_Lens->addItem( (*lenses)->Model, b );
        lenses++;
    };

    emit( lensSelected() );
};

void KLFDeviceSelector::selectLens()
{
    QVariant v = m_Lens->itemData( m_Lens->currentIndex() );
    m_klf->m_usedLens = v.value<KLFDeviceSelector::LensPtr>();

    if ( m_klf->m_cropFactor <= 0.0 ) // this should not happend
       m_klf->m_cropFactor = m_klf->m_usedLens->CropFactor;
}

void KLFDeviceSelector::setDevice( Device &d )
{
    updateCombos();
};

KLensFunFilter::KLensFunFilter(Digikam::DImg *orgImage, QObject *parent, KLensFun *klf)
              : Digikam::DImgThreadedFilter(orgImage, parent, "LensCorrection")
{
    m_klf = klf;
    m_parent = parent;

    initFilter();
}

void KLensFunFilter::filterImage(void)
{
#if 0
    if (!opts.Crop)
        opts.Crop = lens->CropFactor;
    if (!opts.Focal)
        opts.Focal = lens->MinFocal;
    if (!opts.Aperture)
        opts.Aperture = lens->MinAperture;
#endif

    int modifyFlags = 0;
    if ( m_klf->m_filterDist )
       modifyFlags &= LF_MODIFY_DISTORTION;
    if ( m_klf->m_filterGeom )
       modifyFlags &= LF_MODIFY_GEOMETRY;
    if ( m_klf->m_filterCCA )
       modifyFlags &= LF_MODIFY_TCA;
    if ( m_klf->m_filterVig )
       modifyFlags &= LF_MODIFY_VIGNETTING;
    if ( m_klf->m_filterCCI )
       modifyFlags &= LF_MODIFY_CCI;

    // Init lensfun lib, we are working on the full image.
    lfPixelFormat colorDepth = LF_PF_U8;
    switch( m_orgImage.bytesDepth() ){
      case 1:
        colorDepth = LF_PF_U8;
        break;
      case 2:
        colorDepth = LF_PF_U16;
        break;
      case 4:
        colorDepth = LF_PF_U32;
        break;
      default:
        kError() << "ERROR: can not handle bit depth.";
        return;
    };
    m_lfModifier = lfModifier::Create( m_klf->m_usedLens, m_klf->m_cropFactor, m_orgImage.width(), m_orgImage.height() );
    /*int modflags =*/ m_lfModifier->Initialize ( m_klf->m_usedLens, colorDepth, 
            m_klf->m_focalLength, m_klf->m_aperature, m_klf->m_subjectDistance,
            m_klf->m_cropFactor, LF_RECTILINEAR, modifyFlags, 0/*no inverse*/);

    // calc necessary steps for progress bar
    int steps = m_klf->m_filterCCA?1:0 + ( m_klf->m_filterVig || m_klf->m_filterCCI )?1:0
                + ( m_klf->m_filterDist || m_klf->m_filterGeom )?1:0;
    if ( steps < 1 )
       return;

    /*
     * The real correction work
     */
    int ok;
    int lwidth = m_orgImage.width() * 2 * 3;
    float *pos = new float [lwidth];

    // 1: TCA correction 
    if ( m_klf->m_filterCCA ) {
      for ( unsigned int y=0; y < m_orgImage.height(); y++ ){
        ok = m_lfModifier->ApplySubpixelDistortion (0.0, y, m_orgImage.width(), 1, pos);
        if (ok)
        {
          float *src = pos;
          for (unsigned x = 0; x < m_destImage.width(); x++)
          {
            Digikam::DColor destPixel;

            destPixel.setRed  ( m_orgImage.getPixelColor(src[0], src[1]).red() );
            destPixel.setGreen( m_orgImage.getPixelColor(src[2], src[3]).green() );
            destPixel.setBlue ( m_orgImage.getPixelColor(src[4], src[5]).blue() );

            m_destImage.setPixelColor(x, y, destPixel);
            src += 2 * 3;

            // Update progress bar in dialog.
            int progress = (int) (((double)y * 100.0) / m_orgImage.height() );
            if (m_parent && progress%5 == 0)
              postProgress(progress/steps);
          }
        } else
          kError() << "ERROR: Failed to call ApplySubpixelDistortion in lensfun lib !";
      }
    } else
       m_destImage.bitBltImage(&m_orgImage, 0, 0);

    // 2: Color Correction: Vignetting and CCI
    uchar *data       = m_destImage.bits();
    if ( m_klf->m_filterVig || m_klf->m_filterCCI ) {
      float offset = 0.0;
      if ( steps == 3 )
        offset = 33.3;
      else if ( steps == 2 && m_klf->m_filterCCA == true )
        offset = 50.0;
      for ( unsigned int y=0; y < m_destImage.height(); y++ ){
        if ( ! m_lfModifier->ApplyColorModification (data, 0.0, y, m_destImage.width(), 1, m_destImage.bytesDepth(), 0) )
          kError() << "ERROR: Failed to call ApplyColorModification in lensfun lib !";
        data += m_destImage.height() * m_destImage.bytesDepth();

        // Update progress bar in dialog.
        int progress = (int) (((double)y * 100.0) / m_destImage.height() );
        if (m_parent && progress%5 == 0)
          postProgress(progress/steps + offset);
      };
    };

    // 3: Distortion and Geometry
    if ( m_klf->m_filterDist || m_klf->m_filterGeom ) {
      // we need a deep copy first 
      Digikam::DImg tempImage;
      tempImage = m_destImage;
      for ( unsigned long y=0; y < tempImage.height(); y++ ){
        ok = m_lfModifier->ApplyGeometryDistortion (0.0, y, tempImage.width(), 1, pos);
        if (ok)
        {
          float *src = pos;
          for (unsigned long x = 0; x < tempImage.width(); x++)
          {
//qDebug (" ZZ %f %f %i %i", src[0], src[1], src[0], src[1]);

              m_destImage.setPixelColor(x, y, tempImage.getPixelColor( src[0], src[1]) );
              src += 2;
          }

          // Update progress bar in dialog.
          int progress = (int) (((double)y * 100.0) / tempImage.height() );
          if (m_parent && progress%5 == 0)
            postProgress(progress/steps + 33.3*(steps-1));
        } else
          kError() << "ERROR: Failed to call ApplyGeometryDistortion  in lensfun lib !";
      }
//qDebug (" for %f %f %i %i", tempImage.height(), tempImage.width(), tempImage.height(), tempImage.width());
    }

    // clean up
    m_lfModifier->Destroy();
}

