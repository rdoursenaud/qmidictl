// qmidictlAbout.h
//
/****************************************************************************
   Copyright (C) 2010-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qmidictlAbout_h
#define __qmidictlAbout_h

#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32) && !defined(Q_OS_SYMBIAN)
#include "config.h"
#define QMIDICTL_TITLE        PACKAGE_NAME
#else
#define QMIDICTL_TITLE        "QmidiCtl"
#endif

#define QMIDICTL_SUBTITLE     "A MIDI Remote Controller via UDP/IP Multicast"
#define QMIDICTL_WEBSITE      "https://qmidictl.sourceforge.io"
#define QMIDICTL_COPYRIGHT    "Copyright (C) 2010-2023, rncbc aka Rui Nuno Capela. All rights reserved."

#define QMIDICTL_DOMAIN       "rncbc.org"

#endif  // __qmidictlAbout_h

// end of qmidictlAbout.h

