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
#pragma comment (lib, "d3dx9")

///////////////////////////////////////////////////////////////////////////////

#pragma region definitions

typedef HRESULT(WINAPI* pDrawIndexedPrimitive)(LPDIRECT3DDEVICE9,
	D3DPRIMITIVETYPE, int, UINT, UINT, UINT, UINT);
typedef HRESULT(WINAPI* pEndScene)(LPDIRECT3DDEVICE9);
typedef HRESULT(WINAPI* pReset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);

pDrawIndexedPrimitive oDrawIndexedPrimitive;
pEndScene oEndScene;
pReset oReset;

#pragma endregion

#pragma region options

int wallHack = 0;
int crosshairToggle = 0;
int indicator = 1;

#pragma endregion

#pragma region variables

D3DRECT rec = { 10, 10, 120, 30 };
ID3DXFont *m_font = 0;
RECT fontRect = { 10, 15, 120, 120 };
D3DCOLOR bkgColor = D3DCOLOR_XRGB(0, 0, 255);
D3DCOLOR fontColor = D3DCOLOR_XRGB(0, 255, 255);

#pragma endregion

///////////////////////////////////////////////////////////////////////////////

void SetModelColor(LPDIRECT3DDEVICE9 pDev, float r, float g, float b, 
	float a, float glowr, float glowg, float glowb, float glowa)
{
	float lightValues[4] = { r, g, b, a };
	float glowValues[4] = { glowr, glowg, glowb, glowa };
	(pDev)->lpVtbl->SetPixelShaderConstantF(pDev, 1, lightValues, 1);
	(pDev)->lpVtbl->SetPixelShaderConstantF(pDev, 3, glowValues, 1);
}

///////////////////////////////////////////////////////////////////////////////

void DrawIndicator(void* self)
{
	IDirect3DDevice9* dev = (IDirect3DDevice9*)self;
	
	if (m_font == 0)
		D3DXCreateFont(dev, 12, 0, 0, 0, 0, 1, 0, 0, 0, "Terminal", &m_font);	
	IDirect3DDevice9_Clear(dev, 1, &rec, D3DCLEAR_TARGET, bkgColor, 1.0f, 0);	
	(m_font)->lpVtbl->DrawText(m_font, 0, "menu", -1, &fontRect, 0, fontColor);
}

