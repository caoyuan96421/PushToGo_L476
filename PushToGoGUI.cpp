/*
 * PushToGoGUI.cpp
 *
 *  Created on: Aug 24, 2019
 *      Author: caoyu
 */

#include "PushToGoGUI.h"
#include "CelestialMath.h"
#include "TelescopeConfiguration.h"
#include "printf.h"
#include <stdlib.h>

#define BTN_STATE_CHANGED		0x01
#define BTN_LONGPRESS_INC		0x02
#define BTN_LONGPRESS_DEC		0x04
#define BTN_RELEASE				0x08

#define ACTION_DISPLAY_DWELL_FRAME	5

static void save_config() {
	TelescopeConfiguration::saveConfig_NV();
}

struct tm getLocalTime(time_t timestamp) {
	if (!timestamp)
		timestamp = time(NULL);
	time_t ts = timestamp + TelescopeConfiguration::getInt("timezone") * 3600;
	struct tm tts;
#if !( defined(__ARMCC_VERSION) || defined(__CC_ARM) )
	gmtime_r(&ts, &tts);
#else
	core_util_critical_section_enter();
	memcpy(&tts, gmtime(&ts), sizeof(struct tm));
	core_util_critical_section_exit();
#endif
	return tts;
}

PushToGo_GUI::PushToGo_GUI(LCD1602 *lcd, EquatorialMount *eq) :
		lcd(lcd), currentDisplayMenu(&homeMenu), thread(osPriorityBelowNormal,
		OS_STACK_SIZE, NULL, "PushToGo_GUI"), thd_btn_poll(
				osPriorityBelowNormal,
				512, NULL, "button_poll"), guiRunning(false), eqMount(
				eq), prev_idle_time(0), button_state(0), button_last_update(0), homeMenu(
				this) {
	MBED_ASSERT(lcd != NULL);
	MBED_ASSERT(eq != NULL);
	lcd->clear();
	thread.start(callback(this, &PushToGo_GUI::_task_entry));
	thd_btn_poll.start(callback(this, &PushToGo_GUI::_poll_entry));

	// Set LCD contrast
	if (!TelescopeConfiguration::isConfigExist("lcd_contrast")) {
		TelescopeConfiguration::setInt("lcd_contrast", 50);
	}
	lcd->setBrightness(TelescopeConfiguration::getInt("lcd_contrast") / 100.0F);

	constructMenuStructure(&homeMenu);

	homeMenu.menuBack.itemTitle = "   Save & Return";
	homeMenu.menuBack.returned = save_config;

	struct _quickReturnMenu : public PushToGo_MenuItem_Base {
		void switchTo(PushToGo_GUI *gui){
			gui->currentDisplayMenu = &gui->homeMenu;
		}
	} *_qr = new _quickReturnMenu();

	quickMenu.add(_qr);
	quickMenu.parent = &homeMenu;
	quickMenu.firstChild->prev = quickMenu.firstChild;
}

PushToGo_GUI::~PushToGo_GUI() {
	// Should never end
	// Release memory
	thread.terminate();
	thd_btn_poll.terminate();
	lcd->clear();
}

void PushToGo_GUI::startGUI() {
	guiRunning = true;
}

void PushToGo_GUI::stopGUI() {
	guiRunning = false;
	lcd->clear();
}

