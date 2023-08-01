#include "Device.h"
#include "Logger.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>


DeviceHK::DeviceHK() {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("�����豸���յ��쳣��Ϣ,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DeviceHK::DeviceHK(unsigned int channel) : Device(channel) {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("�����豸���յ��쳣��Ϣ,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DeviceHK::~DeviceHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�" << std::endl;
        SPDLOG_INFO("�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�");
        });
}

bool DeviceHK::connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut) {
    NET_DVR_USER_LOGIN_INFO loginInfo = { 0 };
    NET_DVR_DEVICEINFO_V40 loginResult = { 0 };

    //login info
    loginInfo.wPort = info.ui16DevPort;
    loginInfo.byUseTransport = 0;
    loginInfo.bUseAsynLogin = 0;
    strcpy(loginInfo.sDeviceAddress, info.strDevIp.c_str());
    strcpy(loginInfo.sUserName, info.strUserName.c_str());
    strcpy(loginInfo.sPassword, info.strPwd.c_str());
    uintFrameCount = info.ui16FrameCount;
    bPrintFrameLog = info.bPrintFrameLog;

    NET_DVR_SetConnectTime(iTimeOut * 1000, 3);
    m_i64LoginId = NET_DVR_Login_V40(&loginInfo, &loginResult);

    if (m_i64LoginId < 0) {
        DWORD _error_code = NET_DVR_GetLastError();
        std::cout << "��¼�豸ʧ��" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("�����豸��¼�豸ʧ��, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
        return false;
    }

    this->onConnected(m_i64LoginId, &loginResult.struDeviceV30);
    return true;
}

void DeviceHK::disconnect(const relustCB& cb) {
    m_mapChannels.clear();
    if (m_i64LoginId >= 0) {
        NET_DVR_Logout((LONG) m_i64LoginId);
        m_i64LoginId = -1;
        Device::onDisconnected(true);
    }

    cb(true);
}

void DeviceHK::addChannel(int iChn, bool bMainStream) {
    try {
        DevChannel::Ptr channel(new DevChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount));
        m_mapChannels[iChn] = channel;
        IncrSuccessChannel();
        std::cout << "����ģ����Ƶͨ��" << iChn << "�ɹ�" << std::endl;
        SPDLOG_INFO("����ģ����Ƶͨ��{}�ɹ�", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("����ģ����Ƶͨ��{}����,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "����ģ����Ƶͨ������" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DeviceHK::addIpChannel(int iChn, bool bMainStream) {
    try {
        std::weak_ptr<Device> weakSelf = shared_from_this();
        DevChannel::Ptr channel(new DevIpChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount));
        m_mapChannels[iChn] = channel;
        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "����IP��Ƶͨ��" << iChn << "�ɹ�" << std::endl;
        SPDLOG_INFO("����IP��Ƶͨ��{}�ɹ�", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("����IP��Ƶͨ��{}����,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "����IP��Ƶͨ������," << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DeviceHK::delChannel(int chn) {
    m_mapChannels.erase(chn);
}

void DeviceHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
    m_i64LoginId = lUserID;
    m_deviceInfo = *lpDeviceInfo;
    std::cout << "��ʼ��ȡ��Ƶ��" << std::endl;
    SPDLOG_INFO("��ʼ��ȡ��Ƶ��");
    CStructDumpPrinter::PrinterDeviceInfoV30(lpDeviceInfo);

    // ��Ҫ�ж��Ƿ���nvr�豸  �����nvr�豸�Ļ� ��Ҫȥ��ȡ���õ�ipͨ����Ϣ
    if (lpDeviceInfo->byIPChanNum == 0) {
        //��֧��ipͨ�� ����nvr�豸
        CONSOLE_COLOR_DEBUG();
        std::cout << std::endl << "���豸����nvr�豸,��֧��ipͨ��,���ģ��ͨ��" << std::endl;
        SPDLOG_INFO("���豸����nvr�豸,��֧��ipͨ��,���ģ��ͨ��");
        addAllChannel(true);  //���Ĭ�ϵ�ģ��ͨ����Ϣ
        return;
    }
    else {
        CONSOLE_COLOR_DEBUG();
        std::cout << "���豸��nvr�豸,֧��ipͨ��,���ipͨ��" << std::endl;
        SPDLOG_INFO("���豸��nvr�豸,֧��ipͨ��,���ipͨ��");
    }

    //ipͨ����>0˵����nvr�豸  ��Ҫͨ�����ú���NET_DVR_GetDVRConfig ��� NET_DVR_GET_IPPARACFG_V40 ��ѯipͨ������
    CONSOLE_COLOR_INFO();
    SPDLOG_INFO("��ʼʹ��NET_DVR_GetDVRConfig��ѯnvr������Ϣ");
    std::cout << "��ʼʹ��NET_DVR_GetDVRConfig��ѯnvr������Ϣ" << m_i64LoginId << std::endl;
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    memset(&dvrIpParamCfgV40, 0, sizeof(NET_DVR_IPPARACFG_V40));

    DWORD dwReturned = 0;
    CONSOLE_COLOR_YELLOW(); 
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    std::cout << "��ȡnvr�豸��ͨ��������Ϣ," << result << ", " << dwReturned << std::endl;
    SPDLOG_INFO("��ȡnvr�豸��ͨ��������Ϣ{},{}", result, dwReturned);

    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << std::endl << "��֧��IPͨ�����߲���ʧ��." << std::endl;
        CONSOLE_COLOR_RESET();
        SPDLOG_ERROR("��֧��IPͨ�����߲���ʧ��{},{}", result, dwReturned);
        return;
    }

    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸" << std::endl << std::endl;
    std::cout << std::endl << "ע�ᵽ��nvr����Ƶͨ������," << dvrIpParamCfgV40.dwDChanNum << std::endl << std::endl;
    SPDLOG_INFO("��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸,��Ƶͨ������{}", dvrIpParamCfgV40.dwDChanNum);

    //����ͨ��
    unsigned int channel_count = 0;
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        unsigned channel_no = dvrIpParamCfgV40.dwStartDChan + i;
        if (dvrIpParamCfgV40.struIPDevInfo[i].byEnable)  //ipͨ������
        {
            if (uintSuccessChannel >= uintChannel) {
                break;
            }

            CONSOLE_COLOR_YELLOW();
            std::cout << std::endl << std::endl << "��ͨ��"<< channel_no << "�豸Ŀǰ����,��ʼ�����豸��Ƶ������" << std::endl;
            CONSOLE_COLOR_INFO();
            CStructDumpPrinter::PrinterIpDeviceInfoV31(channel_no, &(dvrIpParamCfgV40.struIPDevInfo[i]));
            addIpChannel(channel_no, true);
            SPDLOG_INFO("���豸{}Ŀǰ����,��ʼ�����豸��Ƶ������", channel_no);
        }
        else {
            std::cout << "���豸Ŀǰ������" << std::endl;
            SPDLOG_ERROR("���豸{}Ŀǰ������,��ʼ�����豸��Ƶ������", channel_no);
        }
    }

    Device::onConnected();
}

void DeviceHK::addAllChannel(bool bMainStream) {
    SPDLOG_INFO("��ʼ��ȡģ��ͨ������, ͨ����:{} ʵ��ͨ����:{}", uintChannel, m_deviceInfo.byChanNum);
    for (int i = 0; i < m_deviceInfo.byChanNum; i++) {
        if (uintSuccessChannel >= uintChannel) {
            break;
        }

        addChannel(m_deviceInfo.byStartChan + i, bMainStream);
    }
}

/// <summary>
/// ����ת���豸����
/// </summary>
DecodeDeviceHK::DecodeDeviceHK() {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("�����豸���յ��쳣��Ϣ,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DecodeDeviceHK::DecodeDeviceHK(unsigned int channel) : Device(channel) {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("�����豸���յ��쳣��Ϣ,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DecodeDeviceHK::~DecodeDeviceHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�" << std::endl;
        SPDLOG_INFO("�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�");
        });
}

bool DecodeDeviceHK::connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut) {
    NET_DVR_USER_LOGIN_INFO loginInfo = { 0 };
    NET_DVR_DEVICEINFO_V40 loginResult = { 0 };

    //login info
    loginInfo.wPort = info.ui16DevPort;
    loginInfo.byUseTransport = 0;
    loginInfo.bUseAsynLogin = 0;
    strcpy(loginInfo.sDeviceAddress, info.strDevIp.c_str());
    strcpy(loginInfo.sUserName, info.strUserName.c_str());
    strcpy(loginInfo.sPassword, info.strPwd.c_str());
    uintFrameCount = info.ui16FrameCount;
    bPrintFrameLog = info.bPrintFrameLog;

    NET_DVR_SetConnectTime(iTimeOut * 1000, 3);
    m_i64LoginId = NET_DVR_Login_V40(&loginInfo, &loginResult);

    if (m_i64LoginId < 0) {
        DWORD _error_code = NET_DVR_GetLastError();
        std::cout << "��¼�豸ʧ��" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("�����豸��¼�豸ʧ��, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
        return false;
    }

    this->onConnected(m_i64LoginId, &loginResult.struDeviceV30);
    return true;
}

void DecodeDeviceHK::disconnect(const relustCB& cb) {
    m_mapChannels.clear();
    if (m_i64LoginId >= 0) {
        NET_DVR_Logout((LONG)m_i64LoginId);
        m_i64LoginId = -1;
        Device::onDisconnected(true);
    }
}

void DecodeDeviceHK::addChannel(int iChn, bool bMainStream) {
    try{
        DevChannel::Ptr channel(new DevDecodeChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount, bPrintFrameLog));
        m_mapChannels[iChn] = channel;
        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "����ģ�������Ƶͨ��" << iChn << "�ɹ�" << std::endl;
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("����ģ�������Ƶͨ��{}����,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "����ģ�������Ƶͨ������" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeDeviceHK::addIpChannel(int iChn, bool bMainStream) {
    try {
        DevChannel::Ptr channel(new DevIpDecodeChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount, bPrintFrameLog));
        m_mapChannels[iChn] = channel;
        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "����IP������Ƶͨ��" << iChn << "�ɹ�" << std::endl;
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("����ip������Ƶͨ��{}����,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "����ip������Ƶͨ������" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeDeviceHK::delChannel(int chn) {
    m_mapChannels.erase(chn);
}

void DecodeDeviceHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
    m_i64LoginId = lUserID;
    m_deviceInfo = *lpDeviceInfo;

    std::cout << "��ʼע�������Ƶͨ��" << std::endl;
    SPDLOG_INFO("��ʼע�������Ƶͨ��");
    CStructDumpPrinter::PrinterDeviceInfoV30(lpDeviceInfo);

    // ��Ҫ�ж��Ƿ���nvr�豸  �����nvr�豸�Ļ� ��Ҫȥ��ȡ���õ�ipͨ����Ϣ
    if (lpDeviceInfo->byIPChanNum == 0) {
        //��֧��ipͨ�� ����nvr�豸
        std::cout << std::endl << "���豸����nvr�豸,��֧��ipͨ��,���ģ��ͨ��" << std::endl;
        SPDLOG_INFO("���豸����nvr�豸,��֧��ipͨ��,���ģ��ͨ��");
        addAllChannel(true);  //���Ĭ�ϵ�ģ��ͨ����Ϣ
        return;
    }
    else {
        CONSOLE_COLOR_DEBUG();
        std::cout << "���豸��nvr�豸,֧��ipͨ��,���ipͨ��" << std::endl;
        SPDLOG_INFO("���豸��nvr�豸,֧��ipͨ��,���ipͨ��");
    }

    //ipͨ����>0˵����nvr�豸  ��Ҫͨ�����ú���NET_DVR_GetDVRConfig ��� NET_DVR_GET_IPPARACFG_V40 ��ѯipͨ������
    CONSOLE_COLOR_INFO();
    SPDLOG_INFO("��ʼʹ��NET_DVR_GetDVRConfig��ѯnvr������Ϣ");
    std::cout << "��ʼʹ��NET_DVR_GetDVRConfig��ѯnvr������Ϣ" << m_i64LoginId << std::endl;
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    memset(&dvrIpParamCfgV40, 0, sizeof(NET_DVR_IPPARACFG_V40));

    DWORD dwReturned = 0;
    CONSOLE_COLOR_YELLOW();
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    std::cout << "��ȡnvr�豸��ͨ��������Ϣ," << result << ", " << dwReturned << std::endl;
    SPDLOG_INFO("��ȡnvr�豸��ͨ��������Ϣ{},{}", result, dwReturned);

    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << std::endl << "��֧��IPͨ�����߲���ʧ��." << std::endl;
        CONSOLE_COLOR_RESET();
        SPDLOG_ERROR("��֧��IPͨ�����߲���ʧ��{},{}", result, dwReturned);
        return;
    }

    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸" << std::endl;
    std::cout << std::endl << "ע�ᵽ��nvr����Ƶͨ������," << dvrIpParamCfgV40.dwDChanNum << std::endl;
    SPDLOG_INFO("��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸,��Ƶͨ������{}", dvrIpParamCfgV40.dwDChanNum);

    //����ͨ��
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        unsigned channel_no = dvrIpParamCfgV40.dwStartDChan + i;
        if (dvrIpParamCfgV40.struIPDevInfo[i].byEnable)  //ipͨ������
        {
            if (uintSuccessChannel >= uintChannel) {
                break;
            }

            CONSOLE_COLOR_YELLOW();
            std::cout << std::endl << std::endl << "��ͨ��" << channel_no << "�豸Ŀǰ����,��ʼ�����豸��Ƶ������" << std::endl;
            CONSOLE_COLOR_INFO();
            CStructDumpPrinter::PrinterIpDeviceInfoV31(channel_no, &(dvrIpParamCfgV40.struIPDevInfo[i]));
            addIpChannel(channel_no, true);
            SPDLOG_INFO("���豸{}Ŀǰ����,��ʼ�����豸��Ƶ������", channel_no);
        }
        else {
            std::cout << "���豸Ŀǰ������" << std::endl;
            SPDLOG_ERROR("���豸{}Ŀǰ������,��ʼ�����豸��Ƶ������", channel_no);
        }
    }

    Device::onConnected();
}

