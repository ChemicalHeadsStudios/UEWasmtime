#pragma once

UEWASMTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogUEWasmTime, Log, All);

class UEWASMTIME_API FUEWasmTimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Handle to the test dll we will load */
	void*	WasmTimeHandle;
};
