CONTIKI_PROJECT = lwb
all: $(CONTIKI_PROJECT)

ifdef LWB_CONF
CFLAGS += -D$(LWB_CONF)
endif

APPDIRS += ./configs

include ./uip6/Makefile.uip6
APPDIRS += ./uip6

CFLAGS += -fno-strict-aliasing -ffunction-sections

ifdef ECHO_CLIENT
CFLAGS += -DECHO_CLIENT=$(ECHO_CLIENT)
endif

ifdef ECHO_SERVER
CFLAGS += -DECHO_SERVER=$(ECHO_SERVER)
endif

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

