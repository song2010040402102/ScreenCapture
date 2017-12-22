/********************************************************
文件说明：屏幕抓图的接口文件
作者：宋旭
日期：2015-01-27

********************************************************/

#ifndef SCREEN_CAPTURE_H
#define SCREEN_CAPTURE_H

#include "windows.h"

#define IS_EXPORT

#ifdef IS_EXPORT
	#define PORT_DIR __declspec(dllexport)
#else
	#define PORT_DIR __declspec(dllimport)
#endif

//抓图后用户的操作
typedef enum _capture_oper
{
	CO_CANCEL = 0, //取消抓图
	CO_SURE,       //确定抓图
	CO_SAVEAS,	   //另存为抓图
	CO_EDIT		   //对抓图进行编辑

}CAPTURE_OPER;

//截图的数据结构
typedef struct _capture_data
{
	char save_dir[MAX_PATH]; //截图文件的保存目录
	char filename[MAX_PATH]; //截图文件的文件名称（包含路径）	
	CAPTURE_OPER capture_oper; //抓图后用户的操作行为

}CAPTURE_DATA, *PCAPTURE_DATA;

//执行屏幕截图的函数
PORT_DIR void ExecuteScreenCapture(CAPTURE_DATA* pCaptureData);

#endif
