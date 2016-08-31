NAME=sensortest
FLIGHT_BOARD=flight4

HEXAGON_TOOLS_ROOT	 ?= $(HOME)/Qualcomm/HEXAGON_Tools/7.2.11
HEXAGON_SDK_ROOT	 ?= $(HOME)/Qualcomm/Hexagon_SDK/2.0
HEXAGON_FC_ADDON	 ?= $(HOME)/Qualcomm/HexagonFCAddon
V_ARCH			 = v5
CROSSDEV		 = hexagon-
HEXAGON_BIN		 = $(addsuffix /Tools/bin,$(HEXAGON_TOOLS_ROOT))
HEXAGON_CLANG_BIN	 = $(addsuffix /Tools/bin,$(HEXAGON_TOOLS_ROOT))

CC			 = $(HEXAGON_CLANG_BIN)/$(CROSSDEV)clang
CXX			 = $(HEXAGON_CLANG_BIN)/$(CROSSDEV)clang++
HEXAGON_GCC_BIN		 = $(HEXAGON_TOOLS_ROOT)/gnu/bin
HEXAGON_GXX		 = $(HEXAGON_GCC_BIN)/hexagon-g++
LINUX_QAIC		 = $(HEXAGON_SDK_ROOT)/tools/qaic/Linux/qaic

ARM_GCC_ROOT		 = $(HEXAGON_SDK_ROOT)/gcc-linaro-arm-linux-gnueabihf-4.8-2013.08_linux
ARM_CC			 = $(ARM_GCC_ROOT)/bin/arm-linux-gnueabihf-gcc
ARM_CXX			 = $(ARM_GCC_ROOT)/bin/arm-linux-gnueabihf-g++

LIBSTDCXX		 = $(HEXAGON_TOOLS_ROOT)/Tools/target/hexagon/lib/v5/G0/pic/libstdc++.a

DSP_SRCS=dsp_functions.cpp uartbuffer.cpp
DSP_OBJS=$(DSP_SRCS:.cpp=.do)

SKEL_SRCS=qflight_skel.cpp
SKEL_OBJS=$(SKEL_SRCS:.cpp=.do)

ARM_SRCS=sensortest.c
ARM_OBJS=$(ARM_SRCS:.c=.o)

SENSORS=$(HEXAGON_FC_ADDON)/flight_controller/hexagon/libs/libbmp280.so $(HEXAGON_FC_ADDON)/flight_controller/hexagon/libs/libmpu9x50.so

ifeq ($(VERBOSE),)
v = @
else
v =
endif

all: qflight_shared.h qflight_stub.c qflight_skel.c libqflight_skel.so $(NAME)

.SUFFIXES: .do .o .c .so

# DSP build flags
SHARED_INC		= -I$(HEXAGON_SDK_ROOT)/inc/stddef
DSP_INC=$(SHARED_INC) -I$(HEXAGON_FC_ADDON)/flight_controller/hexagon/inc -I$(HEXAGON_FC_ADDON)/flight_controller/hexagon/inc/dspal/sys -I$(HEXAGON_FC_ADDON)/flight_controller/hexagon/inc/dspal/sys/sys -I$(HEXAGON_FC_ADDON)/flight_controller/hexagon/inc/dspal/include -I$(HEXAGON_SDK_ROOT)/lib/common/qurt/ADSPv5MP/include -I$(HEXAGON_SDK_ROOT)/lib/common/remote/ship/hexagon_ReleaseG
DSP_FLAGS=-mv5 -G0 -g -O0 -fno-exceptions -fno-strict-aliasing -fno-zero-initialized-in-bss -fdata-sections -fpic -D__V_DYNAMIC__ -D_DEBUG $(DSP_INC) -D_PID_T -D_UID_T -D_TIMER_T

qflight_shared.h qflight_skel.c qflight_stub.c:
	@echo Generating Shared lib
	$(v)$(LINUX_QAIC) -mdll -o ./ -I./ $(DSP_INC) qflight_dsp.idl

.c.do:
	@echo Compiling $@
	$(v)$(CCACHE) $(CC) $(DSP_FLAGS) -c $< -o $@

.cpp.do:
	@echo Compiling $@
	$(v)$(CCACHE) $(CXX) -std=gnu++11 $(DSP_FLAGS) -c $< -o $@

# ARM build flags
ARM_INC= $(SHARED_INC) -I$(HEXAGON_SDK_ROOT)/lib/common/adspmsgd/ship/UbuntuARM_ReleaseG -I$(HEXAGON_SDK_ROOT)/lib/common/rpcmem/UbuntuARM_ReleaseG/ship -I$(HEXAGON_SDK_ROOT)/lib/common/rpcmem/UbuntuARM_ReleaseG/ship -I$(HEXAGON_SDK_ROOT)/lib/common/remote/ship/UbuntuARM_ReleaseG
ARM_FLAGS=-fPIC -mword-relocations -mthumb-interwork -mfloat-abi=hard -mfpu=neon -mtune=cortex-a8 -march=armv7-a -g -O0 -fno-strict-aliasing -DARM_ARCH_7A -DUSE_SYSLOG $(ARM_INC)

.cpp.o:
	@echo Compiling $@
	$(v)$(ARM_CXX) -c $(ARM_FLAGS) -std=gnu++11 -o $@ $<

.c.o:
	@echo Compiling $@
	$(v)$(ARM_CC) -c $(ARM_FLAGS) -std=gnu99 -o $@ $<

libqflight_skel.so: $(SKEL_OBJS) $(DSP_OBJS)
	@echo Linking DSP skel $@
	$(v)$(CC) -L$(HEXAGON_FC_ADDON)/flight_controller/hexagon/libs -lmpu9x50 -lbmp280 -g -mv5 -mG0lib -G0 -fpic -shared -Wl,-Bsymbolic -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=free -Wl,--wrap=realloc -Wl,--wrap=memalign -Wl,--wrap=__stack_chk_fail -lc -Wl,-soname=libqflight_skel.so  -o $@ -Wl,--whole-archive  $^ -Wl,--no-whole-archive -Wl,$(LIBSTDCXX)

$(NAME): $(ARM_OBJS) qflight_stub.o
	@echo Linking arm $@
	$(v)$(ARM_CC) -march=armv7-a -mthumb-interwork -mfloat-abi=hard -mfpu=neon -mtune=cortex-a8 -Wl,-unresolved-symbols=ignore-in-shared-libs -Wl,-ldl -o $@ -Wl,"-(" $^ -L$(HEXAGON_SDK_ROOT)/lib/common/remote/ship/UbuntuARM_ReleaseG/ -ladsprpc $(HEXAGON_SDK_ROOT)/lib/common/rpcmem/UbuntuARM_ReleaseG/ship/rpcmem.a $(HEXAGON_SDK_ROOT)/lib/common/adspmsgd/ship/UbuntuARM_ReleaseG/adspmsgd.a -Wl,"-)" -lm -lc -lsupc++ -lgcc_eh -lgcc -lpthread

clean:
	rm -f *.o *.do *.so qflight_dsp.h qflight_stub.c qflight_skel.c *~ $(NAME)

send: all
	rsync -avz $(NAME) root@$(FLIGHT_BOARD):
	rsync -avz libqflight_skel.so root@$(FLIGHT_BOARD):/usr/share/data/adsp/
