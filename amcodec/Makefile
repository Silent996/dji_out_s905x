CFLAGS = -fPIC -Wall -Wextra -O2 -g
CFLAGS += -I./ -I./include -I../amadec/include -I../amavutils/include -I../amplayer/player/include -I../amcodec/include -I../alsa-lib/include/
CFLAGS += -L${LIB_DIR} -DALSA_OUT

ifdef DOLBY_UDC
    CFLAGS += -DDOLBY_USE_ARMDEC
endif

LDFLAGS = -L../alsa-lib/lib/ -shared -lpthread -lm -lasound -lrt
TARGET_LIB = libamcodec.so

SRCS = audio_ctrl.c codec_ctrl.c codec_h_ctrl.c codec_msg.c
OBJS = $(SRCS:.c=.o)

CC = armv8l-linux-gnueabihf-gcc

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
