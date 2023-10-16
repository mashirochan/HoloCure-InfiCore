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
static struct Config {
	bool debugEnabled = false;
} config;

void to_json(json& j, const Config& c) {
	j = json{
		{ "debugEnabled", c.debugEnabled }
	};
}

void from_json(const json& j, Config& c) {
	try {
		j.at("debugEnabled").get_to(c.debugEnabled);
	} catch (const json::out_of_range& e) {
		PrintError(__FILE__, __LINE__, "%s", e.what());
		std::string fileName = formatString(std::string(mod.name)) + "-config.json";
		GenerateConfig(fileName);
	}
}

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
	ArgSetup(long long id, double priority, bool loop) {
		args[0].I64 = id;
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
	ArgSetup(double x, double y, const char* text, double len, long long color0, double angleIncrement, double sep, double w, long long color1, double alpha) {
		args[0].Real = x;
		args[0].Kind = VALUE_REAL;
		args[1].Real = y;
		args[1].Kind = VALUE_REAL;
		args[2].String = RefString::Alloc(text, strlen(text));
		args[2].Kind = VALUE_STRING;
		args[3].Real = len;
		args[3].Kind = VALUE_REAL;
		args[4].I64 = color0;
		args[4].Kind = VALUE_INT64;
		args[5].Real = angleIncrement;
		args[5].Kind = VALUE_REAL;
		args[6].Real = sep;
		args[6].Kind = VALUE_REAL;
		args[7].Real = w;
		args[7].Kind = VALUE_REAL;
		args[8].I64 = color1;
		args[8].Kind = VALUE_INT64;
		args[9].Real = alpha;
		args[9].Kind = VALUE_REAL;

		isInitialized = true;
	}
};

TRoutine assetGetIndexFunc = nullptr;

TRoutine variableInstanceGetFunc = nullptr;
ArgSetup args_container;

ArgSetup args_currentOption;

TRoutine variableInstanceSetFunc = nullptr;

TRoutine scriptExecuteFunc = nullptr;
ArgSetup args_configHeaderDraw;
ArgSetup args_calculateScore;

TRoutine arrayCreateFunc = nullptr;

TRoutine variableGlobalGetFunc = nullptr;
ArgSetup args_charSelected;
ArgSetup args_version;
ArgSetup args_textContainer;

TRoutine drawSetAlphaFunc = nullptr;
ArgSetup args_drawSetAlpha;

TRoutine drawSetColorFunc = nullptr;
ArgSetup args_drawSetColor;

TRoutine drawRectangleFunc = nullptr;
ArgSetup args_drawRectangle;

TRoutine drawButtonFunc = nullptr;

TRoutine drawTextFunc = nullptr;

PFUNC_YYGMLScript drawTextOutlineScript = nullptr;

TRoutine audioPlaySoundFunc = nullptr;
ArgSetup args_audioPlaySound;

TRoutine deviceMouseXToGUIFunc = nullptr;
TRoutine deviceMouseYToGUIFunc = nullptr;
ArgSetup args_deviceMouse;

TRoutine deviceMouseCheckButtonPressedFunc = nullptr;
ArgSetup args_checkButtonPressed;

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

		if (config.debugEnabled) PrintMessage(CLR_GRAY, "- &%s = 0x%p", Name, TargetFuncPointer);
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

inline int getAssetIndexFromName(const char* name) {
	RValue Result;
	RValue arg{};
	arg.Kind = VALUE_STRING;
	arg.String = RefString::Alloc(name, strlen(name));
	assetGetIndexFunc(&Result, nullptr, nullptr, 1, &arg);
	return static_cast<int>(Result.Real);
}

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

typedef YYRValue* (*ScriptFunc)(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args);

// gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0
ScriptFunc origConfirmedTitleScreenScript = nullptr;
YYRValue* ConfirmedTitleScreenFuncDetour(CInstance* Self, CInstance* Other, YYRValue* ReturnValue, int numArgs, YYRValue** Args) {
	YYRValue* res = nullptr;
	YYRValue yyrv_currentOption;
	variableInstanceGetFunc(&yyrv_currentOption, Self, Other, 2, args_currentOption.args);
	if (static_cast<int>(yyrv_currentOption) == 3) { // 3 = achievements button index
		PrintMessage(CLR_BRIGHTPURPLE, "Achievements selected!");
	} else {
		YYRValue* res = origConfirmedTitleScreenScript(Self, Other, ReturnValue, numArgs, Args);
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
static const char* configStr = "Mod Configs";
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
	int			Flags = std::get<4>(args);

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
					if (config.debugEnabled) PrintMessage(CLR_AQUA, "[%s:%d] variable_global_get : version", GetFileName(__FILE__).c_str(), __LINE__);
					if (yyrv_version.operator std::string().find("Modded") == std::string::npos) {
						std::string moddedVerStr = yyrv_version.operator std::string() + " (Modded)";
						CallBuiltin(yyrv_version, "variable_global_set", Self, Other, { "version", moddedVerStr.c_str() });
						if (config.debugEnabled) PrintMessage(CLR_TANGERINE, "[%s:%d] variable_global_set : version", GetFileName(__FILE__).c_str(), __LINE__);
					}
					versionTextChanged = true;
				}
				
				if (args_currentOption.isInitialized == false) args_currentOption = ArgSetup(Self->i_id, "currentOption");
				if (args_configHeaderDraw.isInitialized == false)
					args_configHeaderDraw = ArgSetup((double)320, (double)(48 + 13), "TEST", (double)1, (long long)0, (double)32, (double)4, (double)100, (long long)0, (double)1);

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
				if (config.debugEnabled) PrintMessage(CLR_AQUA, "[%s:%d] variable_global_get : TextContainer", GetFileName(__FILE__).c_str(), __LINE__);
				YYRValue yyrv_titleButtons;
				CallBuiltin(yyrv_titleButtons, "struct_get", Self, Other, { yyrv_textContainer, "titleButtons" });
				if (config.debugEnabled) PrintMessage(CLR_AQUA, "[%s:%d] struct_get : titleButtons", GetFileName(__FILE__).c_str(), __LINE__);
				YYRValue yyrv_eng;
				CallBuiltin(yyrv_eng, "struct_get", Self, Other, { yyrv_titleButtons, "eng" });
				if (config.debugEnabled) PrintMessage(CLR_AQUA, "[%s:%d] struct_get : eng", GetFileName(__FILE__).c_str(), __LINE__);
				if (std::string(yyrv_eng.RefArray->m_Array[0].String->Get()).find("Modded") == std::string::npos) {
					yyrv_eng.RefArray->m_Array[0].String = &moddedRefStr;
					if (config.debugEnabled) PrintMessage(CLR_TANGERINE, "[%s:%d] variable_global_set : eng[0]", GetFileName(__FILE__).c_str(), __LINE__);
				}
				if (std::string(yyrv_eng.RefArray->m_Array[3].String->Get()).find("Mod Configs") == std::string::npos) {
					yyrv_eng.RefArray->m_Array[3].String = &configRefStr;
					if (config.debugEnabled) PrintMessage(CLR_TANGERINE, "[%s:%d] variable_global_set : eng[3]", GetFileName(__FILE__).c_str(), __LINE__);
				}
			};
			TextController_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TextController_Create_0;
		} else if (_strcmpi(Code->i_pName, "gml_Object_obj_TitleScreen_Draw_0") == 0) {
			auto TitleScreen_Draw_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				CallOriginal(pCodeEvent, Self, Other, Code, Res, Flags);

				drawTextOutlineScript(Self, Other, &result, 10, reinterpret_cast<YYRValue**>(&args_configHeaderDraw.args));
			};
			TitleScreen_Draw_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TitleScreen_Draw_0;
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

	// Register a callback for frame events
	YYTKStatus Status = PmCreateCallback(
		PluginAttributes,					// Plugin Attributes
		g_pFrameCallbackAttributes,				// (out) Callback Attributes
		FrameCallback,						// The function to register as a callback
		static_cast<EventType>(EVT_PRESENT | EVT_ENDSCENE), // Which events trigger this callback
		nullptr								// The optional argument to pass to the function
	);

	if (Status) {
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmCreateCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
		return YYTK_FAIL;
	}

	// Register a callback for frame events
	Status = PmCreateCallback(
		PluginAttributes,					// Plugin Attributes
		g_pCodeCallbackAttributes,			// (out) Callback Attributes
		CodeCallback,						// The function to register as a callback
		static_cast<EventType>(EVT_CODE_EXECUTE), // Which events trigger this callback
		nullptr								// The optional argument to pass to the function
	);

	if (Status) {
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmCreateCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
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

			config = data.template get<Config>();
		}
		PrintMessage(CLR_GREEN, "[%s v%d.%d.%d] - %s loaded successfully!", mod.name, mod.version.major, mod.version.minor, mod.version.build, fileName.c_str());
	}

	// Function Pointers
	GetFunctionByName("asset_get_index", assetGetIndexFunc);
	GetFunctionByName("variable_instance_get", variableInstanceGetFunc);
	GetFunctionByName("variable_instance_set", variableInstanceSetFunc);
	GetFunctionByName("variable_global_get", variableGlobalGetFunc);
	GetFunctionByName("draw_set_alpha", drawSetAlphaFunc);
	GetFunctionByName("draw_set_color", drawSetColorFunc);
	GetFunctionByName("draw_rectangle", drawRectangleFunc);
	GetFunctionByName("draw_button", drawButtonFunc);
	GetFunctionByName("draw_text", drawTextFunc);
	GetFunctionByName("audio_play_sound", audioPlaySoundFunc);
	GetFunctionByName("device_mouse_x_to_gui", deviceMouseXToGUIFunc);
	GetFunctionByName("device_mouse_y_to_gui", deviceMouseYToGUIFunc);
	GetFunctionByName("device_mouse_check_button_pressed", deviceMouseCheckButtonPressedFunc);
	GetFunctionByName("method_call", scriptExecuteFunc);
	GetFunctionByName("array_create", arrayCreateFunc);

	// Function Hooks
	MH_Initialize();
	MmGetScriptData(scriptList);

	HookScriptFunction("gml_Script_Confirmed_gml_Object_obj_TitleScreen_Create_0", (void*)&ConfirmedTitleScreenFuncDetour, (void**)&origConfirmedTitleScreenScript);

	int drawTextOutlineIndex = getAssetIndexFromName("gml_Script_draw_text_outline") - 100000;
	CScript* drawTextOutlineCScript = scriptList(drawTextOutlineIndex);
	drawTextOutlineScript = drawTextOutlineCScript->s_pFunc->pScriptFunc;

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