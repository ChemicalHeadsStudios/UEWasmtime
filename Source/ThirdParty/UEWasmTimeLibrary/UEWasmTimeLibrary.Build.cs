// Copyright SIA Chemical Heads 2022

using System.IO;
using UnrealBuildTool;

public class UEWasmTimeLibrary : ModuleRules
{
	public UEWasmTimeLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		string VERSION = "v0.26.0";

		PublicDefinitions.Add("WASMTIME_VERSION=\"" + VERSION + "\"");
		PublicDefinitions.Add("WITH_WASMTIME_LIB=1");
		
		string BaseLibraryPath = Path.Combine(ModuleDirectory,  string.Format("wasmtime-{0}-x86_64-windows-c-api", VERSION));
		string BaseBinaryPath = Path.Combine(Path.Combine(PluginDirectory, "Binaries", "ThirdParty", "UEWasmTimeLibrary"));

		PublicIncludePaths.Add(Path.Combine(BaseLibraryPath, "include"));
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDelayLoadDLLs.Add("wasmtime.dll");
			RuntimeDependencies.Add(Path.Combine(BaseBinaryPath, string.Format("wasmtime-{0}-x86_64-windows-c-api", VERSION), "lib", "wasmtime.dll"));
			
			PublicAdditionalLibraries.Add(Path.Combine(BaseLibraryPath, "lib", string.Format("wasmtime.dll.lib")));
			PublicAdditionalLibraries.Add(Path.Combine(BaseLibraryPath, "lib", string.Format("wasmtime.lib")));
		} else if(Target.Platform.IsInGroup(UnrealPlatformGroup.Linux) && Target.Architecture.StartsWith("x86_64")) {
			PublicDelayLoadDLLs.Add("libwasmtime.so");
			RuntimeDependencies.Add(Path.Combine(BaseBinaryPath, string.Format("wasmtime-{0}-x86_64-linux-c-api", VERSION), "lib", "libwasmtime.so"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, string.Format("wasmtime-{0}-x86_64-linux-c-api", VERSION), "lib", "libwasmtime.a"));
		}
	}
}
