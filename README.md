# IPCam SDK

## 1. 概述

ipcam_sdk 旨在为嵌入式摄像头提供 rtmp 推流功能，功能特性如下：

- [x] 支持 RTMP 推流
- [x] 支持 H.264 视频帧
- [x] 支持 AAC/G.711A/G711U 音频帧
- [x] 支持 PCM 16bit 音频数据编码
- [x] 支持 FLV 封包
- [x] 低内存占用

## 2. 代码编译

- 首先安装 autoreconf 和 Libtool 工具
- 配置 Makefile 文件的 `CROSS_PREFIX` ，给出交叉编译工具链
- 在根目录执行 make 命令，编译成功后，静态库文件会位于 lib 目录下

## 3. API 文档和用法

参考 [API Guide](docs/api.md)

## 4. 封包详解

可参考 Adobe 的 [Video File Format Specification Version 10](http://www.adobe.com/content/dam/Adobe/en/devnet/flv/pdfs/video_file_format_spec_v10.pdf)

## 5. 反馈及意见

可以通过在 GitHub 的 repo 提交 issues 来反馈问题，请尽可能的描述清楚遇到的问题，如果有错误信息也一同附带，并且在 Labels 中指明类型为 bug 或者其他。
