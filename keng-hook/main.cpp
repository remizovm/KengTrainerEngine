#include <d3d9.h> //Needed header files
#include <d3dx9core.h>
#pragma comment(lib,"d3dx9.lib") //and libs

typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int); //function prototypes, we will need them for the hooking
typedef long (__stdcall *PRESENT9)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, void*);
typedef long (__stdcall *DIP9)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);

//all the needed variables
PRESENT9 g_D3D9_Present = 0;
DIP9 g_D3D9_DIP = 0;
BYTE g_codeFragment_p9[5] = {0, 0, 0, 0, 0};
//BYTE g_codeFragment_DIP[5] = {0, 0, 0, 0, 0};
BYTE g_jmp_p9[5] = {0, 0, 0, 0, 0};
//BYTE g_jmp_DIP[5] = {0, 0, 0, 0, 0};
DWORD g_savedProtection_p9 = 0;
//DWORD g_savedProtection_DIP = 0;
DWORD present9 = 0;
//DWORD dip9 = 0;
bool indicator = 0;
D3DRECT rec = {10, 10, 160, 240}; //menu size
ID3DXFont *m_font = 0;
RECT fontRect = {10, 10, 160, 240};
D3DCOLOR bkgColor = 0;
D3DCOLOR fontColor = 0;
LPCSTR menu = "keng"; //menu text

//ingame menu draw function
void DrawIndicator(void* self)
{
	IDirect3DDevice9* dev = (IDirect3DDevice9*)self; //get the actual D3D-device
	/*DWORD vmt = 0;
	__asm
	{
		mov eax,[dev]
		mov eax,[eax]
		mov vmt,eax
	}	*/

	if (m_font==0) D3DXCreateFont(dev, 12, 0, FW_BOLD, 0, 0, 1, 0, 0, 0 | FF_DONTCARE, TEXT("Terminal"), &m_font); //create D3D-font (if not created yet)
	//pD3D->lpVtbl->CreateDevice(pD3D, [other args]);
	dev->BeginScene(); //begin drawing
	if(indicator) //if we need to draw the menu
	{
		bkgColor = D3DCOLOR_XRGB(0, 0, 0); //black background
		fontColor = D3DCOLOR_XRGB(255, 255, 255); //write text
		dev->Clear(1, &rec, D3DCLEAR_TARGET, bkgColor, 1.0f, 0); //draw the rectangle
		m_font->DrawText(0, menu, -1, &fontRect, 0, fontColor); //draw text
	}
	dev->EndScene(); //finish drawing
}

//get the VMT and the hooking functions offsets
void GetDevice9Methods()
{
	HWND hWnd = CreateWindowExA(0, "STATIC","dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0); //create a dummy invisible window, get the handle
	HMODULE hD3D9 = LoadLibrary("d3d9"); //load the d3d9.dll
	DIRECT3DCREATE9 Direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(hD3D9, "Direct3DCreate9"); //get the Create func address
	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION); //create the D3D-object
    D3DPRESENT_PARAMETERS d3dpp; //init the needed parameters
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	IDirect3DDevice9* d3dDevice = 0;
    d3d->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice); //create the D3D-device
	DWORD* vtablePtr = (DWORD*)(*((DWORD*)d3dDevice)); //get the VMT address
	present9 = vtablePtr[17] - (DWORD)hD3D9; //DIP == 82 //get the Present() address by offset
	//dip9 = vtablePtr[82] - (DWORD)hD3D9;
	d3dDevice->Release(); //destroy the d3d-device
	d3d->Release(); //destroy the d3d-object
	FreeLibrary(hD3D9); //unload the d3d9.dll
	CloseHandle(hWnd); //close the dummy window handle
}

//hooked Present() function
long __stdcall HookedPresent9(IDirect3DDevice9* pDevice, const RECT* src, const RECT* dest, HWND hWnd, void* unused)
{
	BYTE* codeDest = (BYTE*)g_D3D9_Present; //restore the original Present() bytes
	codeDest[0] = g_codeFragment_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_codeFragment_p9 + 1));
	DrawIndicator(pDevice); //draw the menu
	DWORD res = g_D3D9_Present(pDevice, src, dest, hWnd, unused); //place the hook back
	codeDest[0] = g_jmp_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_jmp_p9 + 1));
	return res;
}

//long __stdcall HookedDIP9(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE type, INT baseVertexIndex,UINT minIndex,UINT numVertices, UINT startIndex, UINT primCount)
//{
//	BYTE* codeDest = (BYTE*)g_D3D9_DIP;
//	codeDest[0] = g_codeFragment_DIP[0];
//	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_codeFragment_DIP + 1));	
//	//pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
//	DWORD res = g_D3D9_DIP(pDevice, type, baseVertexIndex, minIndex, numVertices, startIndex, primCount);
//	codeDest[0] = g_jmp_DIP[0];
//	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_jmp_DIP + 1));
//	return res;
//}

//main hooking function
void HookDevice9Methods()
{
	HMODULE hD3D9 = GetModuleHandle("d3d9.dll"); //get the actual d3d9.dll address
	g_D3D9_Present = (PRESENT9)((DWORD)hD3D9 + present9); //calculate the actual Present() address
	g_jmp_p9[0] = 0xE9; //fill the codecave array ("jmp hooked_present")
	DWORD addr = (DWORD)HookedPresent9 - (DWORD)g_D3D9_Present - 5; //calculate the hooked Present() address
	memcpy(g_jmp_p9 + 1, &addr, sizeof(DWORD)); //write it into the cave
	memcpy(g_codeFragment_p9, g_D3D9_Present, 5); //save the first 5 (jmp + addr) bytes of the original Present() 
	VirtualProtect(g_D3D9_Present, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_p9); //make the code writable\executable
	memcpy(g_D3D9_Present, g_jmp_p9, 5); //write the codecave in the beginning of the original Present()

	/*g_D3D9_DIP = (DIP9)((DWORD)hD3D9 + dip9);
	g_jmp_DIP[0] = 0xE9;
	DWORD addrDIP = (DWORD)HookedDIP9 - (DWORD)g_D3D9_DIP - 5;
	memcpy(g_jmp_DIP + 1, &addrDIP, sizeof(DWORD));
	memcpy(g_codeFragment_DIP, g_D3D9_DIP, 5);
	VirtualProtect(g_D3D9_DIP, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_DIP); 
	memcpy(g_D3D9_DIP, g_jmp_DIP, 5);*/
}

//hooking thread main function
DWORD __stdcall TF(void* lpParam)
{
	GetDevice9Methods(); //get the VMT and the offsets
	HookDevice9Methods(); //actually, hook the needed functions
	return 0;
}

//hotkeys thread main function
DWORD __stdcall KeyboardHook(void* lpParam)
{
        while(1)
        {
                if(GetAsyncKeyState(VK_F1)) //if F1 is pressed
                {
                        indicator = !indicator; //draw or hide the menu
                        Beep(500,200); //signal to the user
                }
                Sleep(100); //wait 100msec
        }      
        return 0;
}

//DLL entry point
int __stdcall DllMain(HINSTANCE hInst, DWORD  ul_reason_for_call, void* lpReserved)
{
        switch (ul_reason_for_call)    
        {
        case DLL_PROCESS_ATTACH: //if we are attached into the game's process
			CreateThread(0, 0, &TF, 0, 0, 0); //start D3D-hooking thread
			CreateThread(0, 0, &KeyboardHook, 0, 0, 0); //start hotkeys thread
        }
        return 1;
}