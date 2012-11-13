#include <d3d9.h>
#include <d3dx9core.h>
#pragma comment(lib,"d3dx9.lib")

typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int);
typedef long (__stdcall *PRESENT9)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, void*);
typedef long (__stdcall *DIP9)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);

PRESENT9 g_D3D9_Present = 0;
DIP9 g_D3D9_DIP = 0;
BYTE g_codeFragment_p9[5] = {0, 0, 0, 0, 0};
BYTE g_codeFragment_DIP[5] = {0, 0, 0, 0, 0};
BYTE g_jmp_p9[5] = {0, 0, 0, 0, 0};
BYTE g_jmp_DIP[5] = {0, 0, 0, 0, 0};
DWORD g_savedProtection_p9 = 0;
DWORD g_savedProtection_DIP = 0;
DWORD present9 = 0;
DWORD dip9 = 0;
bool indicator = 0;
D3DRECT rec = {10, 10, 160, 240};
ID3DXFont *m_font = 0;
RECT fontRect = {10, 10, 160, 240};
D3DCOLOR bkgColor = 0;
D3DCOLOR fontColor = 0;
LPCSTR menu = "123456\nkekeke\ngamehacklab.ru";

void DrawIndicator(void* self)
{
	IDirect3DDevice9* dev = (IDirect3DDevice9*)self;
	DWORD vmt = 0;
	__asm
	{
		mov eax,[dev]
		mov eax,[eax]
		mov vmt,eax
	}	

	if (m_font==0) D3DXCreateFont(dev, 12, 0, FW_BOLD, 0, 0, 1, 0, 0, 0 | FF_DONTCARE, TEXT("Terminal"), &m_font);
	//pD3D->lpVtbl->CreateDevice(pD3D, [other args]);
	IDirect3DDevice9Ex_BeginScene(dev);
	dev->BeginScene();
	if(indicator)
	{
		bkgColor = D3DCOLOR_XRGB(0, 0, 0);
		fontColor = D3DCOLOR_XRGB(255, 255, 255);
		dev->Clear(1, &rec, D3DCLEAR_TARGET, bkgColor, 1.0f, 0);
		m_font->DrawText(0, menu, -1, &fontRect, 0, fontColor);
	}
	dev->EndScene();
}

void GetDevice9Methods()
{
	HWND hWnd = CreateWindowExA(0, "STATIC","dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);	
	HMODULE hD3D9 = LoadLibrary("d3d9");
	DIRECT3DCREATE9 Direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(hD3D9, "Direct3DCreate9");
	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	IDirect3DDevice9* d3dDevice = 0;
    d3d->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
	DWORD* vtablePtr = (DWORD*)(*((DWORD*)d3dDevice));
	/*__asm
	{
		mov eax,[d3dDevice]
		mov eax,[eax]
		mov vtablePtr,eax
	}	*/
	present9 = vtablePtr[17] - (DWORD)hD3D9; //DIP == 82
	dip9 = vtablePtr[82] - (DWORD)hD3D9;
	d3dDevice->Release();
	d3d->Release();
	FreeLibrary(hD3D9);
	CloseHandle(hWnd);
}

long __stdcall HookedPresent9(IDirect3DDevice9* pDevice, const RECT* src, const RECT* dest, HWND hWnd, void* unused)
{
	BYTE* codeDest = (BYTE*)g_D3D9_Present;
	codeDest[0] = g_codeFragment_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_codeFragment_p9 + 1));
	DrawIndicator(pDevice);
	DWORD res = g_D3D9_Present(pDevice, src, dest, hWnd, unused);
	codeDest[0] = g_jmp_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_jmp_p9 + 1));
	return res;
}

long __stdcall HookedDIP9(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE type, INT baseVertexIndex,UINT minIndex,UINT numVertices, UINT startIndex, UINT primCount)
{
	BYTE* codeDest = (BYTE*)g_D3D9_DIP;
	codeDest[0] = g_codeFragment_DIP[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_codeFragment_DIP + 1));	
	//pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	DWORD res = g_D3D9_DIP(pDevice, type, baseVertexIndex, minIndex, numVertices, startIndex, primCount);
	codeDest[0] = g_jmp_DIP[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_jmp_DIP + 1));
	return res;
}

void HookDevice9Methods()
{
	HMODULE hD3D9 = GetModuleHandle("d3d9.dll");
	g_D3D9_Present = (PRESENT9)((DWORD)hD3D9 + present9);
	g_jmp_p9[0] = 0xE9;
	DWORD addr = (DWORD)HookedPresent9 - (DWORD)g_D3D9_Present - 5;
	memcpy(g_jmp_p9 + 1, &addr, sizeof(DWORD));
	memcpy(g_codeFragment_p9, g_D3D9_Present, 5);
	VirtualProtect(g_D3D9_Present, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_p9); 
	memcpy(g_D3D9_Present, g_jmp_p9, 5);

	/*g_D3D9_DIP = (DIP9)((DWORD)hD3D9 + dip9);
	g_jmp_DIP[0] = 0xE9;
	DWORD addrDIP = (DWORD)HookedDIP9 - (DWORD)g_D3D9_DIP - 5;
	memcpy(g_jmp_DIP + 1, &addrDIP, sizeof(DWORD));
	memcpy(g_codeFragment_DIP, g_D3D9_DIP, 5);
	VirtualProtect(g_D3D9_DIP, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_DIP); 
	memcpy(g_D3D9_DIP, g_jmp_DIP, 5);*/
}

DWORD __stdcall TF(void* lpParam)
{
	GetDevice9Methods();
	HookDevice9Methods();
	return 0;
}

DWORD __stdcall KeyboardHook(void* lpParam)
{
        while(1)
        {
                if(GetAsyncKeyState(VK_F1))    
                {
                        indicator = !indicator;
                        Beep(500,200);
                }
                Sleep(100);
        }      
        return 0;
}

int __stdcall DllMain(HINSTANCE hInst, DWORD  ul_reason_for_call, void* lpReserved)
{
        switch (ul_reason_for_call)    
        {
        case DLL_PROCESS_ATTACH:
			CreateThread(0, 0, &TF, 0, 0, 0);
			CreateThread(0, 0, &KeyboardHook, 0, 0, 0);
        }
        return 1;
}