TARGET = tnmenu
OBJS = main.o option_menu.o cache.o utils.o utility.o unzip.o unzip/unzip.o unzip/ioapi.o iso/ciso.o iso/iso.o

CFLAGS = -O2 -g -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = TNMenu

LIBS = -lpspsystemctrl_user -losl -lpng -lz -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspgum  -lpspaudiolib -lpspaudio -lpsphttp -lpspssl -lpspwlan -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl -lm

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak