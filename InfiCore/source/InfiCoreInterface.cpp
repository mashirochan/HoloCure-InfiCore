#include "InfiCoreInterface.h"
#include <iostream>

using namespace Aurie;

AurieStatus InfiCoreInterface::Create() {
	return AURIE_SUCCESS;
}
void InfiCoreInterface::Destroy() {

}

void InfiCoreInterface::QueryVersion(
	OUT short& Major,
	OUT short& Minor,
	OUT short& Patch
) {
	Major = 1;
	Minor = 0;
	Patch = 0;
}

AurieStatus InfiCoreInterface::SetReadConfigCallback(
	IN const std::string ModFileName,
	IN const std::function<void()> Function
) {
	m_ConfigFunctionMap[ModFileName] = Function;
	return AURIE_SUCCESS;
}

// Function to call the callback for a specific mod
AurieStatus InfiCoreInterface::CallConfigCallback(std::string ModFileName) {
	for (const auto& pair : m_ConfigFunctionMap) {
		std::string name = pair.first;
		std::cout << "'" << name << "'\n";
	}
	if (m_ConfigFunctionMap.contains(ModFileName)) {
		// Call the assigned function for the specified mod
		m_ConfigFunctionMap.at(ModFileName)();
		return AURIE_SUCCESS;
	} else {
		std::cout << "'" << ModFileName << "' was not found in the map!" << std::endl;
		return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;
	}
}