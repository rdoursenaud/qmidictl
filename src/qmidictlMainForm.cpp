// qmidictlMainForm.cpp
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

#include "qmidictlAbout.h"
#include "qmidictlMainForm.h"
#include "qmidictlOptionsForm.h"

#include "qmidictlOptions.h"
#include "qmidictlUdpDevice.h"
#include "qmidictlDialStyle.h"

#include <QTimer>

#include <QMessageBox>


//----------------------------------------------------------------------------
// qmidictlMainForm -- UI wrapper form.
//

qmidictlMainForm *qmidictlMainForm::g_pMainForm = NULL;

// Constructor.
qmidictlMainForm::qmidictlMainForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QMainWindow(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// The main network device.
	m_pUdpDevice = new qmidictlUdpDevice(this);

	// Strip page/states.
	m_iStripPages = 4;
	m_pStripStates = NULL;
	m_iCurrentStripPage = 0;

	// Jog shuttle last known state.
	m_iJogShuttleDelta = 0;
	m_iJogShuttleValue = 0;

	// Activity LED counters.
	m_iMidiInLed  = 0;
	m_iMidiOutLed = 0;

	// Kind of soft-mutex.
	m_iBusy = 0;

	// Hack the jog-shuttle dial style and palette...
	m_pDialStyle = new qmidictlDialStyle();
	m_ui.jogShuttleDial->setStyle(m_pDialStyle);

	// Pseudo-singleton reference setup.
	g_pMainForm = this;

	// Initialize strip connections...
	initMixerStrips();

	// Main menu action connections.
	QObject::connect(m_ui.optionsAction,
		SIGNAL(triggered(bool)),
		SLOT(optionsSlot()));
	QObject::connect(m_ui.aboutAction,
		SIGNAL(triggered(bool)),
		SLOT(aboutSlot()));
	QObject::connect(m_ui.aboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(aboutQtSlot()));
	QObject::connect(m_ui.exitAction,
		SIGNAL(triggered(bool)),
		SLOT(exitSlot()));

	// UI widgets signal/slot connections...
	QObject::connect(m_ui.prevStripPageButton,
		SIGNAL(clicked()),
		SLOT(prevStripPageSlot()));
	QObject::connect(m_ui.nextStripPageButton,
		SIGNAL(clicked()),
		SLOT(nextStripPageSlot()));

	QObject::connect(m_ui.jogShuttleDial,
		SIGNAL(valueChanged(int)),
		SLOT(jogShuttleSlot(int)));

	QObject::connect(m_ui.resetButton,
		SIGNAL(clicked()),
		SLOT(resetSlot()));
	QObject::connect(m_ui.rewindButton,
		SIGNAL(toggled(bool)),
		SLOT(rewindSlot(bool)));
	QObject::connect(m_ui.playButton,
		SIGNAL(toggled(bool)),
		SLOT(playSlot(bool)));
	QObject::connect(m_ui.stopButton,
		SIGNAL(clicked()),
		SLOT(stopSlot()));
	QObject::connect(m_ui.recordButton,
		SIGNAL(toggled(bool)),
		SLOT(recordSlot(bool)));
	QObject::connect(m_ui.forwardButton,
		SIGNAL(toggled(bool)),
		SLOT(forwardSlot(bool)));

	// Network signal/slot connections...
	QObject::connect(m_pUdpDevice,
		SIGNAL(received(const QByteArray&)),
		SLOT(receiveSlot(const QByteArray&)));
}


// Destructor.
qmidictlMainForm::~qmidictlMainForm (void)
{
	// No need for fancy styling no more.
	delete m_pDialStyle;

	// No more strip states.
	delete [] m_pStripStates;

	// No more network device.
	delete m_pUdpDevice;

	// Pseudo-singleton reference shut-down.
	g_pMainForm = NULL;
}


// Kind of singleton reference.
qmidictlMainForm *qmidictlMainForm::getInstance (void)
{
	return g_pMainForm;
}


// Mixer strip page accessors.
void qmidictlMainForm::setCurrentStripPage ( int iStripPage )
{
	if (iStripPage < 0 || iStripPage > m_iStripPages - 1)
		return;

	if (iStripPage != m_iCurrentStripPage)
		saveStripPage(m_iCurrentStripPage);

	m_iCurrentStripPage = iStripPage;

	loadStripPage(m_iCurrentStripPage);
	stabilizeForm();
}

