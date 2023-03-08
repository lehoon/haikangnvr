
#ifndef HAIKANG_NVR_TESTING_STRUCT_PRINTER_H_
#define HAIKANG_NVR_TESTING_STRUCT_PRINTER_H_


#include "common_define.h"


class CStructDumpPrinter
{
public:
	CStructDumpPrinter() = delete;
	~CStructDumpPrinter() = delete;

public:
	//��ӡnet_dvr_deviceinfo_v30
	static void PrinterDeviceInfoV30(NET_DVR_DEVICEINFO_V30*);

	//��ӡnet_dvr_deviceinfo_v40
	static void PrinterDeviceInfoV40(NET_DVR_DEVICEINFO_V40*);

	//��ӡipͨ��������Ϣ
	static void PrinterIpDeviceInfoV31(unsigned int, NET_DVR_IPDEVINFO_V31*);

	//��ӡ���յ���֡��Ϣ
	static void PrinterFrameInfo(const char *, unsigned int, FRAME_INFO*);
};

#endif HAIKANG_NVR_TESTING_STRUCT_PRINTER_H_