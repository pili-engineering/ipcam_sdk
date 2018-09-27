#ifndef DEVSDK_H
#define DEVSDK_H

#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
        DEV_SDK_PROCESS_IPVS = 0,
        DEV_SDK_PROCESS_P2P = 1,
        DEV_SDK_PROCESS_APP = 2,
        DEV_SDK_PROCESS_GB28181 = 3,
}DevSdkServerType;

typedef int (*VIDEO_CALLBACK)(int streamno, char *frame, int len, int iskey, double timestatmp, unsigned long frame_index,      unsigned long keyframe_index, void *pcontext);
typedef int (*AUDIO_CALLBACK)(char *frame, int len, double timestatmp, unsigned long frame_index, void *pcontext);


int dev_sdk_init(DevSdkServerType type);
int dev_sdk_start_video(int camera, int stream, VIDEO_CALLBACK vcb, void *pcontext);
int dev_sdk_start_audio(int camera, int stream, AUDIO_CALLBACK acb, void *pcontext);
int dev_sdk_stop_video(int camera, int stream);
int dev_sdk_stop_audio(int camera, int stream);
int dev_sdk_release(void);
#ifdef  __cplusplus
}
#endif
#endif
