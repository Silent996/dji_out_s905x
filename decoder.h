
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

#define EXTERNAL_PTS (1)
#define SYNC_OUTSIDE (2)
#define UNIT_FREQ 96000
#define PTS_FREQ 90000
#define AV_SYNC_THRESH PTS_FREQ * 120

bool setAxis(int x, int y, int w, int h);

codec_para_t *amlogic_decoder_init();

void amlogic_decoder_write(unsigned char *buffer, int length);

void amlogic_decoder_uninstall();