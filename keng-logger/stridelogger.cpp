#include <vector>
#include <d3d9.h>
#include <d3dx9core.h>

#include "detours.h"

#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3dx9")

using namespace std;

typedef HRESULT(WINAPI* pDrawIndexedPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, int, UINT, UINT, UINT, UINT);
typedef HRESULT(WINAPI* pEndScene)(LPDIRECT3DDEVICE9);
typedef HRESULT(WINAPI* pReset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);

pDrawIndexedPrimitive oDrawIndexedPrimitive;
pEndScene oEndScene;
pReset oReset;

typedef struct _STRIDELOG{
	INT Base; UINT Min;
	UINT Num; UINT Start;
	UINT Prim;
}STRIDELOG, *PSTRIDELOG;

int indicator = 0;
D3DRECT rec = {10, 10, 160, 240}; //menu size
RECT fontRect = { 10, 15, 120, 120 };
ID3DXFont *m_font = 0;
D3DCOLOR bkgColor;
D3DCOLOR fontColor;

D3DVIEWPORT9 Vpt;
LPDIRECT3DTEXTURE9 Green = NULL; 
LPDIRECT3DTEXTURE9 pTx = NULL;
D3DLOCKED_RECT d3dlr;
LPD3DXFONT pFont = NULL;
char strbuff[260];
UINT iStride = 0;
UINT iBaseTex = 0;
vector<STRIDELOG> STRIDE;
vector<DWORD> BASETEX;
bool Startlog = false;
LPDIRECT3DBASETEXTURE9 BTEX = NULL;
bool Found = false;
STRIDELOG StrideLog;

void DrawIndicator(void* self)
{
	IDirect3DDevice9* dev = (IDirect3DDevice9*)self; //get the actual D3D-device
	//create D3D-font (if not created yet)
	if (m_font == 0) 
		D3DXCreateFont(dev, 12, 0, FW_BOLD, 0, 0, 1, 0, 0, 0 | FF_DONTCARE, TEXT("Terminal"), &m_font);	
	IDirect3DDevice9_BeginScene(dev);
	//(dev)->lpVtbl->BeginScene(dev);
	//if(indicator) { //if we need to draw the menu	
		bkgColor = D3DCOLOR_XRGB(0, 0, 255);
		fontColor = D3DCOLOR_XRGB(0, 255, 255);
		//(m_font)->lpVtbl->DrawTextA(m_font, 0, "keng.gamehacklab.ru", -1, &fontRect, 0, fontColor);
		IDirect3DDevice9_Clear(dev, 1, &rec, D3DCLEAR_TARGET, bkgColor, 1.0f, 0);
		//m_font->DrawText(0, menu, -1, &fontRect, 0, fontColor); //draw text
	//}
	IDirect3DDevice9_EndScene(dev);
}

