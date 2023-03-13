
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <codec.h>
#include <stdbool.h>
#include <ctype.h>

#include <decoder.h>

#define TRICKMODE_NONE 0x00
#define TRICKMODE_I 0x01
#define TRICKMODE_FFFB 0x02

static int axis[8] = {0};
static codec_para_t v_codec_para;
static codec_para_t *vpcodec;

bool set_file(const char *path, const char *val)
{
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0)
    {
        write(fd, val, strlen(val));
        close(fd);
        return true;
    }
    printf("[aml] Cannot set sysfs (string): path=%s, val=%s\n", path, val);
    return false;
}

bool get_file(const char *path, char *valstr, const int size)
{
    int fd = open(path, O_RDONLY);
    if (fd >= 0)
    {
        int len = read(fd, valstr, size - 1);
        valstr[len - 1] = '\0';
        close(fd);
        return true;
    }
    printf("[aml] Cannot get sysfs (string): path=%s\n", path);
    sprintf(valstr, "%s", "fail");
    return false;
}

bool setAxis(int x, int y, int w, int h)
{
    printf("[aml] Set display axis: (%d,%d,%d,%d)\n", x, y, w, h);
    char daxis_str[255] = {0};
    sprintf(daxis_str, "%d %d %d %d", x, y, w, h);
    return set_file("/sys/class/video/axis", daxis_str);
}

int osd_blank(char *path, int cmd)
{
    int fd;
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);

    if (fd >= 0)
    {
        sprintf(bcmd, "%d", cmd);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int set_disable_video(int disable)
{
    int fd;
    char *path = "/sys/class/video/disable_video";
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0)
    {
        sprintf(bcmd, "%d", disable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int set_disable_cdsw(int disable)
{
    int fd;
    char *path = "/sys/class/video/disable_cdsw";
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0)
    {
        sprintf(bcmd, "%d", disable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int set_tsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0)
    {
        sprintf(bcmd, "%d", enable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int parse_para(const char *para, int para_num, int *result)
{
    char *endp;
    const char *startp = para;
    int *out = result;
    int len = 0, count = 0;

    if (!startp)
    {
        return 0;
    }

    len = strlen(startp);

    do
    {
        // filter space out
        while (startp && (isspace(*startp) || !isgraph(*startp)) && len)
        {
            startp++;
            len--;
        }

        if (len == 0)
        {
            break;
        }

        *out++ = strtol(startp, &endp, 0);

        len -= endp - startp;
        startp = endp;
        count++;

    } while ((endp) && (count < para_num) && (len > 0));

    return count;
}

int set_display_axis(int recovery)
{
    int fd;
    char *path = "/sys/class/display/axis";
    char str[128];
    int count;
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0)
    {
        if (!recovery)
        {
            read(fd, str, 128);
            printf("read axis %s, length %d\n", str, strlen(str));
            count = parse_para(str, 8, axis);
        }
        if (recovery)
        {
            sprintf(str, "%d %d %d %d %d %d %d %d",
                    axis[0], axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        }
        else
        {
            sprintf(str, "2048 %d %d %d %d %d %d %d",
                    axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        }
        write(fd, str, strlen(str));
        close(fd);
        return 0;
    }

    return -1;
}

// static void signal_handler(int signum)
// {
//     fprintf(stdout, "Get signum=%x\n", signum);
//     codec_close(vpcodec);
//     // set_display_axis(1);
//     set_disable_video(1);
//     set_disable_cdsw(1);
//     set_tsync_enable(1);
//     signal(signum, SIG_DFL);
//     raise(signum);
// }

codec_para_t *amlogic_decoder_init()
{
    int ret = CODEC_ERROR_NONE;

    osd_blank("/sys/class/graphics/fb0/blank", 1);
    osd_blank("/sys/class/graphics/fb1/blank", 0);
    // set_display_axis(0);
    set_disable_video(0);
    set_disable_cdsw(0);

    vpcodec = &v_codec_para;
    memset(vpcodec, 0, sizeof(codec_para_t));

    vpcodec->has_video = 1;
    vpcodec->video_type = VFORMAT_H264;

    vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
    vpcodec->am_sysinfo.param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE);
    vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;
    vpcodec->am_sysinfo.rate = 60;
    vpcodec->am_sysinfo.height = 960;
    vpcodec->am_sysinfo.width = 720;
    vpcodec->has_audio = 0;
    vpcodec->noblock = 0;

    ret = codec_init(vpcodec);
    if (ret != CODEC_ERROR_NONE)
    {
        fprintf(stderr, "codec init failed, ret=-0x%x", -ret);
        codec_close(vpcodec);
        set_display_axis(1);
        return NULL;
    }
    fprintf(stdout, "video codec ok!\n");

    // codec_set_cntl_avthresh(vpcodec, AV_SYNC_THRESH);
    // codec_set_cntl_syncthresh(vpcodec, 0);
    codec_set_cntl_mode(vpcodec, TRICKMODE_I);

    set_tsync_enable(0);

    return vpcodec;
}

void amlogic_decoder_write(unsigned char *buffer, int length)
{
    struct buf_status vbuf;
    int isize = 0, ret = 0;
    do
    {
        ret = codec_write(vpcodec, buffer + isize, length);
        if (ret < 0)
        {
            if (errno != EAGAIN)
            {
                fprintf(stderr, "write data failed, errno %d\n", errno);
                break;
            }
            else
            {
                continue;
            }
        }
        else
        {
            isize += ret;
        }
        // printf("ret %d, isize %d\n", ret, isize);
    } while (isize < length);

    // signal(SIGCHLD, SIG_IGN);
    // signal(SIGTSTP, SIG_IGN);
    // signal(SIGTTOU, SIG_IGN);
    // signal(SIGTTIN, SIG_IGN);
    // signal(SIGHUP, signal_handler);
    // signal(SIGTERM, signal_handler);
    // signal(SIGSEGV, signal_handler);
    // signal(SIGINT, signal_handler);
    // signal(SIGQUIT, signal_handler);

    do
    {
        ret = codec_get_vbuf_state(vpcodec, &vbuf);
        if (ret != 0)
        {
            printf("codec_get_vbuf_state error: %x\n", -ret);
            return;
        }
    } while (vbuf.data_len > 0x100);
}

void amlogic_decoder_uninstall()
{
    codec_close(vpcodec);
    // set_display_axis(1);
    set_disable_video(1);
    set_disable_cdsw(1);
    set_tsync_enable(1);
}