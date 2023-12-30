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

/*
		Config Variables
*/
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
		Helper Functions
*/
std::string FormatString(const std::string& input) {
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
		std::string fileName = FormatString(std::string("InfiCore")) + "-config.json";
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
	bool was_toggled = false;
	RemoteConfig config;
	bool has_config = false;

	RemoteMod(std::string n, bool e) {
		name = n;
		enabled = e;
	}
};

/*
		GML Inline Functions
*/
inline static RValue variable_instance_get(CInstance* instance_id, const char* name) {
	RValue id, result;
	AurieStatus status = g_ModuleInterface->GetBuiltin("id", instance_id, NULL_INDEX, id);
	if (AurieSuccess(status)) {
		result = CallBuiltin("variable_instance_get", { id, RValue(name, g_ModuleInterface) });
	}
	return result;
}

inline static void variable_instance_set(CInstance* instance_id, const char* name, double val) {
	RValue id;
	AurieStatus status = g_ModuleInterface->GetBuiltin("id", instance_id, NULL_INDEX, id);
	if (AurieSuccess(status)) {
		CallBuiltin("variable_instance_set", { id, RValue(name, g_ModuleInterface), val });
	}
}

inline static RValue variable_global_get(const char* name) {
	return CallBuiltin("variable_global_get", { RValue(name, g_ModuleInterface) });
}

inline static void variable_global_set(const char* name, const char* value) {
	CallBuiltin("variable_global_set", { RValue(name, g_ModuleInterface), RValue(value, g_ModuleInterface) });
}

inline static void variable_global_set(const char* name, double value) {
	CallBuiltin("variable_global_set", { RValue(name, g_ModuleInterface), value });
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

PFUNC_YYGMLScript draw_text_outline_script = nullptr;
std::vector<RValue*> draw_text_outline_args = {
	new RValue((double)320),
	new RValue((double)(48 + 13)),
	new RValue("MOD SETTINGS", g_ModuleInterface),
	new RValue((double)1),
	new RValue((double)0),
	new RValue((double)32),
	new RValue((double)4),
	new RValue((double)300),
	new RValue((double)0),
	new RValue((double)1)
};

PFUNC_YYGMLScript command_promps_script = nullptr;
std::vector<RValue*> command_promps_args = {
	new RValue((double)1),
	new RValue((double)1),
	new RValue((double)1)
};


static void HookScriptFunction(AurieModule* Module, const char* HookIdentifier, const char* ScriptFunctionName, void* DetourFunction, void** OrigScript) {
	int script_function_index = 0;
	AurieStatus status = g_ModuleInterface->GetNamedRoutineIndex(ScriptFunctionName, &script_function_index);

	if (!AurieSuccess(status)) return;

	CScript* script_function = nullptr;
	status = g_ModuleInterface->GetScriptData(script_function_index - 100000, script_function);

	if (!AurieSuccess(status)) return;

	status = MmCreateHook(Module, HookIdentifier, script_function->m_Functions->m_CodeFunction, DetourFunction, reinterpret_cast<PVOID*>(&OrigScript));
}

bool HAS_CONFIG = true;

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
		if (mod.was_toggled != true) continue;

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

		mod.was_toggled = false;
	}
}

