/********************************************************
文件说明：屏幕抓图的界面实现文件
作者：宋旭
日期：2015-01-27

********************************************************/

#ifndef SCREEN_CAPTURE_DLG_H
#define SCREEN_CAPTURE_DLG_H

#pragma warning (disable: 4786)

#define ULONG_PTR unsigned long
#include <ComDef.h>
#include <GdiPlus.h>
#pragma comment(lib, "Gdiplus.lib")
using namespace Gdiplus;

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <map>
using namespace std;

#define IDC_BUTTON_RECT                 1000
#define IDC_BUTTON_ELLIPSE              1001
#define IDC_BUTTON_ARROW                1002
#define IDC_BUTTON_BRUSH                1003
#define IDC_BUTTON_WORD                 1004
#define IDC_BUTTON_BACK                 1005
#define IDC_BUTTON_SAVE                 1006
#define IDC_BUTTON_CANCEL               1007
#define IDC_BUTTON_OK                   1008

#define IDC_BUTTON_SIZE_SMA				1020
#define IDC_BUTTON_SIZE_MID				1021
#define IDC_BUTTON_SIZE_BIG				1022

#define IDC_BUTTON_COLOR_BASE			1030

#define IDC_EDIT_WORD					1100

class CWnd
{
public:		
	HWND m_hWnd;		
	
	static map<HWND, CWnd*> m_mapWndList;
	
	WNDPROC m_oldWndProc;
	
	CWnd();
	~CWnd();
	
	static void ToWndList(CWnd* pWnd);
	static CWnd* FromWndList(HWND hWnd);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam); //message route
	
	virtual LRESULT CALLBACK WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0; //callback function	
};

class CMyButton;

//自定义按钮绘制回调函数
typedef BOOL (__stdcall *pFunCustomDrawButtonCallback)(CMyButton*, HDC, RECT*);

//按钮状态
typedef enum _button_state
{
	BS_NORMAL = 0,
	BS_HOVER,
	BS_DOWN,
	BS_DISABLE
		
}BUTTON_STATE;

//按钮样式
typedef enum _button_style
{
	BS_BUTTON = 0,
	BS_RADIO
		
}BUTTON_STYLE;

class CMyButton : public CWnd
{
private:
	//按钮ID
	UINT m_nButtonID;

	//按钮状态
	BUTTON_STATE m_buttonState;

	//按钮状态对应的背景图片
	Image* m_pBkImage[4];
	
	//鼠标在按钮上的最后一次位置
	int m_lastCursorPos;

	//设置按钮自绘回调函数
	pFunCustomDrawButtonCallback m_pFunCustomDrawButton;

	//设置按钮样式
	BUTTON_STYLE m_buttonStyle;
public:
	CMyButton();
	virtual ~CMyButton();

	//创建button
	HWND CreateButton(UINT idCtl, HWND hParent = NULL, DWORD dwStyle = 0);

	//给按钮设置PNG背景图片
	void SetPNGBitmaps(Image* pPngImageNormal, Image* pPngImageFocus = NULL, Image* pPngImageDown = NULL, 
		Image* pPngImageDisable = NULL);

	//设置自定义按钮绘制回调函数
	void SetCustomDrawButtonCallback(pFunCustomDrawButtonCallback pFun) {m_pFunCustomDrawButton = pFun;}

	//获取按钮ID
	UINT GetButtonID() {return m_nButtonID;}

	//获取按钮状态
	BUTTON_STATE GetButtonState() {return m_buttonState;}

	//获取按钮样式
	BUTTON_STYLE GetButtonStyle() {return m_buttonStyle;}
	
	//设置按钮状态
	void SetButtonState(BUTTON_STATE button_state) {m_buttonState = button_state; ::InvalidateRect(m_hWnd, NULL, FALSE);}

	//设置按钮样式
	void SetButtonStyle(BUTTON_STYLE button_style) {m_buttonStyle = button_style;}	

	//消息处理回调函数
	virtual LRESULT CALLBACK WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	//窗体绘制处理
	BOOL OnPaint(WPARAM wParam, LPARAM lParam);

