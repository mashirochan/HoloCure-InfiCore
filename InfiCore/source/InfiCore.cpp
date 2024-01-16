#include <YYToolkit/Shared.hpp>
#include <CallbackManager/CallbackManagerInterface.h>
#include "InfiCoreInterface.h"
#include <nlohmann/json.hpp>

#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <unordered_map>

using namespace Aurie;
using namespace YYTK;
using json = nlohmann::json;

static YYTKInterface* g_ModuleInterface = nullptr;
static CallbackManagerInterface* g_CmInterface = nullptr;
static InfiCoreInterface g_IcInterface;

std::string g_ModName = "InfiCore";
bool g_HasConfig = true;

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
		Localization Variables
*/
using LocalizationMap = std::unordered_map<std::string, std::unordered_map<std::string, std::u8string>>;

LocalizationMap locales = {
	{"eng", {
		{"version_modded", u8" (Modded)"},
		{"play_modded", u8"Play Modded!"},
		{"mod_settings", u8"Mod Settings"},
		{"mod_settings_upper", u8"MOD SETTINGS"},
		{"config", u8"CONFIG"},
		{"no_config", u8"NO CONFIG"}
	}},
	{"jp", {
		{"version_modded", u8" (MODあり)"},
		{"play_modded", u8"MODをオンにする！"},
		{"mod_settings", u8"MOD設定"},
		{"mod_settings_upper", u8"MOD設定"},
		{"config", u8"コンフィグ"},
		{"no_config", u8"ノーコンフィグ"}
	}},
	{"Id", {
		{"version_modded", u8" (Dimodifikasi)"},
		{"play_modded", u8"Mainkan Dimodifikasi!"},
		{"mod_settings", u8"Pengaturan Modifikasi"},
		{"mod_settings_upper", u8"PENGATURAN MODIFIKASI"},
		{"config", u8"KONFIGURASI"},
		{"no_config", u8"TANPA KONFIGURASI"}
	}}
};

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
	double index = CallBuiltin("asset_get_index", { name }).AsReal();
	return index;
}

inline static std::string U8ToStr(std::u8string u8str) {
	std::string str(u8str.cbegin(), u8str.cend());
	return str;
}

/*
		Helper Functions
*/
std::string FormatString(const std::string& Input) {
	std::string formatted_string = Input;

	formatted_string.erase(std::remove(formatted_string.begin(), formatted_string.end(), ' '), formatted_string.end());

	return formatted_string;
}

static void GenerateConfig(std::string FileName) {
	json data;

	for (const Setting& setting : config.settings) {
		json setting_data;
		setting_data["icon"] = setting.icon;
		if (setting.type == SETTING_BOOL) setting_data["value"] = setting.boolValue;
		data[setting.name.c_str()] = setting_data;
	}

	std::ofstream config_file("modconfigs/" + FileName);
	if (config_file.is_open()) {
		Print(CM_WHITE, "[%s] - Config file \"%s\" created!", g_ModName, FileName.c_str());
		config_file << std::setw(4) << data << std::endl;
		config_file.close();
	} else {
		PrintError(__FILE__, __LINE__, "[%s] - Error opening config file \"%s\"", g_ModName, FileName.c_str());
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
		std::string fileName = FormatString(g_ModName) + ".config.json";
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
	bool config_needs_reload = false;

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
		result = CallBuiltin("variable_instance_get", { id, name });
	}
	return result;
}

inline static void variable_instance_set(CInstance* instance_id, const char* name, double val) {
	RValue id;
	AurieStatus status = g_ModuleInterface->GetBuiltin("id", instance_id, NULL_INDEX, id);
	if (AurieSuccess(status)) {
		CallBuiltin("variable_instance_set", { id, name, val });
	}
}

inline static void variable_instance_set(CInstance* instance_id, const char* name, int val) {
	RValue id;
	AurieStatus status = g_ModuleInterface->GetBuiltin("id", instance_id, NULL_INDEX, id);
	if (AurieSuccess(status)) {
		CallBuiltin("variable_instance_set", { id, name, (double)val });
	}
}

