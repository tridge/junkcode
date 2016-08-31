#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include "qflight_dsp.h"
#include "rpcmem.h"
#include "qflight_buffer.h"
#include <pthread.h>

#define RPC_ALLOCATE(type) (type *)rpcmem_alloc_def(sizeof(type))

static uint64_t micros64()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec*1000*1000ULL + ts.tv_nsec/1000U;
}

int main()
{
    printf("Starting DSP code\n");

    rpcmem_init();
    DSPBuffer::IMU  *imubuf  = RPC_ALLOCATE(DSPBuffer::IMU);
    DSPBuffer::MAG  *magbuf  = RPC_ALLOCATE(DSPBuffer::MAG);
    DSPBuffer::BARO *barobuf = RPC_ALLOCATE(DSPBuffer::BARO);
    
    uint64_t imu_timestamp = 0;
    int ret;

    int32_t fd[4];
    for (uint8_t i=0; i<4; i++) {
        char path[] = "/dev/tty-X";
        path[9] = '1' + i;
        ret = qflight_UART_open(path, &fd[i]);
        printf("UART_open %s ret=%d fd=%d\n",
               path, ret, (int)fd);
        qflight_UART_set_baudrate(fd[i], 57600);
    }

    
    while (true) {
        usleep(100000);
        uint64_t start_us = micros64();
        ret = qflight_get_imu_data((uint8_t *)imubuf, sizeof(*imubuf));
        uint64_t dt = micros64() - start_us;
        if (ret != 0) {
            printf("FAIL: get_imu_data ret=%d\n", ret);
            break;
        }
        ret = qflight_get_mag_data((uint8_t *)magbuf, sizeof(*magbuf));
        if (ret != 0) {
            printf("FAIL: get_mag_data ret=%d\n", ret);
            break;
        }
        ret = qflight_get_baro_data((uint8_t *)barobuf, sizeof(*barobuf));
        if (ret != 0) {
            printf("FAIL: get_baro_data ret=%d\n", ret);
            break;
        }
        // only display new data
        printf("samples: imu=%u mag=%u baro=%u call_cost=%u\n",
               imubuf->num_samples,
               magbuf->num_samples,
               barobuf->num_samples,
               (unsigned)dt);
        for (uint8_t i=0; i<4; i++) {
            char ubuf[200];
            snprintf(ubuf, sizeof(ubuf),
                     "UART %u samples: imu=%u mag=%u baro=%u call_cost=%u\n",
                     i,
                     imubuf->num_samples,
                     magbuf->num_samples,
                     barobuf->num_samples,
                     (unsigned)dt);
            int32_t nwritten=0;
            qflight_UART_write(fd[i], (const uint8_t *)ubuf, strlen(ubuf), &nwritten);
        }

        for (uint16_t i=0; i<imubuf->num_samples; i++) {
            DSPBuffer::IMU::BUF &b = imubuf->buf[i];
            if (i < 10) {
                printf("dt=%u accel=(%.3f %.3f %.3f) gyro=(%.3f %.3f %.3f)\n",
                       (unsigned)(b.timestamp - imu_timestamp),
                       b.accel[0], b.accel[1], b.accel[2],
                       b.gyro[0], b.gyro[1], b.gyro[2]);
            }
            imu_timestamp = b.timestamp;
        }
        for (uint16_t i=0; i<magbuf->num_samples; i++) {
            DSPBuffer::MAG::BUF &m = magbuf->buf[i];
            printf("mag=(%d %d %d)\n",
                   (int)m.mag_raw[0], (int)m.mag_raw[1], (int)m.mag_raw[2]);
        }
        for (uint16_t i=0; i<barobuf->num_samples; i++) {
            DSPBuffer::BARO::BUF &b = barobuf->buf[i];
            printf("baro=%.1f %.3f\n",
                   b.pressure_pa,
                   b.temperature_C);
        }

        for (uint8_t i=0; i<4; i++) {
            // check for incoming uart data
            char buf[100];
            int32_t nread;
            ret = qflight_UART_read(fd[i], (uint8_t *)buf, 100, &nread);
            if (ret == 0 && nread > 0) {
                printf("UART[%u](%d %d): %.*s\n", i, ret, (int)nread, (int)nread, buf);
            }
        }
    }
    
    rpcmem_free(imubuf);
    rpcmem_free(magbuf);
    rpcmem_free(barobuf);
    rpcmem_deinit();
    return 1;
}
