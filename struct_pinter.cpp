#include "struct_pinter.h"
#include "Logger.h"
#include "haikang_model.h"


static std::string login_mode(int mode) {
	if (mode == 0) {
		return "SDK˽��Э��";
	}

	return "ISAPIЭ��";
}

static std::string protocol_name(int protocol) {
	if (protocol == 0) {
		return "˽��Э��";
	}
	else if (protocol == 1) {
		return "����Э��";
	}
	else if (protocol == 3) {
		return "����Э��";
	}

	return "����Э��";
}

static std::string is_online(int state) {
	return state == 0 ? "����" : "����";
}

static std::string is_lock_support(int state) {
	return state == 1 ? "֧��" : "��֧��";
}

//��ӡnet_dvr_deviceinfo_v30
void CStructDumpPrinter::PrinterDeviceInfoV30(NET_DVR_DEVICEINFO_V30* v30) {
	CONSOLE_COLOR_YELLOW();
	std::cout
		<< "�豸��Ϣ:" << std::endl
		<< "	�豸�ͺ�: " << HaikangDeviceModel::ModelName((int)v30->wDevType)
		<< std::endl
		<< "	�豸���к�: " << v30->sSerialNumber
		<< std::endl
		<< "	ģ�ⱨ���������:" << (int) v30->byAlarmInPortNum 
		<< std::endl
		<< "	ģ�ⱨ���������: " << (int) v30->byAlarmOutPortNum
		<< std::endl
		<< "	Ӳ�̸���: " << (int)v30->byDiskNum
		<< std::endl
		<< "	�豸����:" << (int)v30->byDVRType
		<< std::endl
		<< "	�豸ģ��ͨ������: " << (int)v30->byChanNum
		<< std::endl
		<< "	ģ��ͨ������ʼͨ����: " << (int)v30->byStartChan
		<< std::endl
		<< "	�豸�����Խ�ͨ����:" << (int) v30->byAudioChanNum
		<< std::endl
		<< "	�豸�������ͨ������: " << (int) v30->byIPChanNum
		<< std::endl
		<< "	��ͨ���������: " << (int) v30->byZeroChanNum
		<< std::endl
		<< "	����������Э������:" << (int) v30->byMainProto
		<< std::endl
		<< "	����������Э������: " << (int) v30->bySubProto
		<< std::endl
		<< "	����: " << (int) v30->bySupport
		<< std::endl
		<< "	����������1:" << (int) v30->bySupport1
		<< std::endl
		<< "	����������2   : " << (int) v30->bySupport2
		<< std::endl
		<< "	�Ƿ�֧�ֶ�����:" << (int)v30->byMultiStreamProto
		<< std::endl
		<< "	��ʼ����ͨ����: " << (int)v30->byStartDChan
		<< std::endl
		<< "	��ʼ���ֶԽ�ͨ����: " << (int) v30->byStartDTalkChan
		<< std::endl
		<< "	����ͨ������high:" << (int) v30->byHighDChanNum
		<< std::endl
		<< "	��������չ4: " << (int)v30->bySupport4
		<< std::endl
		<< "	֧����������: " << (int)v30->byLanguageType
		<< std::endl
		<< "	��Ƶ����ͨ����:" << (int)v30->byVoiceInChanNum
		<< std::endl
		<< "	��Ƶ������ʼͨ����: " << (int)v30->byStartVoiceInChanNo
		<< std::endl;
}

//��ӡnet_dvr_deviceinfo_v40
void CStructDumpPrinter::PrinterDeviceInfoV40(NET_DVR_DEVICEINFO_V40* v40) {
	CONSOLE_COLOR_INFO();
	std::cout
		<< "�豸��¼������Ϣ:" << std::endl
		<< "	��¼ģʽ:" << login_mode((int)v40->byLoginMode)
		<< std::endl
		<< "	���û�����ʣ����Ч����:" << v40->iResidualValidity
		<< std::endl
		<< "	�豸�Ƿ�֧����������:" << is_lock_support((int)v40->bySupportLock);

		if ((int) v40->bySupportLock == 1) {
			std::cout
				<< std::endl
				<< "	�豸����ʣ��ʱ��:" << (int) v40->dwSurplusLockTime
				<< std::endl
				<< "	ʣ��ɳ��Ե�½�Ĵ���:" << (int) v40->byRetryLoginTime;
		}

		std::cout << std::endl << std::endl << std::endl;
}

//��ӡipͨ��������Ϣ
void CStructDumpPrinter::PrinterIpDeviceInfoV31(unsigned int idx, NET_DVR_IPDEVINFO_V31* v31) {
	CONSOLE_COLOR_INFO();
	std::cout
		<< "	ͨ�����: " << idx
		<< ",�Ƿ�����:" << is_online((int) v31->byEnable)
		<< ",Э������: " << protocol_name((int)v31->byProType)
		<< ",IP��ַ: " << v31->struIP.sIpV4
		<< ",�˿ں�: " << v31->wDVRPort
		<< ",�û���: " << v31->sUserName
		<< ",����: " << v31->sPassword
		<< std::endl;
}

// devName      �豸����
// channel      ͨ����
// pFrameInfo   ֡����
void CStructDumpPrinter::PrinterFrameInfo(const char* devName, unsigned int channel, FRAME_INFO* pFrameInfo) {
	CONSOLE_COLOR_DEBUG();
	std::cout << "���յ����͵���Ƶ��֡����,"
		<< ", �豸����:"
		<< devName
		<< ", ͨ�����:"
		<< channel
		<< ", ��Ƶ�ֱ���, ��:"
		<< pFrameInfo->nWidth
		<< ", ��:"
		<< pFrameInfo->nHeight
		<< ", ͼ��֡��:"
		<< pFrameInfo->nFrameRate
		<< ", ʱ���:"
		<< pFrameInfo->nStamp
		<< ", ����:"
		<< pFrameInfo->nType
		<< ", ֡��:"
		<< pFrameInfo->dwFrameNum
		<< std::endl
		<< std::endl;
}

