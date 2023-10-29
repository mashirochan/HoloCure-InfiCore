#define YYSDK_PLUGIN
#include "InfiCore.hpp"	// Include our header
#include "ModInfo.h"
#include <Windows.h>    // Include Windows's mess.
#include <vector>       // Include the STL vector.
#include <unordered_map>
#include <functional>
#include <iostream>
#include <fstream>
#include "json.hpp"
#include <filesystem>
#include "Utils/MH/MinHook.h"
#include "Features/API/Internal.hpp"
using json = nlohmann::json;

static struct Version {
	int major = VERSION_MAJOR;
	int minor = VERSION_MINOR;
	int build = VERSION_BUILD;
} version;

static struct Mod {
	Version version;
	const char* name = MOD_NAME;
} mod;

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
		std::string fileName = formatString(std::string(mod.name)) + "-config.json";
		GenerateConfig(fileName);
	}
}

static struct RemoteConfig {
	std::string name;
	std::vector<Setting> settings;
};

static struct RemoteMod {
	std::string name;
	bool enabled;
	RemoteConfig config;
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
	json data = config;

	std::ofstream configFile("modconfigs/" + fileName);
	if (configFile.is_open()) {
		PrintMessage(CLR_DEFAULT, "[%s v%d.%d.%d] - Config file \"%s\" created!", mod.name, mod.version.major, mod.version.minor, mod.version.build, fileName.c_str());
		configFile << std::setw(4) << data << std::endl;
		configFile.close();
	} else {
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - Error opening config file \"%s\"", mod.name, mod.version.major, mod.version.minor, mod.version.build, fileName.c_str());
	}
}

// Function Hooks
using FNScriptData = CScript * (*)(int);
FNScriptData scriptList = nullptr;

TRoutine assetGetIndexFunc = nullptr;

inline int getAssetIndexFromName(const char* name) {
	RValue Result;
	RValue arg{};
	arg.Kind = VALUE_STRING;
	arg.String = RefString::Alloc(name, strlen(name));
	assetGetIndexFunc(&Result, nullptr, nullptr, 1, &arg);
	return static_cast<int>(Result.Real);
}

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
	}	// (double)320, (double)(48 + 13), "TEST", (double)1, (long long)0, (double)32, (double)4, (double)100, (long long)0, (double)1)
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
YYRValue* drawTextOutlineArgs[10];
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

PFUNC_YYGMLScript drawTextOutlineScript = nullptr;
PFUNC_YYGMLScript commandPrompsScript = nullptr;

