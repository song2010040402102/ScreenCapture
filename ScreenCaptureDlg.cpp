#include "ScreenCaptureDlg.h"
#include "ScreenCapture.h"
#include "resource.h"

#define WM_MSG_FROM_TOOL (WM_USER + 0X1001)
#define WM_MSG_FROM_EDIT (WM_USER + 0X1002)
#define WM_MSG_FROM_CUSTOM (WM_USER + 0X1003)
#define WM_MOUSEHOVER_FROM_TIMER (WM_USER + 0x1004)
#define WM_MOUSELEAVE_FROM_TIMER (WM_USER + 0x1005)

#define TIMER_ID_MOUSE_LISTEN 0X2001
#define TIMER_ID_PROCESS_DRAW 0X3001

#define WORD_WIDTH	  100
#define WORD_HEIGHT    40
#define WORD_GAP		2

#define TOOL_BK_COLOR RGB(252, 252, 252)

enum IMAGE_FORMAT
{
	IF_BMP = 0,
	IF_JPG, 
	IF_PNG
};

extern CAPTURE_DATA* g_pCaptureData;
extern CSCDialog g_scDialog;
extern HANDLE g_hModule;

 //三种自定义线宽
int g_custom_line_width[3] = {1, 2, 4};

//10种自定义字体大小
int g_custom_font_size[10] = {8, 9, 10, 11, 12, 14, 16,18, 20, 22};

//16种自定义颜色
COLORREF g_custom_color[16] = {RGB(0, 0, 0), RGB(128, 128, 128), RGB(128, 0, 0), RGB(247, 136, 58), 
							   RGB(48, 132, 48), RGB(56, 90, 211), RGB(128, 0, 128), RGB(0, 153, 153), 
							   RGB(255, 255, 255), RGB(192, 192, 192), RGB(251, 56, 56), RGB(255, 255, 0), 
							   RGB(153, 204, 0), RGB(56, 148, 228), RGB(243, 27, 243), RGB(22, 220, 220)
};

//写日志
void WriteLog(char *string, ...)
{
	char buffer[1024];
	char curDir[MAX_PATH] = {0}, filename[MAX_PATH] = {0};
	::GetCurrentDirectory(MAX_PATH, curDir);
	strcpy(filename, curDir), strcat(filename, "\\log.txt");
	FILE *file = fopen(filename, "a");	
	
	if (!string || !file)
		return;
	
	va_list arglist;
	
	va_start(arglist,string);
	vsprintf(buffer,string,arglist); 
	va_end(arglist);
	
	fprintf(file,buffer);	
	fflush(file);
	
	fclose(file);
}

//错误提示框
VOID ErrorBox(LPTSTR ErrorInfo)
{
	CHAR error1[50],error2[20];
	strcpy(error1,ErrorInfo);
	sprintf(error2," with error %d",GetLastError());	
	strcat(error1,error2);	
	::MessageBox(NULL, error1, "error", MB_OK);
}

//创建普通字体
HFONT CreateSimpleFont(int width,int height)
{
	HFONT hFont = ::CreateFont(
		height,
		width,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH,
		NULL
		);
	return hFont;
}

//把位图句柄写入到文件
bool SaveBitmapToFile(HBITMAP hBitmap, int x, int y, int width, int height, char* filename, bool bSaveCB)
{
	if(!hBitmap || !filename)
		return false;

	if(width<=0 || height<=0)
		return false;

	BITMAP bitmapInfo;
	::GetObject(hBitmap, sizeof(BITMAP), &bitmapInfo);
	if(bitmapInfo.bmWidth <=0 || bitmapInfo.bmHeight <=0)
		return false;
	
	if(x<0) x = 0;
	if(y<0) y = 0;	
	if(x+width > bitmapInfo.bmWidth)
		width = bitmapInfo.bmWidth-x;
	if(y+height > bitmapInfo.bmHeight)
		height = bitmapInfo.bmHeight-y;
	
	HDC hDC = ::GetDC(NULL);
	HDC hMemDCSrc = ::CreateCompatibleDC(hDC);	
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDCSrc, hBitmap);

	HDC hMemDCCut = ::CreateCompatibleDC(hDC);
	HBITMAP hBitmapCut = ::CreateCompatibleBitmap(hDC, width, height);
	HBITMAP hOldBitmapCut = (HBITMAP)::SelectObject(hMemDCCut, hBitmapCut);
	
	::BitBlt(hMemDCCut, 0, 0, width, height, hMemDCSrc, x, y, SRCCOPY);

	::SelectObject(hMemDCSrc, hOldBitmap);
	::DeleteDC(hMemDCSrc);
	
	BITMAPINFO BmpInfo;
	ZeroMemory(&BmpInfo, sizeof(BmpInfo));
	BmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BmpInfo.bmiHeader.biWidth = width;
	BmpInfo.bmiHeader.biHeight = height;
	BmpInfo.bmiHeader.biPlanes = 1;
	BmpInfo.bmiHeader.biBitCount = 32;
	BmpInfo.bmiHeader.biCompression = BI_RGB;	
	
	int OffSize = sizeof(BITMAPFILEHEADER)+BmpInfo.bmiHeader.biSize;
	int BmpSize = BmpInfo.bmiHeader.biWidth*BmpInfo.bmiHeader.biHeight*4;
	int FileSize = OffSize+BmpSize;
	
	BITMAPFILEHEADER FileHeader;
	ZeroMemory(&FileHeader, sizeof(FileHeader));
	FileHeader.bfSize = FileSize;
	FileHeader.bfType = 'B'+('M'<<8);
	FileHeader.bfOffBits = OffSize;
	
	PVOID pBmpData;
	pBmpData = ::HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, BmpSize);
	ZeroMemory(pBmpData, BmpSize);	
	
	if(::GetDIBits(hMemDCCut, hBitmapCut, 0, BmpInfo.bmiHeader.biHeight, pBmpData, &BmpInfo, DIB_RGB_COLORS) == 0)
	{
		return false;
	}
	
	HANDLE hFile;
	DWORD dwWriten;
	hFile = ::CreateFile(
		filename, 
		GENERIC_READ |GENERIC_WRITE, 
		FILE_SHARE_READ |FILE_SHARE_WRITE, 
		NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	BOOL Written[3];
	Written[0] = ::WriteFile(hFile, &FileHeader, sizeof(FileHeader),   &dwWriten, NULL);
	Written[1] = ::WriteFile(hFile, &BmpInfo, sizeof(BmpInfo),   &dwWriten, NULL);
	Written[2] = ::WriteFile(hFile, pBmpData, BmpSize, &dwWriten, NULL);
	if(!Written[0] ||!Written[1] ||!Written[2])
	{
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	
	::HeapFree(GetProcessHeap(), 0, pBmpData);

	if(bSaveCB)
	{
		if(::OpenClipboard(NULL)) 
		{			
			::EmptyClipboard();			
			::SetClipboardData(CF_BITMAP, hBitmapCut);			
			::CloseClipboard();
		}
	}
	
	::SelectObject(hMemDCCut, hOldBitmapCut);
	::DeleteObject(hBitmapCut);
	::DeleteDC(hMemDCCut);

	::ReleaseDC(::GetDesktopWindow(), hDC);

	return true;
}

//从资源中加载PNG
Gdiplus::Image* LoadPNGFromRes(UINT pResourceID, HMODULE hInstance, LPCTSTR pResourceType)
{
	HBITMAP hBitmap = NULL;
	LPCTSTR pResourceName = MAKEINTRESOURCE(pResourceID);
	
	HRSRC hResource = ::FindResource(hInstance, pResourceName, pResourceType);
	if(!hResource)
		return NULL;
	
	DWORD dwResourceSize = ::SizeofResource(hInstance, hResource);
	if(!dwResourceSize)
		return NULL;
	
	const void* pResourceData = ::LockResource(::LoadResource(hInstance, hResource));
	if(!pResourceData)
		return NULL;
	
	HGLOBAL hResourceBuffer = ::GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
	if(!hResourceBuffer)
	{
		::GlobalFree(hResourceBuffer);
		return NULL;
	}
	
	void* pResourceBuffer = ::GlobalLock(hResourceBuffer);
	if(!pResourceBuffer)
	{
		::GlobalUnlock(hResourceBuffer);
		::GlobalFree(hResourceBuffer);
		return NULL;
	}
	
	::CopyMemory(pResourceBuffer, pResourceData, dwResourceSize);
	IStream* pIStream = NULL;
	
	if(::CreateStreamOnHGlobal(hResourceBuffer, FALSE, &pIStream)==S_OK)
	{
		Gdiplus::Image *pImage = Gdiplus::Image::FromStream(pIStream);
		pIStream->Release();
		::GlobalUnlock(hResourceBuffer);
		::GlobalFree(hResourceBuffer);		

		if(pImage == NULL)
			return NULL;	
		
		Gdiplus::Status result = pImage->GetLastStatus();
		if(result==Gdiplus::Ok)
			return pImage;
		
		delete pImage;
	}
	::GlobalUnlock(hResourceBuffer);
	::GlobalFree(hResourceBuffer);
	
	return NULL;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)    
{    
	UINT  num = 0;          // number of image encoders     
	UINT  size = 0;         // size of the image encoder array in bytes     
	
	ImageCodecInfo* pImageCodecInfo = NULL;    
	
	//2.获取GDI+支持的图像格式编码器种类数以及ImageCodecInfo数组的存放大小     
	GetImageEncodersSize(&num, &size);    
	if(size == 0)    
		return -1;  // Failure     
	
	//3.为ImageCodecInfo数组分配足额空间     
	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));    
	if(pImageCodecInfo == NULL)    
		return -1;  // Failure     
	
	//4.获取所有的图像编码器信息     
	GetImageEncoders(num, size, pImageCodecInfo);    
	
	//5.查找符合的图像编码器的Clsid     
	for(UINT j = 0; j < num; ++j)    
	{    
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )    
		{    
			*pClsid = pImageCodecInfo[j].Clsid;    
			free(pImageCodecInfo);    
			return j;  // Success     
		}        
	}    
	
	//6.释放步骤3分配的内存     
	free(pImageCodecInfo);    
	return -1;  // Failure     
} 

void ConvertImageFomat(char *filename, IMAGE_FORMAT image_format)
{	
	wchar_t capPath[MAX_PATH] = {0};
	int len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0); 
	MultiByteToWideChar (CP_ACP, 0, filename, -1, capPath, len);
	
	CLSID   encoderClsid;    
	Status  stat;    	
	Image*   image = new Image(capPath);  
	Bitmap* bitmap = new Bitmap(image->GetWidth(), image->GetHeight(), PixelFormat16bppRGB565);
	Gdiplus::Graphics gr(bitmap);
	Gdiplus::Rect desRect(0, 0, image->GetWidth(), image->GetHeight());
	gr.DrawImage(image, desRect, 0, 0, image->GetWidth(), image->GetHeight(), Gdiplus::UnitPixel);	
	
    if(image)
	{
		delete image;
		image = NULL;
	}
	
	switch(image_format)
	{
	case IF_BMP:
		GetEncoderClsid(L"image/bmp", &encoderClsid);  
		break;
	case IF_JPG:
		GetEncoderClsid(L"image/jpeg", &encoderClsid);  
		break;
	case IF_PNG:
		GetEncoderClsid(L"image/png", &encoderClsid);  
		break;
	default:
		GetEncoderClsid(L"image/bmp", &encoderClsid);  
		break;
	}	  	
	stat = bitmap->Save(capPath, &encoderClsid, NULL); 
	
	if(bitmap)
	{
		delete bitmap;
		bitmap = NULL;
	}	
}

map<HWND, CWnd*> CWnd::m_mapWndList;

CWnd::CWnd()
{
	m_hWnd = NULL;		
}

CWnd::~CWnd()
{
	
}

void CWnd::ToWndList(CWnd* pWnd)
{
	if(pWnd)
	{
		m_mapWndList.insert(make_pair(pWnd->m_hWnd, pWnd));	
	}	
	return;
}

CWnd* CWnd::FromWndList(HWND hWnd)
{
	map<HWND, CWnd*>::iterator iter = m_mapWndList.find(hWnd);
	if(iter != m_mapWndList.end())
	{
		return iter->second;
	}
	return NULL;
}

LRESULT CALLBACK CWnd::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWnd* pWnd = FromWndList(hWnd);
	if(!pWnd)
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	else
		return pWnd->WindowProc(uMsg, wParam, lParam);
}

CMyButton::CMyButton()
{
	m_nButtonID = 0;
	m_buttonState = BS_NORMAL;
	m_buttonStyle = BS_BUTTON;
	memset(m_pBkImage, 0, sizeof(m_pBkImage));
	m_lastCursorPos = 0;
	m_pFunCustomDrawButton = NULL;
}

CMyButton::~CMyButton()
{
	
}

HWND CMyButton::CreateButton(UINT idCtl, HWND hParent, DWORD dwStyle)
{
	m_nButtonID = idCtl;
	DWORD dwButtonStyle = WS_VISIBLE | WS_CHILD;
	dwButtonStyle |= dwStyle;		
	m_hWnd = ::CreateWindow(			
		"BUTTON",
		"",
		dwButtonStyle, 
		CW_USEDEFAULT,0,
		CW_USEDEFAULT,0,
		hParent,	
		(HMENU)idCtl,
		(HINSTANCE)g_hModule,
		NULL
		);
	if(m_hWnd==NULL)
	{		
		//ErrorBox("CreateButton failed");
		return NULL;
	}
	else
	{
		ToWndList(this);
	}
	
	m_oldWndProc = (WNDPROC)::SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG)WndProc);
	if(!m_oldWndProc)
	{
		//ErrorBox("SetWindowLong failed");
	}
	
	::SetTimer(m_hWnd, TIMER_ID_MOUSE_LISTEN, 100, NULL);

	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);	

	return m_hWnd;
}

void CMyButton::SetPNGBitmaps(Image* pPngImageNormal, Image* pPngImageFocus, Image* pPngImageDown, 
		Image* pPngImageDisable)
{
	if(m_pBkImage[0])
	{
		delete m_pBkImage[0];
		m_pBkImage[0] = NULL;
	}	
	m_pBkImage[0] = pPngImageNormal;

	if(m_pBkImage[1])
	{
		delete m_pBkImage[1];
		m_pBkImage[1] = NULL;
	}	
	m_pBkImage[1] = pPngImageFocus;

	if(m_pBkImage[2])
	{
		delete m_pBkImage[2];
		m_pBkImage[2] = NULL;
	}	
	m_pBkImage[2] = pPngImageDown;

	if(m_pBkImage[3])
	{
		delete m_pBkImage[3];
		m_pBkImage[3] = NULL;
	}	
	m_pBkImage[3] = pPngImageDisable;
}

LRESULT CALLBACK CMyButton::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bCut = FALSE;
	switch(uMsg)
	{
	case WM_PAINT:
		bCut = OnPaint(wParam, lParam);
		break;
	case WM_ERASEBKGND:
		bCut = OnEraseBkgnd(wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		bCut = OnLButtonDown(wParam, lParam);
		break;
	case WM_LBUTTONUP:
		bCut = OnLButtonUp(wParam, lParam);
		break;
	case WM_TIMER:
		bCut = OnTimer(wParam, lParam);
		break;
	case WM_MOUSEHOVER_FROM_TIMER:
		bCut = OnMouseHover(wParam, lParam);
		break;
	case WM_MOUSELEAVE_FROM_TIMER:
		bCut = OnMouseLeave(wParam, lParam);
		break;
	case WM_DESTROY:
		bCut = OnDestroy(wParam, lParam);
		break;
	}

	if(!bCut)
		return ::CallWindowProc(m_oldWndProc, m_hWnd, uMsg, wParam, lParam);
	
	return 1;
}

