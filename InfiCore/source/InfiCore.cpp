#include <YYToolkit/Shared.hpp>
#include <nlohmann/json.hpp>

#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string_view>

using namespace Aurie;
using namespace YYTK;
using json = nlohmann::json;

static YYTKInterface* g_ModuleInterface = nullptr;

// Config variables
static enum SettingType {
	SETTING_BOOL = 0
};

static struct Setting {
	std::string name = "";
	int icon = -1;
	SettingType type = SETTING_BOOL;
	bool boolValue = false;

	Setting(std::string n, int i, bool bV) {
		name = n;
		icon = i;
		type = SETTING_BOOL;
		boolValue = bV;
	}
};

static struct ModConfig {
	Setting debugEnabled = Setting("debugEnabled", 9, false);
	std::vector<Setting> settings = { debugEnabled };
} config;

void to_json(json& j, const ModConfig& c) {
	j = json {
		"debugEnabled", {
			{ "icon", c.debugEnabled.icon },
			{ "value", c.debugEnabled.boolValue }
		}
	};
}

void from_json(const json& j, ModConfig& c) {
	try {
		j.at("debugEnabled").at("icon").get_to(c.debugEnabled.icon);
		j.at("debugEnabled").at("value").get_to(c.debugEnabled.boolValue);
	} catch (const json::out_of_range& e) {
		PrintError(__FILE__, __LINE__, "%s", e.what());
		std::string fileName = formatString(std::string("InfiCore")) + "-config.json";
		GenerateConfig(fileName);
	}
}

static struct RemoteConfig {
	std::string name;
	std::vector<Setting> settings;
};

static struct RemoteMod {
	std::string name;
	bool enabled = false;
	bool wasToggled = false;
	RemoteConfig config;
	bool hasConfig = false;

	RemoteMod(std::string n, bool e) {
		name = n;
		enabled = e;
	}
};

std::string formatString(const std::string& input) {
	std::string formattedString = input;

	for (char& c : formattedString) {
		c = std::tolower(c);
	}

	for (char& c : formattedString) {
		if (c == ' ') {
			c = '-';
		}
	}

	return formattedString;
}

void GenerateConfig(std::string fileName) {
	json data;
	
	for (const Setting& setting : config.settings) {
		json settingData;
		settingData["icon"] = setting.icon;
		if (setting.type == SETTING_BOOL) settingData["value"] = setting.boolValue;
		data[setting.name.c_str()] = settingData;
	}

	std::ofstream configFile("modconfigs/" + fileName);
	if (configFile.is_open()) {
		Print(CM_WHITE, "[InfiCore] - Config file \"%s\" created!", fileName.c_str());
		configFile << std::setw(4) << data << std::endl;
		configFile.close();
	} else {
		PrintError(__FILE__, __LINE__, "[InfiCore] - Error opening config file \"%s\"", fileName.c_str());
	}
}

/*
		Inline Functions
*/
template <typename... TArgs>
inline static void Print(CmColor Color, std::string_view Format, TArgs&&... Args) {
	g_ModuleInterface->Print(Color, Format, Args...);
}

template <typename... TArgs>
inline static void PrintError(std::string_view Filepath, int Line, std::string_view Format, TArgs&&... Args) {
	g_ModuleInterface->PrintError(Filepath, Line, Format, Args...);
}

inline static RValue CallBuiltin(const char* FunctionName, std::vector<RValue> Arguments) {
	return g_ModuleInterface->CallBuiltin(FunctionName, Arguments);
}

inline static double GetAssetIndexFromName(const char* name) {
	double index = CallBuiltin("asset_get_index", { RValue(name, g_ModuleInterface) }).AsReal();
	return index;
}

/*
		GML Inline Functions
*/

/*


*/

inline static RValue variable_global_get(const char* name) {
	return CallBuiltin("variable_global_get", { RValue(name, g_ModuleInterface) });
}

inline static void variable_global_set(const char* name, const char* value) {
	CallBuiltin("variable_global_set", { RValue(name, g_ModuleInterface), RValue(value, g_ModuleInterface) });
}

inline static RValue struct_get(RValue variable, const char* name) {
	return CallBuiltin("struct_get", { variable, RValue(name, g_ModuleInterface) });
}

inline static std::string_view array_get(RValue variable, double index) {
	return CallBuiltin("array_get", { variable, index }).AsString(g_ModuleInterface);
}

inline static void array_set(RValue variable, double index, std::string value) {
	CallBuiltin("array_set", { variable, index, RValue(value, g_ModuleInterface) });
}

inline static void draw_sprite(const char* sprite, double subimg, double x, double y) {
	CallBuiltin("draw_sprite", { GetAssetIndexFromName(sprite), subimg, x, y });
}

inline static void draw_sprite_ext(const char* sprite, double subimg, double x, double y, double xscale, double yscale, double rot, double color, double alpha) {
	CallBuiltin("draw_sprite_ext", { GetAssetIndexFromName(sprite), subimg, x, y, xscale, yscale, rot, color, alpha });
}

inline static void draw_set_font(const char* font) {
	CallBuiltin("draw_set_font", { GetAssetIndexFromName(font) });
}

inline static void draw_set_halign(double halign) {
	CallBuiltin("draw_set_halign", { halign });
}

inline static void draw_set_valign(double valign) {
	CallBuiltin("draw_set_valign", { valign });
}

inline static void draw_set_color(double col) {
	CallBuiltin("draw_set_color", { col });
}

inline static void draw_text(double x, double y, const char* string) {
	CallBuiltin("draw_text", { x, y, RValue(string, g_ModuleInterface) });
}
// "snd_menu_confirm", 30, false
inline static void audio_play_sound(const char* name, double priority, bool loop) {
	CallBuiltin("audio_play_sound", { GetAssetIndexFromName(name), priority, loop });
}

