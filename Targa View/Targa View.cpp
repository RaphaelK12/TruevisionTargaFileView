// Editor_001.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS 1
#include "stdafx.h"
#include "Targa View.h"
#include "Util.h"

char g_szClassName[] = "Targa View";
char a[MAX_PATH*2] = "2/rle_rgba_8.tga\0\0\0";
char list[MAX_PATH];

static HINSTANCE g_hInst = NULL;
HBITMAP hbm1, 
		hbmTmp;
HBRUSH background;
FILE *fi = NULL;
int screenx = 300, 
	screeny = 300;
bool adaptive=1;

BOOL GetFileName(HWND hwnd, LPSTR pszFileName, BOOL bSave)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	pszFileName[0] = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "True Vision Targa (*.tga)\0*.tga\0All Files (*.*)\0*.*\0\0\0\0\0\0";
	ofn.lpstrFile = pszFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = "tga";
	if (bSave)
	{
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		if (!GetSaveFileName(&ofn))
			return FALSE;
	}
	else
	{
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (!GetOpenFileName(&ofn))
			return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_CREATE:
		break;
	case WM_PAINT:
	{
		BITMAP bm;
		PAINTSTRUCT ps;
		HDC hdcMem, hdc;
		HBITMAP hbmOld;
		float w1=1, w2=1;
		float x, y, scx, scy;
		if (hbm1) {
			scx = (float)screenx;
			scy = (float)screeny;
			GetObject(hbm1, sizeof(bm), &bm);
			x = (float)bm.bmWidth+2;
			y = (float)bm.bmHeight+2;

			if (scx < x) w1 = scx / x; 
			if (scy < y) w2 = scy / y; 
			x = (bm.bmWidth+2) * min(w1, w2);
			y = (bm.bmHeight+2) * min(w1, w2);

			hdc = BeginPaint(hwnd, &ps);
			hdcMem = CreateCompatibleDC(hdc);
			hbmOld = (HBITMAP)SelectObject(hdcMem, hbm1);
			GetObject(hbm1, sizeof(bm), &bm);
			if (adaptive)
			//					posstr		  posend									posstr2
				StretchBlt(hdc, 1, 1, int(floorf(x - 1.5f)), int(floorf(y - 1.5f)), hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			else
				BitBlt(hdc, 1, 1, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			//StretchDIBits(hdc, 10, 10, 200, 200, 0, 0, bm.bmWidth, bm.bmHeight, 0, 0, 0, 0);
			SelectObject(hdcMem, hbmOld);
			EndPaint(hwnd, &ps);
			DeleteDC(hdcMem);
			DeleteObject(hbmOld);
		}
		else {
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
		}
		break;
	}
	case WM_SIZE:
	{
		screenx = LOWORD(lParam);
		screeny = HIWORD(lParam);
		InvalidateRect(hwnd, NULL, true);
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		DeleteObject(hbm1);
		DeleteObject(hbmTmp);
		DeleteObject(background);
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case ID_FILE_OPEN:
				{
					DeleteObject(hbm1);
					BOOL get = GetFileName(hwnd,a,false);
					if(get)
					{
						FILE * file;
						if (!(file = fopen(a, "rb"))) return NULL;
						img_basis* img ;
						int sf = 0;
						TGA h;
						fread(&h, 1, sizeof(TGA), file);
						img = loadTGA(file, h, sf);
						fclose(file);
						if (!img) {
							printf("***********************\n*** Unsuported File ***\n");
							printf("%s\n id_length   = %i\n color_type  = %i\n image_type  = %i\n "
								"color_start = %i\n pal_length  = %i\n pal_depth   = %i\n "
								"x_offset = %i\n y_offset = %i\n xres = %i\n yres = %i\n "
								"bpp  = %i\n orientation = %i\n ",
								a, h.id_length, h.color_type, h.image_type,
								h.color_start, h.pal_length, h.pal_depth,
								h.x_offset, h.y_offset, h.xres, h.yres,
								h.bpp, h.orientation);
							char Text[500];
							sprintf(Text, "\"%s\"\nTGA File HEADER::\n "
								"id_length   = %i\n color_type  = %i\n image_type = %i\n "
								"color_start  = %i\n pal_length  = %i\n pal_depth   = %i\n "
								"x_offset = %i\n y_offset = %i\n xres = %i\n yres = %i\n "
								"bpp = %i\n orientation = %i\n ",
								a, h.id_length, h.color_type, h.image_type,
								h.color_start, h.pal_length, h.pal_depth,
								h.x_offset, h.y_offset, h.xres, h.yres,
								h.bpp, h.orientation);
							InvalidateRect(hwnd, NULL, true);
							MessageBox(0, Text, "*** Unsuported TGA File ***", 0);

							break;
						}
						BITMAPINFO bi = {0,0,0};
						img->unflip();
						bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
						bi.bmiHeader.biWidth = img->xres;
						bi.bmiHeader.biHeight = img->yres;
						bi.bmiHeader.biSizeImage = img->dataSize;
						bi.bmiHeader.biPlanes = 1;
						bi.bmiHeader.biBitCount = img->bpp;
						bi.bmiHeader.biCompression = 0;
						void* pData = NULL;
						if (!img->pixels) {
							delete img;
							InvalidateRect(hwnd, NULL, true);
							MessageBox(0, "error: No Pixels;", "img->pixels == NULL", 0);
							return NULL;
						}
						hbm1 = CreateDIBSection(NULL, &bi, 0, &pData, NULL, 0);
						if (hbm1)
							SetBitmapBits(hbm1, img->dataSize, img->pixels);
						if (!hbm1)
						{
							char Text[500];
							sprintf(Text, "hbm = %p, %i %i %i %p", hbm1, img->xres, img->yres, img->dataSize, img->pixels);
							MessageBox(0, Text, "bytes vazios", 0);
						}
						InvalidateRect(hwnd, NULL, true);
						char Text[500];
						sprintf(Text,	"%s\n,  id_length = %i\n color_type = %i\n image_type = %i\n "
										"color_start = %i\n pal_length = %i\n pal_depth = %i\n "
										"x_offset = %i\n y_offset = %i\n xres = %i\n yres = %i\n "
										"bpp = %i\n orientation = %i\n ", 
										a, h.id_length, h.color_type, h.image_type, 
										h.color_start, h.pal_length, h.pal_depth, 
										h.x_offset, h.y_offset, h.xres, h.yres, 
										h.bpp, h.orientation );
						/*s*/printf(			"%s\nid_length = %i\ncolor_type = %i\nimage_type = %i\n"
										"color_start = %i\npal_length = %i\npal_depth = %i\n"
										"x_offset = %i\ny_offset = %i\nxres = %i\nyres = %i\n"
										"bpp = %i\norientation = %i\n\n", 
										a, h.id_length, h.color_type, h.image_type, 
										h.color_start, h.pal_length, h.pal_depth, 
										h.x_offset, h.y_offset, h.xres, h.yres, 
										h.bpp, h.orientation );
						//MessageBox(0, Text, "TGA Header", 0);
						delete img;
					}
					break;
				}
			case ID_FILE_SAVE:
				break;
			case ID_FILE_SAVE_AS:
				break;
			case ID_FILE_SAVE_ALL:
				break;
			case ID_FILE_CLONE:
				break;
			case ID_FILE_CLOSE:
				break;
			case ID_FILE_CLOSE_ALL:
				break;
			case ID_FILE_EXIT:
				PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
			case ID_HELP_ABOUT:
				DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
				break;
			case ID_ADAPTIVE:
				adaptive = !adaptive;
				InvalidateRect(hwnd, NULL, true);
				break;
		}
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

#ifdef _DEBUG
int main(int argc, char* argv[])
#else
int WINAPI WinMain(	_In_ HINSTANCE hInstance,	_In_opt_ HINSTANCE hPrevInstance,	_In_ LPSTR lpCmdLine,	_In_ int nShowCmd)
#endif // _DEBUG
{
	WNDCLASSEX WndClass;
	HWND hwnd;
	MSG Msg;
	HACCEL hAccelTable;
	background = CreateSolidBrush(0x999999);
#ifdef _DEBUG
	g_hInst = GetModuleHandle(NULL);
#else
	g_hInst = hInstance;
#endif // _DEBUG
	WndClass.cbSize        = sizeof(WNDCLASSEX);
	WndClass.style         = 0;
	WndClass.lpfnWndProc   = WndProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = g_hInst;
	WndClass.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = background;
	WndClass.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);
	WndClass.lpszClassName = g_szClassName;
	WndClass.hIconSm       = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	if(!RegisterClassEx(&WndClass))
	{
		MessageBox(0, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
		return 0;
	}
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"A Bitmap Program",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
		NULL, NULL, g_hInst, NULL);
	if(hwnd == NULL)
	{
		MessageBox(0, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
		return 0;
	}
	ShowWindow(hwnd,
#ifdef _DEBUG
		1
#else
		nShowCmd
#endif // _DEBUG
	);
	UpdateWindow(hwnd);
	hAccelTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	while(GetMessage(&Msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(Msg.hwnd, hAccelTable, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	return Msg.wParam;
}






