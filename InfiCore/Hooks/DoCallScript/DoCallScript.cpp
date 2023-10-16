#include "DoCallScript.hpp"
#include "../../Features/API/API.hpp"
#include "../../Features/PluginManager/PluginManager.hpp"
#include "../../SDK/Plugins/YYTKEvent/YYTKEvent.hpp"

namespace Hooks
{
	namespace DoCallScript
	{
		char* Function(CScript* pScript, int argc, char* pStackPointer, VMExec* pVM, YYObjectBase* pLocals, YYObjectBase* pArguments)
		{
			YYTKScriptEvent Event = YYTKScriptEvent(pfnOriginal, pScript, argc, pStackPointer, pVM, pLocals, pArguments);
			API::PluginManager::RunHooks(&Event);

			if (Event.CalledOriginal())
				return Event.GetReturn();

			return pfnOriginal(pScript, argc, pStackPointer, pVM, pLocals, pArguments);
		}

		void* GetTargetAddress()
		{
#if _WIN64
			uintptr_t FuncCallPattern = API::FindPattern("\xE8\x00\x00\x00\x00\x48\x8B\xF0\x4D\x85\xFF", "x????xxxxxx", 0, 0);

			if (!FuncCallPattern)
				return nullptr;

			uintptr_t Relative = *reinterpret_cast<uint32_t*>(FuncCallPattern + 1);
			Relative = (FuncCallPattern + 5) + Relative; // eip = instruction base + 5 + relative offset

#else
			unsigned long DoCallGMLPattern = API::FindPattern("\x8B\x00\x05\x60\x79\xFE\xFF\x00\xE8", "xxxxxxx?x", 0, 0);

			if (!DoCallGMLPattern)
				return nullptr;

			unsigned long FuncCallPattern = API::FindPattern("\xE8\x00\x00\x00\x00\x83\xC4\x1C", "x????xxx", DoCallGMLPattern, 128);

			if (!FuncCallPattern)
				return nullptr;

			unsigned long Relative = *reinterpret_cast<unsigned long*>(FuncCallPattern + 1);
			Relative = (FuncCallPattern + 5) + Relative; // eip = instruction base + 5 + relative offset
#endif
			return reinterpret_cast<void*>(Relative);
		}
	}
}