// Function Hooks
using FNScriptData = CScript * (*)(int);
FNScriptData scriptList = nullptr;

// Function Pointers
struct ArgSetup {
	RValue args[10] = {};
	bool isInitialized = false;
	ArgSetup() {}
	ArgSetup(const char* name) {
		args[0].String = RefString::Alloc(name, strlen(name), false);
		args[0].Kind = VALUE_STRING;
		isInitialized = true;
	}
	ArgSetup(long long value) {
		args[0].I64 = value;
		args[0].Kind = VALUE_INT64;
		isInitialized = true;
	}
	ArgSetup(double value) {
		args[0].Real = value;
		args[0].Kind = VALUE_REAL;
		isInitialized = true;
	}
	ArgSetup(double device, double mouseButton) {
		args[0].Real = device;
		args[0].Kind = VALUE_REAL;
		args[1].Real = mouseButton;
		args[1].Kind = VALUE_REAL;
		isInitialized = true;
	}
	ArgSetup(long long id, const char* name) {
		args[0].I64 = id;
		args[0].Kind = VALUE_INT64;
		args[1].String = RefString::Alloc(name, strlen(name), false);
		args[1].Kind = VALUE_STRING;
		isInitialized = true;
	}
	ArgSetup(long long x, long long y, const char* text) {
		args[0].I64 = x;
		args[0].Kind = VALUE_INT64;
		args[1].I64 = y;
		args[1].Kind = VALUE_INT64;
		args[2].String = RefString::Alloc(text, strlen(text), false);
		args[2].Kind = VALUE_STRING;
		isInitialized = true;
	}
	ArgSetup(const char* name, double priority, bool loop) {
		args[0].I64 = getAssetIndexFromName(name);
		args[0].Kind = VALUE_INT64;
		args[1].Real = priority;
		args[1].Kind = VALUE_REAL;
		args[2].I64 = loop;
		args[2].Kind = VALUE_BOOL;
		isInitialized = true;
	}
	ArgSetup(long long id, const char* name, double value) {
		args[0].I64 = id;
		args[0].Kind = VALUE_INT64;
		args[1].String = RefString::Alloc(name, strlen(name), false);
		args[1].Kind = VALUE_STRING;
		args[2].Real = value;
		args[2].Kind = VALUE_REAL;
		isInitialized = true;
	}
	ArgSetup(double confirm, double enter, double cancel) {
		args[0].Real = confirm;
		args[0].Kind = VALUE_REAL;
		args[1].Real = enter;
		args[1].Kind = VALUE_REAL;
		args[2].Real = cancel;
		args[2].Kind = VALUE_REAL;
		isInitialized = true;
	}
	ArgSetup(const char* name, double subimg, double x, double y) {
		args[0].I64 = getAssetIndexFromName(name);
		args[0].Kind = VALUE_INT64;
		args[1].Real = subimg;
		args[1].Kind = VALUE_REAL;
		args[2].Real = x;
		args[2].Kind = VALUE_REAL;
		args[3].Real = y;
		args[3].Kind = VALUE_REAL;
		isInitialized = true;
	}
	ArgSetup(long long topl_x, long long topl_y, long long botr_x, long long botr_y, bool isOutline) {
		args[0].I64 = topl_x;
		args[0].Kind = VALUE_INT64;
		args[1].I64 = topl_y;
		args[1].Kind = VALUE_INT64;
		args[2].I64 = botr_x;
		args[2].Kind = VALUE_INT64;
		args[3].I64 = botr_y;
		args[3].Kind = VALUE_INT64;
		args[4].I64 = isOutline;
		args[4].Kind = VALUE_BOOL;
		isInitialized = true;
	}
	// "hud_shopButton", 1, 320, 48, 0.75, 0.75, 0, 16777215, 1
	ArgSetup(const char* sprite, double subimg, double x, double y, double xscale, double yscale, double rot, long long color, double alpha) {
		args[0].I64 = getAssetIndexFromName(sprite);
		args[0].Kind = VALUE_INT64;
		args[1].Real = subimg;
		args[1].Kind = VALUE_REAL;
		args[2].Real = x;
		args[2].Kind = VALUE_REAL;
		args[3].Real = y;
		args[3].Kind = VALUE_REAL;
		args[4].Real = xscale;
		args[4].Kind = VALUE_REAL;
		args[5].Real = yscale;
		args[5].Kind = VALUE_REAL;
		args[6].Real = rot;
		args[6].Kind = VALUE_REAL;
		args[7].I64 = color;
		args[7].Kind = VALUE_INT64;
		args[8].Real = alpha;
		args[8].Kind = VALUE_REAL;
	}
	// (double)320, (double)(48 + 13), "TEST", (double)1, (long long)0, (double)32, (double)4, (double)100, (long long)0, (double)1)
	ArgSetup(double x, double y, const char* text, double len, double color0, double angleIncrement, double sep, double w, double color1, double alpha) {
		args[0].Real = x;
		args[0].Kind = VALUE_REAL;
		args[1].Real = y;
		args[1].Kind = VALUE_REAL;
		args[2].String = RefString::Alloc(text, strlen(text), false);
		args[2].Kind = VALUE_STRING;
		args[3].Real = len;
		args[3].Kind = VALUE_REAL;
		args[4].Real = color0;
		args[4].Kind = VALUE_REAL;
		args[5].Real = angleIncrement;
		args[5].Kind = VALUE_REAL;
		args[6].Real = sep;
		args[6].Kind = VALUE_REAL;
		args[7].Real = w;
		args[7].Kind = VALUE_REAL;
		args[8].Real = color1;
		args[8].Kind = VALUE_REAL;
		args[9].Real = alpha;
		args[9].Kind = VALUE_REAL;

		isInitialized = true;
	}
};