inline static void variable_instance_set(CInstance* instance_id, const char* name, const char* val) {
	RValue id;
	AurieStatus status = g_ModuleInterface->GetBuiltin("id", instance_id, NULL_INDEX, id);
	if (AurieSuccess(status)) {
		CallBuiltin("variable_instance_set", { id, name, val });
	}
}

inline static RValue variable_global_get(const char* name) {
	return CallBuiltin("variable_global_get", { name });
}

inline static void variable_global_set(const char* name, const char* value) {
	CallBuiltin("variable_global_set", { name, value });
}

inline static void variable_global_set(const char* name, double value) {
	CallBuiltin("variable_global_set", { name, value });
}

inline static RValue struct_get(RValue variable, const char* name) {
	return CallBuiltin("struct_get", { variable, name });
}

inline static std::string_view array_get(RValue variable, double index) {
	return CallBuiltin("array_get", { variable, index }).AsString();
}

inline static void array_set(RValue variable, double index, const char* value) {
	CallBuiltin("array_set", { variable, index, value });
}

inline static void draw_sprite(const char* sprite, double subimg, double x, double y) {
	CallBuiltin("draw_sprite", { GetAssetIndexFromName(sprite), subimg, x, y });
}

inline static void draw_sprite_ext(const char* sprite, double subimg, double x, double y, double xscale, double yscale, double rot, double color, double alpha) {
	CallBuiltin("draw_sprite_ext", { GetAssetIndexFromName(sprite), subimg, x, y, xscale, yscale, rot, color, alpha });
}

inline static void draw_rectangle(double x1, double y1, double x2, double y2, bool outline) {
	CallBuiltin("draw_rectangle", { x1, y1, x2, y2, outline });
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
	CallBuiltin("draw_text", { x, y, string });
}

inline static void audio_play_sound(const char* name, double priority, bool loop) {
	CallBuiltin("audio_play_sound", { GetAssetIndexFromName(name), priority, loop });
}

PFUNC_YYGMLScript draw_text_outline_script = nullptr;
std::vector<RValue*> draw_text_outline_args = {
	new RValue(320),
	new RValue(48 + 13),
	new RValue("MOD SETTINGS", g_ModuleInterface),
	new RValue(1),
	new RValue(0),
	new RValue(32),
	new RValue(4),
	new RValue(300),
	new RValue(0),
	new RValue(1)
};

PFUNC_YYGMLScript command_promps_script = nullptr;
std::vector<RValue*> command_promps_args = {
	new RValue(1),
	new RValue(1),
	new RValue(1)
};

static bool draw_title_chars = true;
static bool draw_config_menu = false;
static bool config_hovered = false;
static bool config_selected = false;
static bool no_config_error = false;
static int no_config_error_timer = 0;
static std::vector<RemoteMod> mods;
static int current_mod = 0;
static int current_mod_setting = 0;
static std::string curr_lang = "eng";
static std::string original_version = "NO_VER";

