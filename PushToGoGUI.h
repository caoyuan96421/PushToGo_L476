/*
 * PushToGoGUI.h
 *
 *  Created on: Aug 24, 2019
 *      Author: caoyu
 */

#ifndef _PUSHTOGOGUI_H_
#define _PUSHTOGOGUI_H_

#include "mbed.h"
#include "mbed_events.h"
#include "LCD1602.h"
#include "Buttons.h"
#include "EquatorialMount.h"

// Meaning of buttons
#define BUTTON_ENTER	0x01
#define BUTTON_INC		0x02
#define BUTTON_DEC		0x04

class PushToGo_GUI: private ButtonListener {
public:
	PushToGo_GUI(LCD1602 *lcd, EquatorialMount *eq);
	virtual ~PushToGo_GUI();

	void startGUI();
	void stopGUI();

	void listenTo(Buttons *btn) {
		btn->registerListener(this);
	}
	enum ButtonEvent {
		BUTTON_EVENT_INC = 1,
		BUTTON_EVENT_INC_REP = BUTTON_EVENT_INC | 0x4000,
		BUTTON_EVENT_INC_REP_FAST = BUTTON_EVENT_INC | 0xC000,
		BUTTON_EVENT_DEC = 2,
		BUTTON_EVENT_DEC_REP = BUTTON_EVENT_DEC | 0x4000,
		BUTTON_EVENT_DEC_REP_FAST = BUTTON_EVENT_DEC | 0xC000,
		BUTTON_EVENT_ENTER = 4,
		BUTTON_EVENT_RELEASE = 8,

		BUTTON_EVENT_REP = 0x4000,
		BUTTON_EVENT_FAST = 0x8000
	};

	struct PushToGo_MenuItem {
		const char *itemTitle;
		enum {
			MENU_TYPE_DEFAULT = 0, MENU_TYPE_EDIT, MENU_TYPE_ACTION,
//			MENU_TYPE_BOOL,
//			MENU_TYPE_INT,
//			MENU_TYPE_FIXEDPOINT,
//			MENU_TYPE_ANGLE_DMS,
//			MENU_TYPE_ANGLE_HMS,
//			MENU_TYPE_STRING,
			MENU_TYPE_SUBMENU
		} itemType;
		union {
			bool b_value;
			int i_value;
			double f_value;
			char s_value[32];
		} itemValue;
		int digits;
		PushToGo_MenuItem *next;
		PushToGo_MenuItem *prev;
		PushToGo_MenuItem *parent;
		PushToGo_MenuItem *firstChild;

		PushToGo_MenuItem() :
				itemTitle(""), itemType(MENU_TYPE_DEFAULT), digits(6), next(
				NULL), prev(NULL), parent(NULL), firstChild(NULL) {
		}
		virtual ~PushToGo_MenuItem() {

		}

		/**
		 * Show menu structure
		 */
		virtual void showMenu(PushToGo_GUI *gui) {
		}

		/**
		 * Respond to button event
		 * @return if any action is taken
		 */
		virtual bool reactToButton(ButtonEvent evt, PushToGo_GUI *gui) {
			return false;
		}

		/**
		 * Called when the menu is just switched to
		 */
		virtual void switchTo(PushToGo_GUI *gui) {
			gui->lcd->clear();
		}

		/**
		 * Called when the menu is switched away from
		 */
		virtual void switchAway(PushToGo_GUI *gui) {
		}

		virtual void add(PushToGo_MenuItem *son);
		/**
		 * Recursively find parent GUI
		 */
		virtual PushToGo_GUI* getGUI() {
			if (parent)
				return parent->getGUI();
			else
				return NULL;
		}
	};