HRESULT WINAPI hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	LPDIRECT3DVERTEXBUFFER9 Stream_Data;
	UINT Offset = 0;
	UINT Stride = 0;

	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == S_OK)
		Stream_Data->Release();

	if (Startlog) {
		if (Stride == iStride) {
			pDevice->GetTexture(0, &BTEX);
			Found = false;
			for (UINT i = 0; i < BASETEX.size(); i++)
			if (BASETEX[i] == (DWORD)BTEX)
				Found = true;
			if (Found == false)
				BASETEX.push_back
				((DWORD)BTEX);
			if (BASETEX[iBaseTex] == (DWORD)BTEX && Green) {
				pDevice->SetTexture(0, Green);
				pDevice->SetRenderState(D3DRS_ZENABLE, 0);
				oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
				pDevice->SetRenderState(D3DRS_ZENABLE, 1);
				if (Startlog == true) {
					Found = false;
					for (UINT i = 0; i < STRIDE.size(); i++)
					if (STRIDE[i].Base == BaseVertexIndex &&
						STRIDE[i].Min == MinVertexIndex &&
						STRIDE[i].Num == NumVertices &&
						STRIDE[i].Start == startIndex &&
						STRIDE[i].Prim == primCount) {
						Found = true;
						break;
					}
					if (Found == false)	{
						StrideLog.Base = BaseVertexIndex;
						StrideLog.Min = MinVertexIndex;
						StrideLog.Num = NumVertices;
						StrideLog.Start = startIndex;
						StrideLog.Prim = primCount;
						STRIDE.push_back(StrideLog);
					}
				}
			}
		}
	}

	
	if (Stride == 32 && BaseVertexIndex == 33) {
		pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	}
	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDev)
{
	if (Startlog) {
		pDev->GetViewport(&Vpt);
		RECT FRect = { Vpt.Width - 250, Vpt.Height - 300, Vpt.Width, Vpt.Height };
		if (Green == 0)
		if (pDev->CreateTexture(8, 8, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Green, 0) == S_OK)
		if (pDev->CreateTexture(8, 8, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pTx, 0) == S_OK)
		if (pTx->LockRect(0, &d3dlr, 0, D3DLOCK_DONOTWAIT | D3DLOCK_NOSYSLOCK) == S_OK)	{
			for (UINT xy = 0; xy < 8 * 8; xy++)
				((PDWORD)d3dlr.pBits)[xy] = 0xFF00FF00;
			pTx->UnlockRect(0);
			pDev->UpdateTexture(pTx, Green);
			pTx->Release();
		}
		if (pFont == 0)
			D3DXCreateFontA(pDev, 16, 0, 700, 0, 0, 1, 0, 0, DEFAULT_PITCH | FF_DONTCARE, "Calibri", &pFont);
		sprintf_s(strbuff, "Num of Textures: %i\nStride: %i\nBase Tex Num: %i\n" \
			"Log Enable: %i\n\nF1: Stride++\nF2: Stride--\nF3: BaseTexNum++" \
			"\nF4: BaseTexNum--\nF5: Log On/Off", \
			BASETEX.size(), iStride, iBaseTex + 1, Startlog);
		if (pFont)
			pFont->DrawTextA(0, strbuff, -1, &FRect, DT_CENTER | DT_NOCLIP, 0xFF00FF00);
	}	
	return oEndScene(pDev);
}

HRESULT WINAPI hkReset(LPDIRECT3DDEVICE9 pDev,D3DPRESENT_PARAMETERS* PresP)
{
	if (pFont) { 
		pFont->Release(); 
		pFont = NULL; 
	}
	if (Green) { 
		Green->Release(); 
		Green = NULL; 
	}
	return oReset(pDev, PresP);
}

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
	HWND hWnd = CreateWindowExA(0, "STATIC", "dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
	HMODULE hD3D9 = GetModuleHandleA("d3d9.dll");
	if (hD3D9 != 0) {
		d3d9_ptr = Direct3DCreate9(D3D_SDK_VERSION);
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3d9_ptr->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
		vtablePtr = (PDWORD)(*((PDWORD)d3dDevice));
		present9 = vtablePtr[17] - (DWORD)hD3D9;
		dip9 = vtablePtr[82] - (DWORD)hD3D9;
		endScene9 = vtablePtr[42] - (DWORD)hD3D9;
		reset9 = vtablePtr[16] - (DWORD)hD3D9;
		d3dDevice->Release();
		d3d9_ptr->Release();
	}
	CloseHandle(hWnd);
		
	oDrawIndexedPrimitive = (pDrawIndexedPrimitive)DetourFunction((PBYTE)((DWORD)hD3D9 + dip9), (PBYTE)hkDrawIndexedPrimitive);
	oEndScene = (pEndScene)DetourFunction((PBYTE)((DWORD)hD3D9 + endScene9), (PBYTE)hkEndScene);
	oReset = (pReset)DetourFunction((PBYTE)((DWORD)hD3D9 + reset9), (PBYTE)hkReset);
}

void HookDevice9Methods()
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

void TF() {
	GetDevice9Methods();
	while (1) {		
		if (GetAsyncKeyState(VK_F1) & 1) {
			iStride++; BASETEX.clear(); iBaseTex = 0;
		}
		if (GetAsyncKeyState(VK_F2) & 1)
		if (iStride > 0) {
			iStride--; BASETEX.clear(); iBaseTex = 0;
		};
		if (GetAsyncKeyState(VK_F3) & 1)
		if (iBaseTex < BASETEX.size() - 1)iBaseTex++;
		if (GetAsyncKeyState(VK_F4) & 1)
		if (iBaseTex > 0)
			iBaseTex--;
		if (GetAsyncKeyState(VK_F5) & 1) {
			Startlog = !Startlog; STRIDE.clear();
		}
		Sleep(100);
	}
}

int WINAPI DllMain(HINSTANCE hInst, DWORD ul_reason_for_call, void* lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TF, 0, 0, 0);
	return 1;
}