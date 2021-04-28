#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogUEWasmTime, Log, All);

class FUEWasmTimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Handle to the test dll we will load */
	void*	WasmTimeHandle;
};