	// Default Menu item
	struct PushToGo_MenuItem_Base: public PushToGo_MenuItem {
		virtual void showMenu(PushToGo_GUI *gui);
		virtual void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem::switchTo(gui);
			showingIndex = 0;
			showingCount = -5;
		}
	private:
		int showingIndex;
		int showingCount;
	};

	// Back item
	struct PushToGo_MenuItem_Back: public PushToGo_MenuItem_Base {
		PushToGo_MenuItem_Back() {
			itemTitle = "          Return";
			returned = NULL;
		}
		// Called when returning from this level of menu structure
		void (*returned)();
		virtual bool reactToButton(ButtonEvent evt, PushToGo_GUI *gui);
	};

	struct PushToGo_MenuItem_SubMenu: public PushToGo_MenuItem_Base {
		PushToGo_MenuItem_SubMenu(const char *name = "Menu",
				PushToGo_MenuItem *parent = NULL) {
			itemTitle = name;
			itemType = MENU_TYPE_SUBMENU;
			PushToGo_MenuItem::add(&menuBack);
			if (parent)
				parent->add(this);
		}
		PushToGo_MenuItem_Back menuBack;
		virtual void add(PushToGo_MenuItem *son);
	};

	struct PushToGo_MenuItem_Home: public PushToGo_MenuItem_SubMenu {
		PushToGo_MenuItem_Home(PushToGo_GUI *gui) :
				PushToGo_MenuItem_SubMenu("home", NULL), gui(gui), dwell_sec(0) {
		}
		virtual void showMenu(PushToGo_GUI *gui);
		virtual bool reactToButton(ButtonEvent evt, PushToGo_GUI *gui);
		PushToGo_GUI* getGUI() {
			return gui;
		}
		PushToGo_GUI *gui;
		int dwell_sec;
	};

	// Action Menu item
	struct PushToGo_MenuItem_Action: public PushToGo_MenuItem_Base {
		virtual bool reactToButton(ButtonEvent evt, PushToGo_GUI *gui);
		virtual void showMenu(PushToGo_GUI *gui);
		virtual void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			dwell = 0;
		}
		virtual bool action(PushToGo_GUI *gui) = 0;

		int dwell;
		bool ret;
	};

	struct PushToGo_MenuItem_Action_WithConfirm: public PushToGo_MenuItem_Action {
		PushToGo_MenuItem_Action_WithConfirm() {
			triggered = false;
			yesno = false;
		}
		virtual bool reactToButton(ButtonEvent evt, PushToGo_GUI *gui);
		virtual void showMenu(PushToGo_GUI *gui);

		bool triggered;
		bool yesno;
	};

	struct PushToGo_MenuItem_Display: public PushToGo_MenuItem_Base {
		PushToGo_MenuItem_Display(const char *name = "Edit") {
			itemTitle = name;
		}
		virtual void showMenu(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::showMenu(gui);
			gui->lcd->setPosition(1, 0);
			display(gui);
		}
		virtual void display(PushToGo_GUI *gui) = 0;
	};

	template<class T>
	struct PushToGo_MenuItem_Display_Bind: public PushToGo_MenuItem_Display {
		PushToGo_MenuItem_Display_Bind(const char *name,
				const volatile T *target) :
				PushToGo_MenuItem_Display(name), target(target) {
		}
		virtual void display(PushToGo_GUI *gui);
		const volatile T *target;
	};

	// Generic editable menu item
	struct PushToGo_MenuItem_Edit: public PushToGo_MenuItem_Base {
		PushToGo_MenuItem_Edit(const char *name = "Edit", int mask = 0xFFFF,
				const char *fmt = "") {
			itemTitle = name;
			itemType = MENU_TYPE_EDIT;
			editMask = mask;
			currPosition = 0;
			shiftFromRight = false;
			int len = strlen(fmt);
			len = (len > 16) ? len : 16;
			strncpy(disp, fmt, len); // Copy format string into the display buffer
			if (len < 16)
				memset(disp + len, ' ', 16 - len); // Fill with space
		}
		virtual bool reactToButton(ButtonEvent evt, PushToGo_GUI *gui);
		virtual void showMenu(PushToGo_GUI *gui);
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate() {
		}

		// Inc/Dec values at a given position
		virtual void incValue(int pos, bool fast = false) = 0;
		virtual void decValue(int pos, bool fast = false) = 0;

		// Called when the finished editing the last digit
		virtual void valueUpdate(const char buf[]) {
		}

		virtual void switchAway(PushToGo_GUI *gui);

		int editMask; // 16-bit integer, each bit means whether that position is editable
		int currPosition; // Indicate which digit is editing. 0 means not editing, 1-16 count from left to right
		bool shiftFromRight; // Direction of iteration between digits
		char disp[17]; // Display string buffer
	};

	struct PushToGo_MenuItem_Edit_Integer: public PushToGo_MenuItem_Edit {
		PushToGo_MenuItem_Edit_Integer(const char *name = "Edit", int min = 0,
				int max = 0x7FFFFFFF, int init = 0) :
				PushToGo_MenuItem_Edit(name), max(max), min(min) {
			shiftFromRight = true; // More natural for numerics
			setValue(init);
		}

		virtual void valueUpdate(const char buf[]) {
			intValueUpdate(strtol(buf, NULL, 10));
		}

		virtual void intValueUpdate(int val) = 0;
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate();

		// Inc/Dec values at a given position
		virtual void incValue(int pos, bool fast = false);
		virtual void decValue(int pos, bool fast = false);

		void setValue(int val);

		int max;
		int min;
	};

	struct PushToGo_MenuItem_Edit_FixedPoint: public PushToGo_MenuItem_Edit {
		PushToGo_MenuItem_Edit_FixedPoint(const char *name = "Edit",
				double min = 0, double max = 0x7FFFFFFF, double init = 0,
				int prec = 2) :
				PushToGo_MenuItem_Edit(name), max(max), min(min), precision(
						prec) {
			shiftFromRight = true; // More natural for numerics
			setValue(init);
		}

		virtual void valueUpdate(const char buf[]) {
			doubleValueUpdate(strtod(buf, NULL));
		}

		virtual void doubleValueUpdate(double val) = 0;
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate();

		// Inc/Dec values at a given position
		virtual void incValue(int pos, bool fast = false);
		virtual void decValue(int pos, bool fast = false);

		void setValue(double val);

		double max;
		double min;
		int precision;
	};

	struct PushToGo_MenuItem_Edit_Bool: public PushToGo_MenuItem_Edit {
		PushToGo_MenuItem_Edit_Bool(const char *name = "Edit",
				bool init = false) :
				PushToGo_MenuItem_Edit(name, 0x8000) {
			setValue(init);
		}

		virtual void valueUpdate(const char buf[]) {
			boolValueUpdate(strcmp(buf + 12, "true") == 0);
		}

		virtual void boolValueUpdate(bool val) = 0;
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate();

		// Returns next/previous values at a given position
		virtual void incValue(int pos, bool fast = false);
		virtual void decValue(int pos, bool fast = false);

		void setValue(bool val);
	};

	struct PushToGo_MenuItem_Edit_Date: public PushToGo_MenuItem_Edit {
		PushToGo_MenuItem_Edit_Date(const char *name = "Edit") :
				PushToGo_MenuItem_Edit(name, 0x8480), yy(2000), mm(1), dd(1) {
			shiftFromRight = false; // More natural for dates
		}

		virtual void valueUpdate(const char buf[]) {
			dateValueUpdate(yy, mm, dd);
		}

		virtual void dateValueUpdate(int yy, int mm, int ss) = 0;
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate();

		// Inc/Dec values at a given position
		virtual void incValue(int pos, bool fast = false);
		virtual void decValue(int pos, bool fast = false);

		void setValue(int yy, int mm, int dd);

		int yy, mm, dd;
	};

	struct PushToGo_MenuItem_Edit_Time: public PushToGo_MenuItem_Edit {
		PushToGo_MenuItem_Edit_Time(const char *name = "Edit",
				int mask = 0xDB00) :
				PushToGo_MenuItem_Edit(name, mask), hh(0), mm(0), ss(0) {
			shiftFromRight = false; // More natural for dates
		}

		virtual void valueUpdate(const char buf[]) {
			timeValueUpdate(hh, mm, ss);
		}

		virtual void timeValueUpdate(int hh, int mm, int ss) = 0;
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate();

		// Inc/Dec values at a given position
		virtual void incValue(int pos, bool fast = false);
		virtual void decValue(int pos, bool fast = false);

		void setValue(int hh, int mm, int ss);

		int hh, mm, ss;
	};

	struct PushToGo_MenuItem_Edit_Angle: public PushToGo_MenuItem_Edit {
		PushToGo_MenuItem_Edit_Angle(const char *name = "Edit", double min =
				-180, double max = 180, const char *sign = "+-", int mask =
				0x6DE0, const char *fmt = "     %c%3d\xDF%02d'%02d\"") :
				PushToGo_MenuItem_Edit(name, mask), angle(0), sign(sign), min(
						min), max(max), fmt(fmt) {
			shiftFromRight = false;
		}

		virtual void valueUpdate(const char buf[]) {
			angleValueUpdate(angle);
		}

		virtual void angleValueUpdate(double angle) = 0;
		/*
		 * validate the data buffer. If data is invalid, it will be initialized with default value
		 */
		virtual void validate();
		virtual void display();

		// Inc/Dec values at a given position
		virtual void incValue(int pos, bool fast = false);
		virtual void decValue(int pos, bool fast = false);

		void setValue(double angle);

		double angle;
		const char *sign;
		double min, max;
		const char *fmt;
	};

private:
	LCD1602 *lcd;

	PushToGo_MenuItem *currentDisplayMenu;
	Thread thread;
	Thread thd_btn_poll;
	bool guiRunning;
	EquatorialMount *eqMount;
	uint64_t prev_idle_time;
	uint64_t prev_up_time;
	int button_state;
	int button_last_update;
	Queue<void, 16> btnEvtQueue;
//	EventQueue evt_queue;

	PushToGo_MenuItem_Home homeMenu;
	PushToGo_MenuItem_Base quickMenu;

	void _task_entry();
	void _poll_entry();
	float calc_cpu_usage();
	void buttonStateChanged(int oldstate, int newstate);
	static void constructMenuStructure(PushToGo_MenuItem *root);
	static void addConfigMenu(PushToGo_MenuItem*);
	static void addCalibrationMenu(PushToGo_MenuItem*);
	static void addUtilitiesMenu(PushToGo_MenuItem*);
	static void addMiscMenu(PushToGo_MenuItem*);
	static void addDebugMenu(PushToGo_MenuItem*);
	static void addQuickMenu(PushToGo_MenuItem*);
};

struct tm getLocalTime(time_t timestamp = 0);


#endif /* _PUSHTOGOGUI_H_ */
