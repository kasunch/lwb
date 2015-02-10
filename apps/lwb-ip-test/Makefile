CONTIKI_PROJECT = tcp-client-server
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += lwb-print.c deployment.c
CFLAGS += -DRF_CHANNEL=26 -DRF_POWER=17 -DLWB_CONF_HSLP=0 -DLWB_CUSTOM_CONF_H=\"lwb-custom-conf.h\"

WITH_UIP=1

CFLAGS += -fno-strict-aliasing

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

