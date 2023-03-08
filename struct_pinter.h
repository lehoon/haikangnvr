
#ifndef HAIKANG_NVR_TESTING_STRUCT_PRINTER_H_
#define HAIKANG_NVR_TESTING_STRUCT_PRINTER_H_


#include "common_define.h"


class CStructDumpPrinter
{
public:
	CStructDumpPrinter() = delete;
	~CStructDumpPrinter() = delete;

public:
	//打印net_dvr_deviceinfo_v30
	static void PrinterDeviceInfoV30(NET_DVR_DEVICEINFO_V30*);

	//打印net_dvr_deviceinfo_v40
	static void PrinterDeviceInfoV40(NET_DVR_DEVICEINFO_V40*);

	//打印ip通道配置信息
	static void PrinterIpDeviceInfoV31(unsigned int, NET_DVR_IPDEVINFO_V31*);

	//打印接收到的帧信息
	static void PrinterFrameInfo(const char *, unsigned int, FRAME_INFO*);
};

#endif HAIKANG_NVR_TESTING_STRUCT_PRINTER_H_