BOOL CMyButton::OnPaint(WPARAM wParam, LPARAM lParam)
{	
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hWnd, &ps);	

	RECT rtClient;
	::GetClientRect(m_hWnd, &rtClient);

	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, rtClient.right - rtClient.left, rtClient.bottom - rtClient.top);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);

	HBRUSH hBrush = (HBRUSH)::CreateSolidBrush(TOOL_BK_COLOR);
	HBRUSH hOldBrush = (HBRUSH)::SelectObject(hMemDC, hBrush);

	::FillRect(hMemDC, &rtClient, hBrush);

	::SelectObject(hMemDC, hOldBrush);
	::DeleteObject(hBrush);		

	if(m_pFunCustomDrawButton)
	{
		m_pFunCustomDrawButton(this, hMemDC, &rtClient);
	}
	else
	{
		Image* pImage = m_pBkImage[m_buttonState];
		if(pImage)
		{
			Graphics graphics(hMemDC);	
			UINT width = pImage->GetWidth();
			UINT height = pImage->GetHeight();		
			graphics.DrawImage(pImage, 0, 0, width, height);		
		}		
	}
	
 	::BitBlt(hDC, 0, 0, rtClient.right - rtClient.left, rtClient.bottom - rtClient.top, hMemDC, 0, 0, SRCCOPY);
 
	::SelectObject(hMemDC, hOldBitmap);
	::DeleteObject(hBitmap);	

 	::DeleteObject(hMemDC);

	::EndPaint(m_hWnd, &ps);
	return TRUE;
}

BOOL CMyButton::OnEraseBkgnd(WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

BOOL CMyButton::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	m_buttonState = BS_DOWN;
	::InvalidateRect(m_hWnd, NULL, FALSE);
	return FALSE;
}

BOOL CMyButton::OnLButtonUp(WPARAM wParam, LPARAM lParam)	
{
	if(m_buttonStyle == BS_RADIO && m_buttonState == BS_DOWN)
	{
		
	}
	else
	{
		m_buttonState = BS_NORMAL;		
	}	
	::InvalidateRect(m_hWnd, NULL, FALSE);	
	return FALSE;
}

BOOL CMyButton::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
	if(m_buttonStyle == BS_RADIO && m_buttonState == BS_DOWN)
	{
		
	}
	else
	{
		m_buttonState = BS_HOVER;	
	}	
	::InvalidateRect(m_hWnd, NULL, FALSE);
	return FALSE;
}

BOOL CMyButton::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{	
	if(m_buttonStyle == BS_RADIO && m_buttonState == BS_DOWN)
	{
		
	}
	else
	{
		m_buttonState = BS_NORMAL;		
	}	
	::InvalidateRect(m_hWnd, NULL, FALSE);	
	return FALSE;
}

BOOL CMyButton::OnTimer(WPARAM wParam, LPARAM lParam)
{	
	if(wParam == TIMER_ID_MOUSE_LISTEN)
	{
		const int mHover = 0;
		const int mLeave = 1;

		POINT ptCursor;
		::GetCursorPos(&ptCursor);
		RECT rtWindow;
		::GetWindowRect(m_hWnd, &rtWindow);
		if(ptCursor.x > rtWindow.left && ptCursor.x < rtWindow.right &&
			ptCursor.y > rtWindow.top && ptCursor.y < rtWindow.bottom)
		{
			if(m_lastCursorPos!=mHover)
			{
				m_lastCursorPos = mHover;
				PostMessage(m_hWnd, WM_MOUSEHOVER_FROM_TIMER, 0, 0);
			}
		}
		else
		{
			if(m_lastCursorPos!=mLeave)
			{
				m_lastCursorPos = mLeave;
				PostMessage(m_hWnd, WM_MOUSELEAVE_FROM_TIMER, 0, 0);
			}
		}
	}
	return FALSE;
}

BOOL CMyButton::OnDestroy(WPARAM wParam, LPARAM lParam)
{	
	//释放背景图片
	for(int i = 0; i< 4; i++)
	{
		if(m_pBkImage[i])
		{
			delete m_pBkImage[i], m_pBkImage[i] = NULL;
		}
	}
	m_buttonState = BS_NORMAL;
	m_hWnd = NULL;

	return FALSE;
}

CMyEdit::CMyEdit()
{
	m_bEdit = FALSE;
	m_hFont = NULL;
}

CMyEdit::~CMyEdit()
{
	
}

void CMyEdit::SetFont(HFONT hFont)
{
	m_hFont = hFont;
	::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
}

HWND CMyEdit::CreateEdit(UINT idCtl, HWND hParent, DWORD dwStyle)
{
	DWORD dwEditStyle = WS_VISIBLE | WS_CHILD;
	dwEditStyle |= dwStyle;		
	m_hWnd = ::CreateWindow(			
		"EDIT",
		"",
		dwEditStyle, 
		CW_USEDEFAULT,0,
		CW_USEDEFAULT,0,
		hParent,	
		(HMENU)idCtl,
		(HINSTANCE)g_hModule,
		NULL
		);
	if(m_hWnd==NULL)
	{				
		return NULL;
	}
	else
	{
		ToWndList(this);
	}
	
	m_oldWndProc = (WNDPROC)::SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG)WndProc);
	if(!m_oldWndProc)
	{
		
	}		
	
	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);	
	
	return m_hWnd;
}

LRESULT CALLBACK CMyEdit::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bCut = FALSE;
	switch(uMsg)
	{
	case WM_PAINT:
		bCut = OnPaint(wParam, lParam);
		break;
	case WM_DESTROY:
		bCut = OnDestroy(wParam, lParam);
		break;
	case WM_CHAR:
		{
			m_chLastCode = (TCHAR)wParam;
		}
		break;
	case WM_KILLFOCUS:
		{
			::PostMessage(::GetParent(m_hWnd), WM_MSG_FROM_EDIT, (WPARAM)m_hWnd, (LPARAM)uMsg);
		}
		break;
	}
	
	if(!bCut)
		return ::CallWindowProc(m_oldWndProc, m_hWnd, uMsg, wParam, lParam);
	
	return 1;
}

BOOL CMyEdit::OnPaint(WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

BOOL CMyEdit::OnDestroy(WPARAM wParam, LPARAM lParam)
{
	if(m_hFont)
	{
		::DeleteObject(m_hFont);
		m_hFont = NULL;
	}
	return FALSE;
}

CCustomDialog::CCustomDialog()
{
	m_hDlg = NULL;
	m_bShow = FALSE;
	m_nSelButtonID = 0;
	m_nSelSizeButtonID = 0;
	for(int i = 0; i< 4; i++)
	{
		m_customInfo[i].size.line_width = 1;
		m_customInfo[i].color = RGB(255, 0, 0);
	}
	m_customInfo[4].size.font_size = 12;
	m_customInfo[4].color = RGB(255, 0, 0);

	memset(&m_ptSepLine, 0, sizeof(m_ptSepLine));
}

CCustomDialog::~CCustomDialog()
{
	
}

BOOL CCustomDialog::DrawColorButton(CMyButton *pButton, HDC hDC, RECT *pRect)
{
	if(!pButton || !hDC || !pRect)
		return FALSE;
	
	HBRUSH hBrush = ::CreateSolidBrush(g_custom_color[pButton->GetButtonID()-IDC_BUTTON_COLOR_BASE]);	

	if(pButton->GetButtonState() == BS_NORMAL)
	{
		HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush);

		::Rectangle(hDC, pRect->left+1, pRect->top+1, pRect->right-1, pRect->bottom-1);

		::SelectObject(hDC, hOldBrush);
	}
	else if(pButton->GetButtonState() == BS_HOVER || pButton->GetButtonState() == BS_DOWN)
	{
		HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, ::GetStockObject(WHITE_BRUSH));
		::Rectangle(hDC, pRect->left, pRect->top, pRect->right, pRect->bottom);
		::SelectObject(hDC, hOldBrush);

		RECT rtTemp;
		rtTemp.left = pRect->left + 2, rtTemp.top = pRect->top + 2;
		rtTemp.right = pRect->right - 2, rtTemp.bottom = pRect->bottom - 2;
		::FillRect(hDC, &rtTemp, hBrush);
	}	
	
	::DeleteObject(hBrush);
	return TRUE;
}

void CCustomDialog::UpdateControls()
{	
	if(m_nSelButtonID == IDC_BUTTON_WORD)
	{		
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_SMA), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_MID), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_BIG), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), SW_SHOW);

		char strSize[5] = {0};
		_itoa(m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.font_size, strSize, 10);
		::SendMessage(GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), CB_SELECTSTRING, (WPARAM)-1, (LPARAM)strSize);
	}
	else
	{
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), SW_HIDE);
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_SMA), SW_SHOW);
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_MID), SW_SHOW);
		::ShowWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_BIG), SW_SHOW);		

		if(m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.line_width == 1)
		{
			m_nSelSizeButtonID = IDC_BUTTON_SIZE_SMA;
		}
		else if(m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.line_width == 2)
		{
			m_nSelSizeButtonID = IDC_BUTTON_SIZE_MID;
		}
		else if(m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.line_width == 4)
		{
			m_nSelSizeButtonID = IDC_BUTTON_SIZE_BIG;
		}
		UpdateButtonState();
	}
	
	RECT rtColor;
	::GetWindowRect(::GetDlgItem(m_hDlg, IDC_STATIC_COLOR), &rtColor);
	::ScreenToClient(m_hDlg, (LPPOINT)&rtColor);
	::InvalidateRect(m_hDlg, &rtColor, FALSE);
}

void CCustomDialog::UpdateButtonState()
{
	m_buttonWidth[0].SetButtonState(BS_NORMAL);
	m_buttonWidth[1].SetButtonState(BS_NORMAL);
	m_buttonWidth[2].SetButtonState(BS_NORMAL);
	if(m_nSelSizeButtonID == IDC_BUTTON_SIZE_SMA)
	{
		m_buttonWidth[0].SetButtonState(BS_DOWN);
	}
	else if(m_nSelSizeButtonID == IDC_BUTTON_SIZE_MID)
	{
		m_buttonWidth[1].SetButtonState(BS_DOWN);
	}
	else if(m_nSelSizeButtonID == IDC_BUTTON_SIZE_BIG)
	{
		m_buttonWidth[2].SetButtonState(BS_DOWN);
	}
}

void CCustomDialog::ShowCustomDlg(bool bShow, UINT nButtonID)
{
	if(!m_hDlg)
	{
		m_hDlg = ::CreateDialogParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_DIALOG_CUSTOM), g_scDialog.m_hDlg, DialogProc, (LPARAM)this);	
	}	
	
	if(bShow)
	{
		::ShowWindow(m_hDlg, SW_SHOW);		
	}
	else
	{
		::ShowWindow(m_hDlg, SW_HIDE);
	}
	m_bShow = bShow;	
	m_nSelButtonID = nButtonID;

	RECT rtTool, rtCustom;
	::GetWindowRect(g_scDialog.m_ToolDlg.m_hDlg, &rtTool);
	::GetWindowRect(m_hDlg, &rtCustom);
	int xPos = rtTool.left, yPos = rtTool.bottom+1;	
	if(xPos+rtCustom.right-rtCustom.left > g_scDialog.m_nXScreen)
		xPos = g_scDialog.m_nXScreen - (rtCustom.right-rtCustom.left);
	::SetWindowPos(m_hDlg, NULL, xPos, yPos, 0, 0, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE);	

	UpdateControls();
}

BOOL CALLBACK CCustomDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	static CCustomDialog *pCustomDlg = NULL;
	if(uMsg == WM_INITDIALOG)
	{
		pCustomDlg = (CCustomDialog*)lParam;
	}

	if(pCustomDlg)
	{
		switch(uMsg)
		{
		case WM_SETCURSOR:
			{
				HCURSOR hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
				DestroyCursor(hOldCursor);
				return TRUE;
			}
		case WM_INITDIALOG:
			return pCustomDlg->DialogOnInit(hwndDlg, wParam, lParam);		
		case WM_COMMAND:
			return pCustomDlg->DialogOnCommand(wParam, lParam);				
		case WM_PAINT:
			return pCustomDlg->DialogOnPaint(wParam, lParam);		
		case WM_ERASEBKGND:
			return pCustomDlg->DialogOnEraseBkgnd(wParam, lParam);
		case WM_DESTROY:
			return pCustomDlg->DialogOnDestroy(wParam, lParam);
		}	
	}	
	return FALSE; //对于对话框处理程序，在处理完消息之后，应该返回FALSE，让系统进一步处理
}

