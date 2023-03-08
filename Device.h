#ifndef HAIKANG_NVR_TESTING_DEVICE_H_
#define HAIKANG_NVR_TESTING_DEVICE_H_

#include "common_define.h"

#include <functional>
#include <memory>
#include <string>
#include <map>

class connectInfo {
public:
    connectInfo(){}
    connectInfo(std::string _strDevIp,
        uint16_t _ui16DevPort,
        std::string _strUserName,
        std::string _strPwd,
        uint16_t _ui16FrameCount,
        bool _bPrintFrameLog) {
        strDevIp = _strDevIp;
        ui16DevPort = _ui16DevPort;
        strUserName = _strUserName;
        strPwd = _strPwd;
        ui16FrameCount = _ui16FrameCount;
        bPrintFrameLog = _bPrintFrameLog;
    }

    std::string strDevIp;
    uint16_t ui16DevPort;
    std::string strUserName;
    std::string strPwd;
    uint16_t ui16FrameCount;
    bool bPrintFrameLog;
};

class connectResult {
public:
    std::string strDevName;
    uint16_t ui16ChnStart;
    uint16_t ui16ChnCount;
};

//���н��ͳ��
class reportResult {
public:
    int64_t uint64FrameCount;
    int64_t uint64ByteCount;
};

typedef std::function<void(bool success, const connectResult&)> connectCB;
typedef std::function<void(bool success)> relustCB;
//���Խ�����ܻص�����
typedef std::function<void(const reportResult&)> reportCB;

//��Ƶͨ���������� ͨ���ص�����֡��
typedef std::function<void(int64_t frame_count) > updateFrameCB;

class Device : public std::enable_shared_from_this<Device> {
public:
    using Ptr = std::shared_ptr<Device>;
    Device() {
    }

    Device(unsigned int channel) : uintChannel(channel) {}

    virtual ~Device() {
        disconnect([](bool bSuccess) {
        });
    };

    virtual void connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut = 3) = 0;

    virtual void disconnect(const relustCB& cb) {
    }

    virtual void addChannel(int iChnIndex, bool bMainStream = true) = 0;

    //���ipͨ��
    virtual void addIpChannel(int iChnIndex, bool bMainStream = true) = 0;

    virtual void delChannel(int iChnIndex) = 0;

    virtual void addAllChannel(bool bMainStream = true) = 0;

    //��ȡʵ�ʽ�����Ƶ·��
    virtual unsigned int channelCount() {
        return uintSuccessChannel;
    }

    virtual void IncrFrameCount(int64_t frameCount) {
        int64FrameCount += frameCount;
    }

    virtual int64_t FrameCount() const {
        return int64FrameCount;
    }

protected:
    void onConnected() {
    }
    void onDisconnected(bool bSelfDisconnect) {
    }

    void IncrSuccessChannel() {
        uintSuccessChannel += 1;
    }
protected:
    //������Ƶ·��
    unsigned int uintChannel = 10;
    unsigned int uintFrameCount = 8;
    bool bPrintFrameLog = false;

    //����ɹ���Ƶ·��
    unsigned int uintSuccessChannel = 0;
    //����֡��
    int64_t int64FrameCount = 0;
};

class DevChannel;
class DevChannelHK;

//�����豸ʵ��
class DeviceHK : public Device {
public:
    using Ptr = std::shared_ptr<DeviceHK>;
    DeviceHK();
    DeviceHK(unsigned int channel);
    virtual ~DeviceHK();

    void connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut = 3) override;
    void disconnect(const relustCB& cb) override;

    void addChannel(int iChnIndex, bool bMainStream = true) override;
    void addIpChannel(int iChnIndex, bool bMainStream = true) override;
    void delChannel(int iChnIndex) override;
    void addAllChannel(bool bMainStream = true) override;

private:
    void onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo);

private:
    std::map<int, std::shared_ptr<DevChannel> > m_mapChannels;
    int64_t m_i64LoginId = -1;
    NET_DVR_DEVICEINFO_V30 m_deviceInfo;

    reportCB _reportCb;
};

//�����豸���빦��ʵ��
class DecodeDeviceHK : public Device {
public:
    using Ptr = std::shared_ptr<DecodeDeviceHK>;
    DecodeDeviceHK();
    DecodeDeviceHK(unsigned int channel);
    virtual ~DecodeDeviceHK();

    void connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut = 3) override;
    void disconnect(const relustCB& cb) override;

    void addChannel(int iChnIndex, bool bMainStream = true) override;
    void addIpChannel(int iChnIndex, bool bMainStream = true) override;
    void delChannel(int iChnIndex) override;
    void addAllChannel(bool bMainStream = true) override;

private:
    void onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo);

private:
    std::map<int, std::shared_ptr<DevChannel> > m_mapChannels;
    int64_t m_i64LoginId = -1;
    NET_DVR_DEVICEINFO_V30 m_deviceInfo;
};

//�����豸����תmat����ʵ��
class DecodeToMatDeviceHK : public Device {
public:
    using Ptr = std::shared_ptr<DecodeToMatDeviceHK>;
    DecodeToMatDeviceHK();
    DecodeToMatDeviceHK(unsigned int channel);
    virtual ~DecodeToMatDeviceHK();

