#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <media/AudioTrack.h>
#include <utils/StrongPointer.h>

using namespace android;

int main() {
    const int rate = 48000;
    const int frames = rate; // 1 second
    const int freq = 440;
    int16_t *buf = (int16_t *)malloc(frames * 2 * sizeof(int16_t));
    for (int i = 0; i < frames; i++) {
        int16_t v = (int16_t)(sin(2.0 * M_PI * freq * i / rate) * 20000);
        buf[2*i] = v;
        buf[2*i+1] = v;
    }

    sp<AudioTrack> track = new AudioTrack();

    status_t st = track->set(AUDIO_STREAM_MUSIC,
                             rate,
                             AUDIO_FORMAT_PCM_16_BIT,
                             AUDIO_CHANNEL_OUT_STEREO,
                             frames,
                             AUDIO_OUTPUT_FLAG_PRIMARY,
                             nullptr, /* callback */
                             0,       /* notificationFrames */
                             nullptr, /* sharedBuffer */
                             false,   /* threadCanCallJava */
                             AUDIO_SESSION_ALLOCATE,
                             AudioTrack::TRANSFER_SYNC,
                             nullptr, /* offloadInfo */
                             []() {
                                 AttributionSourceState as;
                                 as.uid = -1;
                                 as.pid = -1;
                                 return as;
                             }(),
                             nullptr /* pAttributes */);
    printf("set: %d\n", st);
    if (st != OK) return 1;

    st = track->initCheck();
    printf("initCheck: %d\n", st);
    if (st != OK) return 1;

    track->start();
    printf("started\n");
    usleep(500000);
    ssize_t written = track->write(buf, frames * 2 * sizeof(int16_t));
    printf("written: %zd bytes\n", written);
    sleep(2);
    track->stop();
    track.clear();
    printf("done\n");
    return 0;
}
