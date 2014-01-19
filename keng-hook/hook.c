///////////////////////////////////////////////////////////////////////////////
//
// Microsoft Direct 3D hooking engine by Mikhail Remizov aka keng
// 2012-2013
//
///////////////////////////////////////////////////////////////////////////////

#include <d3d9.h>
#include <d3dx9core.h>
#include "detours.h"
#pragma comment(lib, "d3d9")
//#pragma comment(lib, "d3dx9")

///////////////////////////////////////////////////////////////////////////////

typedef HRESULT(WINAPI* pDrawIndexedPrimitive)(LPDIRECT3DDEVICE9, 
				D3DPRIMITIVETYPE, int, UINT, UINT, UINT, UINT);
typedef HRESULT(WINAPI* pEndScene)(LPDIRECT3DDEVICE9);
typedef HRESULT(WINAPI* pReset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
pDrawIndexedPrimitive oDrawIndexedPrimitive;
pEndScene oEndScene;
pReset oReset;
int wallHack;

///////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI hkDrawIndexedPrimitive( LPDIRECT3DDEVICE9 pDevice, 
									   D3DPRIMITIVETYPE PrimType, 
									   INT BaseVertexIndex, 
									   UINT MinVertexIndex, 
									   UINT NumVertices, 
									   UINT startIndex, 
									   UINT primCount) {
	LPDIRECT3DVERTEXBUFFER9 Stream_Data;
	UINT Offset = 0;
	UINT Stride = 0;

	if (IDirect3DDevice9_GetStreamSource(pDevice, 0, &Stream_Data, 
										 &Offset, &Stride) == S_OK)
		IDirect3DVertexBuffer9_Release(Stream_Data);
	if (Stride == 32 && BaseVertexIndex == 33) {
		IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ZENABLE, D3DZB_FALSE);
	}
	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, 
								 MinVertexIndex, NumVertices, startIndex, 
								 primCount);
}

///////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDev)
{	
	return oEndScene(pDev);
}

///////////////////////////////////////////////////////////////////////////////

void GetDevice9Methods()
{	
	DWORD present9 = 0;
	DWORD dip9 = 0;
	DWORD endScene9 = 0;
	DWORD reset9 = 0;
	IDirect3D9 *d3d9_ptr;
	IDirect3DDevice9* d3dDevice;
	DWORD* vtablePtr;
	D3DPRESENT_PARAMETERS d3dpp;
	HWND hWnd = CreateWindowExA(0, "STATIC", "dummy", 0, 0, 0, 
												0, 0, 0, 0, 0, 0);
	HMODULE hD3D9 = GetModuleHandleA("d3d9.dll");
	if (hD3D9 != 0) {
		d3d9_ptr = Direct3DCreate9(D3D_SDK_VERSION);
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		IDirect3D9_CreateDevice(d3d9_ptr, 0, D3DDEVTYPE_HAL, hWnd, 
					D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
		vtablePtr = (PDWORD)(*((PDWORD)d3dDevice));
		present9 = vtablePtr[17] - (DWORD)hD3D9;
		dip9 = vtablePtr[82] - (DWORD)hD3D9;
		endScene9 = vtablePtr[42] - (DWORD)hD3D9;
		reset9 = vtablePtr[16] - (DWORD)hD3D9;
		IDirect3DDevice9_Release(d3dDevice);
		IDirect3D9_Release(d3d9_ptr);
	}
	CloseHandle(hWnd);		
	oDrawIndexedPrimitive = 
		(pDrawIndexedPrimitive)DetourFunction((PBYTE)((DWORD)hD3D9 + dip9), 
		(PBYTE)hkDrawIndexedPrimitive);
	oEndScene = 
		(pEndScene)DetourFunction((PBYTE)((DWORD)hD3D9 + endScene9), 
		(PBYTE)hkEndScene);
}

///////////////////////////////////////////////////////////////////////////////

void HookDevice9Methods() // DEPRECATED AND NOT USED BY NOW
{	
	/*
	g_D3D9_Present = (PRESENT9)((DWORD)hD3D9 + present9); //calculate the actual Present() address
	g_jmp_p9[0] = 0xE9; //fill the codecave array ("jmp hooked_present")	
	addr1 = (DWORD)HookedPresent9 - (DWORD)g_D3D9_Present - 5; //calculate the hooked Present() address
	memcpy(g_jmp_p9 + 1, &addr1, sizeof(DWORD)); //write it into the cave
	memcpy(g_codeFragment_p9, g_D3D9_Present, 5); //save the first 5 (jmp + addr) bytes of the original Present() 
	VirtualProtect(g_D3D9_Present, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_p9); //make the code writable\executable
	memcpy(g_D3D9_Present, g_jmp_p9, 5); //write the codecave in the beginning of the original Present()
		
	g_D3D9_DIP = (DIP9)((DWORD)hD3D9 + dip9); //calculate the actual Present() address
	g_jmp_dip9[0] = 0xE9; //fill the codecave array ("jmp hooked_present")	
	addr2 = (DWORD)HookedDIP9 - (DWORD)g_D3D9_DIP - 5; //calculate the hooked Present() address
	memcpy(g_jmp_dip9 + 1, &addr2, sizeof(DWORD)); //write it into the cave
	memcpy(g_codeFragment_dip9, g_D3D9_DIP, 5); //save the first 5 (jmp + addr) bytes of the original Present() 
	VirtualProtect(g_D3D9_DIP, 14, PAGE_EXECUTE_READWRITE, &g_savedProtection_dip9); //make the code writable\executable
	memcpy(g_D3D9_DIP, g_jmp_dip9, 5); //write the codecave in the beginning of the original Present()	
	*/
}

///////////////////////////////////////////////////////////////////////////////

void TF() {	
	GetDevice9Methods();
	while (1) {		
		if (GetAsyncKeyState(VK_F1) & 1) 
			wallHack = !wallHack;
		Sleep(100);
	}
}

///////////////////////////////////////////////////////////////////////////////

int WINAPI DllMain(HINSTANCE hInst, DWORD ul_reason_for_call, void* lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TF, 0, 0, 0);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////