BOOL CCustomDialog::DialogOnInit(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
#define COLOR_BUTTON_HEIGHT 13
#define COLOR_BUTTON_WIDTH COLOR_BUTTON_HEIGHT
#define COLOR_BUTTON_GAP 1
#define SIZE_BUTTON_GAP COLOR_BUTTON_GAP
#define COLOR_STATIC_HEIGHT 2*COLOR_BUTTON_HEIGHT+COLOR_BUTTON_GAP
#define COLOR_STATIC_WIDTH COLOR_STATIC_HEIGHT
#define LINE_SIZE_BUTTON_HEIGHT COLOR_STATIC_HEIGHT-2*SIZE_BUTTON_GAP
#define LINE_SIZE_BUTTON_WIDTH LINE_SIZE_BUTTON_HEIGHT
#define FONT_SIZE_BUTTON_WIDTH 3*LINE_SIZE_BUTTON_HEIGHT+2*SIZE_BUTTON_GAP
#define SEPARATION_LINE_WIDTH 1
#define SEPARATION_LINE_HEIGHT COLOR_STATIC_HEIGHT
#define CUSTOM_DIALOG_MARGIN 5
#define CUSTOM_DIALOG_CY COLOR_STATIC_HEIGHT+2*CUSTOM_DIALOG_MARGIN
#define CUSTOM_DIALOG_CX  5*CUSTOM_DIALOG_MARGIN+3*LINE_SIZE_BUTTON_WIDTH+2*SIZE_BUTTON_GAP+SEPARATION_LINE_WIDTH+ COLOR_STATIC_WIDTH + 8*COLOR_BUTTON_WIDTH+7*COLOR_BUTTON_GAP

	m_hDlg = hWnd;
	
	::SetWindowPos(m_hDlg, HWND_TOP, 0, 0, CUSTOM_DIALOG_CX, CUSTOM_DIALOG_CY, SWP_SHOWWINDOW | SWP_NOMOVE);

	//创建按钮
	m_buttonWidth[0].CreateButton(IDC_BUTTON_SIZE_SMA, this->m_hDlg);
	m_buttonWidth[1].CreateButton(IDC_BUTTON_SIZE_MID, this->m_hDlg);
	m_buttonWidth[2].CreateButton(IDC_BUTTON_SIZE_BIG, this->m_hDlg);
	
	for(int i = 0; i< 16; i++)
	{
		m_buttonColor[i].CreateButton(IDC_BUTTON_COLOR_BASE + i, this->m_hDlg);
	}

	//调整控件的位置
	RECT rtCombol;
	::GetWindowRect(::GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), &rtCombol);
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), (CUSTOM_DIALOG_CY-(rtCombol.bottom-rtCombol.top))/2, CUSTOM_DIALOG_MARGIN+SIZE_BUTTON_GAP, FONT_SIZE_BUTTON_WIDTH, rtCombol.bottom-rtCombol.top, FALSE);	

	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_SMA), CUSTOM_DIALOG_MARGIN, CUSTOM_DIALOG_MARGIN+SIZE_BUTTON_GAP, LINE_SIZE_BUTTON_WIDTH, LINE_SIZE_BUTTON_HEIGHT, FALSE);
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_MID), CUSTOM_DIALOG_MARGIN+LINE_SIZE_BUTTON_WIDTH+SIZE_BUTTON_GAP, CUSTOM_DIALOG_MARGIN+SIZE_BUTTON_GAP, LINE_SIZE_BUTTON_WIDTH, LINE_SIZE_BUTTON_HEIGHT, FALSE);
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SIZE_BIG), CUSTOM_DIALOG_MARGIN+2*(LINE_SIZE_BUTTON_WIDTH+SIZE_BUTTON_GAP), CUSTOM_DIALOG_MARGIN+SIZE_BUTTON_GAP, LINE_SIZE_BUTTON_WIDTH, LINE_SIZE_BUTTON_HEIGHT, FALSE);

	::MoveWindow(::GetDlgItem(m_hDlg, IDC_STATIC_COLOR), 3*CUSTOM_DIALOG_MARGIN + FONT_SIZE_BUTTON_WIDTH + SEPARATION_LINE_WIDTH, CUSTOM_DIALOG_MARGIN, COLOR_STATIC_WIDTH, COLOR_STATIC_HEIGHT, FALSE);

	int xPos = 4*CUSTOM_DIALOG_MARGIN + FONT_SIZE_BUTTON_WIDTH + SEPARATION_LINE_WIDTH + COLOR_STATIC_WIDTH; 
	int yPos = CUSTOM_DIALOG_MARGIN;
	for(i = 0; i< 8; i++)
	{		
		::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_COLOR_BASE + i), xPos, yPos, COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT, FALSE);
		xPos += COLOR_BUTTON_WIDTH + COLOR_BUTTON_GAP;
	}
	xPos = 4*CUSTOM_DIALOG_MARGIN + FONT_SIZE_BUTTON_WIDTH + SEPARATION_LINE_WIDTH + COLOR_STATIC_WIDTH; 
	yPos += COLOR_BUTTON_HEIGHT + COLOR_BUTTON_GAP;
	for(i = 8; i< 16; i++)
	{
		::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_COLOR_BASE + i), xPos, yPos, COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT, FALSE);
		xPos += COLOR_BUTTON_WIDTH + COLOR_BUTTON_GAP;
	}

	//设置分割线的位置
	m_ptSepLine[0].x = m_ptSepLine[1].x = 2*CUSTOM_DIALOG_MARGIN + FONT_SIZE_BUTTON_WIDTH;
	m_ptSepLine[0].y = CUSTOM_DIALOG_MARGIN, m_ptSepLine[1].y = m_ptSepLine[0].y + SEPARATION_LINE_HEIGHT;

	//给按钮添加背景图片
	m_buttonWidth[0].SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_SIZE_SMA_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SIZE_SMA_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SIZE_SMA_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_buttonWidth[1].SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_SIZE_MID_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SIZE_MID_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SIZE_MID_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_buttonWidth[2].SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_SIZE_BIG_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SIZE_BIG_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SIZE_BIG_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	//设置自绘回调函数
	for(i = 0; i< 16; i++)
	{
		m_buttonColor[i].SetCustomDrawButtonCallback(CCustomDialog::DrawColorButton);
	}

	//设置按钮样式
	m_buttonWidth[0].SetButtonStyle(BS_RADIO);
	m_buttonWidth[1].SetButtonStyle(BS_RADIO);
	m_buttonWidth[2].SetButtonStyle(BS_RADIO);

	for(i = 0; i< 16; i++)
	{
		m_buttonColor[i].SetButtonStyle(BS_BUTTON);
	}

	char strFontSize[10][5] = {0};
	for(i = 0; i< 10; i++)
	{		
		_itoa(g_custom_font_size[i], strFontSize[i], 10);
		::SendMessage(::GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), CB_ADDSTRING, (WPARAM)0, (LPARAM)strFontSize[i]);
	}
	::SendMessage(GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), CB_SELECTSTRING, (WPARAM)-1, (LPARAM)strFontSize[4]);

	return FALSE;
}

BOOL CCustomDialog::DialogOnCommand(WPARAM wParam, LPARAM lParam)
{
	int ID = LOWORD(wParam);
	if(ID == IDCANCEL)
	{
		::EndDialog(m_hDlg, 0);
	}
	
	switch(ID)
	{
	case IDC_BUTTON_SIZE_SMA:				
	case IDC_BUTTON_SIZE_MID:		
	case IDC_BUTTON_SIZE_BIG:		
		{		
			if(ID != m_nSelSizeButtonID)
			{
				if(ID == IDC_BUTTON_SIZE_SMA)
					m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.line_width = g_custom_line_width[0];
				else if(ID == IDC_BUTTON_SIZE_MID)
					m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.line_width = g_custom_line_width[1];
				else if(ID == IDC_BUTTON_SIZE_BIG)
					m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.line_width = g_custom_line_width[2];
				
				m_nSelSizeButtonID = ID;
				UpdateButtonState();
				::PostMessage(::GetParent(m_hDlg), WM_MSG_FROM_CUSTOM, ID, 0);
			}			
		}
		break;	
	case IDC_COMBO_FONTSIZE:
		{
			char strFontSize[5]={0};			
			::GetWindowText(::GetDlgItem(m_hDlg, IDC_COMBO_FONTSIZE), strFontSize, sizeof(strFontSize));						
			int font_size = atoi(strFontSize);
			if(font_size != m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.font_size)
			{
				m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].size.font_size = font_size;
				::PostMessage(::GetParent(m_hDlg), WM_MSG_FROM_CUSTOM, ID, 0);
			}
		}
		break;
	}
	if(ID >= IDC_BUTTON_COLOR_BASE && ID < IDC_BUTTON_COLOR_BASE+16)
	{
		if(m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].color != g_custom_color[ID-IDC_BUTTON_COLOR_BASE])
		{
			m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].color = g_custom_color[ID-IDC_BUTTON_COLOR_BASE];

			RECT rtColor;
			::GetWindowRect(::GetDlgItem(m_hDlg, IDC_STATIC_COLOR), &rtColor);
			::ScreenToClient(m_hDlg, (LPPOINT)&rtColor);
			::InvalidateRect(m_hDlg, &rtColor, FALSE);

			::PostMessage(::GetParent(m_hDlg), WM_MSG_FROM_CUSTOM, ID, 0);
		}				
	}	
	return FALSE;
}

BOOL CCustomDialog::DialogOnPaint(WPARAM wParam, LPARAM lParam)
{
	RECT rtClient;
	GetClientRect(m_hDlg, &rtClient);
	
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hDlg, &ps);	
	
	HBRUSH hBrush = ::CreateSolidBrush(TOOL_BK_COLOR);
	HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush);
	
	::FillRect(hDC, &rtClient, hBrush);

	//绘制分割线
	HPEN hGrayPen = ::CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hGrayPen);

	::MoveToEx(hDC, m_ptSepLine[0].x, m_ptSepLine[0].y, NULL);
	::LineTo(hDC, m_ptSepLine[1].x, m_ptSepLine[1].y);

	//绘制color static
	HWND hColorWnd = ::GetDlgItem(m_hDlg, IDC_STATIC_COLOR);
	RECT rtColorClient;
	::GetClientRect(hColorWnd, &rtColorClient);
	HDC hColorDC = ::GetDC(hColorWnd);
	HBRUSH hColorBrush = ::CreateSolidBrush(m_customInfo[m_nSelButtonID-IDC_BUTTON_RECT].color);
	::FillRect(hColorDC, &rtColorClient, hColorBrush);
	::DeleteObject(hColorBrush);
	::ReleaseDC(hColorWnd, hColorDC);

	::SelectObject(hDC, hOldPen);
	::DeleteObject(hGrayPen);
	
	::SelectObject(hDC, hOldBrush);
	::DeleteObject(hBrush);
	::EndPaint(m_hDlg, &ps);	
	return FALSE;
}

BOOL CCustomDialog::DialogOnEraseBkgnd(WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

BOOL CCustomDialog::DialogOnDestroy(WPARAM wParam, LPARAM lParam)
{
	m_hDlg = NULL;
	return FALSE;
}

CToolDialog::CToolDialog()
{
	m_hDlg = NULL;
	m_bShow = FALSE;
}

CToolDialog::~CToolDialog()
{
	
}

void CToolDialog::ShowToolDlg(bool bShow)
{	
	if(!m_hDlg)
	{
		m_hDlg = ::CreateDialogParam((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_DIALOG_TOOL), g_scDialog.m_hDlg, DialogProc, (LPARAM)this);	
	}	

	if(bShow)
	{
		::ShowWindow(m_hDlg, SW_SHOW);		
	}
	else
	{
		::ShowWindow(m_hDlg, SW_HIDE);
	}
	m_bShow = bShow;
}

BOOL CALLBACK CToolDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	static CToolDialog *pToolDlg = NULL;
	if(uMsg == WM_INITDIALOG)
	{
		pToolDlg = (CToolDialog*)lParam;
	}

	if(pToolDlg)
	{
		switch(uMsg)
		{
		case WM_SETCURSOR:
			{
				HCURSOR hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
				DestroyCursor(hOldCursor);
				return TRUE;
			}
		case WM_INITDIALOG:
			return pToolDlg->DialogOnInit(hwndDlg, wParam, lParam);		
		case WM_COMMAND:
			return pToolDlg->DialogOnCommand(wParam, lParam);		
		case WM_TIMER:
			return pToolDlg->DialogOnTimer(wParam, lParam);
		case WM_PAINT:
			return pToolDlg->DialogOnPaint(wParam, lParam);		
		case WM_ERASEBKGND:
			return pToolDlg->DialogOnEraseBkgnd(wParam, lParam);
		case WM_DESTROY:
			return pToolDlg->DialogOnDestroy(wParam, lParam);
		}	
	}	
	return FALSE; //对于对话框处理程序，在处理完消息之后，应该返回FALSE，让系统进一步处理
}

BOOL CToolDialog::DialogOnInit(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
#define TOOL_DLG_GAP 3
#define TOOL_DLG_BUTTON_RECT_CX 19
#define TOOL_DLG_BUTTON_RECT_CY 19

#define TOOL_DLG_BUTTON_ELLIPSE_CX 19
#define TOOL_DLG_BUTTON_ELLIPSE_CY 19

#define TOOL_DLG_BUTTON_ARROW_CX 19
#define TOOL_DLG_BUTTON_ARROW_CY 19

#define TOOL_DLG_BUTTON_BRUSH_CX 19
#define TOOL_DLG_BUTTON_BRUSH_CY 19

#define TOOL_DLG_BUTTON_WORD_CX 19
#define TOOL_DLG_BUTTON_WORD_CY 19

#define TOOL_DLG_BUTTON_BACK_CX 19
#define TOOL_DLG_BUTTON_BACK_CY 19

#define TOOL_DLG_BUTTON_SAVE_CX 19
#define TOOL_DLG_BUTTON_SAVE_CY 19

#define TOOL_DLG_BUTTON_CANCEL_CX 19
#define TOOL_DLG_BUTTON_CANCEL_CY 19

#define TOOL_DLG_BUTTON_OK_CX 50
#define TOOL_DLG_BUTTON_OK_CY 19

#define TOOL_DLG_CX 232
#define TOOL_DLG_CY 25

	m_hDlg = hWnd;

	//调整工具栏对话框的位置
 	::SetWindowPos(m_hDlg, HWND_TOP, 0, 0, TOOL_DLG_CX, TOOL_DLG_CY, SWP_SHOWWINDOW);	

	//创建保存、取消、确定按钮	
	m_btRect.CreateButton(IDC_BUTTON_RECT, this->m_hDlg);
	m_btEllipse.CreateButton(IDC_BUTTON_ELLIPSE, this->m_hDlg);
	m_btArrow.CreateButton(IDC_BUTTON_ARROW, this->m_hDlg);
	m_btBrush.CreateButton(IDC_BUTTON_BRUSH, this->m_hDlg);
	m_btWord.CreateButton(IDC_BUTTON_WORD, this->m_hDlg);
	m_btBack.CreateButton(IDC_BUTTON_BACK, this->m_hDlg);
	m_btSave.CreateButton(IDC_BUTTON_SAVE, this->m_hDlg);
	m_btCancel.CreateButton(IDC_BUTTON_CANCEL, this->m_hDlg);	
	m_btOk.CreateButton(IDC_BUTTON_OK, this->m_hDlg);

	//调整按钮的位置
	RECT rtClient;
	::GetClientRect(m_hDlg, &rtClient);
	
	RECT rtRect, rtEllipse, rtArrow, rtBrush, rtWord, rtBack, rtSave, rtCancel, rtOk;

	rtRect.left = rtClient.left + TOOL_DLG_GAP, rtRect.right = rtRect.left + TOOL_DLG_BUTTON_RECT_CX;
	rtRect.top = rtClient.top + TOOL_DLG_GAP, rtRect.bottom = rtRect.top + TOOL_DLG_BUTTON_RECT_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_RECT), rtRect.left, rtRect.top, 
		rtRect.right-rtRect.left, rtRect.bottom-rtRect.top, FALSE);

	rtEllipse.left = rtRect.right + TOOL_DLG_GAP, rtEllipse.right = rtEllipse.left + TOOL_DLG_BUTTON_ELLIPSE_CX;
	rtEllipse.top = rtClient.top + TOOL_DLG_GAP, rtEllipse.bottom = rtEllipse.top + TOOL_DLG_BUTTON_ELLIPSE_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_ELLIPSE), rtEllipse.left, rtEllipse.top, 
		rtEllipse.right-rtEllipse.left, rtEllipse.bottom-rtEllipse.top, FALSE);

	rtArrow.left = rtEllipse.right + TOOL_DLG_GAP, rtArrow.right = rtArrow.left + TOOL_DLG_BUTTON_ARROW_CX;
	rtArrow.top = rtClient.top + TOOL_DLG_GAP, rtArrow.bottom = rtArrow.top + TOOL_DLG_BUTTON_ARROW_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_ARROW), rtArrow.left, rtArrow.top, 
		rtArrow.right-rtArrow.left, rtArrow.bottom-rtArrow.top, FALSE);

	rtBrush.left = rtArrow.right + TOOL_DLG_GAP, rtBrush.right = rtBrush.left + TOOL_DLG_BUTTON_BRUSH_CX;
	rtBrush.top = rtClient.top + TOOL_DLG_GAP, rtBrush.bottom = rtBrush.top + TOOL_DLG_BUTTON_BRUSH_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_BRUSH), rtBrush.left, rtBrush.top, 
		rtBrush.right-rtBrush.left, rtBrush.bottom-rtBrush.top, FALSE);

	rtWord.left = rtBrush.right + TOOL_DLG_GAP, rtWord.right = rtWord.left + TOOL_DLG_BUTTON_WORD_CX;
	rtWord.top = rtClient.top + TOOL_DLG_GAP, rtWord.bottom = rtWord.top + TOOL_DLG_BUTTON_WORD_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_WORD), rtWord.left, rtWord.top, 
		rtWord.right-rtWord.left, rtWord.bottom-rtWord.top, FALSE);

	rtBack.left = rtWord.right + TOOL_DLG_GAP, rtBack.right = rtBack.left + TOOL_DLG_BUTTON_BACK_CX;
	rtBack.top = rtClient.top + TOOL_DLG_GAP, rtBack.bottom = rtBack.top + TOOL_DLG_BUTTON_BACK_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_BACK), rtBack.left, rtBack.top, 
		rtBack.right-rtBack.left, rtBack.bottom-rtBack.top, FALSE);

	//调整save按钮位置
	rtSave.left = rtBack.right + TOOL_DLG_GAP, rtSave.right = rtSave.left + TOOL_DLG_BUTTON_SAVE_CX;
	rtSave.top = rtClient.top + TOOL_DLG_GAP, rtSave.bottom = rtSave.top + TOOL_DLG_BUTTON_SAVE_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_SAVE), rtSave.left, rtSave.top, 
		rtSave.right-rtSave.left, rtSave.bottom-rtSave.top, FALSE);

	//调整cancel按钮位置
	rtCancel.left = rtSave.right + TOOL_DLG_GAP, rtCancel.right = rtCancel.left + TOOL_DLG_BUTTON_CANCEL_CX;
	rtCancel.top = rtClient.top + TOOL_DLG_GAP, rtCancel.bottom = rtCancel.top + TOOL_DLG_BUTTON_CANCEL_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_CANCEL), rtCancel.left, rtCancel.top, 
		rtCancel.right-rtCancel.left, rtCancel.bottom-rtCancel.top, FALSE);

	//调整ok按钮位置
	rtOk.left = rtCancel.right + TOOL_DLG_GAP, rtOk.right = rtOk.left + TOOL_DLG_BUTTON_OK_CX;
	rtOk.top = rtClient.top + TOOL_DLG_GAP, rtOk.bottom = rtOk.top + TOOL_DLG_BUTTON_OK_CY;
	::MoveWindow(::GetDlgItem(m_hDlg, IDC_BUTTON_OK), rtOk.left, rtOk.top, 
		rtOk.right-rtOk.left, rtOk.bottom-rtOk.top, FALSE);

	//给按钮添加背景图片	
	m_btRect.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_RECT_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_RECT_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_RECT_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btEllipse.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_ELLIPSE_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_ELLIPSE_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_ELLIPSE_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btArrow.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_ARROW_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_ARROW_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_ARROW_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btBrush.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_BRUSH_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_BRUSH_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_BRUSH_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btWord.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_WORD_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_WORD_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_WORD_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btBack.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_BACK_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_BACK_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_BACK_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btSave.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_SAVE_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SAVE_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_SAVE_DOWN, (HINSTANCE)g_hModule, "PNG")
		);

	m_btCancel.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_CANCEL_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_CANCEL_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_CANCEL_DOWN, (HINSTANCE)g_hModule, "PNG")
		);	
	
	m_btOk.SetPNGBitmaps(LoadPNGFromRes(IDR_PNG_OK_NORMAL, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_OK_HOVER, (HINSTANCE)g_hModule, "PNG"), 
		LoadPNGFromRes(IDR_PNG_OK_DOWN, (HINSTANCE)g_hModule, "PNG")
		);
	

	//设置按钮样式
	m_btRect.SetButtonStyle(BS_RADIO);
	m_btEllipse.SetButtonStyle(BS_RADIO);
	m_btArrow.SetButtonStyle(BS_RADIO);
	m_btBrush.SetButtonStyle(BS_RADIO);
	m_btWord.SetButtonStyle(BS_RADIO);
	m_btBack.SetButtonStyle(BS_BUTTON);
	m_btSave.SetButtonStyle(BS_BUTTON);
	m_btCancel.SetButtonStyle(BS_BUTTON);
	m_btOk.SetButtonStyle(BS_BUTTON);

	InitToolData();

	return FALSE;
}