static void LoadUnloadMods() {
	AurieStatus status{};
	for (RemoteMod& mod : mods) {
		if (mod.was_toggled != true) continue;

		std::string mod_name_no_disable = mod.name;
		if (std::string_view(mod.name).ends_with(".disabled"))
			mod_name_no_disable = mod.name.substr(0, mod.name.length() - std::string(".disabled").length());

		fs::path enabled_path = fs::current_path() / "mods" / "Aurie" / mod_name_no_disable;
		fs::path disabled_path = fs::current_path() / "mods" / "Aurie" / std::string(mod_name_no_disable + ".disabled");

		if (mod.enabled == true) {	// load mod
			std::error_code ec;
			fs::rename(disabled_path, enabled_path, ec);
			if (ec.value() == 0) {
				Print(CM_LIGHTPURPLE, "[*] Moved '%s' to '%s'!", disabled_path.string().c_str(), enabled_path.string().c_str());
				AurieModule* temp_module = nullptr;
				status = MdMapImage(enabled_path, temp_module);
				if (AurieSuccess(status)) {
					Print(CM_GREEN, "[+] Loaded '%s' - mapped to 0x%p.", mod.name.c_str(), temp_module);
				} else {
					PrintError(__FILE__, __LINE__, "Failed to load '%s'!", mod.name.c_str());
				}
			} else {
				PrintError(__FILE__, __LINE__, "Failed to move '%s' to '%s'", disabled_path.string().c_str(), enabled_path.string().c_str());
			}
		} else {					// unload mod
			AurieModule* module_to_remove = nullptr;
			status = Internal::MdpLookupModuleByPath(enabled_path, module_to_remove);
			if (AurieSuccess(status)) {
				status = MdUnmapImage(module_to_remove);
				if (AurieSuccess(status)) {
					Print(CM_RED, "[-] Unloaded '%s'", mod.name.c_str());
					std::error_code ec;
					fs::rename(enabled_path, disabled_path, ec);
					if (ec.value() == 0) {
						Print(CM_LIGHTPURPLE, "[*] Moved '%s' to '%s'!", enabled_path.string().c_str(), disabled_path.string().c_str());
					} else {
						PrintError(__FILE__, __LINE__, "Failed to move '%s' to '%s'", enabled_path.string().c_str(), disabled_path.string().c_str());
					}
				} else {
					PrintError(__FILE__, __LINE__, "Failed to unload '%s' at 0x%p!", mod.name.c_str(), module_to_remove);
				}
			} else {
				PrintError(__FILE__, __LINE__, "Failed to get module '%s' at '%s'!", mod.name.c_str(), enabled_path.string().c_str());
			}
		}

		mod.was_toggled = false;
	}
}

static void GetMods() {
	// Find Mods
	mods.clear();
	std::string mods_path = "mods\\Aurie";
	for (const auto& entry : fs::directory_iterator(mods_path)) {
		std::string mod_name = entry.path().filename().string();
		RemoteMod new_mod(mod_name, mod_name.find(".disabled") != std::string::npos ? false : true);
		mods.push_back(new_mod);
	}

	// Sort Mods Alphabetically
	std::sort(mods.begin(), mods.end(), [](const RemoteMod& a, const RemoteMod& b) {
		auto case_insensitive_compare = [](char a, char b) {
			return std::tolower(a) < std::tolower(b);
		};

		// Move "YYToolkit.dll" to the front
		if (a.name == "YYToolkit.dll") return true;
		if (b.name == "YYToolkit.dll") return false;

		// Move "InfiCore.dll" to the front after "YYToolkit.dll"
		if (a.name == (FormatString(g_ModName) + ".dll")) return true;
		if (b.name == (FormatString(g_ModName) + ".dll")) return false;

		// Move "CallbackManagerMod.dll" to the front after "InfiCore.dll"
		if (a.name == "CallbackManagerMod.dll") return true;
		if (b.name == "CallbackManagerMod.dll") return false;

		// For other elements, use lexicographical comparison
		return std::lexicographical_compare(
			a.name.begin(), a.name.end(),
			b.name.begin(), b.name.end(),
			case_insensitive_compare
		);
    });
}

