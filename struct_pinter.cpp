#include "struct_pinter.h"
#include "Logger.h"
#include "haikang_model.h"


static std::string login_mode(int mode) {
	if (mode == 0) {
		return "SDK私有协议";
	}

	return "ISAPI协议";
}

static std::string protocol_name(int protocol) {
	if (protocol == 0) {
		return "私有协议";
	}
	else if (protocol == 1) {
		return "松下协议";
	}
	else if (protocol == 3) {
		return "索尼协议";
	}

	return "其他协议";
}

static std::string is_online(int state) {
	return state == 0 ? "在线" : "离线";
}

static std::string is_lock_support(int state) {
	return state == 1 ? "支持" : "不支持";
}

//打印net_dvr_deviceinfo_v30
void CStructDumpPrinter::PrinterDeviceInfoV30(NET_DVR_DEVICEINFO_V30* v30) {
	CONSOLE_COLOR_YELLOW();
	std::cout
		<< "设备信息:" << std::endl
		<< "	设备型号: " << HaikangDeviceModel::ModelName((int)v30->wDevType)
		<< std::endl
		<< "	设备序列号: " << v30->sSerialNumber
		<< std::endl
		<< "	模拟报警输入个数:" << (int) v30->byAlarmInPortNum 
		<< std::endl
		<< "	模拟报警输出个数: " << (int) v30->byAlarmOutPortNum
		<< std::endl
		<< "	硬盘个数: " << (int)v30->byDiskNum
		<< std::endl
		<< "	设备类型:" << (int)v30->byDVRType
		<< std::endl
		<< "	设备模拟通道个数: " << (int)v30->byChanNum
		<< std::endl
		<< "	模拟通道的起始通道号: " << (int)v30->byStartChan
		<< std::endl
		<< "	设备语音对讲通道数:" << (int) v30->byAudioChanNum
		<< std::endl
		<< "	设备最大数字通道个数: " << (int) v30->byIPChanNum
		<< std::endl
		<< "	零通道编码个数: " << (int) v30->byZeroChanNum
		<< std::endl
		<< "	主码流传输协议类型:" << (int) v30->byMainProto
		<< std::endl
		<< "	子码流传输协议类型: " << (int) v30->bySubProto
		<< std::endl
		<< "	能力: " << (int) v30->bySupport
		<< std::endl
		<< "	能力集扩充1:" << (int) v30->bySupport1
		<< std::endl
		<< "	能力集扩充2   : " << (int) v30->bySupport2
		<< std::endl
		<< "	是否支持多码流:" << (int)v30->byMultiStreamProto
		<< std::endl
		<< "	起始数字通道号: " << (int)v30->byStartDChan
		<< std::endl
		<< "	起始数字对讲通道号: " << (int) v30->byStartDTalkChan
		<< std::endl
		<< "	数字通道个数high:" << (int) v30->byHighDChanNum
		<< std::endl
		<< "	能力集扩展4: " << (int)v30->bySupport4
		<< std::endl
		<< "	支持语种能力: " << (int)v30->byLanguageType
		<< std::endl
		<< "	音频输入通道数:" << (int)v30->byVoiceInChanNum
		<< std::endl
		<< "	音频输入起始通道号: " << (int)v30->byStartVoiceInChanNo
		<< std::endl;
}

//打印net_dvr_deviceinfo_v40
void CStructDumpPrinter::PrinterDeviceInfoV40(NET_DVR_DEVICEINFO_V40* v40) {
	CONSOLE_COLOR_INFO();
	std::cout
		<< "设备登录返回信息:" << std::endl
		<< "	登录模式:" << login_mode((int)v40->byLoginMode)
		<< std::endl
		<< "	该用户密码剩余有效天数:" << v40->iResidualValidity
		<< std::endl
		<< "	设备是否支持锁定功能:" << is_lock_support((int)v40->bySupportLock);

		if ((int) v40->bySupportLock == 1) {
			std::cout
				<< std::endl
				<< "	设备锁定剩余时间:" << (int) v40->dwSurplusLockTime
				<< std::endl
				<< "	剩余可尝试登陆的次数:" << (int) v40->byRetryLoginTime;
		}

		std::cout << std::endl << std::endl << std::endl;
}

//打印ip通道配置信息
void CStructDumpPrinter::PrinterIpDeviceInfoV31(unsigned int idx, NET_DVR_IPDEVINFO_V31* v31) {
	CONSOLE_COLOR_INFO();
	std::cout
		<< "	通道编号: " << idx
		<< ",是否在线:" << is_online((int) v31->byEnable)
		<< ",协议类型: " << protocol_name((int)v31->byProType)
		<< ",IP地址: " << v31->struIP.sIpV4
		<< ",端口号: " << v31->wDVRPort
		<< ",用户名: " << v31->sUserName
		<< ",密码: " << v31->sPassword
		<< std::endl;
}

// devName      设备名称
// channel      通道号
// pFrameInfo   帧数据
void CStructDumpPrinter::PrinterFrameInfo(const char* devName, unsigned int channel, FRAME_INFO* pFrameInfo) {
	CONSOLE_COLOR_DEBUG();
	std::cout << "接收到推送的视频流帧数据,"
		<< ", 设备名称:"
		<< devName
		<< ", 通道编号:"
		<< channel
		<< ", 视频分辨率, 宽:"
		<< pFrameInfo->nWidth
		<< ", 高:"
		<< pFrameInfo->nHeight
		<< ", 图像帧率:"
		<< pFrameInfo->nFrameRate
		<< ", 时间戳:"
		<< pFrameInfo->nStamp
		<< ", 类型:"
		<< pFrameInfo->nType
		<< ", 帧号:"
		<< pFrameInfo->dwFrameNum
		<< std::endl
		<< std::endl;
}