void DecodeDeviceHK::addAllChannel(bool bMainStream) {
    SPDLOG_INFO("��ʼ��ȡģ��ͨ������, ͨ����:{} ʵ��ͨ����:{}", uintChannel, m_deviceInfo.byChanNum);
    for (int i = 0; i < m_deviceInfo.byChanNum; i++) {
        if (uintSuccessChannel >= uintChannel) break;
        addChannel(m_deviceInfo.byStartChan + i, bMainStream);
    }
}

/// <summary>
/// ����ת��mat�豸����
/// </summary>
DecodeToMatDeviceHK::DecodeToMatDeviceHK() {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("�����豸���յ��쳣��Ϣ,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DecodeToMatDeviceHK::DecodeToMatDeviceHK(unsigned int channel) : Device(channel) {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("�����豸���յ��쳣��Ϣ,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DecodeToMatDeviceHK::~DecodeToMatDeviceHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�" << std::endl;
        SPDLOG_INFO("�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�");
        });
}

bool DecodeToMatDeviceHK::connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut) {
    NET_DVR_USER_LOGIN_INFO loginInfo = { 0 };
    NET_DVR_DEVICEINFO_V40 loginResult = { 0 };

    //login info
    loginInfo.wPort = info.ui16DevPort;
    loginInfo.byUseTransport = 0;
    loginInfo.bUseAsynLogin = 0;
    strcpy(loginInfo.sDeviceAddress, info.strDevIp.c_str());
    strcpy(loginInfo.sUserName, info.strUserName.c_str());
    strcpy(loginInfo.sPassword, info.strPwd.c_str());
    uintFrameCount = info.ui16FrameCount;
    bPrintFrameLog = info.bPrintFrameLog;

    NET_DVR_SetConnectTime(iTimeOut * 1000, 3);
    m_i64LoginId = NET_DVR_Login_V40(&loginInfo, &loginResult);

    if (m_i64LoginId < 0) {
        DWORD _error_code = NET_DVR_GetLastError();
        std::cout << "��¼�豸ʧ��" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("�����豸��¼�豸ʧ��, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
        return false;
    }

    this->onConnected(m_i64LoginId, &loginResult.struDeviceV30);
    return true;
}

void DecodeToMatDeviceHK::disconnect(const relustCB& cb) {
    m_mapChannels.clear();
    if (m_i64LoginId >= 0) {
        NET_DVR_Logout((LONG)m_i64LoginId);
        m_i64LoginId = -1;
        Device::onDisconnected(true);
    }
}

void DecodeToMatDeviceHK::addChannel(int iChn, bool bMainStream) {
    try {
        DevChannel::Ptr channel(new DevDecodeToMatChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount, bPrintFrameLog));
        m_mapChannels[iChn] = channel;
        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "����ģ�����תmat��Ƶͨ��" << iChn << "�ɹ�" << std::endl;
        SPDLOG_INFO("����ģ����Ƶͨ��{}�ɹ�", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("����ģ�����תmat��Ƶͨ��{}����,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "����ģ�����תmat��Ƶͨ������" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeToMatDeviceHK::addIpChannel(int iChn, bool bMainStream) {
    try {
        DevChannel::Ptr channel(new DevIpDecodeToMatChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount, bPrintFrameLog));
        m_mapChannels[iChn] = channel;

        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "����ip����תmat��Ƶͨ��" << iChn << "�ɹ�" << std::endl;
        SPDLOG_INFO("����IP��Ƶͨ��{}�ɹ�", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("����ip����תmat��Ƶͨ��{}����,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "����ip����תmat��Ƶͨ������" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeToMatDeviceHK::delChannel(int chn) {
    m_mapChannels.erase(chn);
}

void DecodeToMatDeviceHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
    m_i64LoginId = lUserID;
    m_deviceInfo = *lpDeviceInfo;
    std::cout << "��ʼ��ȡ��Ƶ��" << std::endl;
    SPDLOG_INFO("��ʼ��ȡ��Ƶ��");
    CStructDumpPrinter::PrinterDeviceInfoV30(lpDeviceInfo);

    // ��Ҫ�ж��Ƿ���nvr�豸  �����nvr�豸�Ļ� ��Ҫȥ��ȡ���õ�ipͨ����Ϣ
    if (lpDeviceInfo->byIPChanNum == 0) {
        //��֧��ipͨ�� ����nvr�豸
        std::cout << std::endl << "���豸����nvr�豸,��֧��ipͨ��,���ģ��ͨ��" << std::endl;
        SPDLOG_INFO("���豸����nvr�豸,��֧��ipͨ��,���ģ��ͨ��");
        addAllChannel(true);  //���Ĭ�ϵ�ģ��ͨ����Ϣ
        return;
    }
    else {
        CONSOLE_COLOR_DEBUG();
        std::cout << "���豸��nvr�豸,֧��ipͨ��,���ipͨ��" << std::endl;
        SPDLOG_INFO("���豸��nvr�豸,֧��ipͨ��,���ipͨ��");
    }

    //ipͨ����>0˵����nvr�豸  ��Ҫͨ�����ú���NET_DVR_GetDVRConfig ��� NET_DVR_GET_IPPARACFG_V40 ��ѯipͨ������
    CONSOLE_COLOR_INFO();
    SPDLOG_INFO("��ʼʹ��NET_DVR_GetDVRConfig��ѯnvr������Ϣ");
    std::cout << "��ʼʹ��NET_DVR_GetDVRConfig��ѯnvr������Ϣ" << m_i64LoginId << std::endl;
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    memset(&dvrIpParamCfgV40, 0, sizeof(NET_DVR_IPPARACFG_V40));

    DWORD dwReturned = 0;
    CONSOLE_COLOR_YELLOW();
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    std::cout << "��ȡnvr�豸��ͨ��������Ϣ," << result << ", " << dwReturned << std::endl;
    SPDLOG_INFO("��ȡnvr�豸��ͨ��������Ϣ{},{}", result, dwReturned);


    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << std::endl << "��֧��IPͨ�����߲���ʧ��." << std::endl;
        CONSOLE_COLOR_RESET();
        SPDLOG_ERROR("��֧��IPͨ�����߲���ʧ��{},{}", result, dwReturned);
        return;
    }

    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸" << std::endl;
    std::cout << std::endl << "ע�ᵽ��nvr����Ƶͨ������," << dvrIpParamCfgV40.dwDChanNum << std::endl;
    SPDLOG_INFO("��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸,��Ƶͨ������{}", dvrIpParamCfgV40.dwDChanNum);

    //����ͨ��
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        unsigned channel_no = dvrIpParamCfgV40.dwStartDChan + i;
        if (dvrIpParamCfgV40.struIPDevInfo[i].byEnable)  //ipͨ������
        {
            if (uintSuccessChannel >= uintChannel) {
                break;
            }

            CONSOLE_COLOR_YELLOW();
            std::cout << std::endl << std::endl << "��ͨ��" << channel_no << "�豸Ŀǰ����,��ʼ�����豸��Ƶ������" << std::endl;
            CONSOLE_COLOR_INFO();
            CStructDumpPrinter::PrinterIpDeviceInfoV31(channel_no, &(dvrIpParamCfgV40.struIPDevInfo[i]));
            addIpChannel(channel_no, true);
            SPDLOG_INFO("���豸{}Ŀǰ����,��ʼ�����豸��Ƶ������", channel_no);
        }
        else {
            std::cout << "���豸Ŀǰ������" << std::endl;
            SPDLOG_ERROR("���豸{}Ŀǰ������,��ʼ�����豸��Ƶ������", channel_no);
        }
    }

    Device::onConnected();
}

void DecodeToMatDeviceHK::addAllChannel(bool bMainStream) {
    SPDLOG_INFO("��ʼ��ȡģ��ͨ������, ͨ����:{} ʵ��ͨ����:{}", uintChannel, m_deviceInfo.byChanNum);
    for (int i = 0; i < m_deviceInfo.byChanNum; i++) {
        if (uintSuccessChannel >= uintChannel) break;
        addChannel(m_deviceInfo.byStartChan + i, bMainStream);
    }
}

/// <summary>
/// �����豸ͨ������
/// </summary>
/// <param name="devName"></param>
/// <param name="channel"></param>
/// <param name="bMainStream"></param>
DevChannel::DevChannel(const char* devName, unsigned int channel, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    m_channel(channel), 
    m_bMainStream(bMainStream),
    m_frameCount(frameCount),
    m_bPrintFrameLog(bPrintFrameLog)
{
    m_devName.append(devName);
}

//���뺯��
void DevChannel::decode(char* outYuv, char* inYv12, int width, int height, int widthStep) {
    int col, row;
    unsigned int Y, U, V;
    int tmp;
    int idx;

    for (row = 0; row < height; row++)
    {
        idx = row * widthStep;
        int rowptr = row * width;

        for (col = 0; col < width; col++)
        {
            //int colhalf=col>>1;
            tmp = (row / 2) * (width / 2) + (col / 2);
            //         if((row==1)&&( col>=1400 &&col<=1600))
            //         { 
            //          printf("col=%d,row=%d,width=%d,tmp=%d.\n",col,row,width,tmp);
            //          printf("row*width+col=%d,width*height+width*height/4+tmp=%d,width*height+tmp=%d.\n",row*width+col,width*height+width*height/4+tmp,width*height+tmp);
            //         } 
            Y = (unsigned int)inYv12[row * width + col];
            U = (unsigned int)inYv12[width * height + width * height / 4 + tmp];
            V = (unsigned int)inYv12[width * height + tmp];
            //         if ((col==200))
            //         { 
            //         printf("col=%d,row=%d,width=%d,tmp=%d.\n",col,row,width,tmp);
            //         printf("width*height+width*height/4+tmp=%d.\n",width*height+width*height/4+tmp);
            //         return ;
            //         }
            if ((idx + col * 3 + 2) > (1200 * widthStep))
            {
                //printf("row * widthStep=%d,idx+col*3+2=%d.\n",1200 * widthStep,idx+col*3+2);
            }
            outYuv[idx + col * 3] = Y;
            outYuv[idx + col * 3 + 1] = U;
            outYuv[idx + col * 3 + 2] = V;
        }
    }
}

DevChannelHK::DevChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog),
    m_i64LoginId(i64LoginId) {
    std::cout << "��ʼ����ģ����Ƶͨ��, ģʽ:������, ͨ����:" << iChn << std::endl;
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.lChannel = iChn; //ͨ����
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // �������ͣ�0-��������1-��������2-����3��3-����4 ���Դ�����
    previewInfo.dwLinkMode = 1; // 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //���Ŵ��ڵľ��,ΪNULL��ʾ������ͼ��
    previewInfo.byProtoType = 0; //Ӧ�ò�ȡ��Э�飬0-˽��Э�飬1-RTSPЭ��
    previewInfo.dwDisplayBufNum = 1; //���ſⲥ�Ż�������󻺳�֡������Χ1-50����0ʱĬ��Ϊ1
    previewInfo.bBlocked = 0;
    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG) m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevChannelHK* self = reinterpret_cast<DevChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "�豸[" << pcDevName << "/" << iChn << "]��ʼʵʱԤ��ʧ��:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevChannelHK::~DevChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "�˳�ģ����Ƶ������ģʽ����,ͨ����:" << m_channel;
    if (m_i64PreviewHandle >= 0) {
        NET_DVR_StopRealPlay((LONG)m_i64PreviewHandle);
        m_i64PreviewHandle = -1;
    }
    if (m_iPlayHandle >= 0) {
        PlayM4_StopSoundShare(m_iPlayHandle);
        PlayM4_Stop(m_iPlayHandle);
        m_iPlayHandle = -1;
    }
}

void DevChannelHK::onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize) {
    //TimeTicker1(-1);
    switch (dwDataType) {
    case NET_DVR_SYSHEAD: { //ϵͳͷ����
        if (!PlayM4_GetPort((long *) &m_iPlayHandle)) {  //��ȡ���ſ�δʹ�õ�ͨ����
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
            std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //����ʵʱ������ģʽ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize, 1024 * 1024)) {  //�����ӿ�
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            PlayM4_SetDecCallBackMend(m_iPlayHandle,
                [](long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, void* nUser, void* nReserved2) {
                    DevChannelHK* chn = reinterpret_cast<DevChannelHK*>(nUser);
                    if (chn->m_iPlayHandle != nPort) {
                        return;
                    }
                    chn->onGetDecData(pBuf, nSize, pFrameInfo);
                }, this);
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //���ſ�ʼ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
            SPDLOG_INFO("ģ����Ƶͨ��[{}]��ȡ��Ƶ�ɹ�", m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //�����ݣ�����������������Ƶ�ֿ�����Ƶ�����ݣ�
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: { 
        //��Ƶ����
    }
        break;
    case NET_DVR_PRIVATE_DATA: { 
        //˽������,����������Ϣ
    }
       break;
    default:
        break;
    }
}

void DevChannelHK::onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo) {
    switch (pFrameInfo->nType) {
    case T_YV12: 
        m_lFrameCount += 1;
        if (m_bPrintFrameLog) {
            SPDLOG_INFO("���յ�����ͷͨ��{}��Ƶ������,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
        break;
    case T_AUDIO16: 
        break;
    default:
        break;
    }
}


//////////////////////////////////////��ͨ����ͷ������ͨ��////////////////////////////////////////////////
DevDecodeChannelHK::DevDecodeChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog),
    m_i64LoginId(i64LoginId) {
    std::cout << "��ʼ����ģ����Ƶͨ��, ģʽ:������, ͨ����:" << iChn << std::endl;
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.lChannel = iChn; //ͨ����
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // �������ͣ�0-��������1-��������2-����3��3-����4 ���Դ�����
    previewInfo.dwLinkMode = 1; // 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //���Ŵ��ڵľ��,ΪNULL��ʾ������ͼ��
    previewInfo.byProtoType = 0; //Ӧ�ò�ȡ��Э�飬0-˽��Э�飬1-RTSPЭ��
    previewInfo.dwDisplayBufNum = 1; //���ſⲥ�Ż�������󻺳�֡������Χ1-50����0ʱĬ��Ϊ1
    previewInfo.bBlocked = 0;
    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevDecodeChannelHK* self = reinterpret_cast<DevDecodeChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "�豸[" << pcDevName << "/" << iChn << "]��ʼʵʱԤ��ʧ��:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevDecodeChannelHK::~DevDecodeChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "�˳�ģ����Ƶ������ģʽ����,ͨ����:" << m_channel;
    if (m_i64PreviewHandle >= 0) {
        NET_DVR_StopRealPlay((LONG)m_i64PreviewHandle);
        m_i64PreviewHandle = -1;
    }
    if (m_iPlayHandle >= 0) {
        PlayM4_StopSoundShare(m_iPlayHandle);
        PlayM4_Stop(m_iPlayHandle);
        m_iPlayHandle = -1;
    }
}

void DevDecodeChannelHK::onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize) {
    //TimeTicker1(-1);
    switch (dwDataType) {
    case NET_DVR_SYSHEAD: { //ϵͳͷ����
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //��ȡ���ſ�δʹ�õ�ͨ����
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
            std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //����ʵʱ������ģʽ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //�����ӿ�
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            PlayM4_SetDecCallBackMend(m_iPlayHandle,
                [](long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, void* nUser, void* nReserved2) {
                    DevDecodeChannelHK* chn = reinterpret_cast<DevDecodeChannelHK*>(nUser);
                    if (chn->m_iPlayHandle != nPort) {
                        return;
                    }
                    chn->onGetDecData(pBuf, nSize, pFrameInfo);
                }, this);
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //���ſ�ʼ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            
            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //�����ݣ�����������������Ƶ�ֿ�����Ƶ�����ݣ�
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //��Ƶ����
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //˽������,����������Ϣ
    }
        break;
    default:
        break;
    }
}

void DevDecodeChannelHK::onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo) {
    switch (pFrameInfo->nType) {
    case T_YV12:
    {
        if (pFrameInfo->dwFrameNum % m_frameCount == 0) {
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//�õ�ͼ���Y����  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//�õ�ȫ��RGBͼ��
            cvReleaseImage(&pImgYCrCb);
            SPDLOG_INFO("ģ����Ƶͨ��{}����ɹ�,֡��:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("���յ�����ͷͨ��{}��Ƶ������,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }
        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

//////////////////////////////////////��ͨ����ͷ����תmat����ͨ��////////////////////////////////////////////////
DevDecodeToMatChannelHK::DevDecodeToMatChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog),
    m_i64LoginId(i64LoginId) {
    std::cout << "��ʼ����ģ����Ƶͨ��, ģʽ:���벢תmat����, ͨ����:" << iChn << std::endl;
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.lChannel = iChn; //ͨ����
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // �������ͣ�0-��������1-��������2-����3��3-����4 ���Դ�����
    previewInfo.dwLinkMode = 1; // 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //���Ŵ��ڵľ��,ΪNULL��ʾ������ͼ��
    previewInfo.byProtoType = 0; //Ӧ�ò�ȡ��Э�飬0-˽��Э�飬1-RTSPЭ��
    previewInfo.dwDisplayBufNum = 1; //���ſⲥ�Ż�������󻺳�֡������Χ1-50����0ʱĬ��Ϊ1
    previewInfo.bBlocked = 0;
    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevDecodeToMatChannelHK* self = reinterpret_cast<DevDecodeToMatChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "�豸[" << pcDevName << "/" << iChn << "]��ʼʵʱԤ��ʧ��:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevDecodeToMatChannelHK::~DevDecodeToMatChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "�˳�ģ����Ƶ����תmatģʽ����,ͨ����:" << m_channel;
    if (m_i64PreviewHandle >= 0) {
        NET_DVR_StopRealPlay((LONG)m_i64PreviewHandle);
        m_i64PreviewHandle = -1;
    }
    if (m_iPlayHandle >= 0) {
        PlayM4_StopSoundShare(m_iPlayHandle);
        PlayM4_Stop(m_iPlayHandle);
        m_iPlayHandle = -1;
    }
}

void DevDecodeToMatChannelHK::onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize) {
    //TimeTicker1(-1);
    switch (dwDataType) {
    case NET_DVR_SYSHEAD: { //ϵͳͷ����
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //��ȡ���ſ�δʹ�õ�ͨ����
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
            std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //����ʵʱ������ģʽ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //�����ӿ�
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            PlayM4_SetDecCallBackMend(m_iPlayHandle,
                [](long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, void* nUser, void* nReserved2) {
                    DevDecodeToMatChannelHK* chn = reinterpret_cast<DevDecodeToMatChannelHK*>(nUser);
                    if (chn->m_iPlayHandle != nPort) {
                        return;
                    }
                    chn->onGetDecData(pBuf, nSize, pFrameInfo);
                }, this);
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //���ſ�ʼ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //�����ݣ�����������������Ƶ�ֿ�����Ƶ�����ݣ�
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ģ����Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ģ����Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
        }
    }
                           break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //��Ƶ����
    }
                                break;
    case NET_DVR_PRIVATE_DATA: {
        //˽������,����������Ϣ
    }
                             break;
    default:
        break;
    }
}

void DevDecodeToMatChannelHK::onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo) {
    switch (pFrameInfo->nType) {
    case T_YV12:
        
    {
        if (pFrameInfo->dwFrameNum % m_frameCount == 0) {
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//�õ�ͼ���Y����  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//�õ�ȫ��RGBͼ��
            IplImage* pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);     //rbg ͼ��
            cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);
            //�õ�img���� ��װmat
            cv::Mat mat = cv::cvarrToMat(pImg, false);
            mat.release();
            cvReleaseImage(&pImgYCrCb);
            cvReleaseImage(&pImg);
            SPDLOG_INFO("ģ����Ƶͨ��{}����תmat�ɹ�,֡��:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("���յ�����ͷͨ��{}��Ƶ������,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }
        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

// nvr����ͨ��
DevIpChannelHK::DevIpChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog), m_i64LoginId(i64LoginId)
{
    NET_DVR_PREVIEWINFO previewInfo;
    std::cout << "��ʼ����IP��Ƶͨ��, ģʽ:������, ͨ����:" << iChn << std::endl;
    previewInfo.lChannel = iChn; //ͨ���� Ŀǰ�豸ģ��ͨ���Ŵ�1��ʼ������ͨ������ʼͨ����ͨ��NET_DVR_GetDVRConfig����������NET_DVR_GET_IPPARACFG_V40����ȡ��dwStartDChan����
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // �������ͣ�0-��������1-��������2-����3��3-����4 ���Դ�����
    previewInfo.dwLinkMode = 1; // 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //���Ŵ��ڵľ��,ΪNULL��ʾ������ͼ��
    previewInfo.byProtoType = 0; //Ӧ�ò�ȡ��Э�飬0-˽��Э�飬1-RTSPЭ��
    previewInfo.dwDisplayBufNum = 1; //���ſⲥ�Ż�������󻺳�֡������Χ1-50����0ʱĬ��Ϊ1
    previewInfo.bBlocked = 0; //0- ������ȡ����1- ����ȡ��
    previewInfo.byDataType = 0; //�������ͣ�0-�������ݣ�1-��Ƶ���� 

    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevIpChannelHK* self = reinterpret_cast<DevIpChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "�豸[" << pcDevName << "/" << iChn << "]��ʼʵʱԤ��ʧ��:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevIpChannelHK::~DevIpChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "�˳�Ip��Ƶͨ��������ģʽ����,ͨ����:" << m_channel;
    if (m_i64PreviewHandle >= 0) {
        NET_DVR_StopRealPlay((LONG)m_i64PreviewHandle);
        m_i64PreviewHandle = -1;
    }
    if (m_iPlayHandle >= 0) {
        PlayM4_StopSoundShare(m_iPlayHandle);
        PlayM4_Stop(m_iPlayHandle);
        m_iPlayHandle = -1;
    }
}

void DevIpChannelHK::onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize) {
    //TimeTicker1(-1);
    switch (dwDataType) {
    case NET_DVR_SYSHEAD: { //ϵͳͷ����
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //��ȡ���ſ�δʹ�õ�ͨ����
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
            std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            CONSOLE_COLOR_RESET();
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //����ʵʱ������ģʽ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //�����ӿ�
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }

            PlayM4_SetDecCallBackMend(m_iPlayHandle,
                [](long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, void* nUser, void* nReserved2) {
                    DevIpChannelHK* chn = reinterpret_cast<DevIpChannelHK*>(nUser);
                    if (chn->m_iPlayHandle != nPort) {
                        return;
                    }
                    chn->onGetDecData(pBuf, nSize, pFrameInfo);
                }, this);
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //���ſ�ʼ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }

            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //�����ݣ�����������������Ƶ�ֿ�����Ƶ�����ݣ�
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //��Ƶ����
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //˽������,����������Ϣ
    }
        break;
    default:
        break;
    }
}

