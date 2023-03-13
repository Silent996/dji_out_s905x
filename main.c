#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/time.h>
#include <libavutil/file.h>

#include <pthread.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>
#include <decoder.h>
#include <djiusb.h>
#include <cyclefifo.h>

#include <time.h>

#define AVIO_CTX_BUFFER_SIZE 1024 * 32

#define FRAME_QUEUE_SIZE 60 * 30

#define DJIFPV_VIDEO_OUT_RATE 60

static libusb_device_handle *dev_djifpv = NULL;
static codec_para_t *aml_decoder = NULL;
static Cfqueue frame_queue;

static int device_reconnect = 0;

static unsigned long frame_index = 0;

static unsigned long start_time = 0;

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    (void)opaque;

    int ret_size = 0;
    int ret = 0;

    // if (!buf_size)
    //     return AVERROR_EOF;

    ret = djifpv_usb_transfer(dev_djifpv, buf, &ret_size);

    if (ret == -1)
    {
        for (;;)
        {
            dev_djifpv = djifpv_get_device();
            if (dev_djifpv != 0)
            {
                device_reconnect = 0;
                frame_index = 0;
                start_time = av_gettime();

                // 首次初始化AML解码器，覆盖原有的CoreELEC
                if (aml_decoder == NULL)
                {
                    aml_decoder = amlogic_decoder_init();
                    if (aml_decoder != NULL)
                    {
                        fprintf(stdout, "Stop System kodi Service\n");
                        system("systemctl stop kodi");
                    }
                }

                ret = djifpv_usb_transfer(dev_djifpv, buf, &ret_size);
                break;
            }
            av_usleep(100);
        }
    }
    return ret_size;
}

static void read_frame_thread(Cfqueue *pfqueue)
{
    AVFormatContext *fmt_ctx = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *avio_ctx_buffer = NULL;

    int ret = 0;

    if (!(fmt_ctx = avformat_alloc_context()))
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = av_malloc(AVIO_CTX_BUFFER_SIZE);
    if (!avio_ctx_buffer)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, AVIO_CTX_BUFFER_SIZE,
                                  0, NULL, &read_packet, NULL, NULL);
    if (!avio_ctx)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }

    for (;;)
    {
        // if (device_reconnect == 0)
        // {
        //     ret = avformat_find_stream_info(fmt_ctx, NULL);
        //     if (ret < 0)
        //     {
        //         fprintf(stderr, "Could not find stream information\n");
        //         av_usleep(1000);
        //         continue;
        //     }
        //     for (int i = 0; i < fmt_ctx->nb_streams; i++)
        //     {
        //         if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        //         {
        //             fprintf(stdout, "width:%d\n", fmt_ctx->streams[i]->codecpar->width);
        //             fprintf(stdout, "height:%d\n", fmt_ctx->streams[i]->codecpar->height);
        //         }
        //     }
        //     device_reconnect = 1;
        // }

        AVStream *stream;
        AVPacket *packet = av_packet_alloc();

        ret = av_read_frame(fmt_ctx, packet);
        if (ret < 0)
        {
            av_packet_unref(packet);
            av_usleep(1000);
            continue;
        }

        if (packet->stream_index == AVMEDIA_TYPE_VIDEO)
        {
            // stream = fmt_ctx->streams[packet->stream_index];

            // if (packet->pts == AV_NOPTS_VALUE)
            // {
            //     // Write PTS
            //     AVRational time_base1 = stream->time_base;
            //     // Duration between 2 frames (us)
            //     int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(stream->r_frame_rate);
            //     // Parameters
            //     packet->pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            //     packet->dts = packet->pts;
            //     packet->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            //     fprintf(stdout, "frame_index:%ld,packet_pts:%ld,time_base:%ld:%ld\n", frame_index, packet->pts, time_base1.den, time_base1.num);
            //     frame_index++;
            // }

            if (cfqIsFull(pfqueue))
            {
                AVPacket *tmp = cfqPop(pfqueue);
                av_packet_unref(tmp);
            }

            cfqPush(pfqueue, (void *)packet);
        }
        else
        {
            av_packet_unref(packet);
        }
    }

end:
    djifpv_usb_close(dev_djifpv);
    djifpv_usb_exit();
    amlogic_decoder_uninstall();

    cfqDestory(pfqueue);

    avformat_close_input(&fmt_ctx);

    if (avio_ctx)
        av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);

    if (ret < 0)
    {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(ret);
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int ret = 0;

    ret = djifpv_usb_init();
    if (ret != 0)
        goto end;

    ret = cfqInit(&frame_queue, FRAME_QUEUE_SIZE);
    if (ret != 0)
        goto end;

    pthread_t frame_thread_handle;
    pthread_create(&frame_thread_handle, NULL, &read_frame_thread, &frame_queue);

    AVPacket *packet = NULL;

    for (;;)
    {
        packet = cfqPop(&frame_queue);
        if (packet == NULL)
        {
            av_usleep(2000);
            continue;
        }

        if (packet->size > 0)
        {
            amlogic_decoder_write(packet->data, packet->size);

            // AVRational time_base = packet->time_base;
            // AVRational time_base_cur = {1, AV_TIME_BASE};
            // int64_t pts_time = av_rescale_q(packet->pts, time_base, time_base_cur);
            // int64_t now_time = av_gettime() - start_time;
            // if (pts_time > now_time)
            // {
            //     av_usleep(pts_time - now_time);
            // }
            // fprintf(stdout, "pts_time:%ld,sleep:%ld\n", packet->pts, pts_time - now_time);
        }

        av_packet_unref(packet);
    }

end:

    djifpv_usb_close(dev_djifpv);
    djifpv_usb_exit();

    cfqDestory(&frame_queue);

    return ret;
}
