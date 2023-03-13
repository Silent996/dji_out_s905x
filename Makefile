.PHONY: clean

OUT=fpv-video
IN=main.c djiusb.c decoder.c cyclefifo.c

CFLAGS  = -std=c11 -Wall -Wextra -I$(CURDIR)/ffmpeg/include/ -I$(CURDIR)/libusb/install/include/
CFLAGS += -I$(CURDIR)/amadec/include/ -I$(CURDIR)/amavutils/include/ -I$(CURDIR)/amcodec/include/ -I$(CURDIR)/alsa-lib/include/
CFLAGS += -I./
CFLAGS += -DALSA_OUT

ifdef DOLBY_UDC
    CFLAGS += -DDOLBY_USE_ARMDEC
endif

LIBS  = -L$(CURDIR)/ffmpeg/lib/ -lavformat -lavutil -lavcodec 
LIBS += -L$(CURDIR)/libusb/install/lib/ -lusb-1.0
LIBS += -lswresample -lm -lpthread
LIBS += -L$(CURDIR)/amadec -L$(CURDIR)/amavutils -L$(CURDIR)/amcodec -L$(CURDIR)/alsa-lib/lib/
LIBS += -lamcodec -ldl -lamadec -lasound -lamavutils


CC = armv8l-linux-gnueabihf-gcc

$(OUT): $(IN)
	$(CC) $(CFLAGS) $(IN) $(LIBS) -o $(OUT) 
