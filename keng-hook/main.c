#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")

typedef HRESULT (__stdcall *PRESENT9)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, void*);

PRESENT9 g_D3D9_Present = 0;
BYTE g_codeFragment_p9[5] = {0, 0, 0, 0, 0};
BYTE g_jmp_p9[5] = {0, 0, 0, 0, 0};
DWORD present9 = 0;
int indicator;
D3DRECT rec = {10, 10, 160, 240}; //menu size

void DrawIndicator(void* self)
{
	IDirect3DDevice9* dev = (IDirect3DDevice9*)self; //get the actual D3D-device
	
	//if (m_font==0) D3DXCreateFont(dev, 12, 0, FW_BOLD, 0, 0, 1, 0, 0, 0 | FF_DONTCARE, TEXT("Terminal"), &m_font); //create D3D-font (if not created yet)
	//pD3D->lpVtbl->CreateDevice(pD3D, [other args]);
	IDirect3DDevice9_BeginScene(dev);
	if(indicator) //if we need to draw the menu
	{
		//bkgColor = D3DCOLOR_XRGB(0, 0, 0); //black background
		//fontColor = D3DCOLOR_XRGB(255, 255, 255); //write text
		//IDirect3DDevice9_Clear(dev, 1, &rec, D3DCLEAR_TARGET, bkgColor, 1.0f, 0);
		//m_font->DrawText(0, menu, -1, &fontRect, 0, fontColor); //draw text
	}
	IDirect3DDevice9_EndScene(dev);
}

//get the VMT and the hooking functions offsets
void GetDevice9Methods()
{
	IDirect3D9 *d3d9_ptr;
	IDirect3DDevice9* d3dDevice;
	DWORD* vtablePtr;
	D3DPRESENT_PARAMETERS d3dpp;
	static HMODULE d3d9_handle = 0;
	HWND hWnd = CreateWindowExA(0, "STATIC","dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    d3d9_handle = LoadLibraryA("d3d9.dll");
    d3d9_ptr = Direct3DCreate9(D3D_SDK_VERSION);
	ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	
	IDirect3D9_CreateDevice(d3d9_ptr, 0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
	vtablePtr = (DWORD*)(*((DWORD*)d3dDevice));
	present9 = vtablePtr[17] - (DWORD)d3d9_handle;
	IDirect3DDevice9_Release(d3dDevice);
	IDirect3D9_Release(d3d9_ptr);
	FreeLibrary(d3d9_handle);
	CloseHandle(hWnd);
}

HRESULT HookedPresent9(IDirect3DDevice9* pDevice, const RECT* src, const RECT* dest, HWND hWnd, void* unused)
{
	//IDirect3DSurface9 *g_pSurface;
	DWORD res;
	D3DCOLOR bkgColor;
	BYTE* codeDest = (BYTE*)g_D3D9_Present; //restore the original Present() bytes
	codeDest[0] = g_codeFragment_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_codeFragment_p9 + 1));
	//IDirect3DDevice9_GetBackBuffer(pDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &g_pSurface);
	//pDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&g_pSurface);
	//D3DXSaveSurfaceToFile("c:\\stuff\\temp.bmp",D3DXIFF_BMP,g_pSurface,0,0);
	//
	//
	//
	bkgColor = D3DCOLOR_XRGB(255, 0, 0); //black background
	IDirect3DDevice9_Clear(pDevice, 1, &rec, D3DCLEAR_TARGET, bkgColor, 1.0f, 0);
	//DrawIndicator(pDevice); //draw the menu
	res = g_D3D9_Present(pDevice, src, dest, hWnd, unused); //place the hook back
	codeDest[0] = g_jmp_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_jmp_p9 + 1));
	return res;
}

void HookDevice9Methods()
{
	DWORD addr, g_savedProtection_p9;
	HMODULE hD3D9 = GetModuleHandle("d3d9.dll"); //get the actual d3d9.dll address
	g_D3D9_Present = (PRESENT9)((DWORD)hD3D9 + present9); //calculate the actual Present() address
	g_jmp_p9[0] = 0xE9; //fill the codecave array ("jmp hooked_present")	
	addr = (DWORD)HookedPresent9 - (DWORD)g_D3D9_Present - 5; //calculate the hooked Present() address
	memcpy(g_jmp_p9 + 1, &addr, sizeof(DWORD)); //write it into the cave
	memcpy(g_codeFragment_p9, g_D3D9_Present, 5); //save the first 5 (jmp + addr) bytes of the original Present() 
	VirtualProtect(g_D3D9_Present, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_p9); //make the code writable\executable
	memcpy(g_D3D9_Present, g_jmp_p9, 5); //write the codecave in the beginning of the original Present()
}

//hooking thread main function
void TF()
{
	GetDevice9Methods(); //get the VMT and the offsets
	HookDevice9Methods(); //actually, hook the needed functions
}

//hotkeys thread main function
void KeyboardHook() {
	while(1) {
		if(GetAsyncKeyState(VK_F1)) indicator = !indicator;
		Sleep(100); //wait 100msec
	}      
}

int _stdcall DllMain(HINSTANCE hInst, DWORD ul_reason_for_call, void* lpReserved)
{
        switch (ul_reason_for_call)    
        {
        case DLL_PROCESS_ATTACH: //if we are attached into the game's process
			Beep(500, 200);
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TF, 0, 0, 0); //start D3D-hooking thread
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)KeyboardHook, 0, 0, 0); //start hotkeys thread
        }
        return 1;
}