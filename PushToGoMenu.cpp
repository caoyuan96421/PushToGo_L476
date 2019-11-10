#include "PushToGoGUI.h"
#include "LED.h"
#include "TelescopeConfiguration.h"

void PushToGo_GUI::constructMenuStructure(PushToGo_MenuItem *root) {
	PushToGo_MenuItem_SubMenu *calibration = new PushToGo_MenuItem_SubMenu(
			"Calibration", root);
	PushToGo_MenuItem_SubMenu *configuration = new PushToGo_MenuItem_SubMenu(
			"Configuration", root);
	PushToGo_MenuItem_SubMenu *utilities = new PushToGo_MenuItem_SubMenu(
			"Utilities", root);
	PushToGo_MenuItem_SubMenu *misc = new PushToGo_MenuItem_SubMenu(
			"Misc. Setting", root);

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
			debug_ptg(DEFAULT_DEBUG, "%s = %s\r\n", config->config, newval ? "true" : "false");
			TelescopeConfiguration::setBool(config, newval);
		}
		ConfigItem *config;
	};

	ConfigNode *p = TelescopeConfiguration::getHead();
	while (p && !p->config->extra) {
		switch (p->config->type) {
		case DATATYPE_INT:
			configuration->add(new configItemInt(p->config));
			break;
		case DATATYPE_BOOL:
			configuration->add(new configItemBool(p->config));
			break;
		default:
			break;
		}
		p = p->next;
	}

	/***************Utilities Menu********************/
	struct gotoIndexItem: public PushToGo_MenuItem_Action {
		gotoIndexItem() {
			itemTitle = "Goto Index";
		}
		bool action(PushToGo_GUI *gui) {
			this->getGUI()->eqMount->goToIndex();
			return true;
		}
	} *gotoIndex = new gotoIndexItem();

	utilities->add(gotoIndex);

	struct indexItem: public PushToGo_MenuItem_Action {
		indexItem() {
			itemTitle = "Set Encoder Index Position      ";
		}
		bool action(PushToGo_GUI *gui) {
			this->getGUI()->eqMount->setEncoderIndex();
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
	struct brightnessIntItem: public PushToGo_MenuItem_Edit_Integer {
		brightnessIntItem() :
				PushToGo_MenuItem_Edit_Integer("Contrast", 0, 100, 50) {
		}

		void switchTo(PushToGo_GUI *gui) {
			PushToGo_MenuItem_Base::switchTo(gui);
			int contrast = TelescopeConfiguration::getInt("lcd_contrast");
			setValue(contrast);
		}

		void intValueUpdate(int contrast) {
			getGUI()->lcd->setBrightness(contrast / 100.0F);
			TelescopeConfiguration::setInt("lcd_contrast", contrast);
		}
	} *brightnessItem = new brightnessIntItem();
	misc->add(brightnessItem);
}
