#include "nvse/PluginAPI.h"

NVSEInterface* g_nvseInterface{};

void __forceinline SafeWrite8(UInt32 addr, UInt32 data)
{
	UInt32	oldProtect;

	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((UInt8*)addr) = data;
	VirtualProtect((void*)addr, 4, oldProtect, &oldProtect);
}

void __forceinline SafeWrite32(UInt32 addr, UInt32 data)
{
	UInt32	oldProtect;

	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((UInt32*)addr) = data;
	VirtualProtect((void*)addr, 4, oldProtect, &oldProtect);
}

void WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt)
{
	// call rel32
	SafeWrite8(jumpSrc, 0xE8);
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

void __forceinline ReplaceCall(UInt32 jumpSrc, UInt32 jumpTgt)
{
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

void WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt)
{
	// jmp rel32
	SafeWrite8(jumpSrc, 0xE9);
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

void SafeWriteBuf(UInt32 addr, void* data, UInt32 len) {
	UInt32	oldProtect;

	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((void*)addr, data, len);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
}

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "DoF Fix";
	info->version = 110;

	return true;
}

void(__thiscall* ImageSpaceManager__RenderEffectByID)(DWORD* apThis, UInt32 eID, DWORD* pkRenderer, DWORD* pTexture1, DWORD* pTexture2, DWORD* pEffectParam, bool bEndFrame) = (void(__thiscall*)(DWORD*, UInt32, DWORD*, DWORD*, DWORD*, DWORD*, bool))0xB97550;
void(__thiscall* Main__RenderDepthOfField)(DWORD* apThis, DWORD* apAccum, DWORD* apTexture) = (void(__thiscall*)(DWORD*, DWORD*, DWORD*))0x875E40;

void __forceinline ToggleDoF(bool abState) {
	*(bool*)0x12023D4 = abState;
}

void __fastcall RenderDoF(DWORD* apThis, void*, DWORD* apAccum, DWORD* apTexture) {
	ToggleDoF(true);
	Main__RenderDepthOfField(apThis, apAccum, apTexture);
	ImageSpaceManager__RenderEffectByID(*(DWORD**)0x11F91AC, 4, *(DWORD**)0x11F4748, apTexture, apTexture, 0, true);
	ToggleDoF(false);
}

bool NVSEPlugin_Load(NVSEInterface* nvse) {
	if (!nvse->isEditor) {
		ReplaceCall(0x870FBD, (UInt32)RenderDoF);

		// Thanks lStewieAl for the code
		SafeWriteBuf(0x870F28, (void*)"\xEB\x71\x0F\x84\xB1\x00\x00\x00", 8);
		SafeWriteBuf(0x870FC2, (void*)"\x80\x7D\xED\x00\xE9\x5F\xFF\xFF\xFF", 9);
		SafeWrite8(0x00870F7A, 0x66);

		// Use 16b format for blur RTs
		SafeWrite8(0xB6CBBD + 2, 0x71);
	}

	return true;
}