void DevIpChannelHK::onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo) {
    switch (pFrameInfo->nType) {
    case T_YV12:
        m_lFrameCount += 1;
        if (m_bPrintFrameLog) {
            SPDLOG_INFO("���յ�����ͷͨ��{}��Ƶ������,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

/// <summary>
/// nvr����ͨ�� ����ʵ��
/// </summary>
/// <param name="i64LoginId"></param>
/// <param name="pcDevName"></param>
/// <param name="iChn"></param>
/// <param name="bMainStream"></param>
DevIpDecodeChannelHK::DevIpDecodeChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog) , m_i64LoginId(i64LoginId)
{
    NET_DVR_PREVIEWINFO previewInfo;
    std::cout << "��ʼ����IP��Ƶͨ��, ģʽ:������, ͨ����:" << iChn << std::endl;
    previewInfo.lChannel = iChn; //ͨ���� Ŀǰ�豸ģ��ͨ���Ŵ�1��ʼ������ͨ������ʼͨ����ͨ��NET_DVR_GetDVRConfig����������NET_DVR_GET_IPPARACFG_V40����ȡ��dwStartDChan����
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // �������ͣ�0-��������1-��������2-����3��3-����4 ���Դ�����
    previewInfo.dwLinkMode = 1; // 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //���Ŵ��ڵľ��,ΪNULL��ʾ������ͼ��
    previewInfo.byProtoType = 0; //Ӧ�ò�ȡ��Э�飬0-˽��Э�飬1-RTSPЭ��
    previewInfo.dwDisplayBufNum = 1; //���ſⲥ�Ż�������󻺳�֡������Χ1-50����0ʱĬ��Ϊ1
    previewInfo.bBlocked = 0; //0- ������ȡ����1- ����ȡ��
    previewInfo.byDataType = 0; //�������ͣ�0-�������ݣ�1-��Ƶ���� 

    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevIpDecodeChannelHK* self = reinterpret_cast<DevIpDecodeChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "�豸[" << pcDevName << "/" << iChn << "]��ʼʵʱԤ��ʧ��:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevIpDecodeChannelHK::~DevIpDecodeChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "�˳�Ip��Ƶͨ��ת��ģʽ����,ͨ����:" << m_channel;
    if (m_i64PreviewHandle >= 0) {
        NET_DVR_StopRealPlay((LONG)m_i64PreviewHandle);
        m_i64PreviewHandle = -1;
    }
    if (m_iPlayHandle >= 0) {
        PlayM4_StopSoundShare(m_iPlayHandle);
        PlayM4_Stop(m_iPlayHandle);
        m_iPlayHandle = -1;
    }
}

void DevIpDecodeChannelHK::onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize) {
    //TimeTicker1(-1);
    switch (dwDataType) {
    case NET_DVR_SYSHEAD: { //ϵͳͷ����
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //��ȡ���ſ�δʹ�õ�ͨ����
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
            std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            CONSOLE_COLOR_RESET();
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //����ʵʱ������ģʽ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //�����ӿ�
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }

            PlayM4_SetDecCallBackMend(m_iPlayHandle,
                [](long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, void* nUser, void* nReserved2) {
                    DevIpDecodeChannelHK* chn = reinterpret_cast<DevIpDecodeChannelHK*>(nUser);
                    if (chn->m_iPlayHandle != nPort) {
                        return;
                    }
                    chn->onGetDecData(pBuf, nSize, pFrameInfo);
                }, this);
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //���ſ�ʼ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
                        break;
    case NET_DVR_STREAMDATA: { //�����ݣ�����������������Ƶ�ֿ�����Ƶ�����ݣ�
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //��Ƶ����
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //˽������,����������Ϣ
    }
        break;
    default:
        break;
    }
}

void DevIpDecodeChannelHK::onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo) {
    switch (pFrameInfo->nType) {
    case T_YV12:
    {
        if (pFrameInfo->dwFrameNum % m_frameCount == 0) {
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//�õ�ͼ���Y����  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//�õ�ȫ��RGBͼ��
            SPDLOG_INFO("IP��Ƶͨ��{}����ɹ�,֡��:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("���յ�����ͷͨ��{}��Ƶ������,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }

        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

/// <summary>
/// nvr����ͨ�� ����ʵ��
/// </summary>
/// <param name="i64LoginId"></param>
/// <param name="pcDevName"></param>
/// <param name="iChn"></param>
/// <param name="bMainStream"></param>
DevIpDecodeToMatChannelHK::DevIpDecodeToMatChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog):
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog), m_i64LoginId(i64LoginId)
{
    NET_DVR_PREVIEWINFO previewInfo;
    std::cout << "��ʼ����IP��Ƶͨ��, ģʽ:���벢תmat����, ͨ����:" << iChn << std::endl;
    previewInfo.lChannel = iChn; //ͨ���� Ŀǰ�豸ģ��ͨ���Ŵ�1��ʼ������ͨ������ʼͨ����ͨ��NET_DVR_GetDVRConfig����������NET_DVR_GET_IPPARACFG_V40����ȡ��dwStartDChan����
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // �������ͣ�0-��������1-��������2-����3��3-����4 ���Դ�����
    previewInfo.dwLinkMode = 1; // 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //���Ŵ��ڵľ��,ΪNULL��ʾ������ͼ��
    previewInfo.byProtoType = 0; //Ӧ�ò�ȡ��Э�飬0-˽��Э�飬1-RTSPЭ��
    previewInfo.dwDisplayBufNum = 1; //���ſⲥ�Ż�������󻺳�֡������Χ1-50����0ʱĬ��Ϊ1
    previewInfo.bBlocked = 0; //0- ������ȡ����1- ����ȡ��
    previewInfo.byDataType = 0; //�������ͣ�0-�������ݣ�1-��Ƶ���� 

    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevIpDecodeToMatChannelHK* self = reinterpret_cast<DevIpDecodeToMatChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "�豸[" << pcDevName << "/" << iChn << "]��ʼʵʱԤ��ʧ��:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevIpDecodeToMatChannelHK::~DevIpDecodeToMatChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "�˳�Ip��Ƶͨ��ת�벢תmatģʽ����,ͨ����:" << m_channel;
    if (m_i64PreviewHandle >= 0) {
        NET_DVR_StopRealPlay((LONG)m_i64PreviewHandle);
        m_i64PreviewHandle = -1;
    }
    if (m_iPlayHandle >= 0) {
        PlayM4_StopSoundShare(m_iPlayHandle);
        PlayM4_Stop(m_iPlayHandle);
        m_iPlayHandle = -1;
    }
}

void DevIpDecodeToMatChannelHK::onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize) {
    //TimeTicker1(-1);
    switch (dwDataType) {
    case NET_DVR_SYSHEAD: { //ϵͳͷ����
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //��ȡ���ſ�δʹ�õ�ͨ����
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
            std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            CONSOLE_COLOR_RESET();
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //����ʵʱ������ģʽ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //�����ӿ�
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }

            PlayM4_SetDecCallBackMend(m_iPlayHandle,
                [](long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, void* nUser, void* nReserved2) {
                    DevIpDecodeToMatChannelHK* chn = reinterpret_cast<DevIpDecodeToMatChannelHK*>(nUser);
                    if (chn->m_iPlayHandle != nPort) {
                        return;
                    }
                    chn->onGetDecData(pBuf, nSize, pFrameInfo);
                }, this);
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //���ſ�ʼ
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //�����ݣ�����������������Ƶ�ֿ�����Ƶ�����ݣ�
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip��Ƶͨ��[{}]��ȡ��Ƶʧ��, errocde:", m_channel, errcode);
                std::cout << "ip��Ƶͨ��" << m_channel << "��ȡ��Ƶʧ��, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //��Ƶ����
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //˽������,����������Ϣ
    }
        break;
    default:
        break;
    }
}

void DevIpDecodeToMatChannelHK::onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo) {
    switch (pFrameInfo->nType) {
    case T_YV12:
    {
        if (pFrameInfo->dwFrameNum % m_frameCount == 0) {
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//�õ�ͼ���Y����  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//�õ�ȫ��RGBͼ��
            IplImage* pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);     //rbg ͼ��
            cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);
            //�õ�img���� ��װmat
            cv::Mat mat = cv::cvarrToMat(pImg, false);
            mat.release();
            cvReleaseImage(&pImgYCrCb);
            cvReleaseImage(&pImg);
            SPDLOG_INFO("IP��Ƶͨ��{}����תmat�ɹ�,֡��:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("���յ�����ͷͨ��{}��Ƶ������,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }

        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

///////////////////////////////////////��ӡ�豸(��ͨ�豸/nvr�豸)��Ϣ//////////////////////////////////////
DeviceInfoHK::DeviceInfoHK() {
}

DeviceInfoHK::~DeviceInfoHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�" << std::endl;
        SPDLOG_INFO("�Ͽ�����nvr����, ֹͣ��Ƶͨ�����ųɹ�");
        });
}