	//擦除背景处理
	BOOL OnEraseBkgnd(WPARAM wParam, LPARAM lParam);
		
	//鼠标左键按下的处理
	BOOL OnLButtonDown(WPARAM wParam, LPARAM lParam);
	
	//鼠标左键弹起的处理
	BOOL OnLButtonUp(WPARAM wParam, LPARAM lParam);	
	
	//鼠标Hover的处理
	BOOL OnMouseHover(WPARAM wParam, LPARAM lParam);

	//鼠标离开的处理
	BOOL OnMouseLeave(WPARAM wParam, LPARAM lParam);

	//Timer事件的处理
	BOOL OnTimer(WPARAM wParam, LPARAM lParam);
	
	//窗体破坏的处理
	BOOL OnDestroy(WPARAM wParam, LPARAM lParam);
};

class CMyEdit : public CWnd
{
public:
	//最后一次按下的字符
	TCHAR m_chLastCode;

	//是否处于编辑状态
	BOOL m_bEdit;

private:
	//字体
	HFONT m_hFont;
public:
	CMyEdit();
	virtual ~CMyEdit();
	
	//设置字体
	void SetFont(HFONT hFont);
	
	//创建Edit
	HWND CreateEdit(UINT idCtl, HWND hParent = NULL, DWORD dwStyle = 0);
	
	//消息处理回调函数
	virtual LRESULT CALLBACK WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	//窗体绘制处理
	BOOL OnPaint(WPARAM wParam, LPARAM lParam);

	//窗体破话处理
	BOOL OnDestroy(WPARAM wParam, LPARAM lParam);
};

class CCustomDialog
{
	typedef struct _custom_info
	{
		union
		{
			int line_width; //线宽
			int font_size; //字体大小

		} size;
		COLORREF color; //字体或线的颜色

	}CUSTOM_INFO, *PCUSTOM_INFO;

public:
	HWND m_hDlg;
	BOOL m_bShow;
	UINT m_nSelButtonID; //当前选择按钮ID
	UINT m_nSelSizeButtonID; //当前选择的尺寸按钮ID
	CUSTOM_INFO m_customInfo[5];
	CMyButton m_buttonWidth[3]; //三种可选的线宽按钮
	CMyButton m_buttonColor[16]; //16种可选的颜色按钮
	POINT m_ptSepLine[2]; //分割线的两点坐标
public:
	CCustomDialog();
	virtual ~CCustomDialog();	

	//颜色按钮绘制回调函数
	static BOOL __stdcall DrawColorButton(CMyButton *pButton, HDC hDC, RECT *pRect);

	//更新控件
	void UpdateControls();

	//更新按钮状态
	void UpdateButtonState();

	//展示定制框
	void ShowCustomDlg(bool bShow = true, UINT nButtonID = 0);	

	//工具框的处理程序
	static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	//对话框初始化处理
	BOOL DialogOnInit(HWND hWnd, WPARAM wParam, LPARAM lParam);

	//对话框命令处理
	BOOL DialogOnCommand(WPARAM wParam, LPARAM lParam);		
	
	//对话框绘制处理
	BOOL DialogOnPaint(WPARAM wParam, LPARAM lParam);
	
	//擦除背景的处理
	BOOL DialogOnEraseBkgnd(WPARAM wParam, LPARAM lParam);	
	
	//窗体破坏的处理
	BOOL DialogOnDestroy(WPARAM wParam, LPARAM lParam);
};

class CToolDialog
{
public:
	HWND m_hDlg;
	BOOL m_bShow;
	CCustomDialog m_CustomDlg;
	CMyButton m_btRect;
	CMyButton m_btEllipse;
	CMyButton m_btArrow;
	CMyButton m_btBrush;
	CMyButton m_btWord;
	CMyButton m_btBack;
	CMyButton m_btSave;
	CMyButton m_btCancel;	
	CMyButton m_btOk;	
public:
	CToolDialog();
	virtual ~CToolDialog();
	
	//展示工具框
	void ShowToolDlg(bool bShow = true);	
	