ArgSetup args_commandPromps;
YYRValue* commandPrompsArgs[3];

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
	if (TargetFuncPointer) {
		auto Status = MH_CreateHook(TargetFuncPointer, NewFunc, pfnOriginal);
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

YYRValue result;
YYRValue yyrv_mouseX;
YYRValue yyrv_mouseY;
static bool mouseMoved = false;
int prev_mouseX = 0;
int prev_mouseY = 0;

static bool drawTitleChars = true;
static bool drawConfigMenu = false;
static bool configSelected = false;
static std::vector<RemoteMod> mods;
static int currentMod = 0;
static int currentModSetting = 0;

static void SetConfigSettings() {
	for (int mod = 0; mod < mods.size(); mod++) {
		std::ifstream inFile("modconfigs/" + mods[mod].config.name);
		json data;
		inFile >> data;
		inFile.close();

		for (int setting = 0; setting < mods[mod].config.settings.size(); setting++) {
			if (mods[mod].config.settings[setting].type == SETTING_BOOL) {
				data[mods[mod].config.settings[setting].name]["value"] = mods[mod].config.settings[setting].boolValue;
			}
		}

		std::ofstream outFile("modconfigs/" + mods[mod].config.name);
		outFile << data.dump(4);
		outFile.close();
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
			drawTitleChars = false;
			drawConfigMenu = true;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_confirm");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
		} else if (configSelected == false) {
			configSelected = true;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_confirm");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
		} else if (configSelected == true) {
			if (mods[currentMod].config.settings[currentModSetting].type == SETTING_BOOL) {
				mods[currentMod].config.settings[currentModSetting].boolValue = !mods[currentMod].config.settings[currentModSetting].boolValue;
				args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_confirm");
				audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
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
		if (configSelected == true) {
			configSelected = false;
			currentModSetting = 0;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_back");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
		} else if (static_cast<int>(yyrv_canControl) == 0)  {
			args_canControl.args[2].Real = 1;
			variableInstanceSetFunc(&result, Self, Other, 3, args_canControl.args);
			args_lastTitleOption.args[1].Real = 3;
			variableGlobalSetFunc(&result, Self, Other, 2, args_lastTitleOption.args);
			drawTitleChars = true;
			drawConfigMenu = false;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_back");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
			SetConfigSettings();
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
	if (static_cast<int>(yyrv_currentOption) == 3 && drawConfigMenu == true) { // 3 = mod configs button index
		
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
	if (static_cast<int>(yyrv_currentOption) == 3 && drawConfigMenu == true) { // 3 = mod configs button index

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
	if (static_cast<int>(yyrv_currentOption) == 3 && drawConfigMenu == true) { // 3 = mod configs button index
		if (currentMod > 0 && configSelected == false) {
			currentMod--;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_select");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
		} else if (currentModSetting > 0 && configSelected == true) {
			currentModSetting--;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_select");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
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
	if (static_cast<int>(yyrv_currentOption) == 3 && drawConfigMenu == true) { // 3 = mod configs button index
		if (currentMod < mods.size() - 1 && configSelected == false) {
			currentMod++;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_select");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
		} else if (currentModSetting < mods[currentMod].config.settings.size() - 1 && configSelected == true) {
			currentModSetting++;
			args_audioPlaySound.args[0].I64 = getAssetIndexFromName("snd_menu_select");
			audioPlaySoundFunc(&result, Self, Other, 3, args_audioPlaySound.args);
		}
	} else {
		YYRValue* res = origSelectDownTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
	}
	return res;
};

inline void CallOriginal(YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
	if (!pCodeEvent->CalledOriginal()) {
		pCodeEvent->Call(Self, Other, Code, Res, Flags);
	}
}

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

static const char* moddedStr = "Play Modded!";
RefString moddedRefStr = RefString(moddedStr, strlen(moddedStr), false);
static const char* configStr = "Mod Settings";
RefString configRefStr = RefString(configStr, strlen(configStr), false);
static bool versionTextChanged = false;

// This callback is registered on EVT_PRESENT and EVT_ENDSCENE, so it gets called every frame on DX9 / DX11 games.
YYTKStatus FrameCallback(YYTKEventBase* pEvent, void* OptionalArgument) {
	FrameNumber++;
	// Tell the core the handler was successful.
	return YYTK_OK;
}

// This callback is registered on EVT_CODE_EXECUTE, so it gets called every game function call.
YYTKStatus CodeCallback(YYTKEventBase* pEvent, void* OptionalArgument) {
	YYTKCodeEvent* pCodeEvent = dynamic_cast<decltype(pCodeEvent)>(pEvent);

	std::tuple<CInstance*, CInstance*, CCode*, RValue*, int> args = pCodeEvent->Arguments();

	CInstance* Self = std::get<0>(args);
	CInstance* Other = std::get<1>(args);
	CCode* Code = std::get<2>(args);
	RValue* Res = std::get<3>(args);
	int	Flags = std::get<4>(args);

	if (!Code->i_pName) {
		return YYTK_INVALIDARG;
	}

	if (codeFuncTable.count(Code->i_CodeIndex) != 0) {
		codeFuncTable[Code->i_CodeIndex](pCodeEvent, Self, Other, Code, Res, Flags);
	} else // Haven't cached the function in the table yet. Run the if statements and assign the function to the code index
	{
		codeIndexToName[Code->i_CodeIndex] = Code->i_pName;
		if (_strcmpi(Code->i_pName, "gml_Object_obj_TitleScreen_Create_0") == 0) {
			auto TitleScreen_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				if (args_version.isInitialized == false) args_version = ArgSetup("version");
				if (versionTextChanged == false) {
					YYRValue yyrv_version;
					variableGlobalGetFunc(&yyrv_version, Self, Other, 1, args_version.args);
					if (config.debugEnabled.boolValue) PrintMessage(CLR_AQUA, "[%s:%d] variable_global_get : version", GetFileName(__FILE__).c_str(), __LINE__);
					if (yyrv_version.operator std::string().find("Modded") == std::string::npos) {
						std::string moddedVerStr = yyrv_version.operator std::string() + " (Modded)";
						CallBuiltin(yyrv_version, "variable_global_set", Self, Other, { "version", moddedVerStr.c_str() });
						if (config.debugEnabled.boolValue) PrintMessage(CLR_TANGERINE, "[%s:%d] variable_global_set : version", GetFileName(__FILE__).c_str(), __LINE__);
					}
					versionTextChanged = true;
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
				if (args_commandPromps.isInitialized == false) {
					args_commandPromps = ArgSetup((double)1, (double)1, (double)1);
					commandPrompsArgs[0] = new YYRValue;
					commandPrompsArgs[0]->Real = 1;
					commandPrompsArgs[0]->Kind = VALUE_REAL;
					commandPrompsArgs[1] = new YYRValue;
					commandPrompsArgs[1]->Real = 1;
					commandPrompsArgs[1]->Kind = VALUE_REAL;
					commandPrompsArgs[2] = new YYRValue;
					commandPrompsArgs[2]->Real = 1;
					commandPrompsArgs[2]->Kind = VALUE_REAL;
				}
				if (args_configHeaderDraw.isInitialized == false) {
					args_configHeaderDraw = ArgSetup((double)320, (double)(48 + 13), "TEST", (double)1, (double)0, (double)32, (double)4, (double)100, (double)0, (double)1);

					drawTextOutlineArgs[0] = new YYRValue;
					drawTextOutlineArgs[0]->Real = (double)320;
					drawTextOutlineArgs[0]->Kind = VALUE_REAL;

					drawTextOutlineArgs[1] = new YYRValue;
					drawTextOutlineArgs[1]->Real = (double)(48 + 13);
					drawTextOutlineArgs[1]->Kind = VALUE_REAL;

					drawTextOutlineArgs[2] = new YYRValue;
					drawTextOutlineArgs[2]->String = RefString::Alloc("MOD SETTINGS", strlen("MOD SETTINGS"), false);
					drawTextOutlineArgs[2]->Kind = VALUE_STRING;

					drawTextOutlineArgs[3] = new YYRValue;
					drawTextOutlineArgs[3]->Real = (double)1;
					drawTextOutlineArgs[3]->Kind = VALUE_REAL;

					drawTextOutlineArgs[4] = new YYRValue;
					drawTextOutlineArgs[4]->Real = (double)0;
					drawTextOutlineArgs[4]->Kind = VALUE_REAL;

					drawTextOutlineArgs[5] = new YYRValue;
					drawTextOutlineArgs[5]->Real = (double)32;
					drawTextOutlineArgs[5]->Kind = VALUE_REAL;

					drawTextOutlineArgs[6] = new YYRValue;
					drawTextOutlineArgs[6]->Real = (double)4;
					drawTextOutlineArgs[6]->Kind = VALUE_REAL;

					drawTextOutlineArgs[7] = new YYRValue;
					drawTextOutlineArgs[7]->Real = (double)300;
					drawTextOutlineArgs[7]->Kind = VALUE_REAL;

					drawTextOutlineArgs[8] = new YYRValue;
					drawTextOutlineArgs[8]->Real = (double)0;
					drawTextOutlineArgs[8]->Kind = VALUE_REAL;

					drawTextOutlineArgs[9] = new YYRValue;
					drawTextOutlineArgs[9]->Real = (double)1;
					drawTextOutlineArgs[9]->Kind = VALUE_REAL;
				}

				CallOriginal(pCodeEvent, Self, Other, Code, Res, Flags);
			};
			TitleScreen_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TitleScreen_Create_0;
		} else if (_strcmpi(Code->i_pName, "gml_Object_obj_TextController_Create_0") == 0) {
			auto TextController_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				CallOriginal(pCodeEvent, Self, Other, Code, Res, Flags);
				YYRValue yyrv_textContainer;
				if (args_textContainer.isInitialized == false) args_textContainer = ArgSetup("TextContainer");
				variableGlobalGetFunc(&yyrv_textContainer, Self, Other, 1, args_textContainer.args);
				if (config.debugEnabled.boolValue) PrintMessage(CLR_AQUA, "[%s:%d] variable_global_get : TextContainer", GetFileName(__FILE__).c_str(), __LINE__);
				YYRValue yyrv_titleButtons;
				CallBuiltin(yyrv_titleButtons, "struct_get", Self, Other, { yyrv_textContainer, "titleButtons" });
				if (config.debugEnabled.boolValue) PrintMessage(CLR_AQUA, "[%s:%d] struct_get : titleButtons", GetFileName(__FILE__).c_str(), __LINE__);
				YYRValue yyrv_eng;
				CallBuiltin(yyrv_eng, "struct_get", Self, Other, { yyrv_titleButtons, "eng" });
				if (config.debugEnabled.boolValue) PrintMessage(CLR_AQUA, "[%s:%d] struct_get : eng", GetFileName(__FILE__).c_str(), __LINE__);
				if (std::string(yyrv_eng.RefArray->m_Array[0].String->Get()).find("Modded") == std::string::npos) {
					yyrv_eng.RefArray->m_Array[0].String = &moddedRefStr;
					if (config.debugEnabled.boolValue) PrintMessage(CLR_TANGERINE, "[%s:%d] variable_global_set : eng[0]", GetFileName(__FILE__).c_str(), __LINE__);
				}
				if (std::string(yyrv_eng.RefArray->m_Array[3].String->Get()).find("Mod Configs") == std::string::npos) {
					yyrv_eng.RefArray->m_Array[3].String = &configRefStr;
					if (config.debugEnabled.boolValue) PrintMessage(CLR_TANGERINE, "[%s:%d] variable_global_set : eng[3]", GetFileName(__FILE__).c_str(), __LINE__);
				}
			};
			TextController_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TextController_Create_0;
		} else if (_strcmpi(Code->i_pName, "gml_Object_obj_TitleScreen_Draw_0") == 0) {
			auto TitleScreen_Draw_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				CallOriginal(pCodeEvent, Self, Other, Code, Res, Flags);
				
				if (drawConfigMenu == true) {

					// Draw Mod Config Menu Background

					drawSpriteFunc(&result, Self, Other, 4, args_drawSettingsSprite.args);

					// Draw Controls Text
										
					commandPrompsScript(Self, Other, &result, 3, commandPrompsArgs);

					if (configSelected == false) {
						args_drawSetFont.args[0].Real = (double)getAssetIndexFromName("jpFont_Big");
						drawSetFontFunc(&result, Self, Other, 1, args_drawSetFont.args);

						args_halign.args[0].Real = (double)1;
						drawSetHAlignFunc(&result, Self, Other, 1, args_halign.args);

						// Draw Mod Settings Menu Title

						drawTextOutlineArgs[2]->String = RefString::Alloc("MOD SETTINGS", strlen("MOD SETTINGS"), false);
						drawTextOutlineArgs[1]->Real = (double)(48 + 13);
						drawTextOutlineArgs[8]->Real = (double)0;
						drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

						drawTextOutlineArgs[1]->Real = (double)(48 + 10);
						drawTextOutlineArgs[8]->Real = (double)16777215;
						drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

						args_drawSetFont.args[0].Real = (double)getAssetIndexFromName("jpFont");
						drawSetFontFunc(&result, Self, Other, 1, args_drawSetFont.args);

						args_halign.args[0].Real = (double)0;
						drawSetHAlignFunc(&result, Self, Other, 1, args_halign.args);

						for (int i = 0; i < mods.size(); i++) {
							int y = 48 + 43 + (i * 34);

							// Draw Mod Background

							args_drawOptionButtonSprite.args[2].Real = 320;
							args_drawOptionButtonSprite.args[3].Real = y;
							args_drawOptionButtonSprite.args[1].Real = (currentMod == i);
							drawSpriteFunc(&result, Self, Other, 4, args_drawOptionButtonSprite.args);

							args_drawSetColor.args[0].Real = (currentMod == i ? 0 : 16777215);
							drawSetColorFunc(&result, Self, Other, 1, args_drawSetColor.args);

							// Draw Mod Name

							args_drawConfigNameText.args[0].I64 = (long long)320 - 78;
							args_drawConfigNameText.args[1].I64 = (long long)y + 8;
							args_drawConfigNameText.args[2].String = RefString::Alloc(mods[i].name.c_str(), strlen(mods[i].name.c_str()), false);
							drawTextFunc(&result, Self, Other, 3, args_drawConfigNameText.args);

							// Draw Mod Checkbox

							args_drawToggleButtonSprite.args[1].Real = mods[i].enabled + (2 * (currentMod == i));
							args_drawToggleButtonSprite.args[2].Real = 320 + 69;
							args_drawToggleButtonSprite.args[3].Real = (double)48 + 56 + (i * 34);
							drawSpriteFunc(&result, Self, Other, 4, args_drawToggleButtonSprite.args);
						}

					} else { // configSelected == true
						args_halign.args[0].Real = (double)1;
						drawSetHAlignFunc(&result, Self, Other, 1, args_halign.args);

						args_drawSetFont.args[0].Real = (double)getAssetIndexFromName("jpFont");
						drawSetFontFunc(&result, Self, Other, 1, args_drawSetFont.args);

						// Draw Config Name

						drawTextOutlineArgs[2]->String = RefString::Alloc(mods[currentMod].config.name.c_str(), strlen(mods[currentMod].config.name.c_str()), false);
						drawTextOutlineArgs[1]->Real = (double)(48 + 19);
						drawTextOutlineArgs[8]->Real = (double)0;
						drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

						drawTextOutlineArgs[1]->Real = (double)(48 + 16);
						drawTextOutlineArgs[8]->Real = (double)16777215;
						drawTextOutlineScript(Self, Other, &result, 10, drawTextOutlineArgs);

						args_halign.args[0].Real = (double)0;
						drawSetHAlignFunc(&result, Self, Other, 1, args_halign.args);

						int currSetting = 0;

						for (int i = 0; i < mods[currentMod].config.settings.size(); i++) {
							if (mods[currentMod].config.settings[i].type == SETTING_BOOL) {
								int y = 48 + 43 + (currSetting * 34);

								// Draw Setting Background

								args_drawOptionButtonSprite.args[2].Real = 320 + 12;
								args_drawOptionButtonSprite.args[3].Real = y;
								args_drawOptionButtonSprite.args[1].Real = (currentModSetting == currSetting);
								drawSpriteFunc(&result, Self, Other, 4, args_drawOptionButtonSprite.args);

								args_drawSetColor.args[0].Real = (currentModSetting == currSetting ? 0 : 16777215);
								drawSetColorFunc(&result, Self, Other, 1, args_drawSetColor.args);

								// Draw Setting Name

								args_drawConfigNameText.args[0].I64 = (long long)320 - 66;
								args_drawConfigNameText.args[1].I64 = (long long)y + 8;
								args_drawConfigNameText.args[2].String = RefString::Alloc(mods[currentMod].config.settings[i].name.c_str(), strlen(mods[currentMod].config.settings[i].name.c_str()), false);
								drawTextFunc(&result, Self, Other, 3, args_drawConfigNameText.args);

								// Draw Setting Icon
								// 0 - 8 = hud_optionIcons
								// 9 - 21 = hud_graphicIcons

								int iconIndex = mods[currentMod].config.settings[i].icon;
								int actualIndex = iconIndex * 2;
								if (actualIndex > 17) {
									args_drawOptionIconSprite.args[0].I64 = getAssetIndexFromName("hud_graphicIcons");
									actualIndex -= 18;
								} else {
									args_drawOptionIconSprite.args[0].I64 = getAssetIndexFromName("hud_optionIcons");
								}
								if (actualIndex > -1) {
									args_drawOptionIconSprite.args[1].Real = (currentModSetting == currSetting ? actualIndex + 1 : actualIndex);
									args_drawOptionIconSprite.args[2].Real = 320 - 98;
									args_drawOptionIconSprite.args[3].Real = (double)48 + 56 + (currSetting * 34);
									drawSpriteFunc(&result, Self, Other, 4, args_drawOptionIconSprite.args);
								}

								// Draw Setting Checkbox

								args_drawToggleButtonSprite.args[1].Real = mods[currentMod].config.settings[i].boolValue + (2 * (currentModSetting == currSetting));
								args_drawToggleButtonSprite.args[2].Real = 320 + 81;
								args_drawToggleButtonSprite.args[3].Real = (double)48 + 56 + (currSetting * 34);
								drawSpriteFunc(&result, Self, Other, 4, args_drawToggleButtonSprite.args);

								currSetting++;
							}
						}
					}
				}
			};
			TitleScreen_Draw_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TitleScreen_Draw_0;
		} else if (_strcmpi(Code->i_pName, "gml_Object_obj_TitleCharacter_Draw_0") == 0) {
			auto TitleCharacter_Draw_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				if (drawTitleChars == true) {
					CallOriginal(pCodeEvent, Self, Other, Code, Res, Flags);
				} else {
					pCodeEvent->Cancel(true);
				}

			};
			TitleCharacter_Draw_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TitleCharacter_Draw_0;
		} else {
			auto UnmodifiedFunc = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				CallOriginal(pCodeEvent, Self, Other, Code, Res, Flags);
			};
			UnmodifiedFunc(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = UnmodifiedFunc;
		}
	}
	// Tell the core the handler was successful.
	return YYTK_OK;
}

// Create an entry routine - it must be named exactly this, and must accept these exact arguments.
// It must also be declared DllExport (notice how the other functions are not).
DllExport YYTKStatus PluginEntry(YYTKPlugin* PluginObject) {
	// Set the unload routine
	PluginObject->PluginUnload = PluginUnload;

	// Print a message to the console
	PrintMessage(CLR_DEFAULT, "[%s v%d.%d.%d] - Hello from PluginEntry!", mod.name, mod.version.major, mod.version.minor, mod.version.build);

	PluginAttributes_t* PluginAttributes = nullptr;

	// Get the attributes for the plugin - this is an opaque structure, as it may change without any warning.
	// If Status == YYTK_OK (0), then PluginAttributes is guaranteed to be valid (non-null).
	if (YYTKStatus Status = PmGetPluginAttributes(PluginObject, PluginAttributes)) {
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmGetPluginAttributes failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
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
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmCreateCallbackEx failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
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
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmCreateCallbackEx failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
		return YYTK_FAIL;
	}

	if (HAS_CONFIG == true) {
		// Load mod config file or create one if there isn't one already.
		const wchar_t* dirName = L"modconfigs";

		if (GetFileAttributes(dirName) == INVALID_FILE_ATTRIBUTES) {
			if (CreateDirectory(dirName, NULL)) {
				PrintMessage(CLR_GREEN, "[%s v%d.%d.%d] - Directory \"modconfigs\" created!", mod.name, mod.version.major, mod.version.minor, mod.version.build);
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
		PrintMessage(CLR_GREEN, "[%s v%d.%d.%d] - %s loaded successfully!", mod.name, mod.version.major, mod.version.minor, mod.version.build, fileName.c_str());
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

	// Find Mods
	std::string enabledModPath = "autoexec";
	for (const auto& entry : std::filesystem::directory_iterator(enabledModPath)) {
		std::string modName = entry.path().filename().string();
		RemoteMod newMod;
		newMod.name = modName;
		newMod.enabled = true;
		mods.push_back(newMod);
	}
	std::string disabledModPath = "disabledmods";
	for (const auto& entry : std::filesystem::directory_iterator(disabledModPath)) {
		std::string modName = entry.path().filename().string();
		RemoteMod newMod;
		newMod.name = modName;
		newMod.enabled = false;
		mods.push_back(newMod);
	}
	
	// Sort Mods Alphabetically
	std::sort(mods.begin(), mods.end(), [](const RemoteMod& a, const RemoteMod& b) {
		return a.name < b.name;
	});

	// Get Configs for Mods
	for (RemoteMod& mod : mods) {
		std::string configName = mod.name.substr(0, mod.name.size() - 4) + "-config.json";
		std::filesystem::path configPath = "modconfigs/" + configName;

		if (std::filesystem::exists(configPath)) {
			RemoteConfig config;
			config.name = configName;

			std::ifstream file(configPath);
			json data;
			file >> data;

			for (json::iterator it = data.begin(); it != data.end(); ++it) {
				std::string key = it.key();
				json setting = it.value();
				if (setting.find("value") != setting.end() && setting["value"].is_boolean()) {
					int iconIndex = -1;
					if (setting.find("icon") != setting.end()) iconIndex = setting["icon"];
					Setting newSetting(key, iconIndex, setting["value"]);
					config.settings.insert(config.settings.begin(), newSetting);
				}
			}

			// Move "enabled" Setting to Front

			auto it = std::find_if(config.settings.begin(), config.settings.end(), [](const Setting& setting) {
				return setting.name == "enabled";
			});

			if (it != config.settings.end()) {
				Setting enabledSetting = *it;
				config.settings.erase(it);
				config.settings.insert(config.settings.begin(), enabledSetting);
			}
				 
			mod.config = config;
		}
	}

	// Off it goes to the core.
	return YYTK_OK;
}

// The routine that gets called on plugin unload.
// Registered in PluginEntry - you should use this to release resources.
YYTKStatus PluginUnload() {
	YYTKStatus Removal = PmRemoveCallback(g_pFrameCallbackAttributes);

	// If we didn't succeed in removing the callback.
	if (Removal != YYTK_OK) {
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] PmRemoveCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Removal);
	}

	Removal = PmRemoveCallback(g_pCodeCallbackAttributes);

	// If we didn't succeed in removing the callback.
	if (Removal != YYTK_OK) {
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] PmRemoveCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Removal);
	}

	PrintMessage(CLR_DEFAULT, "[%s v%d.%d.%d] - Goodbye!", mod.name, mod.version.major, mod.version.minor, mod.version.build);

	return YYTK_OK;
}

// Boilerplate setup for a Windows DLL, can just return TRUE.
// This has to be here or else you get linker errors (unless you disable the main method)
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	return 1;
}