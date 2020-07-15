#include "PushToGoGUI.h"
#include "LED.h"
#include "TelescopeConfiguration.h"
#include "ADL355.h"

extern ADL355 accel;
//clearCalibration();

void PushToGo_GUI::addConfigMenu(PushToGo_MenuItem *configuration) {

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
}

void PushToGo_GUI::addUtilitiesMenu(PushToGo_MenuItem *utilities) {

	/***************Utilities Menu********************/
	struct gotoIndexItem: public PushToGo_MenuItem_Action_WithConfirm {
		gotoIndexItem() {
			itemTitle = "Goto Index";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->goToIndex();
			return true;
		}
	} *gotoIndex = new gotoIndexItem();

	utilities->add(gotoIndex);

	struct indexItem: public PushToGo_MenuItem_Action_WithConfirm {
		indexItem() {
			itemTitle = "Set Encoder Index Position      ";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->setEncoderIndex();
			return true;
		}
	} *setIndex = new indexItem();
	utilities->add(setIndex);

	struct linkItem: public PushToGo_MenuItem_Action_WithConfirm {
		linkItem() {
			itemTitle = "Link Encoder Index w/ Calib.      ";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->setEncoderIndex();
			return true;
		}
	} *linkIndex = new linkItem();

	utilities->add(linkIndex);
}

void PushToGo_GUI::addCalibrationMenu(PushToGo_MenuItem *calibration) {

	/***************Calibration Menu********************/

	struct PushToGo_MenuItem_Display_Bind_TwoDouble: public PushToGo_MenuItem_Display_Bind<
			double> {
		PushToGo_MenuItem_Display_Bind_TwoDouble(const char *name,
				const volatile double *a, const volatile double *b) :
				PushToGo_MenuItem_Display_Bind<double>(name, a), target2(b) {
		}
		void display(PushToGo_GUI *gui) {
			gui->lcd->print("%7.2f %7.2f", *target, *target2);
		}
		const volatile double *target2;
	};

	calibration->add(
			new PushToGo_MenuItem_Display_Bind<double>("RMS Calib. Error",
					&calibration->getGUI()->eqMount->getCalibration().error));
	calibration->add(
			new PushToGo_MenuItem_Display_Bind<double>("PA Alt.",
					&calibration->getGUI()->eqMount->getCalibration().pa.alt));
	calibration->add(
			new PushToGo_MenuItem_Display_Bind<double>("PA Azi.",
					&calibration->getGUI()->eqMount->getCalibration().pa.azi));
	calibration->add(
			new PushToGo_MenuItem_Display_Bind_TwoDouble("Axis offset",
					&calibration->getGUI()->eqMount->getCalibration().offset.ra_off,
					&calibration->getGUI()->eqMount->getCalibration().offset.dec_off));
	calibration->add(
			new PushToGo_MenuItem_Display_Bind<double>("Cone Error",
					&calibration->getGUI()->eqMount->getCalibration().cone));

	struct clearCalibItem: public PushToGo_MenuItem_Action_WithConfirm {
		clearCalibItem() {
			itemTitle = "Clear Calib.";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->clearCalibration();
			return true;
		}
	} *clearCalib = new clearCalibItem();
	calibration->add(clearCalib);

	struct showCalibStarsItem: public PushToGo_MenuItem_SubMenu {
		showCalibStarsItem(PushToGo_MenuItem *parent = NULL) :
				PushToGo_MenuItem_SubMenu("Show Calib. Stars", parent) {
		}

		struct calibStarItem: public PushToGo_MenuItem_SubMenu {
			calibStarItem(int index, PushToGo_MenuItem *parent = NULL) :
					PushToGo_MenuItem_SubMenu("", parent), index(index) {
				char *buf = new char[16];
				snprintf(buf, 15, "Star %d", index);
				itemTitle = buf;
			}
			~calibStarItem() {
				delete[] itemTitle;
				for (PushToGo_MenuItem *p = firstChild, *q; p != &menuBack;) {
					q = p->next;
					delete p;
					p = q;
				}
			}

			int index;
		};
		struct starDisplay: public PushToGo_MenuItem_Display {
			starDisplay(int index, const char *name, double a, double b = NAN) :
					PushToGo_MenuItem_Display(""), a(a), b(b) {
				char *buf = new char[16];
				snprintf(buf, 15, "S%02d %s", index, name);
				itemTitle = buf;
			}
			~starDisplay() {
				delete[] itemTitle;
			}
			void display(PushToGo_GUI *gui) {
				if (isnan(b)) {
					// Display timestamp
					char buf[16];

					// Display current time
					struct tm tts = getLocalTime((uint32_t) a);
					strftime(buf, sizeof(buf), " %T %m/%d ", &tts);

					gui->lcd->print("%s", buf);
				} else {
					gui->lcd->print("%7.2f %7.2f", a, b);
				}
			}
			double a, b;
		};

//		void switchTo(PushToGo_GUI *gui) {
//			PushToGo_MenuItem_SubMenu::switchTo(gui);
//		}

		virtual void showMenu(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::showMenu(gui);

			gui->lcd->setPosition(1, 0);
			gui->lcd->print("%8d star(s)", gui->eqMount->getNumAlignmentStar());

			// Update calib structure
			// First remove all child
			PushToGo_MenuItem *p = firstChild, *q;
			while (p != &menuBack) {
				q = p->next;
				delete p;
				p = q;
			}
			firstChild = &menuBack;
			menuBack.prev = NULL;

			// Then add new children based on calibration
			for (int i = 0; i < gui->eqMount->getNumAlignmentStar(); i++) {
				PushToGo_MenuItem_SubMenu *star_menu = new calibStarItem(i,
						this);
				AlignmentStar *as = gui->eqMount->getAlignmentStar(i);
				star_menu->add(
						new starDisplay(i, "Ref. Pos.", as->star_ref.ra,
								as->star_ref.dec));
				star_menu->add(
						new starDisplay(i, "Meas. Pos.", as->star_meas.ra_delta,
								as->star_meas.dec_delta));
				star_menu->add(
						new starDisplay(i, "Timestamp", as->timestamp, NAN));
			}
		}

	} *showCalib = new showCalibStarsItem(calibration); // Will be added automatically
	(void) showCalib;
}

void PushToGo_GUI::addMiscMenu(PushToGo_MenuItem *misc) {

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
}

void PushToGo_GUI::addDebugMenu(PushToGo_MenuItem *dbg) {

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
}

void PushToGo_GUI::addQuickMenu(PushToGo_MenuItem *root) {

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

	struct gotoIndexItem: public PushToGo_MenuItem_Action_WithConfirm {
		gotoIndexItem() {
			itemTitle = "Goto Index";
		}
		bool action(PushToGo_GUI *gui) {
			gui->eqMount->goToIndex();
			return true;
		}
	} *gotoIndex = new gotoIndexItem();
	root->getGUI()->quickMenu.add(gotoIndex);
}

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

	addCalibrationMenu(calibration);
	addConfigMenu(configuration);
	addUtilitiesMenu(utilities);
	addMiscMenu(misc);
	addDebugMenu(dbg);

	addQuickMenu(root);
}