int qmidictlMainForm::currentStripPage (void) const
{
	return m_iCurrentStripPage;
}


// Setup method.
void qmidictlMainForm::setup (void)
{
	qmidictlOptions *pOptions = qmidictlOptions::getInstance();
	if (pOptions == NULL)
		return;

	if (!m_pUdpDevice->open(pOptions->sInterface, pOptions->iUdpPort)) {
		QMessageBox::critical(this, tr("Network Inferface Error"),
			tr("The network interface could not be established.\n\n"
			"Please, make sure you have an on-line network connection "
			"and try again."));
	}
}


// Initialize mixer strips.
void qmidictlMainForm::initMixerStrips (void)
{
	initStripStates();

	initMixerStrip(m_ui.strip1);
	initMixerStrip(m_ui.strip2);
	initMixerStrip(m_ui.strip3);
	initMixerStrip(m_ui.strip4);

	setCurrentStripPage(0);
}

void qmidictlMainForm::initMixerStrip ( qmidictlMixerStrip *pStrip )
{
	QObject::connect(pStrip,
		SIGNAL(recordToggled(int, bool)),
		SLOT(stripRecordSlot(int, bool)));
	QObject::connect(pStrip,
		SIGNAL(muteToggled(int, bool)),
		SLOT(stripMuteSlot(int, bool)));
	QObject::connect(pStrip,
		SIGNAL(soloToggled(int, bool)),
		SLOT(stripSoloSlot(int, bool)));
	QObject::connect(pStrip,
		SIGNAL(sliderChanged(int, int)),
		SLOT(stripSliderSlot(int, int)));
}


// Strip states methods.
void qmidictlMainForm::initStripStates (void)
{
	int iStrips = 4 * m_iStripPages;

	m_pStripStates = new StripState [iStrips];

	for (int iStrip = 0; iStrip < iStrips; ++iStrip) {
		StripState *pState = &m_pStripStates[iStrip];
		pState->strip  = iStrip;
		pState->record = false;
		pState->mute   = false;
		pState->solo   = false;
		pState->slider = false;
	}
}


void qmidictlMainForm::saveStripState ( qmidictlMixerStrip *pStrip, int iStrip )
{
	StripState *pState = &m_pStripStates[iStrip];

	pState->strip  = pStrip->strip();
	pState->record = pStrip->isRecord();
	pState->mute   = pStrip->isMute();
	pState->solo   = pStrip->isSolo();
	pState->slider = pStrip->slider();
}


void qmidictlMainForm::loadStripState ( qmidictlMixerStrip *pStrip, int iStrip )
{
	StripState *pState = &m_pStripStates[iStrip];

	pStrip->setStrip(pState->strip,
		pState->record,
		pState->mute,
		pState->solo,
		pState->slider);
}


void qmidictlMainForm::saveStripPage ( int iStripPage )
{
	int iStrip = (iStripPage * 4);

	saveStripState(m_ui.strip1, iStrip + 0);
	saveStripState(m_ui.strip2, iStrip + 1);
	saveStripState(m_ui.strip3, iStrip + 2);
	saveStripState(m_ui.strip4, iStrip + 3);
}


void qmidictlMainForm::loadStripPage ( int iStripPage )
{
	int iStrip = (iStripPage * 4);

	loadStripState(m_ui.strip1, iStrip + 0);
	loadStripState(m_ui.strip2, iStrip + 1);
	loadStripState(m_ui.strip3, iStrip + 2);
	loadStripState(m_ui.strip4, iStrip + 3);
}


// Common form stabilizer.
void qmidictlMainForm::stabilizeForm(void)
{
	m_ui.prevStripPageButton->setEnabled(m_iCurrentStripPage > 0);
	m_ui.nextStripPageButton->setEnabled(m_iCurrentStripPage < m_iStripPages - 1);
}


// Mixer strip slots.
void qmidictlMainForm::prevStripPageSlot (void)
{
	setCurrentStripPage(currentStripPage() - 1);
}


void qmidictlMainForm::nextStripPageSlot (void)
{
	setCurrentStripPage(currentStripPage() + 1);
}


