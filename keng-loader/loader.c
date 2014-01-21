///////////////////////////////////////////////////////////////////////////////
//
// Hooking DLL loader\injector GUI by Mikhail Remizov aka keng
// 2012-2013
//
///////////////////////////////////////////////////////////////////////////////

#include <d3d9.h>
#include <tlhelp32.h>
//#include <time.h>

#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "Shlwapi.lib")

///////////////////////////////////////////////////////////////////////////////

unsigned long GetTargetThreadIDFromProcName(const char *);
int Inject(const unsigned long, const char *);

LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;

///////////////////////////////////////////////////////////////////////////////

void CleanD3D()
{
	IDirect3DDevice9_Release(d3ddev);
	IDirect3D9_Release(d3d);
}

///////////////////////////////////////////////////////////////////////////////

void RenderFrame()
{	
	IDirect3DDevice9_Clear(d3ddev, 0, 0, D3DCLEAR_TARGET, 
						   D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	IDirect3DDevice9_BeginScene(d3ddev);
	IDirect3DDevice9_EndScene(d3ddev);
	IDirect3DDevice9_Present(d3ddev, 0, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////

void InitD3D(const HWND hWnd) 
{	
	D3DPRESENT_PARAMETERS d3dpp;

	d3d = Direct3DCreate9(D3D_SDK_VERSION);	
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	IDirect3D9_CreateDevice(d3d, 0, D3DDEVTYPE_HAL, hWnd, 
							D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
							&d3dpp, &d3ddev);
}

///////////////////////////////////////////////////////////////////////////////

LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	//PAINTSTRUCT ps; 
    //HDC hdc;
	unsigned long pID;
	char buf[MAX_PATH] = {0};
	//HPEN penWhite;
	//HPEN penBlack;

	switch(msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_LBUTTONDOWN:
		SendMessage(hWnd, 0xA1, 0x2, 0);
			pID = GetTargetThreadIDFromProcName("d3d9_lesson0_keng.exe");
			if(pID == 0)
			{
				MessageBox(0, "ERROR: Unable to find pID!", 
						   "Ocelot Loader @ keng", MB_OK);
				return 0;
			}
			else
			{
				Beep(1000, 100);
				GetFullPathName("keng-hook.dll", MAX_PATH, buf, 0);
				Inject(pID, buf);
				return 0;
			}
		break;
	case WM_RBUTTONUP:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////

int __stdcall WinMain(HINSTANCE hInstance, 
				   HINSTANCE hPrevInstance, 
				   LPSTR	 lpCmdLine, 
				   int		 nCmdShow) 
{
	WNDCLASSEX wc;
	HWND hWnd;
	MSG msg;
	long begin, end;
	double time_spent;

	begin = clock();
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = "WindowClass";
	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
	RegisterClassEx(&wc);
	hWnd = CreateWindowEx(0, "WindowClass", "Ocelot Loader", 0x90008000, 
						  0, 0, 240, 160, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOW); 
    UpdateWindow(hWnd); 
	InitD3D(hWnd);	
	while(GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		RenderFrame();
	}
	CleanD3D();
	end = clock();
	time_spent = (double)(end - begin) / 1000;
	//char length_string[20];
	//_snprintf(length_string, sizeof(length_string), "%f", time_spent);
	//MessageBox(0, length_string, 0, 0);
	return msg.wParam;
}

///////////////////////////////////////////////////////////////////////////////

char* Customstrstr(const char *in, const char *str)
{
	char c;
	size_t len;
	c = *str++;
	if (!c)
		return (char *)in;
	len = strlen(str);
	do {
		char sc;
		do {
			sc = *in++;
			if (!sc)
				return (char *)0;
		} while (sc != c);
	} while (strncmp(in, str, len) != 0);
	return (char *)(in - 1);
}

///////////////////////////////////////////////////////////////////////////////

//
// Game process ID searching function
//
unsigned long GetTargetThreadIDFromProcName(const char *procName) 
{
	PROCESSENTRY32 pe; // process snapshot
	int retval;
	// getting the whole system processes snapshot
	HANDLE thSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	pe.dwSize = sizeof(PROCESSENTRY32);
	
	retval = Process32First(thSnapShot, &pe); // get the 1-st process
	while (retval) { // while there are any processes left
		// if process name == our needed name, then return the pid and quit		
		if (Customstrstr(pe.szExeFile, procName))
			return pe.th32ProcessID;
		// if not - get the next process
		retval = Process32Next(thSnapShot, &pe); 
	} 
	return 0; //if nothing was found - return 0 and quit
}

///////////////////////////////////////////////////////////////////////////////

int Inject(const unsigned long pID, const char* dllName) 
{ 
	HANDLE pHandle;
	unsigned long loadLibAddr;
	LPVOID rString;
	HANDLE hThread;

	if(!pID) 
		return 1;
	pHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, pID);
	// get the LoadLibraryA() address from kernel32.dll
	loadLibAddr = 
		(unsigned long)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"); 
	rString = (LPVOID)VirtualAllocEx(pHandle, 0, strlen(dllName), 
									 MEM_RESERVE | MEM_COMMIT, 
									 PAGE_READWRITE); 
	WriteProcessMemory(pHandle, (LPVOID)rString, dllName, strlen(dllName), 0);
	hThread = CreateRemoteThread(pHandle, 0, 0, 
								(LPTHREAD_START_ROUTINE)loadLibAddr, 
								(LPVOID)rString, 0, 0);
	CloseHandle(pHandle); // close the opened handle 
	return 0; // everything is done, so quit
}

///////////////////////////////////////////////////////////////////////////////