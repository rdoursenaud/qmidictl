// qmidictlMidiControlForm.h
//
/****************************************************************************
   Copyright (C) 2010, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qmidictlMidiControlForm_h
#define __qmidictlMidiControlForm_h

#include "ui_qmidictlMidiControlForm.h"


//----------------------------------------------------------------------------
// qmidictlMidiControlForm -- UI wrapper form.

class qmidictlMidiControlForm : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qmidictlMidiControlForm(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

protected slots:

	void activateCommand(const QString&);
	void activateControlType(const QString&);
	void change();

	void click(QAbstractButton *);

	void accept();
	void reject();
	void reset();

private:

	// The Qt-designer UI struct...
	Ui::qmidictlMidiControlForm m_ui;

	// Instance variables.
	int m_iDirtyCount;
	int m_iDirtySetup;
};


#endif	// __qmidictlMidiControlForm_h


// end of qmidictlMidiControlForm.h
