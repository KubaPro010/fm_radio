#include "radio_module.h"

SDRPP_MOD_INFO{
	/* Name:            */ "fm_radio",
	/* Description:     */ "FM Radio demodulator and RDS decoder",
	/* Author:          */ "kuba201",
	/* Version:         */ 1, 1, 0,
	/* Max instances    */ -1
};

MOD_EXPORT void _INIT_() {
	json def = json({});
	config.setPath(core::args["root"].s() + "/fm_radio_config.json");
	config.load(def);
	config.enableAutoSave();
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
	return new FMRadioModule(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(void* instance) {
	delete (FMRadioModule*)instance;
}

MOD_EXPORT void _END_() {
	config.disableAutoSave();
	config.save();
}