BOOL CToolDialog::DialogOnCommand(WPARAM wParam, LPARAM lParam)
{
	int ID = LOWORD(wParam);
	if(ID == IDCANCEL)
	{
		::EndDialog(m_hDlg, 0);
	}
	if(m_nSelDrawButtonID == IDC_BUTTON_WORD && m_nSelDrawButtonID != ID)
	{
		m_bSwitchTextButton = true;
		if(g_scDialog.m_edWord.m_bEdit)
			::SendMessage(::GetParent(m_hDlg), WM_LBUTTONUP, 0, 0);
	}
	else
	{
		m_bSwitchTextButton = false;		
	}
	switch(ID)
	{
	case IDC_BUTTON_RECT:		
	case IDC_BUTTON_ELLIPSE:		
	case IDC_BUTTON_ARROW:
	case IDC_BUTTON_BRUSH:
	case IDC_BUTTON_WORD:
		{		
			m_bEdit = true;			
			m_nSelDrawButtonID = ID;
			UpdateButtonState();
			m_CustomDlg.ShowCustomDlg(true, m_nSelDrawButtonID);
		}
		break;
	case IDC_BUTTON_BACK:		
	case IDC_BUTTON_SAVE:
	case IDC_BUTTON_CANCEL:
	case IDC_BUTTON_OK:
		{
			m_nSelCtrlButtonID = ID;
			if(m_bSwitchTextButton)
				::SetTimer(m_hDlg, TIMER_ID_PROCESS_DRAW, 200, NULL);
			else
				::PostMessage(::GetParent(m_hDlg), WM_MSG_FROM_TOOL, ID, 0);
		}		
		break;
	}
	return FALSE;
}

BOOL CToolDialog::DialogOnTimer(WPARAM wParam, LPARAM lParam)
{
	if(wParam == TIMER_ID_PROCESS_DRAW)
	{
		::KillTimer(m_hDlg, TIMER_ID_PROCESS_DRAW);
		::PostMessage(::GetParent(m_hDlg), WM_MSG_FROM_TOOL, m_nSelCtrlButtonID, 0);
	}
	return FALSE;
}

BOOL CToolDialog::DialogOnPaint(WPARAM wParam, LPARAM lParam)
{
	RECT rtClient;
	GetClientRect(m_hDlg, &rtClient);

	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hDlg, &ps);	

	HBRUSH hBrush = ::CreateSolidBrush(TOOL_BK_COLOR);
	HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush);

	::FillRect(hDC, &rtClient, hBrush);

	::SelectObject(hDC, hOldBrush);
	::DeleteObject(hBrush);
	::EndPaint(m_hDlg, &ps);
	return FALSE;
}

BOOL CToolDialog::DialogOnEraseBkgnd(WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

BOOL CToolDialog::DialogOnDestroy(WPARAM wParam, LPARAM lParam)
{
	m_hDlg = NULL;
	return FALSE;
}

void CToolDialog::InitToolData()
{
	m_bEdit = false;
	m_nSelDrawButtonID = 0;
	m_nSelCtrlButtonID = 0;
	m_bSwitchTextButton = false;
}

void CToolDialog::UpdateButtonState()
{
	m_btRect.SetButtonState(BS_NORMAL);
	m_btEllipse.SetButtonState(BS_NORMAL);
	m_btArrow.SetButtonState(BS_NORMAL);
	m_btBrush.SetButtonState(BS_NORMAL);
	m_btWord.SetButtonState(BS_NORMAL);
	if(m_nSelDrawButtonID == IDC_BUTTON_RECT)
	{
		m_btRect.SetButtonState(BS_DOWN);	
	}
	if(m_nSelDrawButtonID == IDC_BUTTON_ELLIPSE)
	{
		m_btEllipse.SetButtonState(BS_DOWN);	
	}
	if(m_nSelDrawButtonID == IDC_BUTTON_ARROW)
	{
		m_btArrow.SetButtonState(BS_DOWN);	
	}
	if(m_nSelDrawButtonID == IDC_BUTTON_BRUSH)
	{
		m_btBrush.SetButtonState(BS_DOWN);	
	}
	if(m_nSelDrawButtonID == IDC_BUTTON_WORD)
	{
		m_btWord.SetButtonState(BS_DOWN);	
	}
}

CSCDialog::CSCDialog()
{
	m_hDlg = NULL;
	m_hBitmap = NULL;
	m_hGrayBitmap = NULL;		
	m_bStartPrintScr = false;
	m_bCapWnd = false;	
	m_bShowTip = false;
	m_bStartEdit = false;
	m_bSelBack = false;

	GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);	
}

CSCDialog::~CSCDialog()
{
	Gdiplus::GdiplusShutdown(m_gdiplusToken); 
}

void CSCDialog::InitCaptureData()
{
	m_bStartPrintScr = true;
	m_bCapWnd = true;	
	m_bShowTip = true;
	m_bStartEdit = false;
	m_bSelBack = false;

	m_ptCapStart.x = m_ptCapStart.y = 0;
	m_ptScrawStart.x = m_ptScrawStart.y = 0;
	m_ptScrawEnd.x = m_ptScrawEnd.y = 0;

	memset(&m_rtSel, 0, sizeof(RECT));
	memset(&m_rtTip, 0, sizeof(RECT));

	memset(&m_rtScreenShow, 0, sizeof(RECT));	
	memset(&m_rtCutStart, 0, sizeof(RECT));

	memset(&m_rtScrawl, 0, sizeof(RECT));
	memset(&m_rtWord, 0, sizeof(RECT));
	
	if(m_hCursor)
		::DestroyCursor(m_hCursor);
	m_hCursor = ::LoadCursor((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDC_CURSOR_SC_ARROW));
	m_CaptureState = CS_NORMAL;
	m_CursorPos = CP_OUTSIDE;
	m_ptStart.x = m_ptStart.y = 0;
	m_ptEnd.x = m_ptEnd.y = 0;

	//获取屏幕分辩率
	m_nXScreen = ::GetSystemMetrics(SM_CXSCREEN);
	m_nYScreen = ::GetSystemMetrics(SM_CYSCREEN);

	m_nWordCount = 0;
	
	m_rtScreenShow.right = m_nXScreen, m_rtScreenShow.bottom = m_nYScreen;

	//在窗体展现之前进行屏幕截图
	if(m_hBitmap)
		::DeleteObject(m_hBitmap);
	m_hBitmap = PrintScreenToBitmap(0, 0, m_nXScreen, m_nYScreen);

	if(m_hGrayBitmap)
		::DeleteObject(m_hGrayBitmap);
	m_hGrayBitmap = GetGrayBitmap(m_hBitmap, RGB(0, 0, 255), 0.6);

	m_hCurScrawlBimtap = NULL;

	if(m_rtWnds.size()>0)
		m_rtWnds.clear();

	if(m_ptScrawls.size()>0)
		m_ptScrawls.clear();

	if(m_vecBitmaps.size()>0)
	{
		for(vector<HBITMAP>::const_iterator iter = m_vecBitmaps.begin(); iter != m_vecBitmaps.end(); iter++)
		{
			HBITMAP hBitmap = (*iter);
			DeleteObject(hBitmap);
		}
		m_vecBitmaps.clear();		
	}	

	//查找当前屏幕中的所有窗体
	::EnumDesktopWindows(NULL, EnumWindowsProc, (LPARAM)this);	
}

HBITMAP CSCDialog::PrintScreenToBitmap(int x, int y, int width, int height)
{
	HDC hScreenDC = ::CreateDC("DISPLAY", NULL, NULL, NULL);
	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hScreenDC, width, height);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	
	::BitBlt(hMemDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);	
	
	hBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);
	
	::DeleteDC(hMemDC);
	::DeleteDC(hScreenDC);

	return hBitmap;
}

bool CSCDialog::IsLegalForSelRgn()
{
	//边界判断
	if(m_rtSel.left <0 || m_rtSel.top <0 || m_rtSel.right >m_nXScreen || m_rtSel.bottom >m_nYScreen)
		return false;

	//大小判断
	if(m_rtSel.left>=m_rtSel.right || m_rtSel.top>=m_rtSel.bottom)
		return false;
	return true;
}

BOOL CALLBACK CSCDialog::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	static int count = 0;
	CSCDialog *pDlg = (CSCDialog*)lParam;
	if(pDlg)
	{						
		//过滤掉非窗体和有不可见样式的窗体
		if(!::IsWindow(hwnd) || !IsWindowVisible(hwnd))
			return TRUE;

		//过滤掉隐藏、最小化的窗体
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(hwnd, &wp);
		if(wp.showCmd == SW_HIDE || wp.showCmd == SW_MINIMIZE || wp.showCmd == SW_SHOWMINIMIZED)
			return TRUE;
		
		//过滤掉窗体矩形不合法的窗体
		RECT rtWindow;
		::GetWindowRect(hwnd, &rtWindow);		
		if(rtWindow.left>=rtWindow.right || rtWindow.top>=rtWindow.bottom)
			return TRUE;

		//调整窗体矩形
		if(rtWindow.left<0) rtWindow.left = 0;
		if(rtWindow.top<0) rtWindow.top = 0;
		if(rtWindow.right > pDlg->m_nXScreen) rtWindow.right = pDlg->m_nXScreen;
		if(rtWindow.bottom > pDlg->m_nYScreen) rtWindow.bottom = pDlg->m_nYScreen;

		pDlg->m_rtWnds.push_back(rtWindow);
	}
	return TRUE;
}

void CSCDialog::UpdateTipRect()
{
	POINT ptCursor;
	::GetCursorPos(&ptCursor);

	//提示框的宽度、高度
	const int widthTip = 120, heightTip = 160;

	//由于鼠标指针存在导致的附加的偏移
	const int widthCur = 8, heightCur = 32;
	
	m_rtTip.left = ptCursor.x + widthCur, m_rtTip.top = ptCursor.y + heightCur;
	m_rtTip.right = m_rtTip.left + widthTip, m_rtTip.bottom = m_rtTip.top + heightTip;
	if(m_rtTip.right > m_nXScreen)
	{
		m_rtTip.right = ptCursor.x - widthCur;
		m_rtTip.left = m_rtTip.right - widthTip;
	}
	if(m_rtTip.bottom > m_nYScreen)
	{
		m_rtTip.bottom = ptCursor.y - heightCur;
		m_rtTip.top = m_rtTip.bottom - heightTip;
	}
}

void CSCDialog::UpdateToolWndPos()
{
	RECT rtWindow;
	::GetWindowRect(m_ToolDlg.m_hDlg, &rtWindow);
	
	int right = m_rtSel.right;
	int left = right - (rtWindow.right - rtWindow.left);
	if(left<0)
		left = 0;
	int top = m_rtSel.bottom + 10;
	if(top + rtWindow.bottom - rtWindow.top > m_nYScreen)
	{
		top = m_rtSel.top - (rtWindow.bottom - rtWindow.top);
	}
	if(top<0)
		top = 0;
	
	::SetWindowPos(m_ToolDlg.m_hDlg, HWND_TOP, left, top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);	
}