TRoutine variableInstanceGetFunc = nullptr;
ArgSetup args_container;
ArgSetup args_currentOption;
ArgSetup args_canControl;

TRoutine variableInstanceSetFunc = nullptr;

TRoutine scriptExecuteFunc = nullptr;
ArgSetup args_configHeaderDraw;
RValue* drawTextOutlineArgs[10];
ArgSetup args_calculateScore;

TRoutine variableGlobalGetFunc = nullptr;
ArgSetup args_version;
ArgSetup args_textContainer;
ArgSetup args_lastTitleOption;

TRoutine variableGlobalSetFunc = nullptr;

TRoutine drawSetAlphaFunc = nullptr;
ArgSetup args_drawSetAlpha;

TRoutine drawSetColorFunc = nullptr;
ArgSetup args_drawSetColor;

TRoutine drawTextFunc = nullptr;
ArgSetup args_drawConfigNameText;

TRoutine drawSpriteFunc = nullptr;
ArgSetup args_drawSettingsSprite;
ArgSetup args_drawOptionButtonSprite;
ArgSetup args_drawOptionIconSprite;
ArgSetup args_drawToggleButtonSprite;

TRoutine drawSpriteExtFunc = nullptr;
ArgSetup args_drawShopButtonSpriteExt;
ArgSetup args_drawToggleButtonSpriteExt;

PFUNC_YYGMLScript drawTextOutlineScript = nullptr;
PFUNC_YYGMLScript commandPrompsScript = nullptr;

ArgSetup args_commandPromps;
RValue* commandPrompsArgs[3];

TRoutine audioPlaySoundFunc = nullptr;
ArgSetup args_audioPlaySound;

TRoutine deviceMouseXToGUIFunc = nullptr;
TRoutine deviceMouseYToGUIFunc = nullptr;
ArgSetup args_deviceMouse;

TRoutine deviceMouseCheckButtonPressedFunc = nullptr;
ArgSetup args_checkButtonPressed;

TRoutine drawSetHAlignFunc = nullptr;
TRoutine drawSetVAlignFunc = nullptr;
ArgSetup args_halign;
ArgSetup args_valign;

TRoutine drawSetFontFunc = nullptr;
ArgSetup args_drawSetFont;

YYTKStatus MmGetScriptData(FNScriptData& outScript) {
#ifdef _WIN64

	uintptr_t FuncCallPattern = FindPattern("\xE8\x00\x00\x00\x00\x33\xC9\x0F\xB7\xD3", "x????xxxxx", 0, 0);

	if (!FuncCallPattern)
		return YYTK_INVALIDRESULT;

	uintptr_t Relative = *reinterpret_cast<uint32_t*>(FuncCallPattern + 1);
	Relative = (FuncCallPattern + 5) + Relative;

	if (!Relative)
		return YYTK_INVALIDRESULT;

	outScript = reinterpret_cast<FNScriptData>(Relative);

	return YYTK_OK;
#else
	return YYTK_UNAVAILABLE;
#endif
}

void Hook(void* NewFunc, void* TargetFuncPointer, void** pfnOriginal, const char* Name) {
	AurieStatus status{};
	if (TargetFuncPointer) {
		status = MmCreateHook(, NewFunc, pfnOriginal);
		if (Status != MH_OK)
			PrintMessage(
				CLR_RED,
				"Failed to hook function %s (MH Status %s) in %s at line %d",
				Name,
				MH_StatusToString(Status),
				__FILE__,
				__LINE__
			);
		else
			MH_EnableHook(TargetFuncPointer);

		if (config.debugEnabled.boolValue) PrintMessage(CLR_GRAY, "- &%s = 0x%p", Name, TargetFuncPointer);
	} else {
		PrintMessage(
			CLR_RED,
			"Failed to hook function %s (address not found) in %s at line %d",
			Name,
			__FILE__,
			__LINE__
		);
	}
};

void HookScriptFunction(const char* scriptFunctionName, void* detourFunction, void** origScript) {
	int scriptFunctionIndex = getAssetIndexFromName(scriptFunctionName) - 100000;

	CScript* CScript = scriptList(scriptFunctionIndex);

	Hook(
		detourFunction,
		(void*)(CScript->s_pFunc->pScriptFunc),
		origScript,
		scriptFunctionName
	);
}

RValue result;
RValue yyrv_mouse_x;
RValue yyrv_mouse_y;
static bool mouseMoved = false;
int prev_mouse_x = 0;
int prev_mouse_y = 0;

static bool draw_title_chars = true;
static bool draw_config_menu = false;
static bool config_hovered = false;
static bool config_selected = false;
static bool no_config_error = false;
static int no_config_error_timer = 0;
static std::vector<RemoteMod> mods;
static int current_mod = 0;
static int current_mod_setting = 0;