static void GetModConfigs() {
	// Get Configs for Mods
	for (RemoteMod& mod : mods) {
		std::string config_name = mod.name.substr(0, mod.name.size() - 4) + ".config.json";
		fs::path config_path = "modconfigs/" + config_name;

		if (fs::exists(config_path)) {
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
	for (RemoteMod& mod : mods) {
		if (mod.has_config == false || !mod.config_needs_reload) continue;
		std::ifstream in_file("modconfigs/" + mod.config.name);
		if (in_file.fail()) {
			PrintError(__FILE__, __LINE__, "Config for \"%s\" not found!", mod.name);
			continue;
		}
		json data;
		in_file >> data;
		in_file.close();

		for (int setting = 0; setting < mod.config.settings.size(); setting++) {
			if (mod.config.settings[setting].type == SETTING_BOOL) {
				std::cout << "name: " << mod.config.settings[setting].name << std::endl;
				std::cout << "value: " << mod.config.settings[setting].boolValue << std::endl;
				data[mod.config.settings[setting].name]["value"] = mod.config.settings[setting].boolValue;
			}
		}

		std::ofstream out_file("modconfigs/" + mod.config.name);
		out_file << data.dump(4);
		out_file.close();

		// Tell the mod that it needs to update its settings from config file
		g_IcInterface.CallConfigCallback(mod.name);
	}
}

static std::string GetFileName(const char* File) {
	std::string s_file_name(File);
	size_t last_slash_pos = s_file_name.find_last_of("\\");
	if (last_slash_pos != std::string::npos && last_slash_pos != s_file_name.length()) {
		s_file_name = s_file_name.substr(last_slash_pos + 1);
	}
	return s_file_name;
}

static uint32_t FrameNumber = 0;
static const int max_menu_entries = 7;
static int menu_offset = 0;
static int menu_pos = 0;
static int show_option_range = 0;

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

// gml_Object_obj_TitleScreen_Create_0
void TitleScreenCreateBefore(CodeEventArgs& Args) {
	GetMods();
	GetModConfigs();
	curr_lang = variable_global_get("CurrentLanguage").AsString();
	if (original_version == "NO_VER") original_version = variable_global_get("version").AsString();
	variable_global_set("version", (original_version + U8ToStr(locales[curr_lang]["version_modded"])).c_str());
}

// gml_Object_obj_TextController_Create_0
void TextControllerCreateAfter(CodeEventArgs& Args) {
	RValue text_container = variable_global_get("TextContainer");
	text_container["titleButtons"]["eng"][0] = locales["eng"]["play_modded"];
	text_container["titleButtons"]["jp"][0] = locales["jp"]["play_modded"];
	text_container["titleButtons"]["Id"][0] = locales["Id"]["play_modded"];
	text_container["titleButtons"]["eng"][3] = locales["eng"]["mod_settings"];
	text_container["titleButtons"]["jp"][3] = locales["jp"]["mod_settings"];
	text_container["titleButtons"]["Id"][3] = locales["Id"]["mod_settings"];
}

// gml_Object_obj_TitleScreen_Draw_0
void TitleScreenDrawAfter(CodeEventArgs& Args) {
	CInstance* Self = std::get<0>(Args);
	CInstance* Other = std::get<1>(Args);

	RValue result;

	if (draw_config_menu == true) {

		// Draw Mod Config Menu Background
		draw_sprite("hud_optionsmenu", 0, 320, 48);

		// Draw Controls Text
		command_promps_script(Self, Other, result, 3, command_promps_args.data());

		if (config_selected == false) {
			draw_set_font("jpFont_Big");

			draw_set_halign(1);

			// Draw Mod Settings Menu Title

			draw_text_outline_args[2] = new RValue(locales[curr_lang]["mod_settings_upper"]);
			draw_text_outline_args[1] = new RValue(48 + 13);
			draw_text_outline_args[8] = new RValue(0);
			draw_text_outline_script(Self, Other, result, 10, draw_text_outline_args.data());

			draw_text_outline_args[1] = new RValue(48 + 10);
			draw_text_outline_args[8] = new RValue(16777215);
			draw_text_outline_script(Self, Other, result, 10, draw_text_outline_args.data());

			draw_set_font("jpFont");

			draw_set_halign(0);

			for (int i = 0; i < mods.size() && i < max_menu_entries; i++) {
				int y = 48 + 43 + (i * 34);

				// Draw Mod Background

				draw_sprite("hud_OptionButton", (current_mod == i + menu_offset && !config_hovered), 320, y);

				draw_set_color(mods[i + menu_offset].enabled ? (current_mod == i + menu_offset && !config_hovered ? 0 : 16777215) : 8421504);

				// Draw Mod Name
				std::string formatted_name = mods[i + menu_offset].name;
				size_t pos = formatted_name.find(".disabled");
				if (mods[i + menu_offset].enabled && pos != std::string::npos) {
					formatted_name.erase(pos, std::string(".disabled").length());
				}
				if (formatted_name.length() > 22) {
					formatted_name = formatted_name.substr(0, 19);
					formatted_name += "...";
				}
				draw_text(320 - 78, y + 8, formatted_name.c_str());

				// Draw Mod Checkbox
				if (mods[i].name != "YYToolkit.dll" && mods[i + menu_offset].name != "InfiCore.dll" && mods[i + menu_offset].name != "CallbackManagerMod.dll") {
					if (config_hovered == false || current_mod != i + menu_offset || (config_hovered == true && current_mod == i + menu_offset)) {
						draw_sprite("hud_toggleButton", mods[i + menu_offset].enabled + (!config_hovered * 2 * (current_mod == i + menu_offset)), 320 + 69, 48 + 56 + (i * 34));
					} else {
						draw_sprite_ext("hud_toggleButton", mods[i + menu_offset].enabled, 320 + 69, 48 + 56 + (i * 34), 1, 1, 0, 0, 1);
					}
				}

				// Draw Config Button

				if (current_mod == i + menu_offset) {

					// Draw Config Button Background

					if (config_hovered == false) {
						draw_sprite("hud_shopButton", 0, 320 + 160, y + 14);
					} else {
						draw_sprite_ext("hud_shopButton", 1, 320 + 160, y + 14, 0.75, 0.75, 0, 16777215, 1);
					}

					// Draw Left/Right Arrow

					draw_sprite("hud_scrollArrows2", !config_hovered, 320 + 97, y + 14);

					draw_set_halign(1);
					draw_set_valign(1);

					// Draw Config Text

					std::string draw_config_name_str;
					if (mods[current_mod].has_config == false) {
						if (no_config_error == true) {
							draw_config_name_str = (no_config_error_timer % 10 < 5 ? "" : U8ToStr(locales[curr_lang]["no_config"]));
						} else {
							draw_config_name_str = U8ToStr(locales[curr_lang]["no_config"]);
						}
						draw_set_color(255);
					} else {
						if (config_hovered == false) {
							draw_set_color(16777215);
						} else {
							draw_set_color(0);
						}
						draw_config_name_str = U8ToStr(locales[curr_lang]["config"]);
					}

					draw_text(320 + 161, y + 16, draw_config_name_str.c_str());

					draw_set_halign(0);
					draw_set_valign(0);
				}
			}

			if (mods.size() > max_menu_entries) {
				draw_sprite("hud_scrollArrows", 0, (320 + 109), (48 + 39));
				draw_sprite("hud_scrollArrows", 1, (320 + 109), (48 + 269));

				int rect_height = (1540 / mods.size());
				int scroll_dist = (220 / mods.size());

				draw_set_color(16777215);
				draw_rectangle((320 + 108), ((48 + 44) + (scroll_dist * show_option_range)), (320 + 110), (((48 + 44) + rect_height) + (scroll_dist * show_option_range)), false);
			}

		} else { // config_selected == true
			draw_set_halign(1);

			draw_set_font("jpFont");

			// Draw Config Name

			draw_text_outline_args[2] = new RValue(mods[current_mod].config.name.c_str());
			draw_text_outline_args[1] = new RValue(48 + 19);
			draw_text_outline_args[8] = new RValue(0);
			draw_text_outline_script(Self, Other, result, 10, draw_text_outline_args.data());

			draw_text_outline_args[1] = new RValue(48 + 16);
			draw_text_outline_args[8] = new RValue(16777215);
			draw_text_outline_script(Self, Other, result, 10, draw_text_outline_args.data());

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

// gml_Object_obj_TitleCharacter_Draw_0
void TitleCharacterDrawBefore(CodeEventArgs& Args) {
	if (draw_title_chars == false) {
		g_CmInterface->CancelOriginalFunction();
	}
}

/*
		Script Callback Functions
*/

// gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0
RValue& ConfirmedTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int NumArgs, RValue** Args) {
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
			if (mods[current_mod].name == "YYToolkit.dll" || mods[current_mod].name == "InfiCore.dll" || mods[current_mod].name == "CallbackManagerMod.dll") {
				audio_play_sound("snd_alert", 30, false);
			} else {
				mods[current_mod].enabled = !mods[current_mod].enabled;
				mods[current_mod].was_toggled = !mods[current_mod].was_toggled;
				audio_play_sound("snd_menu_confirm", 30, false);
			}
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
				mods[current_mod].config_needs_reload = true;
				audio_play_sound("snd_menu_confirm", 30, false);
			}
		}
		g_CmInterface->CancelOriginalFunction();
	}
	return ReturnValue;
};

// gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0

RValue& ReturnMenuTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int NumArgs, RValue** Args) {
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3) { // 3 = mod configs button index
		if (config_selected == true) {
			config_selected = false;
			current_mod_setting = 0;
			audio_play_sound("snd_menu_back", 30, false);
		} else if (can_control == 0) {
			variable_instance_set(Self, "canControl", 1);
			variable_global_set("lastTitleOption", 3);
			draw_title_chars = true;
			draw_config_menu = false;
			audio_play_sound("snd_menu_back", 30, false);
			SetConfigSettings();
			LoadUnloadMods();
			GetModConfigs();
		}
		g_CmInterface->CancelOriginalFunction();
	} else {
		curr_lang = variable_global_get("CurrentLanguage").AsString();
		if (original_version == "NO_VER") original_version = variable_global_get("version").AsString();
		variable_instance_set(Self, "version", (original_version + U8ToStr(locales[curr_lang]["version_modded"])).c_str());
	}
	return ReturnValue;
};

// gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0
RValue& SelectLeftTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int NumArgs, RValue** Args) {
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (config_selected == false) {
			if (config_hovered == true) {
				config_hovered = false;
				audio_play_sound("snd_charSelectWoosh", 30, false);
			}
		}
		g_CmInterface->CancelOriginalFunction();
	}
	return ReturnValue;
};

// gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0
RValue& SelectRightTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int NumArgs, RValue** Args) {
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (config_selected == false) {
			if (config_hovered == false) {
				config_hovered = true;
				audio_play_sound("snd_charSelectWoosh", 30, false);
			}
		}
		g_CmInterface->CancelOriginalFunction();
	}
	return ReturnValue;
};

// gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0
RValue& SelectUpTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int NumArgs, RValue** Args) {
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (current_mod > 0 && config_selected == false) {
			current_mod--;
			if (menu_pos > 0) menu_pos--;
			else {
				menu_offset--;
				if (show_option_range > 0) show_option_range--;
			}
			audio_play_sound("snd_menu_select", 30, false);
		} else if (current_mod_setting > 0 && config_selected == true) {
			current_mod_setting--;
			audio_play_sound("snd_menu_select", 30, false);
		}
		g_CmInterface->CancelOriginalFunction();
	}
	return ReturnValue;
};

// gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0
RValue& SelectDownTitleScreenBefore(CInstance* Self, CInstance* Other, RValue& ReturnValue, int NumArgs, RValue** Args) {
	int can_control = variable_instance_get(Self, "canControl").AsReal();
	int current_option = variable_instance_get(Self, "currentOption").AsReal();
	if (current_option == 3 && draw_config_menu == true) { // 3 = mod configs button index
		if (current_mod < mods.size() - 1 && config_selected == false) {
			current_mod++;
			if (menu_pos < max_menu_entries - 1) menu_pos++;
			else {
				menu_offset++;
				if (show_option_range < mods.size() - max_menu_entries) show_option_range++;
			}
			audio_play_sound("snd_menu_select", 30, false);
		} else if (current_mod_setting < mods[current_mod].config.settings.size() - 1 && config_selected == true) {
			current_mod_setting++;
			audio_play_sound("snd_menu_select", 30, false);
		}
		g_CmInterface->CancelOriginalFunction();
	}
	return ReturnValue;
};

