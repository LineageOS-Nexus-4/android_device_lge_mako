#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <media/AudioRecord.h>
#include <utils/StrongPointer.h>
#include <binder/IPCThreadState.h>

using namespace android;

int main() {
    const int rate = 16000;
    const int frames = rate * 3;
    int16_t *buf = (int16_t *)malloc(frames * 2 * sizeof(int16_t));
    memset(buf, 0, frames * 2 * sizeof(int16_t));

    android::content::AttributionSourceState attribution;
    attribution.packageName = "arecord";
    attribution.uid = static_cast<int32_t>(getuid());
    attribution.pid = static_cast<int32_t>(getpid());
    attribution.token = sp<BBinder>::make();

    sp<AudioRecord> rec = new AudioRecord(AUDIO_SOURCE_MIC,
                           rate,
                           AUDIO_FORMAT_PCM_16_BIT,
                           AUDIO_CHANNEL_IN_MONO,
                           attribution,
                           frames / 3);
    status_t st = rec->initCheck();
    printf("initCheck: %d\n", st);
    st = rec->start();
    printf("started: %d\n", st);
    ssize_t total = 0;
    while (total < frames) {
        ssize_t n = rec->read(buf + total, frames - total, false);
        if (n == -EAGAIN) {
            usleep(10000);
            continue;
        }
        if (n <= 0) {
            printf("read err: %zd\n", n);
            break;
        }
        total += n;
    }
    printf("read: %zd frames\n", total);
    int peak = 0;
    for (int i = 0; i < total; i++) {
        int a = abs(buf[i]);
        if (a > peak) peak = a;
    }
    printf("peak: %d\n", peak);
    rec->stop();
    FILE *f = fopen("/data/local/tmp/rec.raw", "wb");
    fwrite(buf, 2, total, f);
    fclose(f);
    printf("saved /data/local/tmp/rec.raw\n");
    return 0;
}