// MMC Command dispatcher.
void qmidictlMainForm::sendMmcCommand (
	MmcCommand cmd, unsigned char *data, unsigned short len )
{
	// Build up the MMC sysex message...
	unsigned char *pSysex;
	unsigned short iSysex = 6;

	if (data && len > 0)
		iSysex += 1 + len;
	pSysex = new unsigned char [iSysex];
	iSysex = 0;

	pSysex[iSysex++] = 0xf0; // Sysex header.
	pSysex[iSysex++] = 0x7f; // Realtime sysex.
	pSysex[iSysex++] = qmidictlOptions::getInstance()->iMmcDevice;
	pSysex[iSysex++] = 0x06; // MMC command mode.
	pSysex[iSysex++] = (unsigned char) cmd;	// MMC command code.
	if (data && len > 0) {
		pSysex[iSysex++] = len;
		::memcpy(&pSysex[iSysex], data, len);
		iSysex += len;
	}
	pSysex[iSysex++] = 0xf7; // Sysex trailer.

	// Go.
	sendData(pSysex, iSysex);

	// Done.
	delete pSysex;
}


// MMC dispatch special commands.
void qmidictlMainForm::sendMmcMaskedWrite (
	MmcSubCommand scmd, int iTrack, bool bOn )
{
	unsigned char data[4];
	int iMask = (1 << (iTrack < 2 ? iTrack + 5 : (iTrack - 2) % 7));

	data[0] = scmd;
	data[1] = (unsigned char) (iTrack < 2 ? 0 : 1 + (iTrack - 2) / 7);
	data[2] = (unsigned char) iMask;
	data[3] = (unsigned char) (bOn ? iMask : 0);

	sendMmcCommand(MMC_MASKED_WRITE, data, sizeof(data));
}


void qmidictlMainForm::sendMmcLocate ( unsigned long iLocate )
{
	unsigned char data[6];

	data[0] = 0x01;
	data[1] = iLocate / (3600 * 30); iLocate -= (3600 * 30) * (int) data[1];
	data[2] = iLocate / (  60 * 30); iLocate -= (  60 * 30) * (int) data[2];
	data[3] = iLocate / (       30); iLocate -= (       30) * (int) data[3];
	data[4] = iLocate;
	data[5] = 0;

	sendMmcCommand(MMC_LOCATE, data, sizeof(data));
}


// Network transmitter.
void qmidictlMainForm::sendData ( unsigned char *data, unsigned short len )
{
#if defined(DEBUG)
	fprintf(stderr, "SEND:");
	for(int i = 0; i < len; ++i)
		fprintf(stderr, " %02X", data[i]);
	fprintf(stderr, "\n");
#endif

	if (m_pUdpDevice->sendData(data, len)) {
		if (++m_iMidiOutLed < 2) {
			m_ui.midiOutLedLabel->setPixmap(QPixmap(":/images/ledOn.png"));
			QTimer::singleShot(200, this, SLOT(timerSlot()));
		}
	}
}


// Mixer strip slots.
void qmidictlMainForm::stripRecordSlot ( int iStrip, bool bOn )
{
	// Update strip state...
	if (iStrip >= 0 && iStrip < (4 * m_iStripPages))
		m_pStripStates[iStrip].record = bOn;

	sendMmcMaskedWrite(MMC_TRACK_RECORD, iStrip, bOn);
}


void qmidictlMainForm::stripMuteSlot ( int iStrip, bool bOn )
{
	// Update strip state...
	if (iStrip >= 0 && iStrip < (4 * m_iStripPages))
		m_pStripStates[iStrip].mute = bOn;

	sendMmcMaskedWrite(MMC_TRACK_MUTE, iStrip, bOn);
}


void qmidictlMainForm::stripSoloSlot ( int iStrip, bool bOn )
{
	// Update strip state...
	if (iStrip >= 0 && iStrip < (4 * m_iStripPages))
		m_pStripStates[iStrip].solo = bOn;

	sendMmcMaskedWrite(MMC_TRACK_SOLO, iStrip, bOn);
}


void qmidictlMainForm::stripSliderSlot ( int iStrip, int iValue )
{
	// Update strip state...
	if (iStrip >= 0 && iStrip < (4 * m_iStripPages))
		m_pStripStates[iStrip].slider = iValue;

	// Special slider (will send raw controller event...
	unsigned char data[3];

	data[0] = 0xbf; // Channel 16 controller.
	data[1] = 0x40 + (iStrip & 0x3f);
	data[2] = (unsigned char) ((127 * iValue) / 100);

	sendData(data, sizeof(data));
}