/*
		Module Pre-initialize
*/
EXPORTED AurieStatus ModulePreinitialize(
	IN AurieModule* Module,
	IN const fs::path& ModulePath
) {
	AurieStatus status = ObCreateInterface(Module, &g_IcInterface, "InfiCore");

	if (!AurieSuccess(status))
		return AURIE_MODULE_INITIALIZATION_FAILED;

	return AURIE_SUCCESS;
}

/*
		Module Initialize
*/
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

	Print(CM_LIGHTGREEN, "[%s] - Hello from PluginEntry!", g_ModName);

	status = ObGetInterface(
		"callbackManager",
		(AurieInterfaceBase*&)(g_CmInterface)
	);

	if (!AurieSuccess(status)) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to get Callback Manager Interface! Make sure that CallbackManagerMod is located in the mods/Aurie directory.", g_ModName);
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	Print(CM_LIGHTGREEN, "[%s] - Callback Manager Interface loaded!", g_ModName);

	// Create callback for Frame Events
	status = g_ModuleInterface->CreateCallback(
		Module,
		EVENT_FRAME,
		FrameCallback,
		999
	);

	if (!AurieSuccess(status)) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback!", g_ModName);
	}

	if (g_HasConfig == true) {
		// Load mod config file or create one if there isn't one already.
		const wchar_t* dir_name = L"modconfigs";

		if (GetFileAttributes(dir_name) == INVALID_FILE_ATTRIBUTES) {
			if (CreateDirectory(dir_name, NULL)) {
				Print(CM_LIGHTGREEN, "[%s] - Directory \"modconfigs\" created!", g_ModName);
			} else {
				PrintError(__FILE__, __LINE__, "[%s] - Failed to create the modconfigs directory. Error code: %lu", g_ModName, GetLastError());
				return AURIE_ACCESS_DENIED;
			}
		}

		std::string file_name = FormatString(g_ModName) + ".config.json";
		std::ifstream config_file("modconfigs/" + file_name);
		json data;
		if (config_file.is_open() == false) {	// no config file
			GenerateConfig(file_name);
		} else {
			try {
				data = json::parse(config_file);
			} catch (json::parse_error& e) {
				PrintError(__FILE__, __LINE__, "[%s] - Message: %s\nException ID: %d\nByte Position of Error: %u", g_ModName, e.what(), e.id, (unsigned)e.byte);
				return AURIE_FILE_PART_NOT_FOUND;
			}

			config = data.template get<ModConfig>();
		}
		Print(CM_LIGHTGREEN, "[%s] - %s loaded successfully!", g_ModName, file_name.c_str());
	}

	/*
			Code Event Hooks
	*/
	if (!AurieSuccess(g_CmInterface->RegisterCodeEventCallback(g_ModName, "gml_Object_obj_TitleScreen_Create_0", TitleScreenCreateBefore, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterCodeEventCallback(g_ModName, "gml_Object_obj_TextController_Create_0", nullptr, TextControllerCreateAfter))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Object_obj_TextController_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterCodeEventCallback(g_ModName, "gml_Object_obj_TitleScreen_Draw_0", nullptr, TitleScreenDrawAfter))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Object_obj_TitleScreen_Draw_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterCodeEventCallback(g_ModName, "gml_Object_obj_TitleCharacter_Draw_0", TitleCharacterDrawBefore, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Object_obj_TitleCharacter_Draw_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	/*
			Script Function Hooks
	*/
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0", ConfirmedTitleScreenBefore, nullptr, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0", ReturnMenuTitleScreenBefore, nullptr, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_ReturnMenu_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0", SelectLeftTitleScreenBefore, nullptr, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_SelectLeft_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0", SelectRightTitleScreenBefore, nullptr, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_SelectRight_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0", SelectUpTitleScreenBefore, nullptr, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_SelectUp_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0", SelectDownTitleScreenBefore, nullptr, nullptr))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_SelectDown_gml_Object_obj_TitleScreen_Create_0");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	/*
			Script Function Pointers
	*/
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_draw_text_outline", nullptr, nullptr, &draw_text_outline_script))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_draw_text_outline");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
	if (!AurieSuccess(g_CmInterface->RegisterScriptFunctionCallback(g_ModName, "gml_Script_commandPromps", nullptr, nullptr, &command_promps_script))) {
		PrintError(__FILE__, __LINE__, "[%s] - Failed to register callback for %s", g_ModName, "gml_Script_commandPromps");
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}

	Print(CM_LIGHTGREEN, "[%s] - Everything initialized successfully!", g_ModName);

	return AURIE_SUCCESS;
}