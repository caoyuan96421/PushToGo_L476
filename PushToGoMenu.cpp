#include "PushToGoGUI.h"
#include "LED.h"
#include "TelescopeConfiguration.h"
#include "ADL355.h"

extern ADL355 accel;

void PushToGo_GUI::constructMenuStructure(PushToGo_MenuItem *root) {
	PushToGo_MenuItem_SubMenu *calibration = new PushToGo_MenuItem_SubMenu(
			"Calibration", root);
	PushToGo_MenuItem_SubMenu *configuration = new PushToGo_MenuItem_SubMenu(
			"Configuration", root);
	PushToGo_MenuItem_SubMenu *utilities = new PushToGo_MenuItem_SubMenu(
			"Utilities", root);
	PushToGo_MenuItem_SubMenu *misc = new PushToGo_MenuItem_SubMenu(
			"Misc. Setting", root);
	PushToGo_MenuItem_SubMenu *dbg = new PushToGo_MenuItem_SubMenu("Debugging",
			root);

	/***************Config Menu********************/
	struct configItemInt: public PushToGo_MenuItem_Edit_Integer {
		configItemInt(ConfigItem *config) :
				PushToGo_MenuItem_Edit_Integer(config->name, config->min.idata,
						config->max.idata, config->value.idata), config(config) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			setValue(TelescopeConfiguration::getInt(config));
		}
		void intValueUpdate(int newval) {
			debug_ptg(DEFAULT_DEBUG, "%s = %d\r\n", config->config, newval);
			TelescopeConfiguration::setInt(config, newval);
		}
		ConfigItem *config;
	};
	struct configItemBool: public PushToGo_MenuItem_Edit_Bool {
		configItemBool(ConfigItem *config) :
				PushToGo_MenuItem_Edit_Bool(config->name, config->value.bdata), config(
						config) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			setValue(TelescopeConfiguration::getBool(config));
		}
		void boolValueUpdate(bool newval) {
			debug_ptg(DEFAULT_DEBUG, "%s = %s\r\n", config->config,
					newval ? "true" : "false");
			TelescopeConfiguration::setBool(config, newval);
		}
		ConfigItem *config;
	};

	struct configItemDouble: public PushToGo_MenuItem_Edit_FixedPoint {
		configItemDouble(ConfigItem *config, int precision = 2) :
				PushToGo_MenuItem_Edit_FixedPoint(config->name,
						config->min.ddata, config->max.ddata,
						config->value.ddata, precision), config(config) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			setValue(TelescopeConfiguration::getDouble(config));
		}
		void doubleValueUpdate(double newval) {
			debug_ptg(DEFAULT_DEBUG, "%s = %d\r\n", config->config, newval);
			TelescopeConfiguration::setDouble(config, newval);
		}
		ConfigItem *config;
	};
	struct configItemLatitude: public PushToGo_MenuItem_Edit_Angle {
		configItemLatitude(ConfigItem *config) :
				PushToGo_MenuItem_Edit_Angle(config->name, -90, 90, "NS",
						0x6DA0), config(config) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			setValue(TelescopeConfiguration::getDouble(config));
		}
		void angleValueUpdate(double newval) {
			debug_ptg(DEFAULT_DEBUG, "%s = %f\r\n", config->config, newval);
			TelescopeConfiguration::setDouble(config, newval);
		}
		ConfigItem *config;
	};

	struct configItemLongitude: public PushToGo_MenuItem_Edit_Angle {
		configItemLongitude(ConfigItem *config) :
				PushToGo_MenuItem_Edit_Angle(config->name, -180, 180, "EW"), config(
						config) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			setValue(TelescopeConfiguration::getDouble(config));
		}
		void angleValueUpdate(double newval) {
			debug_ptg(DEFAULT_DEBUG, "%s = %f\r\n", config->config, newval);
			TelescopeConfiguration::setDouble(config, newval);
		}
		ConfigItem *config;
	};

	ConfigNode *p = TelescopeConfiguration::getHead();
	while (p && !p->config->extra) {
		if (strcmp(p->config->config, "latitude") == 0) {
			configuration->add(new configItemLatitude(p->config));
		} else if (strcmp(p->config->config, "longitude") == 0) {
			configuration->add(new configItemLongitude(p->config));
		} else {
			switch (p->config->type) {
			case DATATYPE_INT:
				configuration->add(new configItemInt(p->config));
				break;
			case DATATYPE_BOOL:
				configuration->add(new configItemBool(p->config));
				break;
			case DATATYPE_DOUBLE:
				configuration->add(new configItemDouble(p->config));
				break;
			default:
				break;
			}
		}
		p = p->next;
	}

	/***************Utilities Menu********************/
	struct gotoIndexItem: public PushToGo_MenuItem_Action {
		gotoIndexItem() {
			itemTitle = "Goto Index";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->goToIndex();
			return true;
		}
	} *gotoIndex = new gotoIndexItem();

	utilities->add(gotoIndex);

	struct indexItem: public PushToGo_MenuItem_Action {
		indexItem() {
			itemTitle = "Set Encoder Index Position      ";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->setEncoderIndex();
			return true;
		}
	} *setIndex = new indexItem();

	utilities->add(setIndex);

	/***************Calibration Menu********************/
	struct testIntItem: public PushToGo_MenuItem_Edit_Integer {
		testIntItem() :
				PushToGo_MenuItem_Edit_Integer("test value", -100, 100, 5) {
		}
		void intValueUpdate(int newval) {
			debug_ptg(DEFAULT_DEBUG, "new value: %d\r\n", newval);
		}
	} *testint = new testIntItem();
	calibration->add(testint);

	/***************MISC Menu********************/
	struct dateItem: public PushToGo_MenuItem_Edit_Date {
		dateItem() :
				PushToGo_MenuItem_Edit_Date("Set Date") {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Edit_Date::switchTo(gui);
			struct tm ts = getLocalTime();
			setValue(ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday);
		}

		void dateValueUpdate(int y, int m, int s) {
			struct tm ts = getLocalTime();
			ts.tm_year = y - 1900;
			ts.tm_mon = m - 1;
			ts.tm_mday = s;

			time_t timestamp = mktime(&ts)
					- TelescopeConfiguration::getInt("timezone") * 3600;
			set_time(timestamp);
		}
	} *datesetItem = new dateItem();

	misc->add(datesetItem);

	struct timeItem: public PushToGo_MenuItem_Edit_Time {
		timeItem() :
				PushToGo_MenuItem_Edit_Time("Set Time"), delta(0) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Edit_Time::switchTo(gui);

			ts = getLocalTime();
			setValue(ts.tm_hour, ts.tm_min, ts.tm_sec);
		}

		void showMenu(PushToGo_GUI *gui) {
			// update display time
			time_t tnow = time(NULL) + delta;
			ts = getLocalTime(tnow);
			setValue(ts.tm_hour, ts.tm_min, ts.tm_sec);

			PushToGo_MenuItem_Edit_Time::showMenu(gui);
		}

		void validate() {
			PushToGo_MenuItem_Edit_Time::validate();

			delta += (hh - ts.tm_hour) * 3600 + (mm - ts.tm_min) * 60
					+ (ss - ts.tm_sec);
			ts.tm_hour = hh;
			ts.tm_min = mm;
			ts.tm_sec = ss;
		}

		void timeValueUpdate(int h, int m, int s) {
			time_t timestamp = time(NULL) + delta;
			set_time(timestamp);
			delta = 0;
		}

		int delta;
		struct tm ts;
	} *timesetItem = new timeItem();

	misc->add(timesetItem);

	struct brightnessIntItem: public PushToGo_MenuItem_Edit_Integer {
		brightnessIntItem() :
				PushToGo_MenuItem_Edit_Integer("Contrast", 0, 100, 50) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			int contrast = TelescopeConfiguration::getInt("lcd_contrast");
			setValue(contrast);
		}

		void validate() {
			int contrast = strtol(disp, NULL, 10);
			getGUI()->lcd->setBrightness(contrast / 100.0F);
		}

		void intValueUpdate(int contrast) {
			getGUI()->lcd->setBrightness(contrast / 100.0F);
			TelescopeConfiguration::setInt("lcd_contrast", contrast);
		}
	} *brightnessItem = new brightnessIntItem();
	misc->add(brightnessItem);

	struct refreshIncItem: public PushToGo_MenuItem_Action {
		refreshIncItem() {
			itemTitle = "Update Inclinometer Reading";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->getInclinometer()->refresh();
			return true;
		}
	} *refreshInc = new refreshIncItem();

	misc->add(refreshInc);

	// Debug menu

	struct dbgItem1: public PushToGo_MenuItem_Display {
		dbgItem1() :
				PushToGo_MenuItem_Display("RA Encoder Raw") {
		}
		void display(PushToGo_GUI *gui) {
			gui->lcd->print("%16d",
					gui->eqMount->getRA().getEncoder()->readPos());
		}
	} *itm1 = new dbgItem1();

	struct dbgItem2: public PushToGo_MenuItem_Display {
		dbgItem2() :
				PushToGo_MenuItem_Display("DEC Encoder Raw") {
		}
		void display(PushToGo_GUI *gui) {
			gui->lcd->print("%16d",
					gui->eqMount->getDEC().getEncoder()->readPos());
		}
	} *itm2 = new dbgItem2();

	struct dbgItem3: public PushToGo_MenuItem_Display {
		dbgItem3(char axis = 'X') :
				axis(axis) {
			itemTitle = new char[16];
			sprintf((char*) itemTitle, "ADL355 %c=", axis);
		}
		~dbgItem3() {
			delete[] itemTitle;
		}
		void display(PushToGo_GUI *gui) {
			double x, y, z;
			if (!accel.getAcceleration(x, y, z)) {
				gui->lcd->print("  DISCONNECTED  ");
				return;
			}
			double theta = atan2(x, sqrt(y * y + z * z));
			double phi = atan2(y, z);
			double t = asin(cos(theta) * sin(phi));
			double g = sqrt(x * x + y * y + z * z);
			switch (axis) {
			case 'X':
				gui->lcd->print("%16.5f", x);
				break;
			case 'Y':
				gui->lcd->print("%16.5f", y);
				break;
			case 'Z':
				gui->lcd->print("%16.5f", z);
				break;
			case 'g':
				gui->lcd->print("%16.5f", g);
				break;
			case 0xF2:
				gui->lcd->print("%15.5f\xDF", theta * 180 / M_PI);
				break;
				;
			case 't':
				gui->lcd->print("%15.5f\xDF", t * 180 / M_PI);
				break;
			}
		}
		char axis;
	} *itm3 = new dbgItem3('X'), *itm4 = new dbgItem3('Y'), *itm5 =
			new dbgItem3('Z'), *itm6 = new dbgItem3(0xF2), *itm7 = new dbgItem3(
			't'), *itm8 = new dbgItem3('g');

	dbg->add(itm1);
	dbg->add(itm2);
	dbg->add(itm3);
	dbg->add(itm4);
	dbg->add(itm5);
	dbg->add(itm8);
	dbg->add(itm6);
	dbg->add(itm7);

	enum jogState {
		jog_idle, jog_up, jog_down
	};

	// Quick Menu
	struct jogItem: public PushToGo_MenuItem_Base {
		jogItem(bool ra) :
				enabled(false), state(jog_idle), ra(ra) {
			itemTitle = ra ? "Jog RA" : "Jog DEC";
		}
		void switchTo(PushToGo_GUI *gui) {
			enabled = false;
		}
		void switchAway(PushToGo_GUI *gui) {
			gui->eqMount->stopNudge();
		}
		bool reactToButton(ButtonEvent btn, PushToGo_GUI *gui) {
			if (btn == BUTTON_EVENT_ENTER) {
				enabled = !enabled;
				return true;
			}
			if (enabled) {
				switch (state) {
				case jog_idle:
					if (btn == BUTTON_EVENT_INC) {
						state = jog_up;
						gui->eqMount->startNudge(ra ? NUDGE_EAST : NUDGE_NORTH);
						return true;
					} else if (btn == BUTTON_EVENT_DEC) {
						state = jog_down;
						gui->eqMount->startNudge(ra ? NUDGE_WEST : NUDGE_SOUTH);
						return true;
					}
					break;
				case jog_up:
				case jog_down:
					if (btn == BUTTON_EVENT_RELEASE) {
						state = jog_idle;
						gui->eqMount->stopNudge();
						return true;
					}
					break;
				}
			}
			return false;
		}
		void showMenu(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::showMenu(gui);
			gui->lcd->setPosition(1, 0);
			if (enabled) {
				const EquatorialCoordinates &eq =
						gui->eqMount->getEquatorialCoordinates();
				gui->lcd->print("%c", (state == jog_down) ? '<' : ' ');
				if (ra) {
					char we = (eq.ra > 0) ? 'E' : 'W';
					double r = (eq.ra < 0) ? eq.ra + 360.0 : eq.ra;
					gui->lcd->print("  %2dh%02d'%02d\"%c  ", int(r / 15),
							int(fmod(r, 15.0) * 4),
							(int) floor(fmod(r, 0.25) * 240), we);
				} else {
					char ns = (eq.dec > 0) ? 'N' : 'S';
					double d = fabs(eq.dec);
					gui->lcd->print("  %2d\xDF%02d'%02d\"%c  ", int(d),
							int(fmod(d, 1.0) * 60),
							(int) floor(fmod(d, 1.0 / 60) * 3600), ns);
				}
				gui->lcd->print("%c", (state == jog_up) ? '>' : ' ');
			} else {
				gui->lcd->print("         Press o");
			}
		}
		bool enabled;
		jogState state;
		bool ra;
	} *jogRA = new jogItem(true), *jogDEC = new jogItem(false);

	root->getGUI()->quickMenu.add(jogRA);
	root->getGUI()->quickMenu.add(jogDEC);

	struct gotoIndexItem *goToIndex2 = new gotoIndexItem();
	root->getGUI()->quickMenu.add(goToIndex2);

}