// Jog wheel slot.
void qmidictlMainForm::jogShuttleSlot ( int iValue )
{
	int iDelta = (iValue - m_iJogShuttleValue);
	if ((iDelta * m_iJogShuttleDelta) < 0)
		iDelta = -(m_iJogShuttleDelta);

	unsigned char data;
	if (iDelta < 0) {
		data = (-iDelta & 0x3f) | 0x40;
	} else {
		data = (+iDelta & 0x3f);
	}

	sendMmcCommand(MMC_STEP, &data, sizeof(data));

	m_iJogShuttleValue = iValue;
	m_iJogShuttleDelta = iDelta;
}


// Reset action slot
void qmidictlMainForm::resetSlot (void)
{
	sendMmcLocate(0);
}


// Rewind action slot
void qmidictlMainForm::rewindSlot ( bool bOn )
{
	if (m_iBusy > 0)
		return;

	m_iBusy++;
	m_ui.forwardButton->setChecked(false);
	m_iBusy--;

	sendMmcCommand(bOn ? MMC_REWIND : MMC_STOP);
}


// Start/Play action slot
void qmidictlMainForm::playSlot ( bool bOn )
{
	if (m_iBusy > 0)
		return;

	sendMmcCommand(bOn ? MMC_PLAY : MMC_PAUSE);
}


// Stop action slot
void qmidictlMainForm::stopSlot (void)
{
	if (m_iBusy > 0)
		return;

	m_iBusy++;
	m_ui.rewindButton->setChecked(false);
	m_ui.forwardButton->setChecked(false);
	m_ui.recordButton->setChecked(false);
	m_ui.playButton->setChecked(false);
	m_iBusy--;

	sendMmcCommand(MMC_STOP);
}


// Record action slot
void qmidictlMainForm::recordSlot ( bool bOn )
{
	if (m_iBusy > 0)
		return;

	sendMmcCommand(bOn ? MMC_RECORD_STROBE : MMC_RECORD_EXIT);
}


// Forward action slot
void qmidictlMainForm::forwardSlot ( bool bOn )
{
	if (m_iBusy > 0)
		return;

	m_iBusy++;
	m_ui.rewindButton->setChecked(false);
	m_iBusy--;

	sendMmcCommand(bOn ? MMC_FAST_FORWARD : MMC_STOP);
}


// Network listener/receiver slot.
void qmidictlMainForm::receiveSlot ( const QByteArray& data )
{
	recvData((unsigned char *) data.constData(), data.length());
}