void CSCDialog::DrawScrawl(HDC hDC, HBITMAP hBitmap)
{
	if(!m_ToolDlg.m_bEdit)
		return ;
	
	int cus_line_width = m_ToolDlg.m_CustomDlg.m_customInfo[m_ToolDlg.m_CustomDlg.m_nSelButtonID-IDC_BUTTON_RECT].size.line_width;
	int cus_font_size = m_ToolDlg.m_CustomDlg.m_customInfo[m_ToolDlg.m_CustomDlg.m_nSelButtonID-IDC_BUTTON_RECT].size.font_size;
	COLORREF cus_color = m_ToolDlg.m_CustomDlg.m_customInfo[m_ToolDlg.m_CustomDlg.m_nSelButtonID-IDC_BUTTON_RECT].color;

	HPEN hPen = ::CreatePen(PS_SOLID, cus_line_width, cus_color);
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);
	HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, (HBRUSH)GetStockObject(NULL_BRUSH));

	RECT rtDraw;
	rtDraw.left = m_rtScrawl.left - m_rtSel.left, rtDraw.top = m_rtScrawl.top - m_rtSel.top;
	rtDraw.right = m_rtScrawl.right - m_rtSel.left, rtDraw.bottom = m_rtScrawl.bottom - m_rtSel.top;
	if(m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_WORD || m_ToolDlg.m_bSwitchTextButton)
	{				
		m_ToolDlg.m_bSwitchTextButton = false;
		if(m_nWordCount != 0 )
		{
			if(m_nWordCount %2 == 1)
			{
				HPEN hPen = ::CreatePen(PS_DOT, 1, RGB(0, 0, 0));
				HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);
				::Rectangle(hDC, m_rtWord.left - m_rtSel.left, m_rtWord.top - m_rtSel.top, m_rtWord.right - m_rtSel.left, m_rtWord.bottom - m_rtSel.top);
				::SelectObject(hDC, hOldPen);
				::DeleteObject(hPen);				
			}
			else
			{				
				RECT rtWindow;
				::GetWindowRect(m_edWord.m_hWnd, &rtWindow);
				::BitBlt(hDC, rtWindow.left - m_rtSel.left, rtWindow.top - m_rtSel.top, rtWindow.right - rtWindow.left, 
					rtWindow.bottom - rtWindow.top, ::GetDC(m_edWord.m_hWnd), 0, 0, SRCCOPY);				
			}
		}
	}
	else if(m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_RECT)
	{
		Rectangle(hDC, rtDraw.left, rtDraw.top, rtDraw.right, rtDraw.bottom);
	}
	else if(m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_ELLIPSE)
	{
		Ellipse(hDC, rtDraw.left, rtDraw.top, rtDraw.right, rtDraw.bottom);
	}
	else if(m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_ARROW)
	{		
		CONST double PI = 3.14159;		
		POINT pt[6] = {0};
		pt[0].x = m_ptScrawStart.x - m_rtSel.left, pt[0].y = m_ptScrawStart.y - m_rtSel.top;
		pt[3].x = m_ptScrawEnd.x - m_rtSel.left, pt[3].y = m_ptScrawEnd.y - m_rtSel.top;

		if(pt[0].x != pt[3].x || pt[0].y != pt[3].y)
		{
			double length = pow((pow((pt[3].x - pt[0].x), 2) + pow((pt[3].y - pt[0].y), 2)), 0.5);		
			int width = 0, side = 80;
			if(length < side)
				width = length*cus_line_width/8;
			else
				width = side*cus_line_width/8;
			
			double alpha = acos((double)(pt[3].x - pt[0].x)/length);
			if(pt[3].y > pt[0].y)
				alpha = 2*PI - alpha;
			
			pt[2].x = pt[3].x - width*cos(PI/6 - alpha);
			pt[2].y = pt[3].y - width*sin(PI/6 - alpha);
			
			double x0 = pt[3].x - (pt[3].x - pt[0].x)*sin(PI/3)*width/length;
			double y0 = pt[3].y + (pt[0].y - pt[3].y)*sin(PI/3)*width/length;
			
			pt[1].x = (pt[2].x + x0)/2, pt[1].y = (pt[2].y + y0)/2;
			pt[4].x = 2*x0 - pt[2].x, pt[4].y = 2*y0 - pt[2].y;
			pt[5].x = 2*x0 - pt[1].x, pt[5].y = 2*y0 - pt[1].y;	
			
			HBRUSH hFillBrush = ::CreateSolidBrush(cus_color);
			HBRUSH hOldFillBrush = (HBRUSH)::SelectObject(hDC, hFillBrush);
			Polygon(hDC, pt, 6);
			::SelectObject(hDC, hOldFillBrush);
			::DeleteObject(hFillBrush);	
		}		
	}
	else if(m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_BRUSH)
	{
		vector<POINT>::const_iterator iter = m_ptScrawls.begin();
		POINT pt = (*iter);
		MoveToEx(hDC, pt.x - m_rtSel.left, pt.y - m_rtSel.top, NULL);
		for(; iter != m_ptScrawls.end(); iter++)
		{			
			pt = (*iter);
			LineTo(hDC, pt.x - m_rtSel.left, pt.y - m_rtSel.top);
		}
	}	

	::SelectObject(hDC, hOldBrush);
	::SelectObject(hDC, hOldPen);
	::DeleteObject(hPen);
}

void CSCDialog::DrawSelRgnEdge(HDC hDC)
{
	if(!IsLegalForSelRgn())
		return;

	int penWidth = 1;
	COLORREF penColor = RGB(0, 0, 255);
	if(m_bCapWnd)
	{
		penWidth = 3;
		penColor = RGB(0, 181, 255);
	}

	HPEN hPen = ::CreatePen(PS_SOLID, penWidth, penColor);
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);

	//绘制选中区域的四条边
	::MoveToEx(hDC, m_rtSel.left, m_rtSel.top, NULL), ::LineTo(hDC, m_rtSel.left, m_rtSel.bottom);
	::LineTo(hDC, m_rtSel.right, m_rtSel.bottom), ::LineTo(hDC, m_rtSel.right, m_rtSel.top);
	::LineTo(hDC, m_rtSel.left, m_rtSel.top);
	
	if(!m_bCapWnd)
	{
		//设置拉伸点的边长
		const int side = 4;
		
		HBRUSH hBrush = ::CreateSolidBrush(RGB(0, 0, 255));
		HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush);
		
		//填充4个角点
		RECT rtTL, rtTR, rtBL, rtBR;
		rtTL.left = m_rtSel.left - side/2, rtTL.right = m_rtSel.left + side/2;
		rtTL.top = m_rtSel.top - side/2, rtTL.bottom = m_rtSel.top + side/2;
		::FillRect(hDC, &rtTL, hBrush);
		
		rtTR.left = m_rtSel.right - side/2, rtTR.right = m_rtSel.right + side/2;
		rtTR.top = m_rtSel.top - side/2, rtTR.bottom = m_rtSel.top + side/2;
		::FillRect(hDC, &rtTR, hBrush);
		
		rtBL.left = m_rtSel.left - side/2, rtBL.right = m_rtSel.left + side/2;
		rtBL.top = m_rtSel.bottom - side/2, rtBL.bottom = m_rtSel.bottom + side/2;
		::FillRect(hDC, &rtBL, hBrush);
		
		rtBR.left = m_rtSel.right - side/2, rtBR.right = m_rtSel.right + side/2;
		rtBR.top = m_rtSel.bottom - side/2, rtBR.bottom = m_rtSel.bottom + side/2;
		::FillRect(hDC, &rtBR, hBrush);
		
		//填充4个边点
		RECT rtL, rtR, rtT, rtB;
		rtL.left = m_rtSel.left - side/2, rtL.right = m_rtSel.left + side/2;
		rtL.top = (m_rtSel.top+m_rtSel.bottom)/2 - side/2, rtL.bottom = (m_rtSel.top+m_rtSel.bottom)/2 + side/2;
		::FillRect(hDC, &rtL, hBrush);
		
		rtR.left = m_rtSel.right - side/2, rtR.right = m_rtSel.right + side/2;
		rtR.top = (m_rtSel.top+m_rtSel.bottom)/2 - side/2, rtR.bottom = (m_rtSel.top+m_rtSel.bottom)/2 + side/2;
		::FillRect(hDC, &rtR, hBrush);
		
		rtT.left = (m_rtSel.left+m_rtSel.right)/2 - side/2, rtT.right = (m_rtSel.left+m_rtSel.right)/2 + side/2;
		rtT.top = m_rtSel.top - side/2, rtT.bottom = m_rtSel.top + side/2;
		::FillRect(hDC, &rtT, hBrush);
		
		rtB.left = (m_rtSel.left+m_rtSel.right)/2 - side/2, rtB.right = (m_rtSel.left+m_rtSel.right)/2 + side/2;
		rtB.top = m_rtSel.bottom - side/2, rtB.bottom = m_rtSel.bottom + side/2;
		::FillRect(hDC, &rtB, hBrush);
		
		::SelectObject(hDC, hOldBrush);
		::DeleteObject(hBrush);		
	}	

	::SelectObject(hDC, hOldPen);	
	::DeleteObject(hPen);	
}

void CSCDialog::DrawTip(HDC hDC)
{
	if(!m_bShowTip)
		return;

	UpdateTipRect();

	POINT ptCursor;
	::GetCursorPos(&ptCursor);

	//填充提示框的放大区域
	RECT rtTipExt;
	rtTipExt.left = m_rtTip.left, rtTipExt.right = m_rtTip.right;
	rtTipExt.top = m_rtTip.top, rtTipExt.bottom = (m_rtTip.top + m_rtTip.bottom)/2;

	//以当前鼠标点为中心，取出32*48的矩形区域到放大区域显示
	RECT rtCap;
	rtCap.left = ptCursor.x - 12, rtCap.right = ptCursor.x + 12;
	rtCap.top = ptCursor.y - 8, rtCap.bottom = ptCursor.y + 8;

	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, m_hBitmap);

	//获取当前点的即时颜色
	COLORREF insColor = ::GetPixel(hMemDC, ptCursor.x, ptCursor.y);

	::StretchBlt(hDC, rtTipExt.left, rtTipExt.top, rtTipExt.right - rtTipExt.left, rtTipExt.bottom - rtTipExt.top, hMemDC,
		rtCap.left, rtCap.top, rtCap.right - rtCap.left, rtCap.bottom - rtCap.top, SRCCOPY);

	::SelectObject(hMemDC, hOldBitmap);
	::DeleteDC(hMemDC);

	//绘制放大区域的十字
	HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
	HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);

	::MoveToEx(hDC, rtTipExt.left, (rtTipExt.top + rtTipExt.bottom)/2, NULL);
	::LineTo(hDC, rtTipExt.right, (rtTipExt.top + rtTipExt.bottom)/2);
	::MoveToEx(hDC, (rtTipExt.left + rtTipExt.right)/2, rtTipExt.top, NULL);
	::LineTo(hDC, (rtTipExt.left + rtTipExt.right)/2, rtTipExt.bottom);	

	::SelectObject(hDC, hOldPen);
	::DeleteObject(hPen);

 	//绘制提示文字	
	RECT rtText;
	rtText.left = m_rtTip.left, rtText.right = m_rtTip.right;
	rtText.top = (m_rtTip.top + m_rtTip.bottom)/2, rtText.bottom = m_rtTip.bottom; 	
 
 	HBRUSH hBrush = ::CreateSolidBrush(RGB(0, 0, 0));
 	HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, hBrush);
 
 	::FillRect(hDC, &rtText, hBrush);
 
 	::SelectObject(hDC, hOldBrush);
 	::DeleteObject(hBrush);
	
	//字体高度
	const int heightFont = 12;

	int nOldBkMode = ::SetBkMode(hDC, TRANSPARENT);
	COLORREF oldTextColor = ::SetTextColor(hDC, RGB(255, 255, 255));
	HFONT hFont = CreateSimpleFont(0, heightFont);
	HFONT hOldFont = (HFONT)::SelectObject(hDC, hFont);

	char strSize[64] = {0}, strColor[64] = {0}, strTip1[64] = {0}, strTip2[64] = {0};
	sprintf(strSize, "%d×%d", m_rtSel.right - m_rtSel.left, m_rtSel.bottom - m_rtSel.top);
	sprintf(strColor, "RGB:(%d,%d,%d)", GetRValue(insColor), GetGValue(insColor), GetBValue(insColor));
	sprintf(strTip1, "双击快速完成截图");
	sprintf(strTip2, "点右键或按ESC退出");

	const int nGap = 3, nSep = 9;
	RECT rtSize, rtColor, rtTip1, rtTip2;
	rtSize.left = rtText.left + nGap, rtSize.right = rtText.right - nGap;
	rtSize.top = rtText.top + nGap, rtSize.bottom = rtSize.top + heightFont;

	rtColor.left = rtText.left + nGap, rtColor.right = rtText.right - nGap;
	rtColor.top = rtSize.bottom + nGap, rtColor.bottom = rtColor.top + heightFont;

	rtTip1.left = rtText.left + nGap, rtTip1.right = rtText.right - nGap;
	rtTip1.top = rtColor.bottom + nSep, rtTip1.bottom = rtTip1.top + heightFont;

	rtTip2.left = rtText.left + nGap, rtTip2.right = rtText.right - nGap;
	rtTip2.top = rtTip1.bottom + nGap, rtTip2.bottom = rtTip2.top + heightFont;

	::DrawText(hDC, strSize, strlen(strSize), &rtSize, DT_LEFT|DT_VCENTER);	
	::DrawText(hDC, strColor, strlen(strColor), &rtColor, DT_LEFT|DT_VCENTER);	
	::DrawText(hDC, strTip1, strlen(strTip1), &rtTip1, DT_LEFT|DT_VCENTER);	
	::DrawText(hDC, strTip2, strlen(strTip2), &rtTip2, DT_LEFT|DT_VCENTER);	

	::SelectObject(hDC, hOldFont);
	::DeleteObject(hFont);

	::SetTextColor(hDC, oldTextColor);
	::SetBkMode(hDC, nOldBkMode);

	//绘制提示框的边框
	::MoveToEx(hDC, m_rtTip.left, m_rtTip.top, NULL), ::LineTo(hDC, m_rtTip.left, m_rtTip.bottom);
	::LineTo(hDC, m_rtTip.right, m_rtTip.bottom), ::LineTo(hDC, m_rtTip.right, m_rtTip.top);
	::LineTo(hDC, m_rtTip.left, m_rtTip.top);
}

void CSCDialog::UpdateCaptureState()
{
	SHORT keyState = ::GetKeyState(VK_LBUTTON);
	if(keyState & 0X8000) //鼠标左键被按下
	{
		if(m_CursorPos == CP_OUTSIDE)
		{
			 //鼠标在裁剪边界以外
			m_CaptureState = CS_NORMAL;
		}
		else if(m_CursorPos == CP_INSIDE)
		{
			 //鼠标在裁剪边界以内
			m_CaptureState = CS_MOVE;
		}
		else
		{
			//鼠标在裁剪边上
			if(m_CursorPos == CP_SIDE_UP) //上边
			{
				m_CaptureState = CS_SIZE_UP;
			}
			else if(m_CursorPos == CP_SIDE_DOWN) //下边
			{
				m_CaptureState = CS_SIZE_DOWN;
			}
			else if(m_CursorPos == CP_SIDE_LEFT) //左边
			{
				m_CaptureState = CS_SIZE_LEFT;
			}
			else if(m_CursorPos == CP_SIDE_RIGHT) //右边
			{
				m_CaptureState = CS_SIZE_RIGHT;
			}	
			else if(m_CursorPos == CP_CORNER_UPLEFT) //左上角
			{
				m_CaptureState = CS_SIZE_UPLEFT;
			}
			else if(m_CursorPos == CP_CORNER_DOWNLEFT) //左下角
			{
				m_CaptureState = CS_SIZE_DOWNLEFT;
			}
			else if(m_CursorPos == CP_CORNER_UPRIGHT) //右上角
			{
				m_CaptureState = CS_SIZE_UPRIGHT;
			}
			else if(m_CursorPos == CP_CORNER_DOWNRIGHT) //右下角
			{
				m_CaptureState = CS_SIZE_DOWNRIGHT;
			}					
		}
	}
	else
	{
		m_CaptureState = CS_NORMAL;
	}
}

void CSCDialog::UpdateCursor()
{
	HCURSOR hCursor = NULL;
	if(m_ToolDlg.m_bEdit && m_CursorPos == CP_INSIDE)
	{
		hCursor = ::LoadCursor(NULL, IDC_CROSS);	
	}
	else
	{
		SHORT keyState = ::GetKeyState(VK_LBUTTON);
		if(keyState & 0X8000) //鼠标左键被按下
		{
			switch(m_CaptureState)
			{
			case CS_NORMAL:
				{				
					hCursor = ::LoadCursor((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDC_CURSOR_SC_ARROW));	
				}		
				break;
			case CS_MOVE:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZEALL);	
				}
				break;
			case CS_SIZE_UP:
			case CS_SIZE_DOWN:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZENS);	
				}
				break;
			case CS_SIZE_LEFT:
			case CS_SIZE_RIGHT:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZEWE);	
				}
				break;
			case CS_SIZE_UPLEFT:
			case CS_SIZE_DOWNRIGHT:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZENWSE);	
				}
				break;
			case CS_SIZE_UPRIGHT:
			case CS_SIZE_DOWNLEFT:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZENESW);	
				}
				break;
			}
		}
		else
		{
			switch(m_CursorPos)
			{
			case CP_OUTSIDE:
				{
					hCursor = ::LoadCursor((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDC_CURSOR_SC_ARROW));		
				}
				break;
			case CP_INSIDE:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZEALL);	
				}
				break;
			case CP_SIDE_UP:
			case CP_SIDE_DOWN:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZENS);			
				}
				break;
			case CP_SIDE_LEFT:
			case CP_SIDE_RIGHT:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZEWE);			
				}
				break;
			case CP_CORNER_UPLEFT:
			case CP_CORNER_DOWNRIGHT:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZENWSE);			
				}
				break;
			case CP_CORNER_UPRIGHT:
			case CP_CORNER_DOWNLEFT:
				{
					hCursor = ::LoadCursor(NULL, IDC_SIZENESW);			
				}
				break;
			}
		}		
	}	
	
	if(m_hCursor)
	{
		::DestroyCursor(m_hCursor);
		m_hCursor = hCursor;	
	}

	::SetCursor(m_hCursor);	
}

