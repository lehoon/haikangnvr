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
            SPDLOG_ERROR("海康设备接收到异常信息,{}", pAlarmInfo);
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
            SPDLOG_ERROR("海康设备接收到异常信息,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DeviceHK::~DeviceHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "断开海康nvr链接, 停止视频通道播放成功" << std::endl;
        SPDLOG_INFO("断开海康nvr链接, 停止视频通道播放成功");
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
        std::cout << "登录设备失败" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("海康设备登录设备失败, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
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
        std::cout << "接入模拟视频通道" << iChn << "成功" << std::endl;
        SPDLOG_INFO("接入模拟视频通道{}成功", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("接入模拟视频通道{}错误,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "接入模拟视频通道错误" << e.what() << std::endl;
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
        std::cout << "接入IP视频通道" << iChn << "成功" << std::endl;
        SPDLOG_INFO("接入IP视频通道{}成功", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("接入IP视频通道{}错误,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "接入IP视频通道错误," << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DeviceHK::delChannel(int chn) {
    m_mapChannels.erase(chn);
}

void DeviceHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
    m_i64LoginId = lUserID;
    m_deviceInfo = *lpDeviceInfo;
    std::cout << "开始拉取视频流" << std::endl;
    SPDLOG_INFO("开始拉取视频流");
    CStructDumpPrinter::PrinterDeviceInfoV30(lpDeviceInfo);

    // 需要判断是否是nvr设备  如果是nvr设备的话 需要去获取配置的ip通道信息
    if (lpDeviceInfo->byIPChanNum == 0) {
        //不支持ip通道 不是nvr设备
        CONSOLE_COLOR_DEBUG();
        std::cout << std::endl << "该设备不是nvr设备,不支持ip通道,添加模拟通道" << std::endl;
        SPDLOG_INFO("该设备不是nvr设备,不支持ip通道,添加模拟通道");
        addAllChannel(true);  //添加默认的模拟通道信息
        return;
    }
    else {
        CONSOLE_COLOR_DEBUG();
        std::cout << "该设备是nvr设备,支持ip通道,添加ip通道" << std::endl;
        SPDLOG_INFO("该设备是nvr设备,支持ip通道,添加ip通道");
    }

    //ip通道数>0说明是nvr设备  需要通过调用函数NET_DVR_GetDVRConfig 配合 NET_DVR_GET_IPPARACFG_V40 查询ip通道参数
    CONSOLE_COLOR_INFO();
    SPDLOG_INFO("开始使用NET_DVR_GetDVRConfig查询nvr参数信息");
    std::cout << "开始使用NET_DVR_GetDVRConfig查询nvr参数信息" << m_i64LoginId << std::endl;
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    memset(&dvrIpParamCfgV40, 0, sizeof(NET_DVR_IPPARACFG_V40));

    DWORD dwReturned = 0;
    CONSOLE_COLOR_YELLOW(); 
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    std::cout << "获取nvr设备的通道配置信息," << result << ", " << dwReturned << std::endl;
    SPDLOG_INFO("获取nvr设备的通道配置信息{},{}", result, dwReturned);

    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << std::endl << "不支持IP通道或者操作失败." << std::endl;
        CONSOLE_COLOR_RESET();
        SPDLOG_ERROR("不支持IP通道或者操作失败{},{}", result, dwReturned);
        return;
    }

    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "检测到nvr设备,开始查询注册到nvr的视频设备" << std::endl << std::endl;
    std::cout << std::endl << "注册到该nvr的视频通道数量," << dvrIpParamCfgV40.dwDChanNum << std::endl << std::endl;
    SPDLOG_INFO("检测到nvr设备,开始查询注册到nvr的视频设备,视频通道数量{}", dvrIpParamCfgV40.dwDChanNum);

    //数字通道
    unsigned int channel_count = 0;
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        unsigned channel_no = dvrIpParamCfgV40.dwStartDChan + i;
        if (dvrIpParamCfgV40.struIPDevInfo[i].byEnable)  //ip通道在线
        {
            if (uintSuccessChannel >= uintChannel) {
                break;
            }

            CONSOLE_COLOR_YELLOW();
            std::cout << std::endl << std::endl << "该通道"<< channel_no << "设备目前在线,开始接入设备视频流数据" << std::endl;
            CONSOLE_COLOR_INFO();
            CStructDumpPrinter::PrinterIpDeviceInfoV31(channel_no, &(dvrIpParamCfgV40.struIPDevInfo[i]));
            addIpChannel(channel_no, true);
            SPDLOG_INFO("该设备{}目前在线,开始接入设备视频流数据", channel_no);
        }
        else {
            std::cout << "该设备目前不在线" << std::endl;
            SPDLOG_ERROR("该设备{}目前不在线,开始接入设备视频流数据", channel_no);
        }
    }

    Device::onConnected();
}

void DeviceHK::addAllChannel(bool bMainStream) {
    SPDLOG_INFO("开始拉取模拟通道数据, 通道数:{} 实际通道数:{}", uintChannel, m_deviceInfo.byChanNum);
    for (int i = 0; i < m_deviceInfo.byChanNum; i++) {
        if (uintSuccessChannel >= uintChannel) {
            break;
        }

        addChannel(m_deviceInfo.byStartChan + i, bMainStream);
    }
}

/// <summary>
/// 海康转码设备定义
/// </summary>
DecodeDeviceHK::DecodeDeviceHK() {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("海康设备接收到异常信息,{}", pAlarmInfo);
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
            SPDLOG_ERROR("海康设备接收到异常信息,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DecodeDeviceHK::~DecodeDeviceHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "断开海康nvr链接, 停止视频通道播放成功" << std::endl;
        SPDLOG_INFO("断开海康nvr链接, 停止视频通道播放成功");
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
        std::cout << "登录设备失败" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("海康设备登录设备失败, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
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
        std::cout << "接入模拟解码视频通道" << iChn << "成功" << std::endl;
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("接入模拟解码视频通道{}错误,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "接入模拟解码视频通道错误" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeDeviceHK::addIpChannel(int iChn, bool bMainStream) {
    try {
        DevChannel::Ptr channel(new DevIpDecodeChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount, bPrintFrameLog));
        m_mapChannels[iChn] = channel;
        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "接入IP解码视频通道" << iChn << "成功" << std::endl;
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("接入ip解码视频通道{}错误,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "接入ip解码视频通道错误" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeDeviceHK::delChannel(int chn) {
    m_mapChannels.erase(chn);
}

void DecodeDeviceHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
    m_i64LoginId = lUserID;
    m_deviceInfo = *lpDeviceInfo;

    std::cout << "开始注册解码视频通道" << std::endl;
    SPDLOG_INFO("开始注册解码视频通道");
    CStructDumpPrinter::PrinterDeviceInfoV30(lpDeviceInfo);

    // 需要判断是否是nvr设备  如果是nvr设备的话 需要去获取配置的ip通道信息
    if (lpDeviceInfo->byIPChanNum == 0) {
        //不支持ip通道 不是nvr设备
        std::cout << std::endl << "该设备不是nvr设备,不支持ip通道,添加模拟通道" << std::endl;
        SPDLOG_INFO("该设备不是nvr设备,不支持ip通道,添加模拟通道");
        addAllChannel(true);  //添加默认的模拟通道信息
        return;
    }
    else {
        CONSOLE_COLOR_DEBUG();
        std::cout << "该设备是nvr设备,支持ip通道,添加ip通道" << std::endl;
        SPDLOG_INFO("该设备是nvr设备,支持ip通道,添加ip通道");
    }

    //ip通道数>0说明是nvr设备  需要通过调用函数NET_DVR_GetDVRConfig 配合 NET_DVR_GET_IPPARACFG_V40 查询ip通道参数
    CONSOLE_COLOR_INFO();
    SPDLOG_INFO("开始使用NET_DVR_GetDVRConfig查询nvr参数信息");
    std::cout << "开始使用NET_DVR_GetDVRConfig查询nvr参数信息" << m_i64LoginId << std::endl;
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    memset(&dvrIpParamCfgV40, 0, sizeof(NET_DVR_IPPARACFG_V40));

    DWORD dwReturned = 0;
    CONSOLE_COLOR_YELLOW();
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    std::cout << "获取nvr设备的通道配置信息," << result << ", " << dwReturned << std::endl;
    SPDLOG_INFO("获取nvr设备的通道配置信息{},{}", result, dwReturned);

    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << std::endl << "不支持IP通道或者操作失败." << std::endl;
        CONSOLE_COLOR_RESET();
        SPDLOG_ERROR("不支持IP通道或者操作失败{},{}", result, dwReturned);
        return;
    }

    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "检测到nvr设备,开始查询注册到nvr的视频设备" << std::endl;
    std::cout << std::endl << "注册到该nvr的视频通道数量," << dvrIpParamCfgV40.dwDChanNum << std::endl;
    SPDLOG_INFO("检测到nvr设备,开始查询注册到nvr的视频设备,视频通道数量{}", dvrIpParamCfgV40.dwDChanNum);

    //数字通道
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        unsigned channel_no = dvrIpParamCfgV40.dwStartDChan + i;
        if (dvrIpParamCfgV40.struIPDevInfo[i].byEnable)  //ip通道在线
        {
            if (uintSuccessChannel >= uintChannel) {
                break;
            }

            CONSOLE_COLOR_YELLOW();
            std::cout << std::endl << std::endl << "该通道" << channel_no << "设备目前在线,开始接入设备视频流数据" << std::endl;
            CONSOLE_COLOR_INFO();
            CStructDumpPrinter::PrinterIpDeviceInfoV31(channel_no, &(dvrIpParamCfgV40.struIPDevInfo[i]));
            addIpChannel(channel_no, true);
            SPDLOG_INFO("该设备{}目前在线,开始接入设备视频流数据", channel_no);
        }
        else {
            std::cout << "该设备目前不在线" << std::endl;
            SPDLOG_ERROR("该设备{}目前不在线,开始接入设备视频流数据", channel_no);
        }
    }

    Device::onConnected();
}

void DecodeDeviceHK::addAllChannel(bool bMainStream) {
    SPDLOG_INFO("开始拉取模拟通道数据, 通道数:{} 实际通道数:{}", uintChannel, m_deviceInfo.byChanNum);
    for (int i = 0; i < m_deviceInfo.byChanNum; i++) {
        if (uintSuccessChannel >= uintChannel) break;
        addChannel(m_deviceInfo.byStartChan + i, bMainStream);
    }
}

/// <summary>
/// 海康转码mat设备定义
/// </summary>
DecodeToMatDeviceHK::DecodeToMatDeviceHK() {
    static onceToken token([]() {
        NET_DVR_Init();
        NET_DVR_SetDVRMessageCallBack_V31([](LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser) {
            CONSOLE_COLOR_ERROR();
            std::cout << pAlarmInfo;
            SPDLOG_ERROR("海康设备接收到异常信息,{}", pAlarmInfo);
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
            SPDLOG_ERROR("海康设备接收到异常信息,{}", pAlarmInfo);
            return TRUE;
            }, NULL);
        }, []() {
            NET_DVR_Cleanup();
        });
}

DecodeToMatDeviceHK::~DecodeToMatDeviceHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "断开海康nvr链接, 停止视频通道播放成功" << std::endl;
        SPDLOG_INFO("断开海康nvr链接, 停止视频通道播放成功");
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
        std::cout << "登录设备失败" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("海康设备登录设备失败, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
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
        std::cout << "接入模拟解码转mat视频通道" << iChn << "成功" << std::endl;
        SPDLOG_INFO("接入模拟视频通道{}成功", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("接入模拟解码转mat视频通道{}错误,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "接入模拟解码转mat视频通道错误" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeToMatDeviceHK::addIpChannel(int iChn, bool bMainStream) {
    try {
        DevChannel::Ptr channel(new DevIpDecodeToMatChannelHK(m_i64LoginId, (char*)m_deviceInfo.sSerialNumber, iChn, bMainStream, uintFrameCount, bPrintFrameLog));
        m_mapChannels[iChn] = channel;

        IncrSuccessChannel();
        CONSOLE_COLOR_YELLOW();
        std::cout << "接入ip解码转mat视频通道" << iChn << "成功" << std::endl;
        SPDLOG_INFO("接入IP视频通道{}成功", iChn);
    }
    catch (std::runtime_error e) {
        SPDLOG_ERROR("接入ip解码转mat视频通道{}错误,{}", iChn, e.what());
        CONSOLE_COLOR_ERROR();
        std::cout << "接入ip解码转mat视频通道错误" << e.what() << std::endl;
        CONSOLE_COLOR_RESET();
    }
}

void DecodeToMatDeviceHK::delChannel(int chn) {
    m_mapChannels.erase(chn);
}

void DecodeToMatDeviceHK::onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo) {
    m_i64LoginId = lUserID;
    m_deviceInfo = *lpDeviceInfo;
    std::cout << "开始拉取视频流" << std::endl;
    SPDLOG_INFO("开始拉取视频流");
    CStructDumpPrinter::PrinterDeviceInfoV30(lpDeviceInfo);

    // 需要判断是否是nvr设备  如果是nvr设备的话 需要去获取配置的ip通道信息
    if (lpDeviceInfo->byIPChanNum == 0) {
        //不支持ip通道 不是nvr设备
        std::cout << std::endl << "该设备不是nvr设备,不支持ip通道,添加模拟通道" << std::endl;
        SPDLOG_INFO("该设备不是nvr设备,不支持ip通道,添加模拟通道");
        addAllChannel(true);  //添加默认的模拟通道信息
        return;
    }
    else {
        CONSOLE_COLOR_DEBUG();
        std::cout << "该设备是nvr设备,支持ip通道,添加ip通道" << std::endl;
        SPDLOG_INFO("该设备是nvr设备,支持ip通道,添加ip通道");
    }

    //ip通道数>0说明是nvr设备  需要通过调用函数NET_DVR_GetDVRConfig 配合 NET_DVR_GET_IPPARACFG_V40 查询ip通道参数
    CONSOLE_COLOR_INFO();
    SPDLOG_INFO("开始使用NET_DVR_GetDVRConfig查询nvr参数信息");
    std::cout << "开始使用NET_DVR_GetDVRConfig查询nvr参数信息" << m_i64LoginId << std::endl;
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    memset(&dvrIpParamCfgV40, 0, sizeof(NET_DVR_IPPARACFG_V40));

    DWORD dwReturned = 0;
    CONSOLE_COLOR_YELLOW();
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);
    std::cout << "获取nvr设备的通道配置信息," << result << ", " << dwReturned << std::endl;
    SPDLOG_INFO("获取nvr设备的通道配置信息{},{}", result, dwReturned);


    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << std::endl << "不支持IP通道或者操作失败." << std::endl;
        CONSOLE_COLOR_RESET();
        SPDLOG_ERROR("不支持IP通道或者操作失败{},{}", result, dwReturned);
        return;
    }

    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "检测到nvr设备,开始查询注册到nvr的视频设备" << std::endl;
    std::cout << std::endl << "注册到该nvr的视频通道数量," << dvrIpParamCfgV40.dwDChanNum << std::endl;
    SPDLOG_INFO("检测到nvr设备,开始查询注册到nvr的视频设备,视频通道数量{}", dvrIpParamCfgV40.dwDChanNum);

    //数字通道
    for (unsigned int i = 0; i < dvrIpParamCfgV40.dwDChanNum; i++)
    {
        unsigned channel_no = dvrIpParamCfgV40.dwStartDChan + i;
        if (dvrIpParamCfgV40.struIPDevInfo[i].byEnable)  //ip通道在线
        {
            if (uintSuccessChannel >= uintChannel) {
                break;
            }

            CONSOLE_COLOR_YELLOW();
            std::cout << std::endl << std::endl << "该通道" << channel_no << "设备目前在线,开始接入设备视频流数据" << std::endl;
            CONSOLE_COLOR_INFO();
            CStructDumpPrinter::PrinterIpDeviceInfoV31(channel_no, &(dvrIpParamCfgV40.struIPDevInfo[i]));
            addIpChannel(channel_no, true);
            SPDLOG_INFO("该设备{}目前在线,开始接入设备视频流数据", channel_no);
        }
        else {
            std::cout << "该设备目前不在线" << std::endl;
            SPDLOG_ERROR("该设备{}目前不在线,开始接入设备视频流数据", channel_no);
        }
    }

    Device::onConnected();
}

void DecodeToMatDeviceHK::addAllChannel(bool bMainStream) {
    SPDLOG_INFO("开始拉取模拟通道数据, 通道数:{} 实际通道数:{}", uintChannel, m_deviceInfo.byChanNum);
    for (int i = 0; i < m_deviceInfo.byChanNum; i++) {
        if (uintSuccessChannel >= uintChannel) break;
        addChannel(m_deviceInfo.byStartChan + i, bMainStream);
    }
}

/// <summary>
/// 海康设备通道定义
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

//解码函数
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
    std::cout << "开始接入模拟视频通道, 模式:仅接入, 通道号:" << iChn << std::endl;
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.lChannel = iChn; //通道号
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    previewInfo.dwLinkMode = 1; // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //播放窗口的句柄,为NULL表示不播放图象
    previewInfo.byProtoType = 0; //应用层取流协议，0-私有协议，1-RTSP协议
    previewInfo.dwDisplayBufNum = 1; //播放库播放缓冲区最大缓冲帧数，范围1-50，置0时默认为1
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
        throw std::runtime_error(StrPrinter << "设备[" << pcDevName << "/" << iChn << "]开始实时预览失败:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevChannelHK::~DevChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "退出模拟视频仅接入模式测试,通道号:" << m_channel;
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
    case NET_DVR_SYSHEAD: { //系统头数据
        if (!PlayM4_GetPort((long *) &m_iPlayHandle)) {  //获取播放库未使用的通道号
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
            std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //设置实时流播放模式
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize, 1024 * 1024)) {  //打开流接口
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
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
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //播放开始
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
            SPDLOG_INFO("模拟视频通道[{}]拉取视频成功", m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //流数据（包括复合流或音视频分开的视频流数据）
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: { 
        //音频数据
    }
        break;
    case NET_DVR_PRIVATE_DATA: { 
        //私有数据,包括智能信息
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
            SPDLOG_INFO("接收到摄像头通道{}视频流数据,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
        break;
    case T_AUDIO16: 
        break;
    default:
        break;
    }
}


//////////////////////////////////////普通摄像头仅解码通道////////////////////////////////////////////////
DevDecodeChannelHK::DevDecodeChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog),
    m_i64LoginId(i64LoginId) {
    std::cout << "开始接入模拟视频通道, 模式:仅解码, 通道号:" << iChn << std::endl;
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.lChannel = iChn; //通道号
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    previewInfo.dwLinkMode = 1; // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //播放窗口的句柄,为NULL表示不播放图象
    previewInfo.byProtoType = 0; //应用层取流协议，0-私有协议，1-RTSP协议
    previewInfo.dwDisplayBufNum = 1; //播放库播放缓冲区最大缓冲帧数，范围1-50，置0时默认为1
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
        throw std::runtime_error(StrPrinter << "设备[" << pcDevName << "/" << iChn << "]开始实时预览失败:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevDecodeChannelHK::~DevDecodeChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "退出模拟视频仅解码模式测试,通道号:" << m_channel;
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
    case NET_DVR_SYSHEAD: { //系统头数据
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //获取播放库未使用的通道号
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
            std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //设置实时流播放模式
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //打开流接口
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
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
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //播放开始
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            
            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //流数据（包括复合流或音视频分开的视频流数据）
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //音频数据
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //私有数据,包括智能信息
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
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//得到图像的Y分量  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//得到全部RGB图像
            cvReleaseImage(&pImgYCrCb);
            SPDLOG_INFO("模拟视频通道{}解码成功,帧号:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("接收到摄像头通道{}视频流数据,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }
        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

//////////////////////////////////////普通摄像头解码转mat对象通道////////////////////////////////////////////////
DevDecodeToMatChannelHK::DevDecodeToMatChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog),
    m_i64LoginId(i64LoginId) {
    std::cout << "开始接入模拟视频通道, 模式:解码并转mat对象, 通道号:" << iChn << std::endl;
    NET_DVR_PREVIEWINFO previewInfo;
    previewInfo.lChannel = iChn; //通道号
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    previewInfo.dwLinkMode = 1; // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //播放窗口的句柄,为NULL表示不播放图象
    previewInfo.byProtoType = 0; //应用层取流协议，0-私有协议，1-RTSP协议
    previewInfo.dwDisplayBufNum = 1; //播放库播放缓冲区最大缓冲帧数，范围1-50，置0时默认为1
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
        throw std::runtime_error(StrPrinter << "设备[" << pcDevName << "/" << iChn << "]开始实时预览失败:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevDecodeToMatChannelHK::~DevDecodeToMatChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "退出模拟视频解码转mat模式测试,通道号:" << m_channel;
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
    case NET_DVR_SYSHEAD: { //系统头数据
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //获取播放库未使用的通道号
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
            std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //设置实时流播放模式
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //打开流接口
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
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
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //播放开始
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }

            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //流数据（包括复合流或音视频分开的视频流数据）
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("模拟视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "模拟视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                break;
            }
        }
    }
                           break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //音频数据
    }
                                break;
    case NET_DVR_PRIVATE_DATA: {
        //私有数据,包括智能信息
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
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//得到图像的Y分量  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//得到全部RGB图像
            IplImage* pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);     //rbg 图像
            cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);
            //拿到img对象 组装mat
            cv::Mat mat = cv::cvarrToMat(pImg, false);
            mat.release();
            cvReleaseImage(&pImgYCrCb);
            cvReleaseImage(&pImg);
            SPDLOG_INFO("模拟视频通道{}解码转mat成功,帧号:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("接收到摄像头通道{}视频流数据,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }
        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

// nvr数字通道
DevIpChannelHK::DevIpChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog), m_i64LoginId(i64LoginId)
{
    NET_DVR_PREVIEWINFO previewInfo;
    std::cout << "开始接入IP视频通道, 模式:仅接入, 通道号:" << iChn << std::endl;
    previewInfo.lChannel = iChn; //通道号 目前设备模拟通道号从1开始，数字通道的起始通道号通过NET_DVR_GetDVRConfig（配置命令NET_DVR_GET_IPPARACFG_V40）获取（dwStartDChan）。
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    previewInfo.dwLinkMode = 1; // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //播放窗口的句柄,为NULL表示不播放图象
    previewInfo.byProtoType = 0; //应用层取流协议，0-私有协议，1-RTSP协议
    previewInfo.dwDisplayBufNum = 1; //播放库播放缓冲区最大缓冲帧数，范围1-50，置0时默认为1
    previewInfo.bBlocked = 0; //0- 非阻塞取流，1- 阻塞取流
    previewInfo.byDataType = 0; //数据类型：0-码流数据，1-音频数据 

    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevIpChannelHK* self = reinterpret_cast<DevIpChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "设备[" << pcDevName << "/" << iChn << "]开始实时预览失败:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevIpChannelHK::~DevIpChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "退出Ip视频通道仅接入模式测试,通道号:" << m_channel;
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
    case NET_DVR_SYSHEAD: { //系统头数据
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //获取播放库未使用的通道号
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
            std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            CONSOLE_COLOR_RESET();
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //设置实时流播放模式
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //打开流接口
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
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
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //播放开始
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }

            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //流数据（包括复合流或音视频分开的视频流数据）
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //音频数据
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //私有数据,包括智能信息
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
            SPDLOG_INFO("接收到摄像头通道{}视频流数据,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

/// <summary>
/// nvr数字通道 解码实现
/// </summary>
/// <param name="i64LoginId"></param>
/// <param name="pcDevName"></param>
/// <param name="iChn"></param>
/// <param name="bMainStream"></param>
DevIpDecodeChannelHK::DevIpDecodeChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog) :
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog) , m_i64LoginId(i64LoginId)
{
    NET_DVR_PREVIEWINFO previewInfo;
    std::cout << "开始接入IP视频通道, 模式:仅解码, 通道号:" << iChn << std::endl;
    previewInfo.lChannel = iChn; //通道号 目前设备模拟通道号从1开始，数字通道的起始通道号通过NET_DVR_GetDVRConfig（配置命令NET_DVR_GET_IPPARACFG_V40）获取（dwStartDChan）。
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    previewInfo.dwLinkMode = 1; // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //播放窗口的句柄,为NULL表示不播放图象
    previewInfo.byProtoType = 0; //应用层取流协议，0-私有协议，1-RTSP协议
    previewInfo.dwDisplayBufNum = 1; //播放库播放缓冲区最大缓冲帧数，范围1-50，置0时默认为1
    previewInfo.bBlocked = 0; //0- 非阻塞取流，1- 阻塞取流
    previewInfo.byDataType = 0; //数据类型：0-码流数据，1-音频数据 

    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevIpDecodeChannelHK* self = reinterpret_cast<DevIpDecodeChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "设备[" << pcDevName << "/" << iChn << "]开始实时预览失败:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevIpDecodeChannelHK::~DevIpDecodeChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "退出Ip视频通道转码模式测试,通道号:" << m_channel;
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
    case NET_DVR_SYSHEAD: { //系统头数据
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //获取播放库未使用的通道号
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
            std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            CONSOLE_COLOR_RESET();
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //设置实时流播放模式
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //打开流接口
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
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
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //播放开始
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
                        break;
    case NET_DVR_STREAMDATA: { //流数据（包括复合流或音视频分开的视频流数据）
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //音频数据
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //私有数据,包括智能信息
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
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//得到图像的Y分量  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//得到全部RGB图像
            SPDLOG_INFO("IP视频通道{}解码成功,帧号:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("接收到摄像头通道{}视频流数据,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
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
/// nvr数字通道 解码实现
/// </summary>
/// <param name="i64LoginId"></param>
/// <param name="pcDevName"></param>
/// <param name="iChn"></param>
/// <param name="bMainStream"></param>
DevIpDecodeToMatChannelHK::DevIpDecodeToMatChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream, unsigned int frameCount, bool bPrintFrameLog):
    DevChannel(pcDevName, iChn, bMainStream, frameCount, bPrintFrameLog), m_i64LoginId(i64LoginId)
{
    NET_DVR_PREVIEWINFO previewInfo;
    std::cout << "开始接入IP视频通道, 模式:解码并转mat对象, 通道号:" << iChn << std::endl;
    previewInfo.lChannel = iChn; //通道号 目前设备模拟通道号从1开始，数字通道的起始通道号通过NET_DVR_GetDVRConfig（配置命令NET_DVR_GET_IPPARACFG_V40）获取（dwStartDChan）。
    previewInfo.dwStreamType = bMainStream ? 0 : 1; // 码流类型，0-主码流，1-子码流，2-码流3，3-码流4 等以此类推
    previewInfo.dwLinkMode = 1; // 0：TCP方式,1：UDP方式,2：多播方式,3 - RTP方式，4-RTP/RTSP,5-RSTP/HTTP
    previewInfo.hPlayWnd = 0; //播放窗口的句柄,为NULL表示不播放图象
    previewInfo.byProtoType = 0; //应用层取流协议，0-私有协议，1-RTSP协议
    previewInfo.dwDisplayBufNum = 1; //播放库播放缓冲区最大缓冲帧数，范围1-50，置0时默认为1
    previewInfo.bBlocked = 0; //0- 非阻塞取流，1- 阻塞取流
    previewInfo.byDataType = 0; //数据类型：0-码流数据，1-音频数据 

    m_i64PreviewHandle = NET_DVR_RealPlay_V40((LONG)m_i64LoginId, &previewInfo,
        [](LONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser) {
            DevIpDecodeToMatChannelHK* self = reinterpret_cast<DevIpDecodeToMatChannelHK*>(pUser);
            if (self->m_i64PreviewHandle != (int64_t)lPlayHandle) {
                return;
            }
            self->onPreview(dwDataType, pBuffer, dwBufSize);
        }, this);
    if (m_i64PreviewHandle == -1) {
        throw std::runtime_error(StrPrinter << "设备[" << pcDevName << "/" << iChn << "]开始实时预览失败:"
            << NET_DVR_GetLastError() << std::endl);
    }
}

DevIpDecodeToMatChannelHK::~DevIpDecodeToMatChannelHK() {
    CONSOLE_COLOR_INFO();
    std::cout << "退出Ip视频通道转码并转mat模式测试,通道号:" << m_channel;
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
    case NET_DVR_SYSHEAD: { //系统头数据
        if (!PlayM4_GetPort((long*)&m_iPlayHandle)) {  //获取播放库未使用的通道号
            CONSOLE_COLOR_ERROR();
            DWORD errcode = NET_DVR_GetLastError();
            SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
            std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
            CONSOLE_COLOR_RESET();
            break;
        }
        if (dwBufSize > 0) {
            if (!PlayM4_SetStreamOpenMode(m_iPlayHandle, STREAME_REALTIME)) { //设置实时流播放模式
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            if (!PlayM4_OpenStream(m_iPlayHandle, pBuffer, dwBufSize,
                1024 * 1024)) {  //打开流接口
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
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
            if (!PlayM4_Play(m_iPlayHandle, 0)) {  //播放开始
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
            CStructDumpPrinter::PrinterPullStreamSuccess(m_channel);
        }
    }
        break;
    case NET_DVR_STREAMDATA: { //流数据（包括复合流或音视频分开的视频流数据）
        if (dwBufSize > 0 && m_iPlayHandle != -1) {
            if (!PlayM4_InputData(m_iPlayHandle, pBuffer, dwBufSize)) {
                CONSOLE_COLOR_ERROR();
                DWORD errcode = NET_DVR_GetLastError();
                SPDLOG_ERROR("ip视频通道[{}]拉取视频失败, errocde:", m_channel, errcode);
                std::cout << "ip视频通道" << m_channel << "拉取视频失败, errocde:" << errcode << ",message:" << NET_DVR_GetErrorMsg((LONG*)&errcode) << std::endl;
                CONSOLE_COLOR_RESET();
                break;
            }
        }
    }
        break;
    case NET_DVR_AUDIOSTREAMDATA: {
        //音频数据
    }
        break;
    case NET_DVR_PRIVATE_DATA: {
        //私有数据,包括智能信息
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
            IplImage* pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);//得到图像的Y分量  
            DevChannel::decode(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep);//得到全部RGB图像
            IplImage* pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);     //rbg 图像
            cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);
            //拿到img对象 组装mat
            cv::Mat mat = cv::cvarrToMat(pImg, false);
            mat.release();
            cvReleaseImage(&pImgYCrCb);
            cvReleaseImage(&pImg);
            SPDLOG_INFO("IP视频通道{}解码转mat成功,帧号:{}", m_channel, pFrameInfo->dwFrameNum);
            m_lFrameCount += 1;
        }

        if (m_bPrintFrameLog) {
            SPDLOG_INFO("接收到摄像头通道{}视频流数据,width={}, height={}, frameNum={},frameRate={}", m_channel, pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->dwFrameNum, pFrameInfo->nFrameRate);
        }
    }

        break;
    case T_AUDIO16:
        break;
    default:
        break;
    }
}

///////////////////////////////////////打印设备(普通设备/nvr设备)信息//////////////////////////////////////
DeviceInfoHK::DeviceInfoHK() {
}

DeviceInfoHK::~DeviceInfoHK() {
    disconnect([&](bool success) {
        CONSOLE_COLOR_DEBUG();
        std::cout << "断开海康nvr链接, 停止视频通道播放成功" << std::endl;
        SPDLOG_INFO("断开海康nvr链接, 停止视频通道播放成功");
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
        std::cout << "登录设备失败" << NET_DVR_GetErrorMsg((LONG*)&_error_code) << std::endl;
        SPDLOG_ERROR("海康设备登录设备失败, errcode={}, message={}", _error_code, NET_DVR_GetErrorMsg((LONG*)&_error_code));
        return false;
    }

    std::cout << "登录设备成功,开始查询设备信息" << std::endl << std::endl;
    SPDLOG_INFO("登录设备成功");
    CStructDumpPrinter::PrinterDeviceInfoV40(&loginResult);

    NET_DVR_DEVICEINFO_V30 v30 = loginResult.struDeviceV30;
    CStructDumpPrinter::PrinterDeviceInfoV30(&v30);

    if (v30.byIPChanNum == 0) {
        CONSOLE_COLOR_INFO();
        std::cout << std::endl << std::endl << "该设备视频通道数量:" << (int)v30.byChanNum << std::endl << std::endl;
        return false;
    }

    //ip通道数>0说明是nvr设备  需要通过调用函数NET_DVR_GetDVRConfig 配合 NET_DVR_GET_IPPARACFG_V40 查询ip通道参数
    NET_DVR_IPPARACFG_V40 dvrIpParamCfgV40 = { 0 };
    DWORD dwReturned = 0;
    BOOL result = NET_DVR_GetDVRConfig(m_i64LoginId, NET_DVR_GET_IPPARACFG_V40, 0, &dvrIpParamCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned);

    if (!result) {
        CONSOLE_COLOR_ERROR();
        std::cout << "不支持IP通道或者操作失败." << std::endl;
        return false;
    }

    CONSOLE_COLOR_INFO();
    std::cout << "检测到nvr设备,开始查询注册到nvr的视频设备" << std::endl;
    CONSOLE_COLOR_YELLOW();
    std::cout << std::endl << std::endl << "注册到该nvr的视频通道数量," << dvrIpParamCfgV40.dwDChanNum << std::endl << std::endl;
 
    CONSOLE_COLOR_INFO();
    std::cout << std::endl << "注册到该nvr的视频通道信息:" << std::endl;

    //数字通道
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

