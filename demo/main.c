#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "QnCommon.h"
#include "qnRtmp.h"
#include "devsdk.h"

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

        //init cam
        dev_sdk_init(DEV_SDK_PROCESS_APP);

        if (argc >= 2) {
                RtmpInit(argv[1]);
        } else {
                RtmpInit("rtmp://localhost/live/test1");
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

