<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [IPCam SDK API Guide](#ipcam-sdk-api-guide)
  - [1. 接口文档](#1-%E6%8E%A5%E5%8F%A3%E6%96%87%E6%A1%A3)
    - [1.1 分配句柄](#11-%E5%88%86%E9%85%8D%E5%8F%A5%E6%9F%84)
    - [1.2 初始化句柄](#12-%E5%88%9D%E5%A7%8B%E5%8C%96%E5%8F%A5%E6%9F%84)
    - [1.3 删除句柄](#13-%E5%88%A0%E9%99%A4%E5%8F%A5%E6%9F%84)
    - [1.4 连接推流服务器](#14-%E8%BF%9E%E6%8E%A5%E6%8E%A8%E6%B5%81%E6%9C%8D%E5%8A%A1%E5%99%A8)
    - [1.5 设置音频时间基准](#15-%E8%AE%BE%E7%BD%AE%E9%9F%B3%E9%A2%91%E6%97%B6%E9%97%B4%E5%9F%BA%E5%87%86)
    - [1.6 设置视频时间基准](#16-%E8%AE%BE%E7%BD%AE%E8%A7%86%E9%A2%91%E6%97%B6%E9%97%B4%E5%9F%BA%E5%87%86)
    - [1.7 设置视频 PPS、SPS、SEI 参数](#17-%E8%AE%BE%E7%BD%AE%E8%A7%86%E9%A2%91-ppsspssei-%E5%8F%82%E6%95%B0)
    - [1.8 发送视频关键帧](#18-%E5%8F%91%E9%80%81%E8%A7%86%E9%A2%91%E5%85%B3%E9%94%AE%E5%B8%A7)
    - [1.9 发送非视频关键帧](#19-%E5%8F%91%E9%80%81%E9%9D%9E%E8%A7%86%E9%A2%91%E5%85%B3%E9%94%AE%E5%B8%A7)
    - [1.10 发送音频数据](#110-%E5%8F%91%E9%80%81%E9%9F%B3%E9%A2%91%E6%95%B0%E6%8D%AE)
    - [1.11 设置 AAC 音频配置数据](#111-%E8%AE%BE%E7%BD%AE-aac-%E9%9F%B3%E9%A2%91%E9%85%8D%E7%BD%AE%E6%95%B0%E6%8D%AE)
  - [2. 使用示例](#2-%E4%BD%BF%E7%94%A8%E7%A4%BA%E4%BE%8B)
    - [2.1 初始化](#21-%E5%88%9D%E5%A7%8B%E5%8C%96)
    - [2.2 发送数据](#22-%E5%8F%91%E9%80%81%E6%95%B0%E6%8D%AE)
      - [2.2.1 发送视频数据](#221-%E5%8F%91%E9%80%81%E8%A7%86%E9%A2%91%E6%95%B0%E6%8D%AE)
      - [2.2.2 发送音频数据](#222-%E5%8F%91%E9%80%81%E9%9F%B3%E9%A2%91%E6%95%B0%E6%8D%AE)
    - [2.3 销毁句柄释放内存](#23-%E9%94%80%E6%AF%81%E5%8F%A5%E6%9F%84%E9%87%8A%E6%94%BE%E5%86%85%E5%AD%98)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# IPCam SDK API Guide

## 1. 接口文档

### 1.1 分配句柄

```c
RtmpPubContext * RtmpPubNew(const char * _url, unsigned int _nTimeout, RtmpPubAudioType _nInputAudioType，RtmpPubAudioType _nOutputAudioType， RtmpPubTimeStampPolicy _nTimePolicy);
```

| 参数名  |描述  |
|----|-----|
| url | RTMP 推流地址 |
| nTimeout | 发送及接收超时时间，单位：秒 |
| nInputAudioType | 音频输入类型，包括：<br/>RTMP_PUB_AUDIO_AAC、RTMP_PUB_AUDIO_G711A、RTMP_PUB_AUDIO_G711U、RTMP_PUB_AUDO_PCM、RTMP_PUB_AUDIO_NONE |
| nOutputAudioType | 值只能和 _nInputAudioType 相同或者 RTMP_PUB_AUDIO_AAC，输出音频不能为RTMP_PUB_AUDO_PCM |
| nTimePolicy | RTMP_PUB_TIMESTAMP_ABSOLUTE：数据包使用绝对时间戳发送；<br/>RTNP_PUB_TIMESTAMP_RELATIVE：数据包尽量使用相对时间戳发送，如果时间发生回转，会发送绝对时间戳进行时间戳同步 |

其中，**RtmpPubAudioType** 的定义：

| 值                   | 描述                  |
| -------------------- | -------------------- |
| RTMP_PUB_AUDIO_AAC   | AAC 音频类型        |
| RTMP_PUB_AUDIO_G711A | G711A 音频类型 |
| RTMP_PUB_AUDIO_G711U | G711U音频类型 |
| RTMP_PUB_AUDIO_PCM | PCM 16位音频类型 |
| RTMP_PUB_AUDIO_NONE | 无音频数据 |

其中，**RtmpPubTimeStampPolicy** 的定义：

| 值                   | 描述                  |
| :------------------- | -------------------- |
| RTMP_PUB_TIMESTAMP_ABSOLUTE | 绝对时间戳   |
| RTMP_PUB_TIMESTAMP_RELATIVE | 相对时间戳 |

**返回值**：

成功返回 **RtmpPubContext** 指针，失败返回NULL

### 1.2 初始化句柄

```c
int RtmpPubInit(RtmpPubContext * _pRtmp);
```

初始化由 RtmpPubNew 分配的 RtmpPubContext * 句柄

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |

**返回值**：

成功返回0，失败返回 -1，失败时，需要调用RtmpPubDel释放资源

**注意**：

输入音频为 G711A/G711U 并且输出音频为 AAC 时，默认会以一下内容初始化句柄：

- 音频采样频率：8000Hz
- 通道数量：1
- 采样位宽：16位

### 1.3 删除句柄

```c
void RtmpPubDel(RtmpPubContext * _pRtmp);
```
删除并释放 RtmpPubContext

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |

### 1.4 连接推流服务器

```c
int RtmpPubConnect(RtmpPubContext * _pRtmp);
```

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpContext 指针 |

**返回值**：

成功返回 0，失败返回 -1

### 1.5 设置音频时间基准

```c
void RtmpPubSetAudioTimebase(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp);
```

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |
| _nTimeStamp | 音频时间基准，和第一个音频包的时间戳相同，后续音频包时间戳在此基准上递增 |

### 1.6 设置视频时间基准

```c
void RtmpPubSetVideoTimebase(RtmpPubContext * _pRtmp, unsigned int _nTimeStamp);
```

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |
| _nTimeStamp | 视频时间基准，和第一个视频包的时间戳相同，后续视频包时间戳在此基准上递增 |

### 1.7 设置视频 PPS、SPS、SEI 参数 

发送 VIDEO 数据之前，必须至少设置 PPS 和 SPS

```c
void RtmpPubSetPps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);
void RtmpPubSetSps(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);
void RtmpPubSetSei(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize);
```

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |
| _pData | PPS/SPS/SEI 数据 |
|_nSize | PPS/SPS/SEI 字节数 |

### 1.8 发送视频关键帧

```c
int RtmpPubSendVideoKeyframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime);
```

发送视频关键帧

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |
| _pData | 关键帧数据 |
|_nSize | 关键帧数据字节数 |
|_presentationTime | pts |

**返回值**：

成功返回 0，失败返回 -1，失败时，表明连接已经失效

### 1.9 发送非视频关键帧

```c
int RtmpPubSendVideoInterframe(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, unsigned int _presentationTime);
```

发送视频非关键帧数据

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |
| _pData | 非关键帧数据 |
|_nSize | 非关键帧数据字节数 |
|_presentationTime | pts |

**返回值**：

成功返回 0，失败返回 -1，失败时，表明连接已经失效

### 1.10 发送音频数据

```c
int RtmpPubSendAudioFrame(RtmpPubContext * _pRtmp, const char * _pData, unsigned int _nSize, int _nPresentationTime);
```

如果输入数据类型为 RTMP_PUB_AUDIO_AAC，发送音频数据前需要先调用 RtmpPubSetAac 发送 AAC 音频配置数据

| 参数名                | 描述                  |
| :------------------- | -------------------- |
| _pRtmp | RtmpPubContext 指针 |
| _pData | 音频数据 |
|_nSize | 音频数据字节数 |
|_presentationTime | pts |

**返回值**：

成功返回 0，失败返回 -1，失败时，表明连接已经失效

### 1.11 设置 AAC 音频配置数据

```c
void RtmpPubSetAac(RtmpPubContext * _pRtmp, const char * _pAacCfgRecord, unsigned int _nSize);
```

如果输入数据类型为 RTMP_PUB_AUDIO_AAC，发送 AAC 数据前，需要先调用此函数设置 AAC Configuration Record

| 参数名         | 描述                |
| :------------- | ------------------- |
| _pRtmp         | RtmpPubContext 指针 |
| _pAacCfgRecord | AAC 配置数据        |
| _nSize         | 配置数据字节数      |


## 2. 使用示例

示例中没有对返回值进行处理，实际使用时需要处理返回值。

### 2.1 初始化
```c
// 分配句柄，读写超时时间为10s，输入音频类型为G711A，输出音频类型为AAC,并使用绝对时间戳策略
RtmpPubContext * pRtmpc = RtmpPubNew(serverUrl, 10, RTMP_PUB_AUDIO_G711A, RTMP_PUB_AUDIO_AAC, RTMP_PUB_TIMESTAMP_ABSOLUTE);
//初始化句柄
RtmpPubInit(pRtmpc);
//连接服务器
RtmpPubConnect(pRtmpc);
```

### 2.2 发送数据

#### 2.2.1 发送视频数据

```c
if (isVideoInit  ==  false) {
	//初始化视频时间基准, presentationTime为第一个视频包时间戳
	RtmpPubSetVideoTimeBase(presentationTime)
 	//设置SPS及PPS参数
 	RtmpPubSetSps(pRtmpc,  spsData, spsDataSize);
 	RtmpPubSetPps(pRtmpc,  ppsData, ppsDataSize);
	isVideoInit = true;
}
if (isKeyFrame == true) {
	//发送关键帧数据
    RtmpPubSendVideoKeyframe(pRtmpc, frameData, frameDataSize,  presentationTime);
}
if (isInterFrame == true) {
	//发送非关键帧数据
	RtmpPubSendVideoKeyframe(pRtmpc, frameData, frameDataSize,  presentationTime);
}
```

#### 2.2.2 发送音频数据

- 输入音频类型为 AAC 

```c
if (isAudioInit == false) {
	//初始化音频时间基准, presentationTime和第一个音频包的时间戳相同
	RtmpPubSetAudioTimebase(pRtmpc, presentationTime);
	//设置AAC配置数据
	RtmpPubSetAac(pRtmpc, pAacCfgRecord, cfgSize);
	isAudioInit = true;
}
//发送音频数据
RtmpPubSendAudioFrame(pRtmpc,  pAacData, aacDataSize,  presentationTime);
```

- 输入音频类型为 G711A、 G711U 或者 PCM 16位

```c
if (isAudioInit == false) {
	//初始化音频时间基准，presentationTime和第一个音频包的时间戳相同
	RtmpPubSetAudioTimebase(pRtmpc, presentationTime);
	isAudioInit = true;
}
//发送音频数据
RtmpPubSendAudioFrame(pRtmpc,  pAudioData,  audioDataSize,  presentationTime);
```

### 2.3 销毁句柄释放内存

```c
//当程序退出、数据发送完成等情况释放资源
RtmpPubDel(pRtmpc)
```