    void connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut = 3) override;
    void disconnect(const relustCB& cb) override;

    void addChannel(int iChnIndex, bool bMainStream = true) override;
    void addIpChannel(int iChnIndex, bool bMainStream = true) override;
    void delChannel(int iChnIndex) override;
    void addAllChannel(bool bMainStream = true) override;

private:
    void onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo);

private:
    std::map<int, std::shared_ptr<DevChannel> > m_mapChannels;
    int64_t m_i64LoginId = -1;
    NET_DVR_DEVICEINFO_V30 m_deviceInfo;
};


/// <summary>
/// �豸ͨ��  һ������Ƶ����ͨ��
/// </summary>
class DevChannel {
public:
    using Ptr = std::shared_ptr<DevChannel>;

    DevChannel() {}
    ~DevChannel() {
        std::cout << ",�ܹ�����֡��:" << m_lFrameCount << std::endl;
    }

public:
    DevChannel(const char* devName, unsigned int channel, bool bMainStream, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);

public:
    int64_t frameCount() const{
        return m_lFrameCount;
    }

protected:
    void decode(char* outYuv, char* inYv12, int width, int height, int widthStep);

protected:
    std::string m_devName;
    unsigned int m_channel = -1;
    bool m_bMainStream = false;
    unsigned int m_frameCount = 8;
    bool m_bPrintFrameLog = false;
    int64_t m_lFrameCount = 0;
};

//����ͷ�豸ͨ�� ����������
class DevChannelHK : public DevChannel {
public:
    using Ptr = std::shared_ptr<DevChannelHK>;
    DevChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream = true, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);
    virtual ~DevChannelHK();

protected:
    int64_t m_i64LoginId = -1;
    int64_t m_i64PreviewHandle = -1;
    int m_iPlayHandle = -1;
    void onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);
    bool m_bVideoSeted = false;
    bool m_bAudioSeted = false;
};

//��ͨ����ͷ�豸ͨ��  ֻ����ͨ��
class DevDecodeChannelHK : public DevChannel {
public:
    using Ptr = std::shared_ptr<DevDecodeChannelHK>;
    DevDecodeChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream = true, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);
    virtual ~DevDecodeChannelHK();

protected:
    int64_t m_i64LoginId = -1;
    int64_t m_i64PreviewHandle = -1;
    int m_iPlayHandle = -1;
    void onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);
    bool m_bVideoSeted = false;
    bool m_bAudioSeted = false;
};

//��ͨ����ͷ�豸ͨ��  ����תmat����ͨ��
class DevDecodeToMatChannelHK : public DevChannel {
public:
    using Ptr = std::shared_ptr<DevDecodeToMatChannelHK>;
    DevDecodeToMatChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream = true, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);
    virtual ~DevDecodeToMatChannelHK();

protected:
    int64_t m_i64LoginId = -1;
    int64_t m_i64PreviewHandle = -1;
    int m_iPlayHandle = -1;
    void onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);
    bool m_bVideoSeted = false;
    bool m_bAudioSeted = false;


};

//nvr ip ͨ��
class DevIpChannelHK : public DevChannel {
public:
    using Ptr = std::shared_ptr<DevIpChannelHK>;
    DevIpChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream = true, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);
    virtual ~DevIpChannelHK();

protected:
    void onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);

protected:
    int64_t m_i64LoginId = -1;
    int64_t m_i64PreviewHandle = -1;
    int m_iPlayHandle = -1;
};

//nvr ip decodeͨ��
class DevIpDecodeChannelHK : public DevChannel {
public:
    using Ptr = std::shared_ptr<DevIpDecodeChannelHK>;
    DevIpDecodeChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream = true, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);
    virtual ~DevIpDecodeChannelHK();

protected:
    void onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);

protected:
    int64_t m_i64LoginId = -1;
    int64_t m_i64PreviewHandle = -1;
    int m_iPlayHandle = -1;
};

// nvr ip channel  decode and to mat
class DevIpDecodeToMatChannelHK : public DevChannel {
public:
    using Ptr = std::shared_ptr<DevIpDecodeToMatChannelHK>;
    DevIpDecodeToMatChannelHK(int64_t i64LoginId, const char* pcDevName, int iChn, bool bMainStream = true, unsigned int frameCount = 24 * 8, bool bPrintFrameLog = false);
    virtual ~DevIpDecodeToMatChannelHK();

protected:
    void onPreview(DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize);
    void onGetDecData(char* pBuf, int nSize, FRAME_INFO* pFrameInfo);

protected:
    int64_t m_i64LoginId = -1;
    int64_t m_i64PreviewHandle = -1;
    int m_iPlayHandle = -1;
};


//��ӡ�����豸��Ϣʵ���� 
class DeviceInfoHK : public Device {
public:
    using Ptr = std::shared_ptr<DeviceInfoHK>;
    DeviceInfoHK();
    ~DeviceInfoHK();

public:
    void connectDevice(const connectInfo& info, const connectCB& cb, int iTimeOut = 3) override;
    void disconnect(const relustCB& cb) override;

    void addChannel(int iChnIndex, bool bMainStream = true) override;
    void addIpChannel(int iChnIndex, bool bMainStream = true) override;
    void delChannel(int iChnIndex) override;
    void addAllChannel(bool bMainStream = true) override;

private:
    void onConnected(LONG lUserID, LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo);


private:
    int64_t m_i64LoginId = -1;
};

#endif //HAIKANG_NVR_TESTING_DEVICE_H_