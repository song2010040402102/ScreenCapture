#include "ScreenCapture.h"
#include "ScreenCaptureDlg.h"

CAPTURE_DATA* g_pCaptureData = NULL;
CSCDialog g_scDialog;
HANDLE g_hModule = NULL;

BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	g_hModule = hModule;
    switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
    }
    return TRUE;
}

PORT_DIR void ExecuteScreenCapture(CAPTURE_DATA* pCaptureData)
{
	if(!pCaptureData)
		return;

	g_pCaptureData = pCaptureData;

	g_scDialog.ShowScreenCaptureDlg();
}