void qmidictlMainForm::recvData ( unsigned char *data, unsigned short len )
{
	m_iBusy++;

#if defined(DEBUG)
	fprintf(stderr, "RECV:");
	for(int i = 0; i < len; ++i)
		fprintf(stderr, " %02X", data[i]);
	fprintf(stderr, "\n");
#endif

	// Flash the MIDI In LED...
	if (++m_iMidiInLed < 2) {
		m_ui.midiInLedLabel->setPixmap(QPixmap(":/images/ledOn.png"));
		QTimer::singleShot(200, this, SLOT(timerSlot()));
	}

	// Handle immediate incoming MIDI data...
	int iTracks = (4 * m_iStripPages);
	int iTrack  = -1;
	
	// SysEx (actually)...
	if (data[0] == 0xf0 && data[len - 1] == 0xf7) {
		// MMC command...
		unsigned char mmcid = qmidictlOptions::getInstance()->iMmcDevice;
		if (data[1] == 0x7f && data[3] == 0x06
			&& (mmcid == 0x7f || data[2] == mmcid)) {
			MmcCommand cmd = MmcCommand(data[4]);
			switch (cmd) {
			case MMC_STOP:
			case MMC_RESET:
				m_ui.rewindButton->setChecked(false);
				m_ui.playButton->setChecked(false);
				m_ui.recordButton->setChecked(false);
				m_ui.forwardButton->setChecked(false);
				break;
			case MMC_PLAY:
			case MMC_DEFERRED_PLAY:
				m_ui.playButton->setChecked(true);
				break;
			case MMC_REWIND:
				m_ui.rewindButton->setChecked(true);
				m_ui.forwardButton->setChecked(false);
				break;
			case MMC_FAST_FORWARD:
				m_ui.rewindButton->setChecked(false);
				m_ui.forwardButton->setChecked(true);
				break;
			case MMC_RECORD_STROBE:
				m_ui.recordButton->setChecked(true);
				break;
			case MMC_RECORD_EXIT:
			case MMC_RECORD_PAUSE:
				m_ui.recordButton->setChecked(false);
				break;
			case MMC_PAUSE:
				m_ui.playButton->setChecked(false);
				break;
			case MMC_MASKED_WRITE:
				if (int(data[5]) > 3) {
					MmcSubCommand scmd = MmcSubCommand(data[6]);
					iTrack = (data[7] > 0 ? (data[7] * 7) : 0) - 5;
					bool bOn = false;
					for (int i = 0; i < 7; ++i) {
						int iMask = (1 << i);
						if (data[8] & iMask) {
							bOn = (data[9] & iMask);
							break;
						}
						++iTrack;
					}
					// Patch corresponding track/strip state...
					if (iTrack >= 0 && iTrack < iTracks) {
						switch (scmd) {
						case MMC_TRACK_RECORD:
							m_pStripStates[iTrack].record = bOn;
							break;
						case MMC_TRACK_MUTE:
							m_pStripStates[iTrack].mute = bOn;
							break;
						case MMC_TRACK_SOLO:
							m_pStripStates[iTrack].solo = bOn;
							break;
						default:
							break;
						}
					}
				}
				// Fall thru...
			default:
				break;
			}
		}
	}
	else
	// Channel Controller...
	if (data[0] == 0xbf && len > 2) {
		iTrack = int(data[1]) - 0x40;
		int iValue = (100 * int(data[2])) / 127;
		// Patch corresponding track/strip state...
		if (iTrack >= 0 && iTrack < iTracks)
			m_pStripStates[iTrack].slider = iValue;
	}

	// Update corresponding strip state, when currently visible...
	int iStrip = (4 * m_iCurrentStripPage);
	if (iTrack >= iStrip && iTrack < iStrip + 4) {
		switch (iTrack % 4) {
		case 0: loadStripState(m_ui.strip1, iTrack); break;
		case 1: loadStripState(m_ui.strip2, iTrack); break;
		case 2: loadStripState(m_ui.strip3, iTrack); break;
		case 3: loadStripState(m_ui.strip4, iTrack); break;
		}
	}

	// Done.
	m_iBusy--;
}


// Timer slot funtion.
void qmidictlMainForm::timerSlot (void)
{
	// Handle pending incoming MIDI data...
	if (m_iMidiInLed > 0) {
		m_iMidiInLed = 0;
		m_ui.midiInLedLabel->setPixmap(QPixmap(":/images/ledOff.png"));
	}

	// Handle pending outgoing MIDI data...
	if (m_iMidiOutLed > 0) {
		m_iMidiOutLed = 0;
		m_ui.midiOutLedLabel->setPixmap(QPixmap(":/images/ledOff.png"));
	}
}


// Options action slot.
void qmidictlMainForm::optionsSlot (void)
{
	if (qmidictlOptionsForm(this).exec())
		setup();
}


// About dialog.
void qmidictlMainForm::aboutSlot (void)
{
	// Stuff the about box text...
	QString sText = "<p>\n";
	sText += "<b>" QMIDICTL_TITLE " - " + tr(QMIDICTL_SUBTITLE) + "</b><br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" QMIDICTL_VERSION "</b><br />\n";
	sText += "<small>" + tr("Build") + ": " __DATE__ " " __TIME__ "</small><br />\n";
	sText += "<br />\n";
	sText += tr("Website") + ": <a href=\"" QMIDICTL_WEBSITE "\">" QMIDICTL_WEBSITE "</a><br />\n";
	sText += "<br />\n";
	sText += "<small>";
	sText += QMIDICTL_COPYRIGHT "<br />\n";
	sText += "<br />\n";
	sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
	sText += tr("under the terms of the GNU General Public License version 2 or later.");
	sText += "</small>";
	sText += "</p>\n";

	QMessageBox::about(this, tr("About %1").arg(QMIDICTL_TITLE), sText);
}


// About Qt dialog.
void qmidictlMainForm::aboutQtSlot (void)
{
	QApplication::aboutQt();
}


// Exit/quit action slot.
void qmidictlMainForm::exitSlot (void)
{
	close();
}


// end of qmidictlMainForm.cpp
