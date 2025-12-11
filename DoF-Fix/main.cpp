#include "nvse/PluginAPI.h"

CallDetour kRenderDOFDetour;
CallDetour kRenderISDetour;

static constexpr AddressPtr<bool, 0x12023D4> bRenderDOF_INI;

class Hook {
public:
	void RenderDoF(void* apAccum, void* apTexture) {
		if (bRenderDOF_INI) {
			ThisCall(kRenderDOFDetour.GetOverwrittenAddr(), this, apAccum, apTexture);
			ThisCall(0xB97550, *reinterpret_cast<DWORD**>(0x11F91AC), 4, *reinterpret_cast<DWORD**>(0x11F4748), apTexture, apTexture, 0, true);
		}
	}

	void RenderImageSpace(bool abMSAA, void* apRenderTarget, void* apRenderer, void* apDestination) {
		bool bRenderDOF = bRenderDOF_INI;
		bRenderDOF_INI = false;
		ThisCall(kRenderISDetour.GetOverwrittenAddr(), this, abMSAA, apRenderTarget, apRenderer, apDestination);
		bRenderDOF_INI = bRenderDOF;
	}
};

EXTERN_DLL_EXPORT bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "DoF Fix";
	info->version = 120;

	return !nvse->isEditor;
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Load(NVSEInterface* nvse) {
	kRenderDOFDetour.ReplaceCallEx(0x870FBD, &Hook::RenderDoF);
	kRenderISDetour.ReplaceCallEx(0x8710E4, &Hook::RenderImageSpace);

	// Thanks lStewieAl for the code
	SafeWriteBuf(0x870F28, "\xEB\x71\x0F\x84\xB1\x00\x00\x00", 8);
	SafeWriteBuf(0x870FC2, "\x80\x7D\xED\x00\xE9\x5F\xFF\xFF\xFF", 9);
	SafeWrite8(0x00870F7A, 0x66);

	// Use 16b format for blur RTs
	SafeWrite8(0xB6CBBD + 2, 0x71);

	return true;
}

BOOL WINAPI DllMain(
	HANDLE  hDllHandle,
	DWORD   dwReason,
	LPVOID  lpreserved
) {
	return TRUE;
}