	//工具框的处理程序
	static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	//对话框初始化处理
	BOOL DialogOnInit(HWND hWnd, WPARAM wParam, LPARAM lParam);
	
	//对话框命令处理
	BOOL DialogOnCommand(WPARAM wParam, LPARAM lParam);

	//对话框Timer处理
	BOOL DialogOnTimer(WPARAM wParam, LPARAM lParam);
	
	//对话框绘制处理
	BOOL DialogOnPaint(WPARAM wParam, LPARAM lParam);
	
	//擦除背景的处理
	BOOL DialogOnEraseBkgnd(WPARAM wParam, LPARAM lParam);	

	//窗体破坏的处理
	BOOL DialogOnDestroy(WPARAM wParam, LPARAM lParam);

public:
	//是否进入编辑模式
	bool m_bEdit;

	//当前选择的绘制按钮ID(矩形、椭圆、箭头、画刷、文本)
	UINT m_nSelDrawButtonID;

	//当前选择的控制按钮ID(撤销、保存、取消、确定)
	UINT m_nSelCtrlButtonID;

	//是否进行文本按钮切换
	bool m_bSwitchTextButton;

public:
	//初始化工具栏数据
	void InitToolData();

	//更新按钮状态
	void UpdateButtonState();
};

class CSCDialog
{
	//鼠标被被捕获的状态
	typedef enum _capture_state
	{
		CS_NORMAL,
		CS_MOVE,
		CS_SIZE_UP,
		CS_SIZE_DOWN,
		CS_SIZE_LEFT,
		CS_SIZE_RIGHT,
		CS_SIZE_UPLEFT,
		CS_SIZE_UPRIGHT,
		CS_SIZE_DOWNLEFT,
		CS_SIZE_DOWNRIGHT,	
			
	}CAPTURE_STATE;
	
	//光标所处位置
	typedef enum _cursor_position
	{
		CP_OUTSIDE,
		CP_INSIDE,
		CP_SIDE_UP,
		CP_SIDE_DOWN,
		CP_SIDE_LEFT,
		CP_SIDE_RIGHT,
		CP_CORNER_UPLEFT,
		CP_CORNER_UPRIGHT,
		CP_CORNER_DOWNLEFT,
		CP_CORNER_DOWNRIGHT,	
			
	}CURSOR_POSITION;

public:
	HWND m_hDlg;
	
	//截图是否开始
	bool m_bStartPrintScr; 

	//是否处于窗口捕获模式
	bool m_bCapWnd;	

	//是否显示提示
	bool m_bShowTip;

	//是否开始编辑截图
	bool m_bStartEdit;

	//是否撤销
	bool m_bSelBack;

	//屏幕分辨率
	int m_nXScreen;
	int m_nYScreen;

	//画文字时鼠标点击的次数
	int m_nWordCount;

	POINT m_ptCapStart; //开始截取时的起始点坐标
	POINT m_ptScrawStart; //开始涂鸦时的起始点坐标
	POINT m_ptScrawEnd; //结束涂鸦时的坐标

	RECT m_rtSel; //选中区域矩形
	RECT m_rtTip; //提示框矩形
	vector<RECT> m_rtWnds; //当前屏幕中含有的窗体矩形

	RECT m_rtScrawl; //涂鸦矩形
	RECT m_rtWord; //文字区域矩形
	vector<POINT> m_ptScrawls; //涂鸦点
	vector<HBITMAP> m_vecBitmaps; //存储用户绘制的位图，便于撤销

	HBITMAP m_hBitmap; //当前屏幕位图
	HBITMAP m_hGrayBitmap; //当前屏幕灰度位图
	HBITMAP m_hCurScrawlBimtap; //当前涂鸦位图

	CMyEdit m_edWord; //用于绘制文字的编辑框
	CToolDialog m_ToolDlg; //工具栏对话框

	//在调用GdiplusStartup之前必须确保GdiplusStartupInput完成初始化，因此作为成员变量来使用
	GdiplusStartupInput   m_gdiplusStartupInput; 
	ULONG_PTR   m_gdiplusToken;

