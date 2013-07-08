CONTIKI_PROJECT = lwb
all: $(CONTIKI_PROJECT)

ifdef LWB_CONF
CFLAGS += -D$(LWB_CONF)
endif

APPDIRS += ./configs

CONTIKI = /Users/fe/ETH/Contiki/Glossy/contiki-stripped-down
include $(CONTIKI)/Makefile.include