bool DeviceInfoHK::connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut) {
    NET_DVR_USER_LOGIN_INFO loginInfo = { 0 };
    NET_DVR_DEVICEINFO_V40 loginResult = { 0 };
    connectResult connect_result;
    //login info
    loginInfo.wPort = info.ui16DevPort;
    loginInfo.byUseTransport = 0;
    loginInfo.bUseAsynLogin = 0;
    strcpy(loginInfo.sDeviceAddress, info.strDevIp.c_str());
    strcpy(loginInfo.sUserName, info.strUserName.c_str());
    strcpy(loginInfo.sPassword, info.strPwd.c_str());

    NET_DVR_SetConnectTime(iTimeOut * 1000, 3);
    m_i64LoginId = NET_DVR_Login_V40(&loginInfo, &loginResult);
    if (m_i64LoginId < 0) {
        DWORD _error_code = NET_DVR_GetLastError();
        std::cout << "��¼�豸ʧ��" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("�����豸��¼�豸ʧ��, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
        return false;
    }

    std::cout << "��¼�豸�ɹ�,��ʼ��ѯ�豸��Ϣ" << std::endl << std::endl;
    SPDLOG_INFO("��¼�豸�ɹ�");
    CStructDumpPrinter::PrinterDeviceInfoV40(&loginResult);

    NET_DVR_DEVICEINFO_V30 v30 = loginResult.struDeviceV30;
    CStructDumpPrinter::PrinterDeviceInfoV30(&v30);

    if (v30.byIPChanNum == 0) {
        CONSOLE_COLOR_INFO();
        std::cout << std::endl << std::endl << "���豸��Ƶͨ������:" << (int)v30.byChanNum << std::endl << std::endl;
        return false;
    }

    //ipͨ����>0˵����nvr�豸  ��Ҫͨ�����ú���NET_DVR_GetDVRConfig ��� NET_DVR_GET_IPPARACFG_V40 ��ѯipͨ������
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    DWORD dwReturned = 0;
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);

    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << "��֧��IPͨ�����߲���ʧ��." << std::endl;
        return false;
    }

    CONSOLE_COLOR_INFO();
    std::cout << "��⵽nvr�豸,��ʼ��ѯע�ᵽnvr����Ƶ�豸" << std::endl;
    CONSOLE_COLOR_YELLOW();
    std::cout << std::endl << std::endl << "ע�ᵽ��nvr����Ƶͨ������," << dvrIpParamCfgV40.dwDChanNum << std::endl << std::endl;
 
    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "ע�ᵽ��nvr����Ƶͨ����Ϣ:" << std::endl;

    //����ͨ��
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        CStructDumpPrinter::PrinterIpDeviceInfoV31(i + 1, &(dvrIpParamCfgV40.struIPDevInfo[i]));
    }

    connect_result.strDevName = (char*)(v30.sSerialNumber);
    connect_result.ui16ChnStart = v30.byStartChan;
    connect_result.ui16ChnCount = v30.byChanNum;
    cb(true, connect_result);
    return true;
}

void DeviceInfoHK::disconnect(const relustCB& cb) {
    if (m_i64LoginId >= 0) {
        NET_DVR_Logout((LONG)m_i64LoginId);
        m_i64LoginId = -1;
        Device::onDisconnected(true);
    }
}

void DeviceInfoHK::addChannel(int iChnIndex, bool bMainStream) {
}

void DeviceInfoHK::addIpChannel(int iChnIndex, bool bMainStream ) {
}

void DeviceInfoHK::delChannel(int iChnIndex) {
}

void DeviceInfoHK::addAllChannel(bool bMainStream) {
}

void DeviceInfoHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
}

