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

#define BTN_STATE_CHANGED		0x01
#define BTN_LONGPRESS_INC		0x02
#define BTN_LONGPRESS_DEC		0x04
#define BTN_RELEASE				0x08

#define ACTION_DISPLAY_DWELL_FRAME	5

static void save_config() {
	TelescopeConfiguration::saveConfig_NV();
}

PushToGo_GUI::PushToGo_GUI(LCD1602 *lcd, EquatorialMount *eq) :
		lcd(lcd), currentDisplayMenu(&homeMenu), thread(osPriorityBelowNormal,
		OS_STACK_SIZE, NULL, "PushToGo_GUI"), thd_btn_poll(
				osPriorityBelowNormal,
				OS_STACK_SIZE, NULL, "button_poll"), guiRunning(false), eqMount(
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
}

PushToGo_GUI::~PushToGo_GUI() {
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

		// Display current time/date
		time_t ts = time(NULL) + TelescopeConfiguration::getInt("timezone") * 3600;
		struct tm tts;
#if !( defined(__ARMCC_VERSION) || defined(__CC_ARM) )
		gmtime_r(&ts, &tts);
#else
		core_util_critical_section_enter();
		memcpy(&tts, gmtime(&ts), sizeof(struct tm));
		core_util_critical_section_exit();
#endif

		// Display time
		int len = strftime(buf, sizeof(buf), "%T", &tts);

		lcd->setPosition(0, 0);
		lcd->write(buf, len);
		lcd->fillWith(' ', 16-len);

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
		dwell_sec = 25;
		return true;
	} else {
		return false;
	}
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
		firstPosition++; // Find first non-space
	if (value > 0)
		firstPosition--; // Include sign position for positive numbers
	if (firstPosition < 1)
		firstPosition = 1;
	editMask = 0xFFFF & ~((1 << firstPosition) - 1); // Remove first i-1 bits

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