	RECT m_rtScreenShow; //屏幕展示区域	
	RECT m_rtCutStart; //鼠标按下时的裁剪区域的矩形
	
	HCURSOR m_hCursor; //当前光标
	CAPTURE_STATE m_CaptureState; //当前被捕获的状态
	CURSOR_POSITION m_CursorPos; //当前光标所处位置
	
	POINT m_ptStart; //鼠标开始移动的位置
	POINT m_ptEnd; //鼠标结束移动的位置

private:
	//初始化抓图数据
	void InitCaptureData();

	//截取指定矩形的屏幕至位图
	HBITMAP PrintScreenToBitmap(int x, int y, int width, int height);	

	//判断当前选择的区域是否符合要求
	bool IsLegalForSelRgn();
	
	//穷举当前桌面顶层窗口的回调函数
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

	//更新提示框矩形
	void UpdateTipRect();

	//更新工具条矩形
	void UpdateToolWndPos();

	//绘制涂鸦
	void DrawScrawl(HDC hDC, HBITMAP hBitmap);

	//绘制选中区域的边界
	void DrawSelRgnEdge(HDC hDC);

	//绘制提示
	void DrawTip(HDC hDC);	

	//更新当前被捕获的状态
	void UpdateCaptureState();
	
	//更新当前鼠标光标
	void UpdateCursor();
	
	//更新当前鼠标位置
	void UpdateCursorPos(POINT point);		

	//刷新显示区域
	void UpdateShowRgn();
public:
	//获取灰化后的位图
	HBITMAP GetGrayBitmap(HBITMAP hBitmap, COLORREF colorGray, double alpha);	

	//位图的复制(如果存在目的位图，则复制原位图到目的位图，如果不存在则创建新位图并复制原位图作为返回值，并且可以指定复制矩形)
	HBITMAP CopyBitmap(HBITMAP hSrcBitmap, HBITMAP hDstBitmap = NULL, const RECT *pRTSrc = NULL, const RECT *pRTDst = NULL);

	//撤销对截图的编辑
	void BackEditBmp();

	//另存为截图
	void SaveAsCaptureBmp();

	//取消截图
	void CancelScreenCapture();

	//确定截图
	void EnsureScreenCapture();
public:
	CSCDialog();
	virtual ~CSCDialog();

	//展示截屏框
	void ShowScreenCaptureDlg();	

	//对话框的处理程序
	static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//对话框初始化处理
	BOOL DialogOnInit(HWND hWnd, WPARAM wParam, LPARAM lParam);

	//对话框命令处理
	BOOL DialogOnCommand(WPARAM wParam, LPARAM lParam);

	//对话框绘制处理
	BOOL DialogOnPaint(WPARAM wParam, LPARAM lParam);

	//擦除背景的处理
	BOOL DialogOnEraseBkgnd(WPARAM wParam, LPARAM lParam);

	//鼠标左键按下的处理
	BOOL DialogOnLButtonDown(WPARAM wParam, LPARAM lParam);

	//鼠标左键双击事件
	BOOL DialogOnLButtonDbClick(WPARAM wParam, LPARAM lParam);

	//鼠标左键弹起的处理
	BOOL DialogOnLButtonUp(WPARAM wParam, LPARAM lParam);

	//鼠标右键弹起的处理
	BOOL DialogOnRButtonUp(WPARAM wParam, LPARAM lParam);

	//鼠标移动的处理
	BOOL DialogOnMouseMove(WPARAM wParam, LPARAM lParam);

	//窗体破坏的处理
	BOOL DialogOnDestroy(WPARAM wParam, LPARAM lParam);

	//来自工具窗口消息的处理
	BOOL OnMsgFromTool(WPARAM wParam, LPARAM lParam);

	//来自编辑框窗口消息的处理
	BOOL OnMsgFromEdit(WPARAM wParam, LPARAM lParam);

	//来自定制窗口消息的处理
	BOOL OnMsgFromCustom(WPARAM wParam, LPARAM lParam);
};


#endif