void CSCDialog::UpdateCursorPos(POINT point)
{
	const int gap = 5; //裁剪边界范围，内边距和外边距各为5
	if(point.x < m_rtSel.left-gap ||point.x > m_rtSel.right+gap 
		|| point.y < m_rtSel.top-gap || point.y > m_rtSel.bottom + gap)
	{
		//鼠标在裁剪边界以外
		m_CursorPos = CP_OUTSIDE;
	}
	else if(point.x > m_rtSel.left+gap && point.x< m_rtSel.right-gap 
		&& point.y > m_rtSel.top+gap && point.y < m_rtSel.bottom-gap)
	{
		//鼠标在裁剪边界以内
		m_CursorPos = CP_INSIDE;
	}
	else
	{
		//鼠标在裁剪边上
		if((point.x >= m_rtSel.left-gap && point.x <= m_rtSel.left+gap) 
			&& (point.y >= m_rtSel.top-gap && point.y <= m_rtSel.top+gap)) //左上角
		{
			m_CursorPos = CP_CORNER_UPLEFT;
		}
		else if((point.x >= m_rtSel.left-gap && point.x <= m_rtSel.left+gap) 
			&& (point.y >= m_rtSel.bottom-gap && point.y <= m_rtSel.bottom+gap)) //左下角
		{
			m_CursorPos = CP_CORNER_DOWNLEFT;
		}
		else if((point.x >= m_rtSel.right-gap && point.x <= m_rtSel.right+gap)
			&& (point.y >= m_rtSel.top-gap && point.y <= m_rtSel.top+gap)) //右上角
		{
			m_CursorPos = CP_CORNER_UPRIGHT;
		}
		else if((point.x >= m_rtSel.right-gap && point.x <= m_rtSel.right+gap)
			&& (point.y >= m_rtSel.bottom-gap && point.y <= m_rtSel.bottom+gap)) //右下角
		{
			m_CursorPos = CP_CORNER_DOWNRIGHT;
		}	
		else if(point.y >= m_rtSel.top-gap && point.y <= m_rtSel.top+gap) //上边
		{
			m_CursorPos = CP_SIDE_UP;
		}
		else if(point.y >= m_rtSel.bottom-gap && point.y <= m_rtSel.bottom+gap) //下边
		{
			m_CursorPos = CP_SIDE_DOWN;
		}
		else if(point.x >= m_rtSel.left-gap && point.x <= m_rtSel.left+gap) //左边
		{
			m_CursorPos = CP_SIDE_LEFT;
		}
		else if(point.x >= m_rtSel.right-gap && point.x <= m_rtSel.right+gap) //右边
		{
			m_CursorPos = CP_SIDE_RIGHT;
		}			
	}
}

void CSCDialog::UpdateShowRgn()
{
	HRGN hRgn = NULL;
	if(m_bStartEdit)
	{
		hRgn = ::CreateRectRgnIndirect(&m_rtSel);
	}
	else
	{
		hRgn = ::CreateRectRgnIndirect(&m_rtScreenShow);					
	}	
	if(hRgn && m_ToolDlg.m_bShow)
	{
		RECT rtWindow;
		::GetWindowRect(m_ToolDlg.m_hDlg, &rtWindow);
		HRGN hRgnTool = ::CreateRectRgnIndirect(&rtWindow);
		::CombineRgn(hRgn, hRgn, hRgnTool, RGN_DIFF);
		::DeleteObject(hRgnTool);
	}
	if(hRgn && m_ToolDlg.m_CustomDlg.m_bShow)
	{
		RECT rtWindow;
		::GetWindowRect(m_ToolDlg.m_CustomDlg.m_hDlg, &rtWindow);
		HRGN hRgnCustom = ::CreateRectRgnIndirect(&rtWindow);
		::CombineRgn(hRgn, hRgn, hRgnCustom, RGN_DIFF);
		::DeleteObject(hRgnCustom);
	}
	if(hRgn)
	{
		::InvalidateRgn(m_hDlg, hRgn, FALSE);
		::DeleteObject(hRgn);	
	}	
}

HBITMAP CSCDialog::GetGrayBitmap(HBITMAP hBitmap, COLORREF colorGray, double alpha)
{	
	BITMAP bitmapInfo;
	::GetObject(hBitmap, sizeof(BITMAP), &bitmapInfo);

	HDC hDC = ::GetDC(NULL);

	HDC hMemDC1 = ::CreateCompatibleDC(hDC);	
	HBITMAP hOldBitmap1 = (HBITMAP)::SelectObject(hMemDC1, hBitmap);

	HDC hMemDC2 = ::CreateCompatibleDC(hDC);	
	HBITMAP hGrayBitmap = ::CreateCompatibleBitmap(hDC, bitmapInfo.bmWidth, bitmapInfo.bmHeight);
	HBITMAP hOldBitmap2 = (HBITMAP)::SelectObject(hMemDC2, hGrayBitmap);
	
	::BitBlt(hMemDC2, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight, hMemDC1, 0, 0, SRCCOPY);

	/**********************************************************************/
	//直接对内存数据进行处理比GetPixel、SetPixel效率高
	BITMAPINFO BmpInfo;
	ZeroMemory(&BmpInfo, sizeof(BmpInfo));
	BmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BmpInfo.bmiHeader.biWidth = bitmapInfo.bmWidth;
	BmpInfo.bmiHeader.biHeight = bitmapInfo.bmHeight;
	BmpInfo.bmiHeader.biPlanes = 1;
	BmpInfo.bmiHeader.biBitCount = 32;
	BmpInfo.bmiHeader.biCompression = BI_RGB;	
	
	int BmpSize = BmpInfo.bmiHeader.biWidth*BmpInfo.bmiHeader.biHeight*4;	
	
	PVOID pBmpData;
	pBmpData = ::HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, BmpSize);
	ZeroMemory(pBmpData, BmpSize);	
	
	::GetDIBits(hMemDC2, hGrayBitmap, 0, BmpInfo.bmiHeader.biHeight, pBmpData, &BmpInfo, DIB_RGB_COLORS);

	unsigned char* pData = (unsigned char*)pBmpData;
	for(int i = 0; i< BmpSize/4; i++)
	{
		pData[i*4+1] = pData[i*4+1]*alpha + GetRValue(colorGray)*(1-alpha);
		pData[i*4+2] = pData[i*4+2]*alpha + GetGValue(colorGray)*(1-alpha);
		pData[i*4+3] = pData[i*4+3]*alpha + GetBValue(colorGray)*(1-alpha);
	}

	::SetDIBits(hMemDC2, hGrayBitmap, 0, BmpInfo.bmiHeader.biHeight, pBmpData, &BmpInfo, DIB_RGB_COLORS);

	::HeapFree(GetProcessHeap(), 0, pBmpData);
	/**********************************************************************/	

	::SelectObject(hMemDC1, hOldBitmap1);
	::DeleteDC(hMemDC1);

	::SelectObject(hMemDC2, hOldBitmap2);
	::DeleteDC(hMemDC2);

	return hGrayBitmap;
}

HBITMAP CSCDialog::CopyBitmap(HBITMAP hSrcBitmap, HBITMAP hDstBitmap, const RECT *pRTSrc, const RECT *pRTDst)
{
	if(!hSrcBitmap)
		return NULL;	

	BITMAP bitmapInfoSrc;
	::GetObject(hSrcBitmap, sizeof(BITMAP), &bitmapInfoSrc);

	RECT rtSrc, rtDst;
	if(pRTSrc)
	{
		memcpy(&rtSrc, pRTSrc, sizeof(RECT));		
	}
	else
	{
		rtSrc.left = rtSrc.top = 0;
		rtSrc.right = bitmapInfoSrc.bmWidth;
		rtSrc.bottom = bitmapInfoSrc.bmHeight;
	}
	if(pRTDst)
	{
		memcpy(&rtDst, pRTDst, sizeof(RECT));
	}
	else
	{
		rtDst.left = rtDst.top = 0;
		rtDst.right = bitmapInfoSrc.bmWidth;
		rtDst.bottom = bitmapInfoSrc.bmHeight;		
	}
	
	HDC hDC = ::GetDC(NULL);	
	HDC hSrcMemDC = ::CreateCompatibleDC(hDC);	
	HBITMAP hOldSrcBitmap = (HBITMAP)::SelectObject(hSrcMemDC, hSrcBitmap);
	HDC hDstMemDC = ::CreateCompatibleDC(hDC);	
	if(!hDstBitmap)
	{
		hDstBitmap = ::CreateCompatibleBitmap(hDC, rtDst.right - rtDst.left, rtDst.bottom - rtDst.top);
	}
	HBITMAP hOldDstBitmap = (HBITMAP)::SelectObject(hDstMemDC, hDstBitmap);	
	
	StretchBlt(hDstMemDC, rtDst.left, rtDst.top, rtDst.right - rtDst.left, rtDst.bottom - rtDst.top, 
		hSrcMemDC, rtSrc.left, rtSrc.top, rtSrc.right - rtSrc.left, rtSrc.bottom - rtSrc.top, SRCCOPY);
	
	::SelectObject(hSrcMemDC, hOldSrcBitmap);
	::SelectObject(hDstMemDC, hOldDstBitmap);
	
	::DeleteDC(hSrcMemDC);
	::DeleteDC(hDstMemDC);
	
	return hDstBitmap;	
}

UINT CALLBACK OFNHookProc(
						  HWND hdlg,      // handle to child dialog window
						  UINT uiMsg,     // message identifier
						  WPARAM wParam,  // message parameter
						  LPARAM lParam   // message parameter
						  )
{
	switch(uiMsg)
	{
	case WM_NOTIFY:
		{
			LPOFNOTIFY lpon = (LPOFNOTIFY) lParam;
			if(lpon->hdr.code == CDN_TYPECHANGE)
			{
				char filetitle[MAX_PATH] = {0};
				_splitpath(lpon->lpOFN->lpstrFile, NULL, NULL, filetitle, NULL);
				switch(lpon->lpOFN->nFilterIndex)
				{
				case 1:
					{												
						strcat(filetitle, ".bmp");											
					}
					break;
				case 2:
					{						
						strcat(filetitle, ".jpg");					
					}
					break;
				case 3:
					{						
						strcat(filetitle, ".png");					
					}
					break;
				}
				::SendMessage(::GetParent(hdlg), CDM_SETCONTROLTEXT, 0X480, (LPARAM)filetitle);	
			}		
		}
		break;
	}
	return 0;
}

void CSCDialog::BackEditBmp()
{
	if(m_vecBitmaps.size() >1)
	{
		vector<HBITMAP>::iterator iter = m_vecBitmaps.end();
		vector<HBITMAP>::iterator iterLast = --iter;
		vector<HBITMAP>::iterator iterPreLast = --iter;
		HBITMAP hLastBimtap = (*iterLast);
		if(hLastBimtap)
		{
			DeleteObject(hLastBimtap);
		}
		m_vecBitmaps.erase(iterLast);
				
		m_hCurScrawlBimtap = (*iterPreLast);
		m_bSelBack = true;
		InvalidateRect(m_hDlg, &m_rtSel, FALSE);	
	}	
}

void CSCDialog::SaveAsCaptureBmp()
{
	char filename[MAX_PATH] = "未命名.bmp"; //打开文件对话框用户所选文件的文件名
	LPCTSTR filter = "BMP (*.bmp)\0*.bmp\0JPG (*jpg;*.jpeg)\0*.jpg;*.jpeg\0PNG (*.png)\0*.png\0\0"; //打开文件对话框滤镜
	OPENFILENAME save_file;
	memset(&save_file, 0, sizeof(OPENFILENAME));
	save_file.lStructSize = sizeof(OPENFILENAME);
	save_file.hwndOwner = m_hDlg;
	save_file.lpstrFile = filename; //打开文件对话框中返回的文件全名
	save_file.nMaxFile = MAX_PATH;	
	save_file.lpstrFilter = filter;
	save_file.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
	save_file.lpfnHook = OFNHookProc;
	int id = ::GetSaveFileName(&save_file); //弹出打开文件对话框，并返回选择的文件全名
	if(id == IDOK)
	{
		IMAGE_FORMAT image_format = IF_BMP;
		switch(save_file.nFilterIndex)
		{
		case 1:
			{
				if(!strstr(filename, ".bmp"))
				{
					strcat(filename, ".bmp");
				}
				image_format = IF_BMP;
			}
			break;
		case 2:
			{
				if(!strstr(filename, ".jpg") && !strstr(filename, ".jpeg"))
				{
					strcat(filename, ".jpg");
				}
				image_format = IF_JPG;
			}
			break;
		case 3:
			{
				if(!strstr(filename, ".png"))
				{
					strcat(filename, ".png");
				}
				image_format = IF_PNG;
			}
			break;
		}

		RECT rtSrc;
		rtSrc.left = rtSrc.top = 0;
		rtSrc.right = m_rtSel.right - m_rtSel.left;
		rtSrc.bottom = m_rtSel.bottom - m_rtSel.top;
		CopyBitmap(m_hCurScrawlBimtap, m_hBitmap, &rtSrc, &m_rtSel);

		bool bRet = SaveBitmapToFile(m_hBitmap, m_rtSel.left, m_rtSel.top, 
			m_rtSel.right - m_rtSel.left, m_rtSel.bottom - m_rtSel.top, filename, false);

		if(bRet)
		{
			ConvertImageFomat(filename, image_format);

			strcpy(g_pCaptureData->filename, filename);
			g_pCaptureData->capture_oper = CO_SAVEAS;
			
			::EndDialog(m_hDlg, 0);	
		}		
	}
}

void CSCDialog::CancelScreenCapture()
{
	g_pCaptureData->capture_oper = CO_CANCEL;	
	::EndDialog(m_hDlg, 0);
}