static void LoadUnloadMods() {
	AurieStatus status{};
	for (RemoteMod& mod : mods) {
		if (mod.wasToggled != true) continue;

		std::string mod_name_no_disable = mod.name;
		if (std::string_view(mod.name).ends_with(".disabled"))
			mod_name_no_disable = mod.name.substr(0, mod.name.length() - std::string(".disabled").length());
		
		std::string enabled_path = "mods\\Aurie\\" + mod_name_no_disable;
		std::string disabled_path = "mods\\Aurie\\" + mod_name_no_disable + ".disabled";

		if (mod.enabled == true) {	// load mod
			std::error_code ec;
			std::filesystem::rename(disabled_path, enabled_path, ec);
			if (ec.value() == 0) {
				Print(CM_LIGHTPURPLE, "[*] Moved '%s' to '%s'!", disabled_path.c_str(), enabled_path.c_str());
				AurieModule* temp_module = nullptr;
				status = MdMapImage(enabled_path, temp_module);
				if (AurieSuccess(status)) {
					Print(CM_GREEN, "[+] Loaded '%s' - mapped to 0x%p.", mod.name, temp_module);
				} else {
					PrintError(__FILE__, __LINE__, "Failed to load '%s'!", mod.name);
				}
			} else {
				PrintError(__FILE__, __LINE__, "Failed to move '%s' to '%s'", disabled_path.c_str(), enabled_path.c_str());
			} 
		} else {					// unload mod
			AurieModule* module_to_remove = nullptr;
			status = Internal::MdpLookupModuleByPath(enabled_path, module_to_remove);
			if (AurieSuccess(status)) {
				status = MdUnmapImage(module_to_remove);
				if (AurieSuccess(status)) {
					Print(CM_RED, "[-] Unloaded '%s'", mod.name);
					std::error_code ec;
					std::filesystem::rename(enabled_path, disabled_path, ec);
					if (ec.value() == 0) {
						Print(CM_LIGHTPURPLE, "[*] Moved '%s' to '%s'!", enabled_path.c_str(), disabled_path.c_str());
					} else {
						PrintError(__FILE__, __LINE__, "Failed to move '%s' to '%s'", enabled_path.c_str(), disabled_path.c_str());
					}
				} else {
					PrintError(__FILE__, __LINE__, "Failed to unload '%s' at 0x%p!", mod.name, module_to_remove);
				}
			} else {
				PrintError(__FILE__, __LINE__, "Failed to get path for '%s' at '%s'!", mod.name, enabled_path);
			}
		}

		mod.wasToggled = false;
	}
}

static void GetMods() {
	// Find Mods
	std::string mods_path = "mods\\Aurie";
	for (const auto& entry : fs::directory_iterator(mods_path)) {
		std::string mod_name = entry.path().filename().string();
		RemoteMod new_mod(mod_name, mod_name.contains(".disabled") ? false : true);
		mods.push_back(new_mod);
	}

	// Sort Mods Alphabetically
	std::sort(mods.begin(), mods.end(), [](const RemoteMod& a, const RemoteMod& b) {
		return a.name < b.name;
	});
}

static void GetModConfigs() {
	// Get Configs for Mods
	for (RemoteMod& mod : mods) {
		std::string config_name = mod.name.substr(0, mod.name.size() - 4) + "-config.json";
		std::filesystem::path config_path = "modconfigs/" + config_name;

		if (std::filesystem::exists(config_path)) {
			RemoteConfig config;
			config.name = config_name;

			std::ifstream file(config_path);
			json data;
			file >> data;

			for (json::iterator it = data.begin(); it != data.end(); ++it) {
				std::string key = it.key();
				json setting = it.value();
				if (setting.find("value") != setting.end()) {
					if (setting["value"].is_boolean()) {
						int icon_index = -1;
						if (setting.find("icon") != setting.end()) icon_index = setting["icon"];
						Setting new_setting(key, icon_index, setting["value"]);
						config.settings.insert(config.settings.begin(), new_setting);
					}
				}
			}

			mod.config = config;
			mod.hasConfig = true;
		} else {
			mod.hasConfig = false;
		}
	}
}

static void SetConfigSettings() {
	for (int mod = 0; mod < mods.size(); mod++) {
		std::ifstream in_file("modconfigs/" + mods[mod].config.name);
		if (in_file.fail()) {
			PrintError(__FILE__, __LINE__, "Config for \"%s\" not found!", mods[mod].name);
			continue;
		}
		json data;
		in_file >> data;
		in_file.close();

		for (int setting = 0; setting < mods[mod].config.settings.size(); setting++) {
			if (mods[mod].config.settings[setting].type == SETTING_BOOL) {
				data[mods[mod].config.settings[setting].name]["value"] = mods[mod].config.settings[setting].boolValue;
			}
		}

		std::ofstream out_file("modconfigs/" + mods[mod].config.name);
		out_file << data.dump(4);
		out_file.close();
	}
}

typedef YYRValue* (*ScriptFunc)(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args);

// gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origConfirmedTitleScreenScript = nullptr;
YYRValue* ConfirmedTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	YYRValue yyrv_canControl;
	variableInstanceGetFunc(&yyrv_canControl, Self, Other, 2, args_canControl.args);
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3) { // 3 = mod configs button index
		if (static_cast<int>(yyrv_canControl) == 1) {
			args_canControl.args[2].Real = 0;
			variableInstanceSetFunc(&result, Self, Other, 3, args_canControl.args);
			args_lastTitleOption.args[1].Real = 3;
			variableGlobalSetFunc(&result, Self, Other, 2, args_lastTitleOption.args);
			draw_title_chars = false;
			draw_config_menu = true;
			audio_play_sound("snd_menu_confirm", 30, false);
		} else if (config_hovered == false) {
			mods[current_mod].enabled = !mods[current_mod].enabled;
			mods[current_mod].wasToggled = !mods[current_mod].wasToggled;
			audio_play_sound("snd_menu_confirm", 30, false);
		} else if (config_selected == false) {
			if (mods[current_mod].hasConfig == false) {
				no_config_error = true;
				no_config_error_timer = 60;
				audio_play_sound("snd_alert", 30, false);
			} else {
				config_selected = true;
				audio_play_sound("snd_menu_confirm", 30, false);
			}
		} else if (config_selected == true) {
			if (mods[current_mod].config.settings[current_mod_setting].type == SETTING_BOOL) {
				mods[current_mod].config.settings[current_mod_setting].boolValue = !mods[current_mod].config.settings[current_mod_setting].boolValue;
				audio_play_sound("snd_menu_confirm", 30, false);
			}
		}
	} else {
		YYRValue* res = origConfirmedTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origReturnMenuTitleScreenScript = nullptr;
YYRValue* ReturnMenuTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	YYRValue yyrv_canControl;
	variableInstanceGetFunc(&yyrv_canControl, Self, Other, 2, args_canControl.args);
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3) { // 3 = mod configs button index
		if (config_selected == true) {
			config_selected = false;
			current_mod_setting = 0;
			audio_play_sound("snd_menu_back", 30, false);
		} else if (static_cast<int>(yyrv_canControl) == 0)  {
			args_canControl.args[2].Real = 1;
			variableInstanceSetFunc(&result, Self, Other, 3, args_canControl.args);
			args_lastTitleOption.args[1].Real = 3;
			variableGlobalSetFunc(&result, Self, Other, 2, args_lastTitleOption.args);
			draw_title_chars = true;
			draw_config_menu = false;
			audio_play_sound("snd_menu_back", 30, false);
			SetConfigSettings();
			LoadUnloadMods();
		}
	} else {
		YYRValue* res = origReturnMenuTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectLeftTitleScreenScript = nullptr;
YYRValue* SelectLeftTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	YYRValue yyrv_canControl;
	variableInstanceGetFunc(&yyrv_canControl, Self, Other, 2, args_canControl.args);
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (config_selected == false) {
			if (config_hovered == true) {
				config_hovered = false;
				audio_play_sound("snd_charSelectWoosh", 30, false);
			}
		}
	} else {
		YYRValue* res = origSelectLeftTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectRightTitleScreenScript = nullptr;
YYRValue* SelectRightTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	YYRValue yyrv_canControl;
	variableInstanceGetFunc(&yyrv_canControl, Self, Other, 2, args_canControl.args);
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (config_selected == false) {
			if (config_hovered == false) {
				config_hovered = true;
				audio_play_sound("snd_charSelectWoosh", 30, false);
			}
		}
	} else {
		YYRValue* res = origSelectRightTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectUpTitleScreenScript = nullptr;
YYRValue* SelectUpTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	YYRValue yyrv_canControl;
	variableInstanceGetFunc(&yyrv_canControl, Self, Other, 2, args_canControl.args);
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (current_mod > 0 && config_selected == false) {
			current_mod--;
			audio_play_sound("snd_menu_select", 30, false);
		} else if (current_mod_setting > 0 && config_selected == true) {
			current_mod_setting--;
			audio_play_sound("snd_menu_select", 30, false);
		}
	} else {
		YYRValue* res = origSelectUpTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectDownTitleScreenScript = nullptr;
YYRValue* SelectDownTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	YYRValue yyrv_canControl;
	variableInstanceGetFunc(&yyrv_canControl, Self, Other, 2, args_canControl.args);
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (current_mod < mods.size() - 1 && config_selected == false) {
			current_mod++;
			audio_play_sound("snd_menu_select", 30, false);
		} else if (current_mod_setting < mods[current_mod].config.settings.size() - 1 && config_selected == true) {
			current_mod_setting++;
			audio_play_sound("snd_menu_select", 30, false);
		}
	} else {
		YYRValue* res = origSelectDownTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

std::string GetFileName(const char* File) {
	std::string sFileName(File);
	size_t LastSlashPos = sFileName.find_last_of("\\");
	if (LastSlashPos != std::string::npos && LastSlashPos != sFileName.length()) {
		sFileName = sFileName.substr(LastSlashPos + 1);
	}
	return sFileName;
}

// CallBuiltIn is way too slow to use per frame. Need to investigate if there's a better way to call in built functions.

// We save the CodeCallbackHandler attributes here, so we can unregister the callback in the unload routine.
static CallbackAttributes_t* g_pFrameCallbackAttributes = nullptr;
static CallbackAttributes_t* g_pCodeCallbackAttributes = nullptr;
static uint32_t FrameNumber = 0;

static std::unordered_map<int, const char*> codeIndexToName;
static std::unordered_map<int, std::function<void(YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags)>> codeFuncTable;

static bool version_text_changed = false;

/*
		Frame Callback Event
*/
static AurieStatus FrameCallback(
	IN FWFrame& FrameContext
) {
	UNREFERENCED_PARAMETER(FrameContext);

	FrameNumber++;
	if (no_config_error_timer > 0) {
		no_config_error_timer--;
	} else if (no_config_error == true) {
		no_config_error = false;
	}
	// Tell the core the handler was successful.
	return AURIE_SUCCESS;
}

/*
		Code Callback Events
*/
static AurieStatus CodeCallback(
	IN FWCodeEvent& CodeContext
)
{
	CInstance* Self = std::get<0>(CodeContext.Arguments());
	CInstance* Other = std::get<1>(CodeContext.Arguments());
	CCode* Code = std::get<2>(CodeContext.Arguments());
	int	Flags = std::get<3>(CodeContext.Arguments());
	RValue* Res = std::get<4>(CodeContext.Arguments());

	// Check if CCode argument has a valid name
	if (!Code->GetName()) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - Failed to find Code Event name!");
		return AURIE_MODULE_INTERNAL_ERROR;
	}

	// Cast Name arg to an std::string for easy usage
	std::string Name = Code->GetName();

	/*
			Title Screen Create Event
	*/
	if (Name == "gml_Object_obj_TitleScreen_Create_0") {
		GetMods();
		GetModConfigs();
		if (version_text_changed == false) {
			std::string_view version = variable_global_get("version").AsString(g_ModuleInterface);
			if (version.contains("Modded")) {
				std::string modded_ver_str = std::string(version) + " (Modded)";
				variable_global_set("version", modded_ver_str.c_str());
			}
			version_text_changed = true;
		}
				
		if (args_currentOption.isInitialized == false) args_currentOption = ArgSetup(Self->i_id, "currentOption");
		if (args_canControl.isInitialized == false) args_canControl = ArgSetup(Self->i_id, "canControl");
		if (args_lastTitleOption.isInitialized == false) args_lastTitleOption = ArgSetup("lastTitleOption");
		if (args_drawSettingsSprite.isInitialized == false) args_drawSettingsSprite = ArgSetup("hud_optionsmenu", 0, 320, 48);
		if (args_drawOptionButtonSprite.isInitialized == false) args_drawOptionButtonSprite = ArgSetup("hud_OptionButton", 0, 320, 96);
		if (args_drawConfigNameText.isInitialized == false) args_drawConfigNameText = ArgSetup(320, 96, "text");
		if (args_halign.isInitialized == false) args_halign = ArgSetup(double(1));
		if (args_valign.isInitialized == false) args_valign = ArgSetup(double(1));
		if (args_drawSetFont.isInitialized == false) args_drawSetFont = ArgSetup((double)getAssetIndexFromName("jpFont_Big"));
		if (args_drawOptionIconSprite.isInitialized == false) args_drawOptionIconSprite = ArgSetup("hud_graphicIcons", 0, 320, 48);
		if (args_drawToggleButtonSprite.isInitialized == false) args_drawToggleButtonSprite = ArgSetup("hud_toggleButton", 0, 320, 48);
		if (args_audioPlaySound.isInitialized == false) args_audioPlaySound = ArgSetup("snd_menu_confirm", 30, false);
		if (args_drawShopButtonSpriteExt.isInitialized == false) args_drawShopButtonSpriteExt = ArgSetup("hud_shopButton", 1, 320 + 160, 48, 0.75, 0.75, 0, 16777215, 1);
		if (args_drawToggleButtonSpriteExt.isInitialized == false) args_drawToggleButtonSpriteExt = ArgSetup("hud_toggleButton", 0, 320 + 160, 48, 1, 1, 0, 0, 1);
		if (args_commandPromps.isInitialized == false) {
			args_commandPromps = ArgSetup((double)1, (double)1, (double)1);
			commandPrompsArgs[0] = new YYRValue((double)1);
					
			commandPrompsArgs[1] = new YYRValue((double)1);
			commandPrompsArgs[2] = new YYRValue((double)1);
		}
		if (args_configHeaderDraw.isInitialized == false) {
			args_configHeaderDraw = ArgSetup((double)320, (double)(48 + 13), "TEST", (double)1, (double)0, (double)32, (double)4, (double)100, (double)0, (double)1);
			drawTextOutlineArgs[0] = new YYRValue((double)320);
			drawTextOutlineArgs[1] = new YYRValue((double)(48 + 13));
			drawTextOutlineArgs[2] = new YYRValue("MOD SETTINGS");
			drawTextOutlineArgs[3] = new YYRValue((double)1);
			drawTextOutlineArgs[4] = new YYRValue((double)0);
			drawTextOutlineArgs[5] = new YYRValue((double)32);
			drawTextOutlineArgs[6] = new YYRValue((double)4);
			drawTextOutlineArgs[7] = new YYRValue((double)300);
			drawTextOutlineArgs[8] = new YYRValue((double)0);
			drawTextOutlineArgs[9] = new YYRValue((double)1);
		}

		if (!CodeContext.CalledOriginal()) CodeContext.Call();
	}
	/*
			Text Controller Create Event
	*/
	else if (Name == "gml_Object_obj_TextController_Create_0") {
		if (!CodeContext.CalledOriginal()) CodeContext.Call();

		RValue text_container = variable_global_get("TextContainer");
		RValue title_buttons = struct_get(text_container, "titleButtons");
		RValue eng = struct_get(title_buttons, "eng");

		std::string_view play_text = array_get(eng, 0);
		if (play_text.contains("Modded")) array_set(eng, 0, "Play Modded!");

		std::string_view lb_text = array_get(eng, 3);
		if (lb_text.contains("Mod Settings")) array_set(eng, 3, "Mod Settings");
	}
	/*
			Title Screen Draw Event
	*/
	else if (Name == "gml_Object_obj_TitleScreen_Draw_0") {
		if (!CodeContext.CalledOriginal()) CodeContext.Call();
				
		if (draw_config_menu == true) {

			// Draw Mod Config Menu Background
			draw_sprite("hud_optionsmenu", 0, 320, 48);

			// Draw Controls Text		
			commandPrompsScript(Self, Other, &result, 3, commandPrompsArgs); FIX_THIS_EVENTUALLY;

			if (config_selected == false) {
				draw_set_font("jpFont_Big");

				draw_set_halign(1);

				// Draw Mod Settings Menu Title

				drawTextOutlineArgs[2]->String = RefString::Alloc("MOD SETTINGS", strlen("MOD SETTINGS"), false);
				drawTextOutlineArgs[1]->Real = (double)(48 + 13);
				drawTextOutlineArgs[8]->Real = (double)0;
				drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

				drawTextOutlineArgs[1]->Real = (double)(48 + 10);
				drawTextOutlineArgs[8]->Real = (double)16777215;
				drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

				draw_set_font("jpFont");

				draw_set_halign(0);

				for (int i = 0; i < mods.size(); i++) {
					int y = 48 + 43 + (i * 34);

					// Draw Mod Background

					draw_sprite("hud_OptionButton", (current_mod == i && !config_hovered), 320, y);

					draw_set_color(current_mod == i && !config_hovered ? 0 : 16777215);

					// Draw Mod Name

					draw_text(320 - 78, y + 8, mods[i].name.c_str());

					// Draw Mod Checkbox

					if (config_hovered == false || current_mod != i || (config_hovered == true && current_mod == i)) {
						draw_sprite("hud_toggleButton", mods[i].enabled + (!config_hovered * 2 * (current_mod == i)), 320 + 69, 48 + 56 + (i * 34));
					} else {
						draw_sprite_ext("hud_toggleButton", mods[i].enabled, 320 + 69, 48 + 56 + (i * 34), 1, 1, 0, 0, 1);
					}

					// Draw Config Button

					if (current_mod == i) {
								
						// Draw Config Button Background

						if (config_hovered == false) {
							draw_sprite("hud_shopButton", 0, 320 + 160, y + 14);
						} else {
							draw_sprite_ext("hud_shopButton", 1, 320 + 160, y + 14, 0.75, 0.75, 0, 16777215, 1);
						}

						// Draw Left/Right Arrow
						
						draw_sprite("hud_scrollArrows2", !config_hovered, 320 + 104, y + 14);

						draw_set_halign(1);
						draw_set_valign(1);

						// Draw Config Text

						args_drawConfigNameText.args[1].I64 = (long long)y + 16;
						const char* draw_config_name_str = nullptr;
						if (mods[current_mod].hasConfig == false) {
							if (no_config_error == true) {
								std::string errorStr = (no_config_error_timer % 10 < 5 ? "" : "NO CONFIG");
								draw_config_name_str = errorStr.c_str();
							} else {
								draw_config_name_str = "NO CONFIG";
							}
							draw_set_color(255);
						} else {
							if (config_hovered == false) {
								draw_set_color(16777215);
							} else {
								draw_set_color(0);
							}
							draw_config_name_str = "CONFIG";
						}

						draw_text(320 + 161, y + 16, draw_config_name_str);

						draw_set_halign(0);
						draw_set_valign(0);
					}
				}

			} else { // config_selected == true
				draw_set_halign(1);

				draw_set_font("jpFont");

				// Draw Config Name

				drawTextOutlineArgs[2]->String = RefString::Alloc(mods[current_mod].config.name.c_str(), strlen(mods[current_mod].config.name.c_str()), false);
				drawTextOutlineArgs[1]->Real = (double)(48 + 19);
				drawTextOutlineArgs[8]->Real = (double)0;
				drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

				drawTextOutlineArgs[1]->Real = (double)(48 + 16);
				drawTextOutlineArgs[8]->Real = (double)16777215;
				drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

				draw_set_halign(0);

				int curr_setting = 0;

				for (int i = 0; i < mods[current_mod].config.settings.size(); i++) {
					if (mods[current_mod].config.settings[i].type == SETTING_BOOL) {
						int y = 48 + 43 + (curr_setting * 34);

						// Draw Setting Background

						draw_sprite("hud_OptionButton", current_mod_setting == curr_setting, 320 + 12, y);

						draw_set_color(current_mod_setting == curr_setting ? 0 : 16777215);

						// Draw Setting Name

						draw_text(320 - 66, y + 8, mods[current_mod].config.settings[i].name.c_str());

						// Draw Setting Icon
						// 0 - 8 = hud_optionIcons
						// 9 - 21 = hud_graphicIcons

						int icon_index = mods[current_mod].config.settings[i].icon;
						int actual_index = icon_index * 2;
						const char* icon_sheet_name = nullptr;
						if (actual_index > 17) {
							icon_sheet_name = "hud_graphicIcons";
							actual_index -= 18;
						} else {
							icon_sheet_name = "hud_optionIcons";
						}
						if (actual_index > -1) {
							draw_sprite(icon_sheet_name, current_mod_setting == curr_setting ? actual_index + 1 : actual_index, 320 - 98, 48 + 56 + (curr_setting * 34));
						}

						// Draw Setting Checkbox

						draw_sprite("hud_toggleButton", mods[current_mod].config.settings[i].boolValue + (2 * (current_mod_setting == curr_setting)), 320 + 81, 48 + 56 + (curr_setting * 34));

						curr_setting++;
					}
				}
			}
		}
	}
	/*
		Title Character Draw Event
	*/
	else if (Name == "gml_Object_obj_TitleCharacter_Draw_0") {
		if (draw_title_chars == true) {
			if (!CodeContext.CalledOriginal()) CodeContext.Call();
		} else {
			CodeContext.Override(true);
		}
	}
	// Tell the core the handler was successful.
	return AURIE_SUCCESS;
}