void DrawCrosshair(void* self)
{
	IDirect3DDevice9* dev = (IDirect3DDevice9*)self;
	D3DVIEWPORT9 viewP;
	// Через функцию GetViewport находим размер игрового экрана
	IDirect3DDevice9_GetViewport(dev, &viewP);
	// И его центр
	DWORD scrCenterX = viewP.Width / 2;
	DWORD scrCenterY = viewP.Height / 2;
	// Задаём размер прицела
	D3DRECT rect1 =
	{ scrCenterX - 5, scrCenterY, scrCenterX + 5, scrCenterY + 1 };
	D3DRECT rect2 =
	{ scrCenterX, scrCenterY - 5, scrCenterX + 1, scrCenterY + 5 };
	// Задаём цвет (RGB, Red Green Blue, Красный Зелёный Синий, 0-255)
	D3DCOLOR color = D3DCOLOR_XRGB(255, 0, 0);
	// Рисуем
	IDirect3DDevice9_Clear(dev, 1, &rect1, D3DCLEAR_TARGET, color, 0, 0);
	IDirect3DDevice9_Clear(dev, 1, &rect2, D3DCLEAR_TARGET, color, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI hkDrawIndexedPrimitive( LPDIRECT3DDEVICE9 pDev, 
									   D3DPRIMITIVETYPE PrimType, 
									   INT BaseVertexIndex, 
									   UINT MinVertexIndex, 
									   UINT NumVertices, 
									   UINT startIndex, 
									   UINT primCount) {
	if (wallHack)
	{
		LPDIRECT3DVERTEXBUFFER9 Stream_Data;
		UINT Offset = 0;
		UINT Stride = 0;

		if (IDirect3DDevice9_GetStreamSource(pDev, 0, &Stream_Data, 
											 &Offset, &Stride) == S_OK)
			IDirect3DVertexBuffer9_Release(Stream_Data);
		// 32 34
		// 32 47
		if (Stride == 32 && (BaseVertexIndex == 18 || BaseVertexIndex == 70 || 
							 BaseVertexIndex == 34 || BaseVertexIndex == 47)) {
			Beep(500, 300);
			SetModelColor(pDev, 0.5f, 0.0f, 0.0f, 0.25f, 0.5f, 0.5f, 0.5f, 0.5f);
			IDirect3DDevice9_SetRenderState(pDev, D3DRS_ZENABLE, 0);
			oDrawIndexedPrimitive(pDev, PrimType, BaseVertexIndex, 
				MinVertexIndex, NumVertices, startIndex, primCount);
		}
	}	
	return oDrawIndexedPrimitive(pDev, PrimType, BaseVertexIndex, 
								 MinVertexIndex, NumVertices, startIndex, 
								 primCount);
}

///////////////////////////////////////////////////////////////////////////////

//
// Перехваченная функция, которая вызывается после окончания рисования сцены.
// В ней происходит всё рисование и выводится всё уже поверх игры.
//
HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDev) {	
	if (indicator)
		DrawIndicator(pDev);
	// Проверяем, что нам нужно нарисовать прицел
	if (crosshairToggle) {
		DrawCrosshair(pDev);
	}
	return oEndScene(pDev);
}

///////////////////////////////////////////////////////////////////////////////

void *DetourFunc(BYTE *src, const BYTE *dst, const int len)
{
	BYTE *jmp = (BYTE*)malloc(len + 5);
	DWORD dwback;
	VirtualProtect(src, len, PAGE_READWRITE, &dwback);
	memcpy(jmp, src, len);	jmp += len;
	jmp[0] = 0xE9;
	*(DWORD*)(jmp + 1) = (DWORD)(src + len - jmp) - 5;
	src[0] = 0xE9;
	*(DWORD*)(src + 1) = (DWORD)(dst - src) - 5;
	VirtualProtect(src, len, dwback, &dwback);
	return (jmp - len);
}

///////////////////////////////////////////////////////////////////////////////

int GetDevice9Methods() {
	DWORD dip9 = 0;
	DWORD endScene9 = 0;
	IDirect3DDevice9* d3dDevice;
	DWORD* vtablePtr;
	D3DPRESENT_PARAMETERS d3dpp;
	HWND hWnd = 
		CreateWindowExA(0, "STATIC", "dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
	HMODULE hD3D9 = GetModuleHandleA("d3d9.dll");
	if (hD3D9 != 0) {
		IDirect3D9* d3d9_ptr = Direct3DCreate9(D3D_SDK_VERSION);
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = 1;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		IDirect3D9_CreateDevice(d3d9_ptr, 0, D3DDEVTYPE_HAL, hWnd, 
					D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
		vtablePtr = (PDWORD)(*((PDWORD)d3dDevice));
		//DWORD present9 = vtablePtr[17] - (DWORD)hD3D9;
		dip9 = vtablePtr[82] - (DWORD)hD3D9;
		endScene9 = vtablePtr[42] - (DWORD)hD3D9;
		//DWORD reset9 = vtablePtr[16] - (DWORD)hD3D9;
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
	// Проверка, поставились ли хуки
	if (oDrawIndexedPrimitive != 0 && oEndScene != 0)
		return 0;
	else
		return 1;
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

// Основной поток хука
void TF() {
	// Перехватываем нужные нам D3D-функции
	if (!GetDevice9Methods())
		Beep(500, 300);
	// И уходим в бесконечный цикл
	while (1) {
		// Проверяем нажатые кнопки - совершаем действие.
		if (GetAsyncKeyState(VK_F1)) {
			Beep(500, 500);
			indicator = !indicator;
		}
		if (GetAsyncKeyState(VK_F2)) {
			Beep(500, 500);
			crosshairToggle = !crosshairToggle;
		}
		if (GetAsyncKeyState(VK_F3)) {
			Beep(500, 500);
			wallHack = !wallHack;
		}
		// Ждём 100мсек, чтобы не нагружать процессор
		Sleep(100);
	}
}

///////////////////////////////////////////////////////////////////////////////

int __stdcall DllMain(HINSTANCE hInst, DWORD ul_reason_for_call, void* lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TF, 0, 0, 0);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////