void CSCDialog::EnsureScreenCapture()
{
	SYSTEMTIME st;
	::GetLocalTime(&st);
	char title[MAX_PATH] = {0};	
	sprintf(title, "\\%d-%d-%d-%d-%d-%d.jpg", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	strcpy(g_pCaptureData->filename, g_pCaptureData->save_dir), strcat(g_pCaptureData->filename, title);

	RECT rtSrc;
	rtSrc.left = rtSrc.top = 0;
	rtSrc.right = m_rtSel.right - m_rtSel.left;
	rtSrc.bottom = m_rtSel.bottom - m_rtSel.top;
	CopyBitmap(m_hCurScrawlBimtap, m_hBitmap, &rtSrc, &m_rtSel);

	bool bRet = SaveBitmapToFile(m_hBitmap, m_rtSel.left, m_rtSel.top, 
			m_rtSel.right - m_rtSel.left, m_rtSel.bottom - m_rtSel.top, g_pCaptureData->filename, true);	

	if(bRet)
	{
		//图片类型转换bmp->jpg
		ConvertImageFomat(g_pCaptureData->filename, IF_JPG);

		g_pCaptureData->capture_oper = CO_SURE;
		::EndDialog(m_hDlg, 0);	
	}	
}

void CSCDialog::ShowScreenCaptureDlg()
{
	::DialogBox((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDD_DIALOG_SCREEN_CAPTURE), NULL, DialogProc);
}

BOOL CALLBACK CSCDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch(uMsg)
	{
	case WM_SETCURSOR:
		{
			::SetCursor(g_scDialog.m_hCursor);
			return TRUE;
		}
	case WM_CTLCOLOREDIT:
		{
			HDC hDC = (HDC)wParam;
			HWND hWnd = (HWND)lParam;
			if(hWnd == GetDlgItem(hwndDlg, IDC_EDIT_WORD))
			{
				COLORREF color = g_scDialog.m_ToolDlg.m_CustomDlg.m_customInfo[g_scDialog.m_ToolDlg.m_CustomDlg.m_nSelButtonID-IDC_BUTTON_RECT].color;
				HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
				::SetBkMode(hDC, TRANSPARENT);
				::SetTextColor(hDC, color);
				::SelectObject(hDC, hBrush);
				return (BOOL)hBrush;			
			}
			else
			{
				return FALSE;
			}
		}
		break;
	case WM_INITDIALOG:
		return g_scDialog.DialogOnInit(hwndDlg, wParam, lParam);		
	case WM_COMMAND:
		return g_scDialog.DialogOnCommand(wParam, lParam);		
	case WM_PAINT:
		return g_scDialog.DialogOnPaint(wParam, lParam);		
	case WM_ERASEBKGND:
		return g_scDialog.DialogOnEraseBkgnd(wParam, lParam);	
	case WM_LBUTTONDOWN:
		return g_scDialog.DialogOnLButtonDown(wParam, lParam);
	case WM_LBUTTONDBLCLK:
		return g_scDialog.DialogOnLButtonDbClick(wParam, lParam);
	case WM_LBUTTONUP:
		return g_scDialog.DialogOnLButtonUp(wParam, lParam);	
	case WM_RBUTTONUP:
		return g_scDialog.DialogOnRButtonUp(wParam, lParam);	
	case WM_MOUSEMOVE:
		return g_scDialog.DialogOnMouseMove(wParam, lParam);	
	case WM_DESTROY:
		return g_scDialog.DialogOnDestroy(wParam, lParam);
	case WM_MSG_FROM_TOOL:
		return g_scDialog.OnMsgFromTool(wParam, lParam);
	case WM_MSG_FROM_EDIT:
		return g_scDialog.OnMsgFromEdit(wParam, lParam);
	case WM_MSG_FROM_CUSTOM:
		return g_scDialog.OnMsgFromCustom(wParam, lParam);
	}
	return FALSE; //对于对话框处理程序，在处理完消息之后，应该返回FALSE，让系统进一步处理
}

BOOL CSCDialog::DialogOnInit(HWND hWnd, WPARAM wParam, LPARAM lParam)
{	
	m_hDlg = hWnd; //绑定主对话框句柄

	InitCaptureData();	

	::SetWindowPos(m_hDlg, HWND_TOPMOST, 0, 0, m_nXScreen, m_nYScreen, SWP_SHOWWINDOW);	

	m_ToolDlg.ShowToolDlg(false);

	m_edWord.CreateEdit(IDC_EDIT_WORD, this->m_hDlg, ES_MULTILINE | ES_WANTRETURN);
	::SetWindowPos(m_edWord.m_hWnd, NULL, 0, 0, WORD_WIDTH, WORD_HEIGHT, SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOMOVE);	
	m_edWord.m_bEdit = FALSE;
	
	return FALSE;
}

BOOL CSCDialog::DialogOnCommand(WPARAM wParam, LPARAM lParam)
{
	int ID = LOWORD(wParam);
	int code = HIWORD(wParam);
	if(ID == IDCANCEL)
	{
		::EndDialog(m_hDlg, 0);
	}
	if(ID == IDC_EDIT_WORD)
	{		
		if(code == EN_CHANGE)
		{	
			char strText[64*1024] = {0};
			::GetWindowText(m_edWord.m_hWnd, strText, sizeof(strText));
			int nLine = 1;
			char *p = strstr(strText, "\r\n");
			int maxChNum = 0;
			static int lastMaxChNum = 0;
			if(p)
				maxChNum = p - strText;
			else
				maxChNum = strlen(strText);
			while(p)
			{				
				nLine++;
				char *p1 = strstr(++p, "\r\n");
				if(p1)
				{
					int col = p1 - p - 1;
					if(col >maxChNum)
					{
						maxChNum = col;
					}
				}					
				else
				{
					int col = strlen(strText) + strText - p -1;
					if(col > maxChNum)
					{
						maxChNum = col;
					}
				}					
				p = p1;
			}	
			if(maxChNum > lastMaxChNum)
			{
				m_rtWord.right += 12;
			}
			lastMaxChNum = maxChNum;

			if(m_edWord.m_chLastCode == '\r' || m_edWord.m_chLastCode == '\n')
			{
				m_rtWord.bottom += 20;
			}
								
			if(m_rtWord.right > m_rtSel.right)
			{
				m_rtWord.right = m_rtSel.right;				
			}
			if(m_rtWord.bottom > m_rtSel.bottom)
			{
				m_rtWord.bottom = m_rtSel.bottom;				
			}
			int wWord = m_rtWord.right - m_rtWord.left - 2*WORD_GAP;
			int hWord = m_rtWord.bottom - m_rtWord.top - 2*WORD_GAP;
			::SetWindowPos(m_edWord.m_hWnd, NULL, 0, 0, wWord, hWord, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE);			
			::InvalidateRect(m_hDlg, &m_rtWord, FALSE);
			m_edWord.m_bEdit = TRUE;
		}
		else if(code == EN_MAXTEXT) 
		{
			m_rtWord.right += 12;

			if(m_rtWord.right > m_rtSel.right)
				m_rtWord.right = m_rtSel.right;

			if(m_rtWord.right == m_rtSel.right)
			{
				m_rtWord.bottom += 20;
				if(m_rtWord.bottom > m_rtSel.bottom)
				{
					m_rtWord.bottom = m_rtSel.bottom;					
				}					
			}
			int wWord = m_rtWord.right - m_rtWord.left - 2*WORD_GAP;
			int hWord = m_rtWord.bottom - m_rtWord.top - 2*WORD_GAP;
			::SetWindowPos(m_edWord.m_hWnd, NULL, 0, 0, wWord, hWord, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE);
			::InvalidateRect(m_hDlg, &m_rtWord, FALSE);
			m_edWord.m_bEdit = TRUE;
		}
	}
	return FALSE;
}

BOOL CSCDialog::DialogOnPaint(WPARAM wParam, LPARAM lParam)
{
	RECT rtClient;
	::GetClientRect(m_hDlg, &rtClient);

	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hDlg, &ps);	
		
	//获取屏幕位图的信息
	BITMAP bitmapInfo;
	::GetObject(m_hBitmap, sizeof(BITMAP), &bitmapInfo);

	//创建屏幕一级缓冲DC
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, bitmapInfo.bmWidth, bitmapInfo.bmHeight);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);

	//绘制灰化效果
	HDC hMemDC1 = ::CreateCompatibleDC(hDC);
	HBITMAP hOldBitmap1 = (HBITMAP)::SelectObject(hMemDC1, m_hGrayBitmap);	
	::BitBlt(hMemDC, 0, 0, rtClient.right - rtClient.left, rtClient.bottom - rtClient.top, hMemDC1, 0, 0, SRCCOPY);
	::SelectObject(hMemDC1, hOldBitmap1);
	::DeleteDC(hMemDC1);
	
	//绘制选中区域
	HDC hMemDC2 = ::CreateCompatibleDC(hDC);
	HBITMAP hOldBitmap2 = (HBITMAP)::SelectObject(hMemDC2, m_hBitmap);

	::BitBlt(hMemDC, m_rtSel.left, m_rtSel.top, m_rtSel.right - m_rtSel.left, m_rtSel.bottom - m_rtSel.top,
		hMemDC2, m_rtSel.left, m_rtSel.top, SRCCOPY);	

	::SelectObject(hMemDC2, hOldBitmap2);
	DeleteDC(hMemDC2);

	//涂鸦绘制
	if(m_ToolDlg.m_bEdit)
	{
		HDC hMemDC3 = ::CreateCompatibleDC(hDC);	
		if(m_bSelBack)
		{
			m_bSelBack = false;

			HBITMAP hOldBitmap3 = (HBITMAP)::SelectObject(hMemDC3, m_hCurScrawlBimtap);					
			
			::BitBlt(hMemDC, m_rtSel.left, m_rtSel.top, m_rtSel.right - m_rtSel.left, m_rtSel.bottom - m_rtSel.top,
				hMemDC3, 0, 0, SRCCOPY);
			
			::SelectObject(hMemDC3, hOldBitmap3);			
		}	
		else
		{
			HBITMAP hTempBitmap = CopyBitmap(m_hCurScrawlBimtap);
			HBITMAP hOldBitmap3 = (HBITMAP)::SelectObject(hMemDC3, hTempBitmap);
			
			//涂鸦绘制
			DrawScrawl(hMemDC3, hTempBitmap);
			
			::BitBlt(hMemDC, m_rtSel.left, m_rtSel.top, m_rtSel.right - m_rtSel.left, m_rtSel.bottom - m_rtSel.top,
				hMemDC3, 0, 0, SRCCOPY);
			
			::SelectObject(hMemDC3, hOldBitmap3);
			
			if(!m_bStartEdit)
			{		
				m_vecBitmaps.push_back(hTempBitmap);
				m_hCurScrawlBimtap = hTempBitmap;
			}
			else
			{
				::DeleteObject(hTempBitmap);	
			}			
		}
		::DeleteDC(hMemDC3);		
	}
	
	//绘制选中区域的边界
	DrawSelRgnEdge(hMemDC);

	//绘制提示
	DrawTip(hMemDC);

	::BitBlt(hDC, 0, 0, rtClient.right - rtClient.left, rtClient.bottom - rtClient.top, hMemDC, 0, 0, SRCCOPY);

	::SelectObject(hMemDC, hOldBitmap);
	::DeleteObject(hBitmap);
	::DeleteDC(hMemDC);	

	::EndPaint(m_hDlg, &ps);

	return TRUE;	
}

BOOL CSCDialog::DialogOnEraseBkgnd(WPARAM wParam, LPARAM lParam)
{	
	return TRUE;
}

BOOL CSCDialog::DialogOnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	DWORD fwKeys = wParam;
	WORD xPos = LOWORD(lParam); 
	WORD yPos = HIWORD(lParam);  

	if(m_ToolDlg.m_bEdit)
	{
		m_ptScrawStart.x = xPos;
		m_ptScrawStart.y = yPos;	
			
		if(m_nWordCount%2 == 0 && m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_WORD)
		{						
			m_rtWord.left = xPos - WORD_GAP, m_rtWord.right = m_rtWord.left + WORD_WIDTH + 2*WORD_GAP;
			m_rtWord.top = yPos - WORD_GAP, m_rtWord.bottom = m_rtWord.top +  WORD_HEIGHT + 2*WORD_GAP;;		
			::SetWindowPos(m_edWord.m_hWnd, NULL, xPos, yPos, WORD_WIDTH, WORD_HEIGHT, SWP_HIDEWINDOW | SWP_NOZORDER);				
			::SetWindowText(m_edWord.m_hWnd, "");
			m_edWord.m_bEdit = FALSE;
		}		

		if(m_ptScrawls.size()>0)
			m_ptScrawls.clear();

		m_bStartEdit = true;

		RECT rtTemp;
		rtTemp.left = rtTemp.top = 0;
		rtTemp.right = m_rtSel.right - m_rtSel.left;
		rtTemp.bottom = m_rtSel.bottom - m_rtSel.top;
		if(!m_hCurScrawlBimtap)
		{
			HBITMAP hFirstBitmap = CopyBitmap(m_hBitmap, NULL, &m_rtSel, &rtTemp);
			m_vecBitmaps.push_back(hFirstBitmap);
			m_hCurScrawlBimtap = hFirstBitmap;	
		}		
	}
	else
	{
		if(m_bStartPrintScr)
		{
			m_ptCapStart.x = xPos;
			m_ptCapStart.y = yPos;	
			m_bCapWnd = false;
		}	
		else
		{
			POINT point;
			point.x = xPos, point.y = yPos;
			
			UpdateCursorPos(point);
			UpdateCaptureState();
			UpdateCursor();	
			
			if(m_CursorPos != CP_OUTSIDE)
			{
				m_ptStart.x = point.x, m_ptStart.y = point.y; //记录鼠标开始按下的位置
				memcpy(&m_rtCutStart, &m_rtSel, sizeof(RECT)); //记录鼠标按下时的裁剪区域
			}	
		}	
	}	
	return FALSE;
}

BOOL CSCDialog::DialogOnLButtonDbClick(WPARAM wParam, LPARAM lParam)
{
	EnsureScreenCapture();
	return FALSE;
}

BOOL CSCDialog::DialogOnLButtonUp(WPARAM wParam, LPARAM lParam)
{
	DWORD fwKeys = wParam;
	WORD xPos = LOWORD(lParam); 
	WORD yPos = HIWORD(lParam);  
	
	if(m_ToolDlg.m_bEdit)
	{
		m_ptScrawEnd.x = xPos;
		m_ptScrawEnd.y = yPos;
		
		if(m_ToolDlg.m_nSelDrawButtonID == IDC_BUTTON_WORD)
		{			
			if(++m_nWordCount%2 == 0)
			{				
				m_bStartEdit = false;
				::HideCaret(m_edWord.m_hWnd);
				::ShowWindow(m_edWord.m_hWnd, SW_HIDE);		
				m_edWord.m_bEdit = FALSE;
			}
			else
			{			
				::ShowCaret(m_edWord.m_hWnd);
				::SetFocus(m_edWord.m_hWnd);
				::ShowWindow(m_edWord.m_hWnd, SW_SHOW);		
				m_edWord.m_bEdit = TRUE;
			}
		}	
		else
		{
			m_bStartEdit = false;			
		}		
	}

	if(!IsLegalForSelRgn())
	{
		m_bCapWnd = true;
		return FALSE;
	}		

	if(m_bStartPrintScr)
	{
		m_bStartPrintScr = false;	
	}	
	else
	{
		POINT point;
		point.x = xPos, point.y = yPos;
		if(m_CaptureState != CS_NORMAL)
		{								
			m_ptEnd.x = point.x, m_ptEnd.y = point.y; //记录鼠标开始抬起的位置
		}
		
		UpdateCursorPos(point);
		UpdateCaptureState();
		UpdateCursor();	
	}
	m_ToolDlg.ShowToolDlg();
	UpdateToolWndPos();	
	m_bShowTip = false;
	UpdateShowRgn();	
	return FALSE;
}

BOOL CSCDialog::DialogOnRButtonUp(WPARAM wParam, LPARAM lParam)
{
	CancelScreenCapture();
	return FALSE;
}

