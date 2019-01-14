#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "QnCommon.h"
#include "qnRtmp.h"
#include "devsdk.h"

//#define DEFAULT_H265 1

int main(int argc, char **argv)
{
        int sig;
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGALRM);
        sigaddset(&set, SIGPIPE);
        sigaddset(&set, SIGUSR1);
        sigaddset(&set, SIGUSR2);
        pthread_sigmask(SIG_BLOCK, &set, NULL);

       
        if (argc >= 2) {
                if (argc >= 3) {
                        //init cam
                        dev_sdk_init(DEV_SDK_PROCESS_APP);
                        printf("push %s\n", argv[2]);
                        RtmpInit(argv[1], argv[2]);
                }else {
                        //init cam
                        dev_sdk_init(0);
                        printf("1push h264\n");
                        RtmpInit(argv[1], "h264");
                }
        } else {
#ifdef DEFAULT_H265
                dev_sdk_init(DEV_SDK_PROCESS_APP);
                printf("2push 265\n");
                RtmpInit("rtmp://pili-publish.caster.test.cloudvdn.com/caster-test/test12", "h265");
#else
                dev_sdk_init(0);
                printf("2push 264\n");
                RtmpInit("rtmp://pili-publish.caster.test.cloudvdn.com/caster-test/test12", "h264");
#endif
        }

        //start a/v callback
        static int context = 1;
        dev_sdk_start_video(0, 0, VideoCallBack, &context);
        dev_sdk_start_audio(0, 0, AudioCallBack, &context);

        
        while(1) {
                sigwait(&set, &sig);
                switch (sig) {
                case SIGALRM:
                    QnDemoPrint(DEMO_NOTICE, "SIGALRM catch.");
                    continue;
                case SIGPIPE:
                    QnDemoPrint(DEMO_NOTICE, "SIGPIPE catch.");
                    continue;
                case SIGUSR1:
                    QnDemoPrint(DEMO_NOTICE, "SIGUSR1 catch.");
                    continue;
                case SIGUSR2:
                    QnDemoPrint(DEMO_NOTICE, "SIGUSR2 catch.");
                    continue;
                case SIGINT:
                    QnDemoPrint(DEMO_NOTICE, "SIGINT catch, exiting...");
                    RtmpRelease();

                    dev_sdk_stop_video(0, 0);
                    dev_sdk_stop_audio(0, 0);
                    dev_sdk_release();
                    break;
                default:
                    QnDemoPrint(DEMO_NOTICE, "unmasked signal %d catch.", sig);
                    continue;
                }
        }
                
        return 0;
}