EXPORTED AurieStatus ModuleInitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
) {
	UNREFERENCED_PARAMETER(ModulePath);

	AurieStatus last_status = AURIE_SUCCESS;

	// Gets a handle to the interface exposed by YYTK
	// You can keep this pointer for future use, as it will not change unless YYTK is unloaded.
	last_status = ObGetInterface(
		"YYTK_Main",
		(AurieInterfaceBase*&)(g_ModuleInterface)
	);

	// If we can't get the interface, we fail loading.
	if (!AurieSuccess(last_status))
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;

	Print(CM_LIGHTGREEN, "[InfiCore] - Hello from PluginEntry!");

	// Create callback for Frame Events
	last_status = g_ModuleInterface->CreateCallback(
		Module,
		EVENT_FRAME,
		FrameCallback,
		999
	);

	if (!AurieSuccess(last_status)) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - Failed to register callback!");
	}

	// Create callback for Object Events
	last_status = g_ModuleInterface->CreateCallback(
		Module,
		EVENT_OBJECT_CALL,
		CodeCallback,
		999
	);

	if (!AurieSuccess(last_status)) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - Failed to register callback!");
	}

	return AURIE_SUCCESS;
}

// Create an entry routine - it must be named exactly this, and must accept these exact arguments.
// It must also be declared DllExport (notice how the other functions are not).
DllExport YYTKStatus PluginEntry(YYTKPlugin* PluginObject) {
	// Set the unload routine
	PluginObject->PluginUnload = PluginUnload;

	// Print a message to the console
	PrintMessage(CLR_DEFAULT, "[InfiCore] - Hello from PluginEntry!");

	PluginAttributes_t* PluginAttributes = nullptr;

	// Get the attributes for the plugin - this is an opaque structure, as it may change without any warning.
	// If Status == YYTK_OK (0), then PluginAttributes is guaranteed to be valid (non-null).
	if (YYTKStatus Status = PmGetPluginAttributes(PluginObject, PluginAttributes)) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - PmGetPluginAttributes failed with 0x%x", Status);
		return YYTK_FAIL;
	}

	// Register a prioritized callback for frame events
	YYTKStatus Status = PmCreateCallbackEx(
		PluginAttributes,					// Plugin Attributes
		999,								// Callback Priority
		FrameCallback,						// The function to register as a callback
		static_cast<EventType>(EVT_PRESENT | EVT_ENDSCENE), // Which events trigger this callback
		nullptr,							// The optional argument to pass to the function
		g_pFrameCallbackAttributes			// (out) Callback Attributes
	);

	if (Status) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - PmCreateCallbackEx failed with 0x%x", Status);
		return YYTK_FAIL;
	}

	// Register a prioritized callback for code events
	Status = PmCreateCallbackEx(
		PluginAttributes,					// Plugin Attributes
		999,								// Callback Priority
		CodeCallback,						// The function to register as a callback
		static_cast<EventType>(EVT_CODE_EXECUTE), // Which events trigger this callback
		nullptr,							// The optional argument to pass to the function
		g_pCodeCallbackAttributes			// (out) Callback Attributes
	);

	if (Status) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - PmCreateCallbackEx failed with 0x%x", Status);
		return YYTK_FAIL;
	}

	if (HAS_CONFIG == true) {
		// Load mod config file or create one if there isn't one already.
		const wchar_t* dirName = L"modconfigs";

		if (GetFileAttributes(dirName) == INVALID_FILE_ATTRIBUTES) {
			if (CreateDirectory(dirName, NULL)) {
				PrintMessage(CLR_GREEN, "[InfiCore] - Directory \"modconfigs\" created!");
			} else {
				PrintError(__FILE__, __LINE__, "Failed to create the modconfigs directory. Error code: %lu", GetLastError());
				return YYTK_FAIL;
			}
		}

		std::string fileName = formatString(std::string(mod.name)) + "-config.json";
		std::ifstream configFile("modconfigs/" + fileName);
		json data;
		if (configFile.is_open() == false) {	// no config file
			GenerateConfig(fileName);
		} else {
			try {
				data = json::parse(configFile);
			} catch (json::parse_error& e) {
				PrintError(__FILE__, __LINE__, "Message: %s\nException ID: %d\nByte Position of Error: %u", e.what(), e.id, (unsigned)e.byte);
				return YYTK_FAIL;
			}

			config = data.template get<ModConfig>();
		}
		PrintMessage(CLR_GREEN, "[InfiCore] - %s loaded successfully!", fileName.c_str());
	}

	// Function Pointers
	GetFunctionByName("asset_get_index", assetGetIndexFunc);
	GetFunctionByName("variable_instance_get", variableInstanceGetFunc);
	GetFunctionByName("variable_instance_set", variableInstanceSetFunc);
	GetFunctionByName("variable_global_get", variableGlobalGetFunc);
	GetFunctionByName("variable_global_set", variableGlobalSetFunc);
	GetFunctionByName("draw_set_alpha", drawSetAlphaFunc);
	GetFunctionByName("draw_set_color", drawSetColorFunc);
	GetFunctionByName("draw_text", drawTextFunc);
	GetFunctionByName("draw_sprite", drawSpriteFunc);
	GetFunctionByName("draw_sprite_ext", drawSpriteExtFunc);
	GetFunctionByName("audio_play_sound", audioPlaySoundFunc);
	GetFunctionByName("device_mouse_x_to_gui", deviceMouseXToGUIFunc);
	GetFunctionByName("device_mouse_y_to_gui", deviceMouseYToGUIFunc);
	GetFunctionByName("device_mouse_check_button_pressed", deviceMouseCheckButtonPressedFunc);
	GetFunctionByName("method_call", scriptExecuteFunc);
	GetFunctionByName("draw_set_halign", drawSetHAlignFunc);
	GetFunctionByName("draw_set_valign", drawSetVAlignFunc);
	GetFunctionByName("draw_set_font", drawSetFontFunc);

	// Function Hooks
	MH_Initialize();
	MmGetScriptData(scriptList);

	HookScriptFunction("gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0", (void*)&ConfirmedTitleScreenFuncDetour, (void**)&origConfirmedTitleScreenScript);
	HookScriptFunction("gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0", (void*)&ReturnMenuTitleScreenFuncDetour, (void**)&origReturnMenuTitleScreenScript);
	HookScriptFunction("gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectLeftTitleScreenFuncDetour, (void**)&origSelectLeftTitleScreenScript);
	HookScriptFunction("gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectRightTitleScreenFuncDetour, (void**)&origSelectRightTitleScreenScript);
	HookScriptFunction("gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectUpTitleScreenFuncDetour, (void**)&origSelectUpTitleScreenScript);
	HookScriptFunction("gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectDownTitleScreenFuncDetour, (void**)&origSelectDownTitleScreenScript);

	int drawTextOutlineIndex = getAssetIndexFromName("gml_Script_draw_text_outline") - 100000;
	CScript* drawTextOutlineCScript = scriptList(drawTextOutlineIndex);
	drawTextOutlineScript = drawTextOutlineCScript->s_pFunc->pScriptFunc;

	int commandPrompsIndex = getAssetIndexFromName("gml_Script_commandPromps") - 100000;
	CScript* commandPrompsCScript = scriptList(commandPrompsIndex);
	commandPrompsScript = commandPrompsCScript->s_pFunc->pScriptFunc;

	// Off it goes to the core.
	return YYTK_OK;
}