BOOL CSCDialog::DialogOnMouseMove(WPARAM wParam, LPARAM lParam)
{
	DWORD fwKeys = wParam;
	WORD xPos = LOWORD(lParam); 
	WORD yPos = HIWORD(lParam);  
	POINT point;
	point.x = xPos, point.y = yPos;	

	if(m_bStartEdit)
	{		
		if(xPos > m_ptScrawStart.x)
		{
			m_rtScrawl.left = m_ptScrawStart.x;
			m_rtScrawl.right = xPos;
		}
		else
		{
			m_rtScrawl.left = xPos;
			m_rtScrawl.right = m_ptScrawStart.x;
		}
		
		if(yPos > m_ptScrawStart.y)
		{
			m_rtScrawl.top = m_ptScrawStart.y;
			m_rtScrawl.bottom = yPos;
		}
		else
		{
			m_rtScrawl.top = yPos;
			m_rtScrawl.bottom = m_ptScrawStart.y;
		}

		if(m_rtScrawl.left < m_rtSel.left)
			m_rtScrawl.left = m_rtSel.left;
		if(m_rtScrawl.top < m_rtSel.top)
			m_rtScrawl.top = m_rtSel.top;
		if(m_rtScrawl.right > m_rtSel.right)
			m_rtScrawl.right = m_rtSel.right;
		if(m_rtScrawl.bottom > m_rtSel.right)
			m_rtScrawl.right = m_rtSel.right;

		m_ptScrawEnd.x = xPos;
		m_ptScrawEnd.y = yPos;		
		
		m_ptScrawls.push_back(point);
	}
	else
	{
		if(m_bStartPrintScr)
		{
			if(fwKeys == MK_LBUTTON)
			{
				if(xPos > m_ptCapStart.x)
				{
					m_rtSel.left = m_ptCapStart.x;
					m_rtSel.right = xPos;
				}
				else
				{
					m_rtSel.left = xPos;
					m_rtSel.right = m_ptCapStart.x;
				}
				
				if(yPos > m_ptCapStart.y)
				{
					m_rtSel.top = m_ptCapStart.y;
					m_rtSel.bottom = yPos;
				}
				else
				{
					m_rtSel.top = yPos;
					m_rtSel.bottom = m_ptCapStart.y;
				}				
			}
			if(m_bCapWnd)
			{
				for(vector<RECT>::iterator iter = m_rtWnds.begin(); iter != m_rtWnds.end(); iter++)
				{
					RECT rt = (*iter);
					if(xPos>rt.left && xPos <rt.right && yPos > rt.top && yPos < rt.bottom)
					{
						memcpy(&m_rtSel, &rt, sizeof(RECT));
						break;
					}			
				}	
			}		
		}
		else
		{
			UpdateCursorPos(point);
			UpdateCursor();
			if(fwKeys == MK_LBUTTON)
			{
				int xOffset = point.x - m_ptStart.x;
				int yOffset = point.y - m_ptStart.y;
				RECT rtTemp;
				memset(&rtTemp, 0, sizeof(RECT));

				if(m_CaptureState == CS_NORMAL)
				{
					return FALSE;
				}
				else if(m_CaptureState == CS_MOVE)
				{								
					rtTemp.left = m_rtCutStart.left + xOffset, rtTemp.right = m_rtCutStart.right + xOffset;
					rtTemp.top = m_rtCutStart.top + yOffset, rtTemp.bottom = m_rtCutStart.bottom + yOffset;
					if(rtTemp.left < m_rtScreenShow.left)
					{
						int offset = m_rtScreenShow.left - rtTemp.left;
						rtTemp.left = m_rtScreenShow.left;
						rtTemp.right += offset;
					}
					if(rtTemp.right > m_rtScreenShow.right)
					{
						int offset = m_rtScreenShow.right - rtTemp.right;
						rtTemp.right = m_rtScreenShow.right;
						rtTemp.left += offset;
					}
					if(rtTemp.top < m_rtScreenShow.top)
					{
						int offset = m_rtScreenShow.top - rtTemp.top;
						rtTemp.top = m_rtScreenShow.top;
						rtTemp.bottom += offset;
					}
					if(rtTemp.bottom > m_rtScreenShow.bottom)
					{
						int offset = m_rtScreenShow.bottom - rtTemp.bottom;
						rtTemp.bottom = m_rtScreenShow.bottom;
						rtTemp.top += offset;
					}

					memcpy(&m_rtSel, &rtTemp, sizeof(RECT));		
					
					m_bShowTip = false;
					UpdateToolWndPos();			
				}	
				else
				{											
					if(m_CaptureState == CS_SIZE_UP)
					{								
						//做裁剪矩形边界调整
						rtTemp.top = m_rtCutStart.top + yOffset;
						if(rtTemp.top < m_rtScreenShow.top)
						{
							rtTemp.top = m_rtScreenShow.top;	
							yOffset = m_rtScreenShow.top - m_rtCutStart.top;
						}
						if(rtTemp.top > m_rtScreenShow.bottom)
						{
							rtTemp.top = m_rtScreenShow.bottom;
							yOffset = m_rtScreenShow.bottom - m_rtCutStart.top;
						}				
						
						rtTemp.bottom = m_rtCutStart.bottom;
						rtTemp.left = m_rtCutStart.left;
						rtTemp.right = m_rtCutStart.right;
					}
					else if(m_CaptureState == CS_SIZE_DOWN)
					{		
						//做裁剪矩形边界调整
						rtTemp.bottom = m_rtCutStart.bottom + yOffset;
						if(rtTemp.bottom > m_rtScreenShow.bottom)
						{
							rtTemp.bottom = m_rtScreenShow.bottom;	
							yOffset = m_rtScreenShow.bottom - m_rtCutStart.bottom;
						}
						if(rtTemp.bottom < m_rtScreenShow.top)
						{
							rtTemp.bottom = m_rtScreenShow.top;
							yOffset = m_rtScreenShow.top - m_rtCutStart.bottom;
						}							
			
						rtTemp.top = m_rtCutStart.top;
						rtTemp.left = m_rtCutStart.left;
						rtTemp.right = m_rtCutStart.right;
					}
					else if(m_CaptureState == CS_SIZE_LEFT)
					{			
						//做裁剪矩形边界调整
						rtTemp.left = m_rtCutStart.left + xOffset;
						if(rtTemp.left < m_rtScreenShow.left)
						{
							rtTemp.left = m_rtScreenShow.left;	
							xOffset = m_rtScreenShow.left - m_rtCutStart.left;
						}
						if(rtTemp.left > m_rtScreenShow.right)
						{
							rtTemp.left = m_rtScreenShow.right;
							xOffset = m_rtScreenShow.right - m_rtCutStart.left;
						}												

						rtTemp.right = m_rtCutStart.right;		
						rtTemp.top = m_rtCutStart.top;
						rtTemp.bottom = m_rtCutStart.bottom;
					}
					else if(m_CaptureState == CS_SIZE_RIGHT)
					{		
						//做裁剪矩形边界调整
						rtTemp.right = m_rtCutStart.right + xOffset;
						if(rtTemp.right > m_rtScreenShow.right)
						{
							rtTemp.right = m_rtScreenShow.right;	
							xOffset = m_rtScreenShow.right - m_rtCutStart.right;
						}
						if(rtTemp.right < m_rtScreenShow.left)
						{
							rtTemp.right = m_rtScreenShow.left;
							xOffset = m_rtScreenShow.left - m_rtCutStart.right;
						}											

						rtTemp.left = m_rtCutStart.left;	
						rtTemp.top = m_rtCutStart.top;
						rtTemp.bottom = m_rtCutStart.bottom;
					}
					else if(m_CaptureState == CS_SIZE_UPLEFT)
					{
						rtTemp.top = m_rtCutStart.top + yOffset;
						if(rtTemp.top < m_rtScreenShow.top)
						{
							rtTemp.top = m_rtScreenShow.top;	
							yOffset = m_rtScreenShow.top - m_rtCutStart.top;
						}
						if(rtTemp.top > m_rtScreenShow.bottom)
						{
							rtTemp.top = m_rtScreenShow.bottom;	
							yOffset = m_rtScreenShow.bottom - m_rtCutStart.top;
						}
						rtTemp.bottom = m_rtCutStart.bottom;

						rtTemp.left = m_rtCutStart.left + xOffset;
						if(rtTemp.left < m_rtScreenShow.left)
						{
							rtTemp.left = m_rtScreenShow.left;	
							xOffset = m_rtScreenShow.left - m_rtCutStart.left;
						}
						if(rtTemp.left > m_rtScreenShow.right)
						{
							rtTemp.left = m_rtScreenShow.right;	
							xOffset = m_rtScreenShow.right - m_rtCutStart.left;
						}					
						rtTemp.right = m_rtCutStart.right;	
					}
					else if(m_CaptureState == CS_SIZE_DOWNLEFT)
					{
						rtTemp.top = m_rtCutStart.top;
						rtTemp.bottom = m_rtCutStart.bottom + yOffset;
						if(rtTemp.bottom > m_rtScreenShow.bottom)
						{
							rtTemp.bottom = m_rtScreenShow.bottom;	
							yOffset = m_rtScreenShow.bottom - m_rtCutStart.bottom;
						}
						if(rtTemp.bottom < m_rtScreenShow.top)
						{
							rtTemp.bottom = m_rtScreenShow.top;	
							yOffset = m_rtScreenShow.top - m_rtCutStart.bottom;
						}

						rtTemp.left = m_rtCutStart.left + xOffset;
						if(rtTemp.left < m_rtScreenShow.left)
						{
							rtTemp.left = m_rtScreenShow.left;	
							xOffset = m_rtScreenShow.left - m_rtCutStart.left;
						}
						if(rtTemp.left > m_rtScreenShow.right)
						{
							rtTemp.left = m_rtScreenShow.right;	
							xOffset = m_rtScreenShow.right - m_rtCutStart.left;
						}
						rtTemp.right = m_rtCutStart.right;				
					}
					else if(m_CaptureState == CS_SIZE_UPRIGHT)
					{
						rtTemp.top = m_rtCutStart.top + yOffset;
						if(rtTemp.top < m_rtScreenShow.top)
						{
							rtTemp.top = m_rtScreenShow.top;	
							yOffset = m_rtScreenShow.top - m_rtCutStart.top;
						}
						if(rtTemp.top > m_rtScreenShow.bottom)
						{
							rtTemp.top = m_rtScreenShow.bottom;	
							yOffset = m_rtScreenShow.bottom - m_rtCutStart.top;
						}
						rtTemp.bottom = m_rtCutStart.bottom;

						rtTemp.left = m_rtCutStart.left;
						rtTemp.right = m_rtCutStart.right + xOffset;	
						if(rtTemp.right > m_rtScreenShow.right)
						{
							rtTemp.right = m_rtScreenShow.right;	
							xOffset = m_rtScreenShow.right - m_rtCutStart.right;
						}
						if(rtTemp.right < m_rtScreenShow.left)
						{
							rtTemp.right = m_rtScreenShow.left;	
							xOffset = m_rtScreenShow.left - m_rtCutStart.right;
						}					
					}
					else if(m_CaptureState == CS_SIZE_DOWNRIGHT)
					{
						rtTemp.top = m_rtCutStart.top;
						rtTemp.bottom = m_rtCutStart.bottom + yOffset;
						if(rtTemp.bottom > m_rtScreenShow.bottom)
						{
							rtTemp.bottom = m_rtScreenShow.bottom;	
							yOffset = m_rtScreenShow.bottom - m_rtCutStart.bottom;
						}
						if(rtTemp.bottom < m_rtScreenShow.top)
						{
							rtTemp.bottom = m_rtScreenShow.top;	
							yOffset = m_rtScreenShow.top - m_rtCutStart.bottom;
						}
						
						rtTemp.left = m_rtCutStart.left;
						rtTemp.right = m_rtCutStart.right + xOffset;
						if(rtTemp.right > m_rtScreenShow.right)
						{
							rtTemp.right = m_rtScreenShow.right;	
							xOffset = m_rtScreenShow.right - m_rtCutStart.right;
						}
						if(rtTemp.right < m_rtScreenShow.left)
						{
							rtTemp.right = m_rtScreenShow.left;	
							xOffset = m_rtScreenShow.left - m_rtCutStart.right;
						}					
					}			
					
					//做裁剪矩形方向调整
					if(rtTemp.right < rtTemp.left)
					{
						int t = rtTemp.right;
						rtTemp.right = rtTemp.left;
						rtTemp.left = t;
					}	
					if(rtTemp.bottom < rtTemp.top)
					{
						int t = rtTemp.bottom;
						rtTemp.bottom = rtTemp.top;
						rtTemp.top = t;
					}								
					memcpy(&m_rtSel, &rtTemp, sizeof(RECT));		
					
					m_bShowTip = true;
					m_ToolDlg.ShowToolDlg(false);
				}
			}		
		}
	}	
	
	if((fwKeys == MK_LBUTTON || m_bStartPrintScr) && m_ToolDlg.m_nSelDrawButtonID != IDC_BUTTON_WORD)
	{		
		UpdateShowRgn();
	}	
	return FALSE;
}

BOOL CSCDialog::DialogOnDestroy(WPARAM wParam, LPARAM lParam)
{
	m_hDlg = NULL;

	if(m_hCursor)
	{
		::DestroyCursor(m_hCursor);
		m_hCursor = NULL;
	}		
		
	if(m_hBitmap)
	{
		::DeleteObject(m_hBitmap);	
		m_hBitmap = NULL;
	}		
	
	if(m_hGrayBitmap)
	{
		::DeleteObject(m_hGrayBitmap);	
		m_hGrayBitmap = NULL;
	}		
	
	m_hCurScrawlBimtap = NULL;
	
	if(m_rtWnds.size()>0)
		m_rtWnds.clear();
	
	if(m_ptScrawls.size()>0)
		m_ptScrawls.clear();
	
	if(m_vecBitmaps.size()>0)
	{
		for(vector<HBITMAP>::const_iterator iter = m_vecBitmaps.begin(); iter != m_vecBitmaps.end(); iter++)
		{
			HBITMAP hBitmap = (*iter);
			DeleteObject(hBitmap);
		}
		m_vecBitmaps.clear();		
	}	
	return FALSE;
}

BOOL CSCDialog::OnMsgFromTool(WPARAM wParam, LPARAM lParam)
{
	int ID = wParam;
	switch(ID)
	{	
	case IDC_BUTTON_BACK:	
		BackEditBmp();
		break;
	case IDC_BUTTON_SAVE:
		SaveAsCaptureBmp();
		break;
	case IDC_BUTTON_CANCEL:
		CancelScreenCapture();
		break;
	case IDC_BUTTON_OK:	
		EnsureScreenCapture();
		break;
	}
	return TRUE;
}

BOOL CSCDialog::OnMsgFromEdit(WPARAM wParam, LPARAM lParam)
{
	if((HWND)wParam == m_edWord.m_hWnd && lParam == WM_KILLFOCUS)
	{
		
	}
	return TRUE;
}

BOOL CSCDialog::OnMsgFromCustom(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case IDC_BUTTON_SIZE_SMA:
	case IDC_BUTTON_SIZE_MID:
	case IDC_BUTTON_SIZE_BIG:
		break;
	case IDC_COMBO_FONTSIZE:
		{
			int size = m_ToolDlg.m_CustomDlg.m_customInfo[m_ToolDlg.m_CustomDlg.m_nSelButtonID-IDC_BUTTON_RECT].size.font_size;
			int height = -MulDiv(size, GetDeviceCaps(::GetDC(::GetDlgItem(m_hDlg, IDC_EDIT_WORD)), LOGPIXELSY), 72);			
			HFONT hFont=CreateFont(
				height,
				0,
				0,
				0,
				FW_NORMAL,
				FALSE,
				FALSE,
				FALSE,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,
				DEFAULT_PITCH,
				NULL
			);
			
			m_edWord.SetFont(hFont);
			::InvalidateRect(m_hDlg, &m_rtWord, FALSE);
		}
		break;		
	}
	if(wParam >= IDC_BUTTON_COLOR_BASE && wParam < IDC_BUTTON_COLOR_BASE + 16)
	{
		if(m_ToolDlg.m_CustomDlg.m_nSelButtonID == IDC_BUTTON_WORD)
		{
			::InvalidateRect(m_hDlg, &m_rtWord, FALSE);
		}
	}
	return TRUE;
}