void PushToGo_GUI::_task_entry() {
	while (true) {
		// Poll for event
		osEvent osevt;
		osevt = btnEvtQueue.get(100);
		if (osevt.status == osEventMessage) {
			ButtonEvent evt = (ButtonEvent) osevt.value.v;

			if (!currentDisplayMenu->reactToButton(evt, this)) {
				// Menu structure actions
				switch (evt) {
				case BUTTON_EVENT_DEC:
					if (currentDisplayMenu->next) {
						currentDisplayMenu->switchAway(this);
						currentDisplayMenu = currentDisplayMenu->next;
						currentDisplayMenu->switchTo(this);
					} else if (currentDisplayMenu->parent) { // End of menu, wrap to beginning
						currentDisplayMenu->switchAway(this);
						currentDisplayMenu =
								currentDisplayMenu->parent->firstChild;
						currentDisplayMenu->switchTo(this);
					}
					break;
				case BUTTON_EVENT_INC:
					if (currentDisplayMenu->prev) {
						currentDisplayMenu->switchAway(this);
						currentDisplayMenu = currentDisplayMenu->prev;
						currentDisplayMenu->switchTo(this);
					} else if (currentDisplayMenu->parent) { // Beginning of menu, wrap to end
						currentDisplayMenu->switchAway(this);
						while (currentDisplayMenu->next)
							currentDisplayMenu = currentDisplayMenu->next;
						currentDisplayMenu->switchTo(this);
					}
					break;
				case BUTTON_EVENT_ENTER:
					if (currentDisplayMenu->itemType
							== PushToGo_MenuItem::MENU_TYPE_SUBMENU) {
						currentDisplayMenu->switchAway(this);
						currentDisplayMenu = currentDisplayMenu->firstChild;
						currentDisplayMenu->switchTo(this);
					}
					break;
				default:
					break;
				}
			}
		}

		// Update GUI
		if (guiRunning) {
			currentDisplayMenu->showMenu(this);
		}
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Home::showMenu(PushToGo_GUI *gui) {
	LCD1602 *lcd = gui->lcd;
	char buf[64];
	// Display home screen, main screen
	if (dwell_sec == 0) {
		const EquatorialCoordinates &eq =
				gui->eqMount->getEquatorialCoordinates();
		char we = (eq.ra > 0) ? 'E' : 'W';
		char ns = (eq.dec > 0) ? 'N' : 'S';
		double r = (eq.ra < 0) ? eq.ra + 360.0 : eq.ra;
		double d = fabs(eq.dec);
		int len;

		// RA string
		len = sprintf(buf, "%2dh%02d'%02d\"%c", int(r / 15),
				int(fmod(r, 15.0) * 4), (int) floor(fmod(r, 0.25) * 240), we);
		lcd->setPosition(0, 0);
		lcd->write(buf, len);
		lcd->fillWith(' ', 16 - len);

		// DEC string
		len = sprintf(buf, "%2d\xDF%02d'%02d\"%c", int(d),
				int(fmod(d, 1.0) * 60), (int) floor(fmod(d, 1.0 / 60) * 3600),
				ns);
		lcd->setPosition(1, 0);
		lcd->write(buf, len);
		lcd->fillWith(' ', 16 - len);

		// Mount status
		lcd->setPosition(1, 15);
		mountstatus_t status = gui->eqMount->getStatus();
		char st;
		switch (status) {
		case MOUNT_STOPPED:
			st = ' ';
			break;
		case MOUNT_SLEWING:
		case MOUNT_NUDGING:
		case MOUNT_NUDGING_TRACKING:
			st = 'S';
			break;

		case MOUNT_TRACKING:
		case MOUNT_TRACKING_GUIDING:
			st = 'T';
			break;
		default:
			break;
		}
		lcd->write(&st, 1);

		// CPU usage
		lcd->setPosition(0, 12);
		int usage = (int) 100 * gui->calc_cpu_usage();

		lcd->print("%3d%%", usage);
	} else {
		dwell_sec--;
		if (dwell_sec == 0){
			// Show quick menu
			gui->currentDisplayMenu = gui->quickMenu.firstChild;
			gui->currentDisplayMenu->switchTo(gui);
		}

		// Display current time/date
		struct tm tts = getLocalTime();

		// Display time
		int len = strftime(buf, sizeof(buf), "%T", &tts);

		lcd->setPosition(0, 0);
		lcd->write(buf, len);
		lcd->fillWith(' ', 16 - len);

		lcd->setPosition(0, 12);
		lcd->print("U%+03d", TelescopeConfiguration::getInt("timezone"));

		// Display date
		len = strftime(buf, sizeof(buf), "%b %d %Y  %a", &tts);
		lcd->setPosition(1, 0);
		lcd->write(buf, len);
	}
}

bool PushToGo_GUI::PushToGo_MenuItem_Home::reactToButton(ButtonEvent evt,
		PushToGo_GUI *gui) {
	if (evt == BUTTON_EVENT_DEC) {
		dwell_sec = 15;
		return true;
	} else if (evt == BUTTON_EVENT_INC) {
		mountstatus_t status = gui->eqMount->getStatus();
		if (status == MOUNT_STOPPED) {
			gui->eqMount->startTracking();
		} else {
			// Emergency stop
			gui->eqMount->stopAsync();
		}
		return true;
	} else if (evt == BUTTON_EVENT_RELEASE) {
		dwell_sec = 0;
		return true;
	}
	else
		return false;
}

float PushToGo_GUI::calc_cpu_usage() {
	mbed_stats_cpu_t stats;
	mbed_stats_cpu_get(&stats);

	uint64_t idle = (stats.idle_time - prev_idle_time);
	uint64_t total = (stats.uptime - prev_up_time);
	prev_idle_time = stats.idle_time;
	prev_up_time = stats.uptime;

	return 1.0F - (float) idle / total;
}

void PushToGo_GUI::buttonStateChanged(int oldstate, int newstate) {
	int change = oldstate ^ newstate;
	button_last_update = osKernelGetTickCount();
	button_state = newstate;

	if ((change & BUTTON_INC) && newstate == BUTTON_INC) {
		// INC pressed (single key)
		btnEvtQueue.put((void*) BUTTON_EVENT_INC, 0);
		thd_btn_poll.flags_set(BTN_LONGPRESS_INC); // Wake up poll thread
	} else if ((change & BUTTON_DEC) && newstate == BUTTON_DEC) {
		// DEC pressed (single key)
		btnEvtQueue.put((void*) BUTTON_EVENT_DEC, 0);
		thd_btn_poll.flags_set(BTN_LONGPRESS_DEC); // Wake up poll thread
	} else if ((change & BUTTON_ENTER) && newstate == BUTTON_ENTER) {
		// ENTER pressed (single key)
		btnEvtQueue.put((void*) BUTTON_EVENT_ENTER, 0);
	} else if (newstate == 0) {
		thd_btn_poll.flags_set(BTN_RELEASE); // Signal state change
		btnEvtQueue.put((void*) BUTTON_EVENT_RELEASE, 0);
	} else {
		thd_btn_poll.flags_set(BTN_STATE_CHANGED); // Signal state change
	}
}

void PushToGo_GUI::_poll_entry() {
	ThisThread::flags_clear(0x7FFFFFFF);
	while (true) {
		int flag = ThisThread::flags_wait_any(
		BTN_LONGPRESS_INC | BTN_LONGPRESS_DEC); // Suspend and wait for wakeup

		ThisThread::flags_clear(BTN_STATE_CHANGED | BTN_RELEASE);
		if ((ThisThread::flags_wait_any_for(BTN_STATE_CHANGED | BTN_RELEASE,
				600) & (BTN_STATE_CHANGED | BTN_RELEASE)) != 0) {
			// Button has changed during initial wait, give up
			continue;
		}
		if (!(((flag & BTN_LONGPRESS_INC) && (button_state == BUTTON_INC))
				|| ((flag & BTN_LONGPRESS_DEC) && (button_state == BUTTON_DEC)))) {
			continue;
		}

		int cnt = 0;
		int timeout = 200; // Initial timeout

		do {
			if ((ThisThread::flags_wait_any_for(BTN_STATE_CHANGED | BTN_RELEASE,
					timeout) & (BTN_STATE_CHANGED | BTN_RELEASE)) != 0) {
				// Button has changed during wait, give up
				break;
			}

			if (!(((flag & BTN_LONGPRESS_INC) && (button_state == BUTTON_INC))
					|| ((flag & BTN_LONGPRESS_DEC)
							&& (button_state == BUTTON_DEC)))) {
				break;
			}

			ButtonEvent evt =
					(flag & BTN_LONGPRESS_INC) ?
							BUTTON_EVENT_INC_REP : BUTTON_EVENT_DEC_REP;

			if (++cnt > 10) { // goes faster when cnt>10
				evt = (ButtonEvent) ((uint32_t) evt | BUTTON_EVENT_FAST);
			}

			btnEvtQueue.put((void*) evt, 0);
		} while (true);
	}
}

void PushToGo_GUI::PushToGo_MenuItem::add(PushToGo_MenuItem *son) {
	PushToGo_MenuItem *p = firstChild;
	if (!p) {
		firstChild = son;
		son->prev = son->next = NULL;
	} else {
		while (p->next)
			p = p->next;
		p->next = son;
		son->prev = p;
		son->next = NULL;
	}
	son->parent = this;
}

void PushToGo_GUI::PushToGo_MenuItem_Base::showMenu(PushToGo_GUI *gui) {
	int len = strlen(itemTitle);
	const char *start = itemTitle;
	if (len > 16) {
		start += showingIndex;
		if (++showingCount == 5) {
			if (++showingIndex + 16 > len) {
				showingCount = -5;
				showingIndex = 0;
			} else {
				showingCount = 0;
			}
		}
		len = 16;
	}
	gui->lcd->setPosition(0, 0);
	gui->lcd->write(start, len);
	gui->lcd->fillWith(' ', 16 - len);
}

bool PushToGo_GUI::PushToGo_MenuItem_Back::reactToButton(ButtonEvent evt,
		PushToGo_GUI *gui) {
	if (evt == BUTTON_EVENT_ENTER) {
		switchAway(gui);
		if (returned)
			returned();
		gui->currentDisplayMenu = parent;
		gui->currentDisplayMenu->switchTo(gui);
		return true;
	} else {
		return false;
	}
}

bool PushToGo_GUI::PushToGo_MenuItem_Action::reactToButton(ButtonEvent evt,
		PushToGo_GUI *gui) {
	if (evt == BUTTON_EVENT_ENTER) {
		ret = action(gui);
		dwell = ACTION_DISPLAY_DWELL_FRAME;
		return true;
	} else {
		return false;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Action::showMenu(PushToGo_GUI *gui) {
	PushToGo_MenuItem_Base::showMenu(gui); // Show title
	gui->lcd->setPosition(1, 0);
	if (dwell == 0) {
		gui->lcd->write("         Press o", 16);
	} else {
		if (ret == true)
			gui->lcd->write("         SUCCESS", 16);
		else
			gui->lcd->write("          FAILED", 16);
		dwell--;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_SubMenu::add(PushToGo_MenuItem *son) {
// First unlink the Back menu
	if (menuBack.prev)
		menuBack.prev->next = NULL;
	else
		firstChild = NULL;
// Add
	PushToGo_MenuItem::add(son);

// Link to Back menu
	son->next = &menuBack;
	menuBack.prev = son;
}

bool PushToGo_GUI::PushToGo_MenuItem_Edit::reactToButton(ButtonEvent evt,
		PushToGo_GUI *gui) {
	if (evt == BUTTON_EVENT_ENTER) {
		// Shift editing position to next available one
		if (shiftFromRight) {
			if (currPosition == 0) {
				currPosition = 17;
			}
			do {
				currPosition--;
			} while (currPosition >= 1
					&& (editMask & (1 << (currPosition - 1))) == 0);
			// currPosition will be zero if last digit encountered
		} else {
			do {
				currPosition++;
			} while (currPosition <= 16
					&& (editMask & (1 << (currPosition - 1))) == 0);
			if (currPosition > 16) // currPosition > 16 when last digit passed
				currPosition = 0;
		}

		if (currPosition == 0) {
			// Write value
			validate();
			valueUpdate(disp);
		}
		return true;
	} else if (evt & BUTTON_EVENT_INC) {
		if (currPosition != 0) {
			incValue(currPosition, evt & BUTTON_EVENT_FAST);
			validate();
			return true;
		} else
			return false; // Let menu structure handler take care
	} else if (evt & BUTTON_EVENT_DEC) {
		if (currPosition != 0) {
			decValue(currPosition, evt & BUTTON_EVENT_FAST);
			validate();
			return true;
		} else
			return false; // Let menu structure handler take care
	} else {
		return false;
	}

}

void PushToGo_GUI::PushToGo_MenuItem_Edit::showMenu(PushToGo_GUI *gui) {
	gui->lcd->setCursor(false);
	PushToGo_MenuItem_Base::showMenu(gui); // Show title
	gui->lcd->setPosition(1, 0);
	gui->lcd->write(disp, 16);
	if (currPosition != 0) {
		// Blink at the editing position
		gui->lcd->setCursor(true);
		gui->lcd->setPosition(1, currPosition - 1);
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit::switchAway(PushToGo_GUI *gui) {
	gui->lcd->setCursor(false);
}

static void _set_buf_int(int value, char *dest) {
	sprintf(dest, "%16d", value);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Integer::validate() {
	char *tail;
	int value = strtol(disp, &tail, 10);
	if (tail == disp) { // Conversion failed
		// Force initialize to zero
		value = 0;
	}
// Limit
	if (value > max)
		value = max;
	if (value < min)
		value = min;
	_set_buf_int(value, disp);
// Calculate new editable positions
	int firstPosition = 0;
	while (disp[firstPosition] == ' ')
		firstPosition++;	// Find first non-space
	if (value > 0)
		firstPosition--;	// Include sign position for positive numbers
	if (firstPosition < 1)
		firstPosition = 1;
	editMask = 0xFFFF & ~((1 << firstPosition) - 1);	// Remove first i-1 bits

	if (currPosition != 0 && (editMask & (1 << (currPosition - 1))) == 0) { // If current position out of range, force it to the first position
		currPosition = firstPosition + 1;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Integer::incValue(int pos,
		bool fast) {
	int significance = 1;
	int value = strtol(disp, NULL, 10); // Should always succeed provided that the buffer is validated
	for (int i = 16; i > pos; i--)
		significance *= 10;
	if (value == 0 || abs(value) >= significance) { // OK to just add
		value += fast ? significance * 10 : significance;
	} else if (abs(value) >= significance / 10) { // Sign bit
		value *= -1;
	}
	_set_buf_int(value, disp);
// Will be validated after
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Integer::decValue(int pos,
		bool fast) {
	int significance = 1;
	int value = strtol(disp, NULL, 10); // Should always succeed provided that the buffer is validated
	for (int i = 16; i > pos; i--)
		significance *= 10;
	if (value == 0 || abs(value) >= significance) { // OK to just add
		value -= fast ? significance * 10 : significance;
	} else if (abs(value) >= significance / 10) { // Sign bit
		value *= -1;
	}
	_set_buf_int(value, disp);
// Will be validated after
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Integer::setValue(int val) {
	_set_buf_int(val, disp);
	validate();
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Bool::validate() {
	if (strcmp(disp + 12, "true") != 0 && strcmp(disp + 11, "false") != 0) {
		// Force to false
		setValue(false);
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Bool::incValue(int pos, bool fast) {
	if (pos == 16) { // Only valid position
		if (strcmp(disp + 12, "true") == 0) {
			setValue(false);
		} else {
			setValue(true);
		}
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Bool::decValue(int pos, bool fast) {
	incValue(pos, fast); // Same
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Bool::setValue(bool val) {
	if (val) {
		strcpy(disp, "            true");
	} else {
		strcpy(disp, "           false");
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Date::validate() {
	if (yy > 2037)
		yy = 2037;
	if (yy < 1970)
		yy = 1970;
	if (mm > 12)
		mm = 12;
	if (mm < 1)
		mm = 1;
	if (dd < 1)
		dd = 1;
	bool leap = ((yy % 4 == 0) && !(yy % 100 == 0)) || (yy % 400 == 0);
	int mfd = leap ? 29 : 28;
	switch (mm) {
	case 4:
	case 6:
	case 9:
	case 11:
		if (dd > 30)
			dd = 30;
		break;
	case 2:
		if (dd > mfd)
			dd = mfd;
		break;
	default:
		if (dd > 31)
			dd = 31;
		break;
	}
	struct tm ts = { 0, 0, 0, dd, mm - 1, yy - 1900 };
	char buf[12];
	strftime(buf, sizeof(buf), "%b %d %Y", &ts);
	sprintf(disp, "%16s", buf);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Date::incValue(int pos, bool fast) {
	if (pos == 8) {
		if (mm < 12)
			mm++;
	} else if (pos == 11) {
		if (dd < 31)
			dd++;
	} else if (pos == 16) {
		if (yy < 2037)
			yy++;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Date::decValue(int pos, bool fast) {
	if (pos == 8) {
		if (mm > 1)
			mm--;
	} else if (pos == 11) {
		if (dd > 0)
			dd--;
	} else if (pos == 16) {
		if (yy > 1970)
			yy--;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Date::setValue(int y, int m, int d) {
	yy = y;
	mm = m;
	dd = d;
	validate();
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Time::validate() {
	if (mm >= 60)
		mm = 59;
	if (mm < 0)
		mm = 0;
	if (ss >= 60)
		mm = 59;
	if (ss < 0)
		ss = 0;
	if (hh >= 24)
		hh = 23;
	if (hh < 0)
		hh = 0;
	sprintf(disp, "        %02d:%02d:%02d", hh, mm, ss);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Time::incValue(int pos, bool fast) {
	switch (pos) {
	case 9:
		hh += 10;
		break;
	case 10:
		hh += 1;
		break;
	case 12:
		mm += 10;
		break;
	case 13:
		mm += 1;
		break;
	case 15:
		ss += 10;
		break;
	case 16:
		ss += 1;
		break;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Time::decValue(int pos, bool fast) {
	switch (pos) {
	case 9:
		hh -= 10;
		break;
	case 10:
		hh -= 1;
		break;
	case 12:
		mm -= 10;
		break;
	case 13:
		mm -= 1;
		break;
	case 15:
		ss -= 10;
		break;
	case 16:
		ss -= 1;
		break;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Time::setValue(int h, int m, int s) {
	hh = h;
	mm = m;
	ss = s;
	validate();
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Angle::validate() {
	if (angle > max)
		angle = max;
	if (angle < min)
		angle = min;
	display();
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Angle::display() {
	char sgn = sign[angle > 0 ? 0 : 1];
	double x = fabs(angle);
	int hh = int(x);
	int mm = int((x - hh) * 60);
	int ss = round((x - hh - mm / 60.0) * 3600);
	if (ss == 60) {
		ss = 0;
		mm++;
	}
	if (mm == 60) {
		mm = 0;
		hh++;
	}

	sprintf(disp, fmt, sgn, hh, mm, ss);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Angle::incValue(int pos, bool fast) {
	bool neg = (angle < 0);
	angle = fabs(angle);
	switch (pos) {
	case 6:
		angle = -angle;
		break;
	case 7:
		angle += 100;
		break;
	case 8:
		angle += 10;
		break;
	case 9:
		angle += 1;
		break;
	case 11:
		angle += 1.0 / 6;
		break;
	case 12:
		angle += 1.0 / 60;
		break;
	case 14:
		angle += 1.0 / 360;
		break;
	case 15:
		angle += 1.0 / 3600;
		break;
	}
	if (angle < 0 && pos != 6)
		angle = 1e-9;
	if (neg)
		angle = -angle;
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Angle::decValue(int pos, bool fast) {
	bool neg = (angle < 0);
	angle = fabs(angle);
	switch (pos) {
	case 6:
		angle = -angle;
		break;
	case 7:
		angle -= 100;
		break;
	case 8:
		angle -= 10;
		break;
	case 9:
		angle -= 1;
		break;
	case 11:
		angle -= 1.0 / 6;
		break;
	case 12:
		angle -= 1.0 / 60;
		break;
	case 14:
		angle -= 1.0 / 360;
		break;
	case 15:
		angle -= 1.0 / 3600;
		break;
	}
	if (angle < 0 && pos != 6)
		angle = 1e-9;
	if (neg)
		angle = -angle;
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_Angle::setValue(double ang) {
	angle = ang;
	validate();
}

static void _set_buf_double(double value, char *dest, int prec) {
	char fmt[16];
	sprintf(fmt, "%%16.%df", prec);
	sprintf(dest, fmt, value);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_FixedPoint::validate() {
	char *tail;
	float value = strtod(disp, &tail);
	if (tail == disp) { // Conversion failed
		// Force initialize to zero
		value = 0;
	}
// Limit
	if (value > max)
		value = max;
	if (value < min)
		value = min;
	_set_buf_double(value, disp, precision);
// Calculate new editable positions
	int firstPosition = 0;
	while (disp[firstPosition] == ' ')
		firstPosition++;	// Find first non-space
	if (value > 0)
		firstPosition--;	// Include sign position for positive numbers
	if (firstPosition < 1)
		firstPosition = 1;
	editMask = 0xFFFF & ~((1 << firstPosition) - 1);	// Remove first i-1 bits

	int dot = 16 - precision; // Find decimal point
	editMask &= ~(1 << (dot - 1)); // Make decimal point not editable

	if (currPosition != 0 && (editMask & (1 << (currPosition - 1))) == 0) { // If current position out of range, force it to the first position
		currPosition = firstPosition + 1;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_FixedPoint::incValue(int pos,
		bool fast) {
	double value = strtod(disp, NULL); // Should always succeed provided that the buffer is validated
	double significance = pow(10, 16 - pos - precision); // LSB
	if (significance > 1)
		significance /= 10; // Count decimal point
	if (value == 0 || fabs(value) >= significance) { // OK to just add
		value += fast ? significance * 10 : significance;
	} else if (fabs(value) >= significance / 10) { // Sign bit
		value *= -1;
	}
	_set_buf_double(value, disp, precision);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_FixedPoint::decValue(int pos,
		bool fast) {
	double value = strtod(disp, NULL); // Should always succeed provided that the buffer is validated
	double significance = pow(10, 16 - pos - precision); // LSB
	if (significance > 1)
		significance /= 10; // Count decimal point
	if (value == 0 || fabs(value) >= significance) { // OK to just add
		value -= fast ? significance * 10 : significance;
	} else if (fabs(value) >= significance / 10) { // Sign bit
		value *= -1;
	}
	_set_buf_double(value, disp, precision);
}

void PushToGo_GUI::PushToGo_MenuItem_Edit_FixedPoint::setValue(double val) {
	_set_buf_double(val, disp, precision);
	validate();
}

bool PushToGo_GUI::PushToGo_MenuItem_Action_WithConfirm::reactToButton(
		ButtonEvent evt, PushToGo_GUI *gui) {
	if (evt == BUTTON_EVENT_ENTER) {
		if (!triggered){
			triggered = true;
			yesno = false;
		}
		else{
			if (yesno) {
				ret = action(gui);
				dwell = ACTION_DISPLAY_DWELL_FRAME;
			}
			else {
				dwell = 0;
			}
			triggered = false;
		}
		return true;
	} else if ((evt == BUTTON_EVENT_INC || evt == BUTTON_EVENT_DEC) && triggered) {
		// Toggle yes/no
		yesno = !yesno;
		return true;
	}
	else{
		return false;
	}
}

void PushToGo_GUI::PushToGo_MenuItem_Action_WithConfirm::showMenu(
		PushToGo_GUI *gui) {
	if (triggered){
		gui->lcd->setPosition(0, 0);
		gui->lcd->print("Confirm?        ");

		gui->lcd->setPosition(1, 0);
		if (yesno)
			gui->lcd->print("             YES");
		else
			gui->lcd->print("              NO");
	}
	else{
		PushToGo_MenuItem_Action::showMenu(gui);
	}
}

template<>
void PushToGo_GUI::PushToGo_MenuItem_Display_Bind<int>::display(
		PushToGo_GUI *gui) {
	gui->lcd->print("%16d", *target);
}

template<>
void PushToGo_GUI::PushToGo_MenuItem_Display_Bind<double>::display(
		PushToGo_GUI *gui) {
	gui->lcd->print("%16.4f", *target);
}

template<>
void PushToGo_GUI::PushToGo_MenuItem_Display_Bind<bool>::display(
		PushToGo_GUI *gui) {
	gui->lcd->print("%16s", (*target) ? "TRUE" : "FALSE");
}
