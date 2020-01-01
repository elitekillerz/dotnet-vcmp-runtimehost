#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include "nethost.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"

#define MESSAGE_PREFIX ("[DOTNET] ")
#define PATH_BUFFER_LEN (4096)

#define SERVER_CONFIG_FILE ("./server.cfg")
#define ENTRY_TYPE_FQN ("EliteKillerz.DotnetVcmp.RuntimeClient.Bootstrap, EliteKillerz.DotnetVcmp.RuntimeClient")
#define ENTRY_METHOD_NAME ("VcmpInitialize")
#define ENTRY_DELEGATE_TYPE_FQN ("EliteKillerz.DotnetVcmp.RuntimeClient.BootstrapEntryPointDelegate, EliteKillerz.DotnetVcmp.RuntimeClient")

#ifdef _WIN32
#include <Windows.h>

#define EXPORT __declspec(dllexport)

typedef HMODULE shared_library_handle_t;
typedef std::wstring platform_string_t;
#else
#include <dlfcn.h>

#define EXPORT

typedef void* shared_library_handle_t;
typedef std::string platform_string_t;
#endif

shared_library_handle_t load_library(std::string p_path) {
#ifdef _WIN32
    return LoadLibrary(p_path.c_str());
#else
    return dlopen(p_path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void* resolve_symbol(shared_library_handle_t p_library, std::string p_symbol) {
#ifdef _WIN32
    return (void*)(GetProcAddress(p_library, p_symbol.c_str()));
#else
    return dlsym(p_library, p_symbol.c_str());
#endif
}

typedef bool (*runtime_client_bootstrap_fn)(void* p_pluginFuncs, void* p_pluginEvents, void* p_pluginInfo);

extern "C" EXPORT unsigned int VcmpPluginInit(void* p_pluginFuncs, void* p_pluginEvents, void* p_pluginInfo) {
    std::string cfgRuntimeConfigPath;
    std::string cfgAssemblyPath;

    std::ifstream serverConfigFile(SERVER_CONFIG_FILE);
    if (!serverConfigFile.is_open()) {
        std::cerr << MESSAGE_PREFIX << "Failed to open server configuration file " << SERVER_CONFIG_FILE << "." << std::endl;
        return 0;
    }

    for (std::string configEntry; std::getline(serverConfigFile, configEntry);) {
        size_t spaceDelimiter = configEntry.find(' ');

        std::string key = configEntry.substr(0, spaceDelimiter);
        std::string value = spaceDelimiter == std::string::npos ? "" : configEntry.substr(spaceDelimiter + 1);

        if (key == "dotnetrtc") {
            cfgRuntimeConfigPath = value;
        } else if (key == "dotnetasm") {
            cfgAssemblyPath = value;
        }
    }

    int error = 0;

    if (cfgRuntimeConfigPath.empty()) {
        std::cerr << MESSAGE_PREFIX << "No dotnet *.runtimeconfig.json path specified; use 'dotnetrtc' directive in server.cfg to specify." << std::endl;
        error = 1;
    }

    if (cfgAssemblyPath.empty()) {
        std::cerr << MESSAGE_PREFIX << "No dotnet *.dll assembly path specified; use 'dotnetasm' directive in server.cfg to specify." << std::endl;
        error = 1;
    }

    if (error != 0) {
        return 0;
    }

    char_t hostfxrPathBuffer[PATH_BUFFER_LEN];
    size_t hostfxrPathBufferSize = sizeof(hostfxrPathBuffer) / sizeof(char_t);

    error = get_hostfxr_path(hostfxrPathBuffer, &hostfxrPathBufferSize, nullptr);

    platform_string_t hostfxrPlatformPath(hostfxrPathBuffer);
    std::string hostfxrPath(hostfxrPlatformPath.begin(), hostfxrPlatformPath.end());

    if (error) {
        std::cerr
            << MESSAGE_PREFIX
            << std::hex << std::showbase
            << "Failed to get hostfxr path (get_hostfxr_path returned nonzero: " << error << ")."
            << std::endl;
        return 0;
    }
    else {
        std::cout << MESSAGE_PREFIX << "Using hostfxr library " << hostfxrPath << "." << std::endl;
    }

    shared_library_handle_t hostfxrLibrary = load_library(hostfxrPath);

    auto hostfxrInitializeForRuntimeConfig =
        (hostfxr_initialize_for_runtime_config_fn)(resolve_symbol(hostfxrLibrary, "hostfxr_initialize_for_runtime_config"));
    auto hostfxrClose =
        (hostfxr_close_fn)(resolve_symbol(hostfxrLibrary, "hostfxr_close"));
    auto hostfxrGetRuntimeDelegate =
        (hostfxr_get_runtime_delegate_fn)(resolve_symbol(hostfxrLibrary, "hostfxr_get_runtime_delegate"));

    hostfxr_handle hostfxrHandle = nullptr;

    error = hostfxrInitializeForRuntimeConfig(platform_string_t(cfgRuntimeConfigPath.begin(), cfgRuntimeConfigPath.end()).c_str(), nullptr, &hostfxrHandle);

    if (error) {
        std::cerr
            << MESSAGE_PREFIX 
            << std::hex << std::showbase 
            << "Failed to initialize hostfxr (hostfxr_initialize_for_runtime_config returned nonzero: " << error << ")."
            << std::endl;
        return 0;
    }
    else if (hostfxrHandle == nullptr) {
        std::cerr << MESSAGE_PREFIX << "Failed to initialize hostfxr (hostfxr_initialize_for_runtime_config did not return handle)." << std::endl;
        return 0;
    } else {
        std::cout << MESSAGE_PREFIX << "Initialized hostfxr runtime." << std::endl;
    }

    load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer = nullptr;

    error = hostfxrGetRuntimeDelegate(hostfxrHandle, hdt_load_assembly_and_get_function_pointer, (void**)(&loadAssemblyAndGetFunctionPointer));

    if (error) {
        std::cerr
            << MESSAGE_PREFIX
            << std::hex << std::showbase
            << "Failed to retrieve load_assembly_and_get_function_pointer delegate (hostfxr_get_runtime_delegate returned nonzero: " << error << ")."
            << std::endl;
        return 0;
    } else if (loadAssemblyAndGetFunctionPointer == nullptr) {
        std::cerr
            << MESSAGE_PREFIX
            << "Failed to retrieve load_assembly_and_get_function_pointer delegate (hostfxr_get_runtime_delegate did not return handle)."
            << std::endl;
        return 0;
    } else {
        std::cout << MESSAGE_PREFIX << "Retrieved load_assembly_and_get_function_pointer delegate." << std::endl;
    }

    hostfxrClose(hostfxrHandle);

    if (error) {
        std::cerr
            << MESSAGE_PREFIX
            << std::hex << std::showbase
            << "Failed to close hostfxr handle (hostfxr_close returned nonzero: " << error << ")."
            << std::endl;
        return 0;
    } else {
        std::cout << MESSAGE_PREFIX << "Closed hostfxr handle." << std::endl;
    }

    runtime_client_bootstrap_fn runtimeClientBootstrap = nullptr;
    std::string entryTypeFqn(ENTRY_TYPE_FQN);
    std::string entryMethodName(ENTRY_METHOD_NAME);
    std::string entryDelegateTypeFqn(ENTRY_DELEGATE_TYPE_FQN);

    error = loadAssemblyAndGetFunctionPointer(
        platform_string_t(cfgAssemblyPath.begin(), cfgAssemblyPath.end()).c_str(),
        platform_string_t(entryTypeFqn.begin(), entryTypeFqn.end()).c_str(),
        platform_string_t(entryMethodName.begin(), entryMethodName.end()).c_str(),
        platform_string_t(entryDelegateTypeFqn.begin(), entryDelegateTypeFqn.end()).c_str(),
        nullptr,
        (void**)(&runtimeClientBootstrap)
    );

    if (error) {
        std::cerr
            << MESSAGE_PREFIX
            << std::hex << std::showbase
            << "Failed to load assembly and get entry function (load_assembly_and_get_function_pointer returned nonzero: " << error << ")."
            << std::endl;
        return 0;
    } else {
        std::cout << MESSAGE_PREFIX << "Loaded assembly and entry function." << std::endl;
    }

    return runtimeClientBootstrap(p_pluginFuncs, p_pluginEvents, p_pluginInfo) ? 1 : 0;
}