static void GetMods() {
	// Find Mods
	std::string mods_path = "mods\\Aurie";
	for (const auto& entry : fs::directory_iterator(mods_path)) {
		std::string mod_name = entry.path().filename().string();
		RemoteMod new_mod(mod_name, mod_name.find(".disabled") != std::string::npos ? false : true);
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
			mod.has_config = true;
		} else {
			mod.has_config = false;
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

typedef RValue* (*ScriptFunc)(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args);

// gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origConfirmedTitleScreenScript = nullptr;
static RValue* ConfirmedTitleScreenFuncDetour(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args) {
	RValue* res = nullptr;
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3) { // 3 = mod configs button index
		if (can_control == 1) {
			variable_instance_set(Self, "canControl", 0);
			variable_global_set("lastTitleOption", 3);
			draw_title_chars = false;
			draw_config_menu = true;
			audio_play_sound("snd_menu_confirm", 30, false);
		} else if (config_hovered == false) {
			mods[current_mod].enabled = !mods[current_mod].enabled;
			mods[current_mod].was_toggled = !mods[current_mod].was_toggled;
			audio_play_sound("snd_menu_confirm", 30, false);
		} else if (config_selected == false) {
			if (mods[current_mod].has_config == false) {
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
		RValue* res = origConfirmedTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origReturnMenuTitleScreenScript = nullptr;
static RValue* ReturnMenuTitleScreenFuncDetour(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args) {
	RValue* res = nullptr;
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3) { // 3 = mod configs button index
		if (config_selected == true) {
			config_selected = false;
			current_mod_setting = 0;
			audio_play_sound("snd_menu_back", 30, false);
		} else if (can_control == 0)  {
			variable_instance_set(Self, "canControl", 1);
			variable_global_set("lastTitleOption", 3);
			draw_title_chars = true;
			draw_config_menu = false;
			audio_play_sound("snd_menu_back", 30, false);
			SetConfigSettings();
			LoadUnloadMods();
		}
	} else {
		RValue* res = origReturnMenuTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectLeftTitleScreenScript = nullptr;
static RValue* SelectLeftTitleScreenFuncDetour(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args) {
	RValue* res = nullptr;
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (config_selected == false) {
			if (config_hovered == true) {
				config_hovered = false;
				audio_play_sound("snd_charSelectWoosh", 30, false);
			}
		}
	} else {
		RValue* res = origSelectLeftTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectRightTitleScreenScript = nullptr;
static RValue* SelectRightTitleScreenFuncDetour(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args) {
	RValue* res = nullptr;
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (config_selected == false) {
			if (config_hovered == false) {
				config_hovered = true;
				audio_play_sound("snd_charSelectWoosh", 30, false);
			}
		}
	} else {
		RValue* res = origSelectRightTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectUpTitleScreenScript = nullptr;
static RValue* SelectUpTitleScreenFuncDetour(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args) {
	RValue* res = nullptr;
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (current_mod > 0 && config_selected == false) {
			current_mod--;
			audio_play_sound("snd_menu_select", 30, false);
		} else if (current_mod_setting > 0 && config_selected == true) {
			current_mod_setting--;
			audio_play_sound("snd_menu_select", 30, false);
		}
	} else {
		RValue* res = origSelectUpTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

// gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origSelectDownTitleScreenScript = nullptr;
static RValue* SelectDownTitleScreenFuncDetour(CInstance* Self, CInstance* Other, RValue* ReturnValue, int numArgs, RValue** Args) {
	RValue* res = nullptr;
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (current_mod < mods.size() - 1 && config_selected == false) {
			current_mod++;
			audio_play_sound("snd_menu_select", 30, false);
		} else if (current_mod_setting < mods[current_mod].config.settings.size() - 1 && config_selected == true) {
			current_mod_setting++;
			audio_play_sound("snd_menu_select", 30, false);
		}
	} else {
		RValue* res = origSelectDownTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
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

static uint32_t FrameNumber = 0;
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
			if (version.find("Modded") != std::string::npos) {
				std::string modded_ver_str = std::string(version) + " (Modded)";
				variable_global_set("version", modded_ver_str.c_str());
			}
			version_text_changed = true;
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
		if (play_text.find("Modded") != std::string::npos) array_set(eng, 0, "Play Modded!");

		std::string_view lb_text = array_get(eng, 3);
		if (lb_text.find("Mod Settings") != std::string::npos) array_set(eng, 3, "Mod Settings");
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
			command_promps_script(Self, Other, &result, 3, command_promps_args.data());

			if (config_selected == false) {
				draw_set_font("jpFont_Big");

				draw_set_halign(1);

				// Draw Mod Settings Menu Title

				draw_text_outline_args[2] = new RValue("MOD SETTINGS", g_ModuleInterface);
				draw_text_outline_args[1] = new RValue((double)(48 + 13));
				draw_text_outline_args[8] = new RValue((double)0);
				draw_text_outline_script(Self, Other, &result, 10, draw_text_outline_args.data());

				draw_text_outline_args[1] = new RValue((double)(48 + 10));
				draw_text_outline_args[8] = new RValue((double)16777215);
				draw_text_outline_script(Self, Other, &result, 10, draw_text_outline_args.data());

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

						const char* draw_config_name_str = nullptr;
						if (mods[current_mod].has_config == false) {
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

				draw_text_outline_args[2] = new RValue(mods[current_mod].config.name.c_str(), g_ModuleInterface);
				draw_text_outline_args[1] = new RValue((double)(48 + 19));
				draw_text_outline_args[8] = new RValue((double)0);
				draw_text_outline_script(Self, Other, &result, 10, draw_text_outline_args.data());

				draw_text_outline_args[1] = new RValue((double)(48 + 16));
				draw_text_outline_args[8] = new RValue((double)16777215);
				draw_text_outline_script(Self, Other, &result, 10, draw_text_outline_args.data());

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

	AurieStatus status = AURIE_SUCCESS;

	// Gets a handle to the interface exposed by YYTK
	// You can keep this pointer for future use, as it will not change unless YYTK is unloaded.
	status = ObGetInterface(
		"YYTK_Main",
		(AurieInterfaceBase*&)(g_ModuleInterface)
	);

	// If we can't get the interface, we fail loading.
	if (!AurieSuccess(status))
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;

	Print(CM_LIGHTGREEN, "[InfiCore] - Hello from PluginEntry!");

	// Create callback for Frame Events
	status = g_ModuleInterface->CreateCallback(
		Module,
		EVENT_FRAME,
		FrameCallback,
		999
	);

	if (!AurieSuccess(status)) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - Failed to register callback!");
	}

	// Create callback for Object Events
	status = g_ModuleInterface->CreateCallback(
		Module,
		EVENT_OBJECT_CALL,
		CodeCallback,
		999
	);

	if (!AurieSuccess(status)) {
		PrintError(__FILE__, __LINE__, "[InfiCore] - Failed to register callback!");
	}

	if (HAS_CONFIG == true) {
		// Load mod config file or create one if there isn't one already.
		const wchar_t* dirName = L"modconfigs";

		if (GetFileAttributes(dirName) == INVALID_FILE_ATTRIBUTES) {
			if (CreateDirectory(dirName, NULL)) {
				Print(CM_GREEN, "[InfiCore] - Directory \"modconfigs\" created!");
			} else {
				PrintError(__FILE__, __LINE__, "Failed to create the modconfigs directory. Error code: %lu", GetLastError());
				return AURIE_ACCESS_DENIED;
			}
		}

		std::string fileName = FormatString(std::string("InfiCore")) + "-config.json";
		std::ifstream configFile("modconfigs/" + fileName);
		json data;
		if (configFile.is_open() == false) {	// no config file
			GenerateConfig(fileName);
		} else {
			try {
				data = json::parse(configFile);
			} catch (json::parse_error& e) {
				PrintError(__FILE__, __LINE__, "Message: %s\nException ID: %d\nByte Position of Error: %u", e.what(), e.id, (unsigned)e.byte);
				return AURIE_FILE_PART_NOT_FOUND;
			}

			config = data.template get<ModConfig>();
		}
		Print(CM_GREEN, "[InfiCore] - %s loaded successfully!", fileName.c_str());
	}

	/*
			Function Hooks
	*/
	HookScriptFunction(Module, "Confirmed", "gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0", (void*)&ConfirmedTitleScreenFuncDetour, (void**)&origConfirmedTitleScreenScript);
	HookScriptFunction(Module, "ReturnMenu", "gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0", (void*)&ReturnMenuTitleScreenFuncDetour, (void**)&origReturnMenuTitleScreenScript);
	HookScriptFunction(Module, "SelectLeft", "gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectLeftTitleScreenFuncDetour, (void**)&origSelectLeftTitleScreenScript);
	HookScriptFunction(Module, "SelectRight", "gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectRightTitleScreenFuncDetour, (void**)&origSelectRightTitleScreenScript);
	HookScriptFunction(Module, "SelectUp", "gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectUpTitleScreenFuncDetour, (void**)&origSelectUpTitleScreenScript);
	HookScriptFunction(Module, "SelectDown", "gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0", (void*)&SelectDownTitleScreenFuncDetour, (void**)&origSelectDownTitleScreenScript);

	int draw_text_outline_index = 0;
	status = g_ModuleInterface->GetNamedRoutineIndex("gml_Script_draw_text_outline", &draw_text_outline_index);
	if (!AurieSuccess(status)) return AURIE_OBJECT_NOT_FOUND;
	CScript* draw_text_outline_cscript = nullptr;
	status = g_ModuleInterface->GetScriptData(draw_text_outline_index - 10000, draw_text_outline_cscript);
	if (!AurieSuccess(status)) return AURIE_OBJECT_NOT_FOUND;
	draw_text_outline_script = draw_text_outline_cscript->m_Functions->m_ScriptFunction;

	int command_promps_index = 0;
	status = g_ModuleInterface->GetNamedRoutineIndex("gml_Script_commandPromps", &command_promps_index);
	if (!AurieSuccess(status)) return AURIE_OBJECT_NOT_FOUND;
	CScript* command_promps_cscript = nullptr;
	status = g_ModuleInterface->GetScriptData(command_promps_index - 10000, command_promps_cscript);
	if (!AurieSuccess(status)) return AURIE_OBJECT_NOT_FOUND;
	command_promps_script = command_promps_cscript->m_Functions->m_ScriptFunction;

	Print(CM_LIGHTPURPLE, "END OF INITIALIZE REACHED");

	return AURIE_SUCCESS;
}