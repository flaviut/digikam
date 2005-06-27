/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2004-02-14
 * Description : 
 * 
 * Copyright 2004 by Renchi Raju
 * Copyright 2005 by Gilles Caulier
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

#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

// Qt includes.

#include <qwidget.h>

namespace Digikam
{

class ImageIface;

class ImageWidget : public QWidget
{
public:

    ImageWidget(int width, int height, QWidget *parent=0);
    ~ImageWidget();

    ImageIface* imageIface();
    void updateImage(void);

protected:

    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent * e);
    
private:

    uint       *m_data;
    int         m_w;
    int         m_h;
    
    QRect       m_rect;
    
    ImageIface *m_iface;
    
};

}

#endif /* IMAGEWIDGET_H */
