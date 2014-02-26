CONTIKI_PROJECT = lwb-ip-test
all: $(CONTIKI_PROJECT)

CFLAGS += -DRF_CHANNEL=26 -DLWB_CONF_HSLP=1

WITH_UIP=1

CFLAGS += -fno-strict-aliasing

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

