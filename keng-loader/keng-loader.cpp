#include <Windows.h>
#include <d3d9.h>
#include <tlhelp32.h>
#include <shlwapi.h> 

#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "Shlwapi.lib")

DWORD GetTargetThreadIDFromProcName(const char*);
int Inject(DWORD, const char *); 

LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;

void CleanD3D()
{
	d3ddev->Release();
	d3d->Release();
}

void RenderFrame()
{	
	d3ddev->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();
	//d3ddev->DrawPrimitiveUP(D3DPT_LINELIST, 1, random_data, sizeof(line_vertex));

	
	d3ddev->EndScene();	
	d3ddev->Present(0, 0, 0, 0);
}

void InitD3D(HWND hWnd) 
{	
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	PAINTSTRUCT ps; 
    HDC hdc; 

	switch(msg)	
	{
	case WM_CREATE:
		{
			
			break;
		}
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
	case WM_LBUTTONDOWN:
		{
			SendMessage(hWnd, 0xA1, 0x2, 0);
			DWORD pID = GetTargetThreadIDFromProcName("d3d9_lesson0_keng.exe");
			if(pID == 0)
			{
				MessageBox(0, "ERROR: Unable to find pID!", "Ocelot Loader @ keng", MB_OK);
				return 0;
			}
			else
			{
				Beep(1000, 100);
				char buf[MAX_PATH] = {0}; 
				GetFullPathName("keng-hook.dll", MAX_PATH, buf, 0);
				Inject(pID, buf);
				return 0;
			}
			break;
		}
	case WM_RBUTTONUP:
		{
			PostQuitMessage(0);
			break;
		}
	case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);
			HPEN penWhite = CreatePen(PS_INSIDEFRAME, 3, RGB(255, 255, 255));
			HPEN penBlack = CreatePen(PS_INSIDEFRAME, 3, RGB(0, 0, 0));
			//HBRUSH hb = CreateSolidBrush(
			SelectObject(hdc, penWhite);
			//Rectangle(hdc, 4, 4, 236, 156);
			//SelectObject(hdc, penBlack);
			//Rectangle(hdc, 8, 8, 232, 152);
            MoveToEx(hdc, 4, 4, 0);
			LineTo(hdc, 4, 155);
			LineTo(hdc, 235, 155);
			LineTo(hdc, 235, 4);
			LineTo(hdc, 4, 4);
            EndPaint(hWnd, &ps);
			break;
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = "WindowClass";
	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindowEx(0, "WindowClass", "Test D3D9", 0x90008000, 0, 0, 240, 160, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOW); 
    UpdateWindow(hWnd); 
	//InitD3D(hWnd);
	MSG msg;
	while(GetMessage(&msg, 0, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
		}
	return msg.wParam;
}

//Game process ID searching function
DWORD GetTargetThreadIDFromProcName(const char * procName) 
{
	PROCESSENTRY32 pe; //process snapshot
	HANDLE thSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //getting the whole system processes snapshot
	if(thSnapShot == INVALID_HANDLE_VALUE) //if something is wrong
	{ //show error message
		MessageBox(0, "ERROR: Unable to create toolhelp snapshot!", "Ocelot Loader @ keng", MB_OK); 
		return 1; //and quit
	}
	pe.dwSize = sizeof(PROCESSENTRY32);
	int retval = Process32First(thSnapShot, &pe); //get the 1-st process
	while(retval) //while there are any processes left
	{ 
		if(StrStrI(pe.szExeFile, procName)) return pe.th32ProcessID; //if process name == our needed name, then return the pid and quit
		retval = Process32Next(thSnapShot, &pe); //if not - get the next process
	} 
	return 0; //if nothing was found - return 0 and quit
}

//dll injection function
int Inject(DWORD pID, const char * dllName) 
{ 
	if(!pID) return 0; //if we've got no pID - quit
	HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, pID); //obtain the process handle
	if(!pHandle) //in case of error - show error message and quit
	{ 
		MessageBox(0, "ERROR: Unable to open target process!", "Loader", MB_OK); 
		return 1;
	}
	LPVOID loadLibAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"); //get the LoadLibraryA() address from kernel32.dll
	//allocate a memory block for the full dll's path
	LPVOID rString = (LPVOID)VirtualAllocEx(pHandle, 0, strlen(dllName), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); 
	//and write the path into target process memory
	WriteProcessMemory(pHandle, (LPVOID)rString, dllName, strlen(dllName), 0);
	//create a LoadLibrary(myDllPath) thread inside the target process
	HANDLE hThread = CreateRemoteThread(pHandle, 0, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, (LPVOID)rString, 0, 0);
	if(hThread == 0)
	{
		MessageBox(0, "ERROR: Unable to create remote thread!", "Loader", MB_OK);
		return 1;
	}
	CloseHandle(pHandle); //close the opened handle 
	return 0; //everything is done, so quit
} 