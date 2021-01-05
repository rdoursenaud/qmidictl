// qmidictlDialStyle.h
//
/****************************************************************************
   Copyright (C) 2010-2021, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __qmidictlDialStyle_h
#define __qmidictlDialStyle_h

#include <QCommonStyle>


class qmidictlDialStyle : public QCommonStyle
{
public:

	void drawComplexControl(
		ComplexControl cc, const QStyleOptionComplex *optc,
		QPainter *p, const QWidget *widget = 0) const;
};


#endif	// __qmidictlDialStyle_h

// end of qmidictlDialStyle.h
