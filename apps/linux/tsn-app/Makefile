
#Set those below to match your environment
# CROSS_COMPILE: path to cros-compiler
# OBJDIR: path where generated binaries should be stored
# GENAVB_PATH: path where the genavb library is located
# GENAVB_INCLUDE: path to the public genavb includes files

CROSS_COMPILE?=/home/user/gcc-linaro-arm-linux-gnueabihf-4.8-2014.05_linux/bin/arm-linux-gnueabihf-
OBJDIR?=
GENAVB_PATH?=../../../lib
GENAVB_INCLUDE?=../../../include
APP_NAME=tsn-app

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -lgenavb -pthread -ldl -L$(GENAVB_PATH) -I$(GENAVB_INCLUDE) -DSTATS_LOG

$(OBJDIR)$(APP_NAME): main.c tsn_task.c tsn_tasks_config.c thread_config.c cyclic_task.c serial_controller.c network_only.c \
			../common/stats.c ../common/stats.c ../common/thread.c ../common/log.c ../common/time.c \
			../common/timer.c ../common/helpers.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME)
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)

clean:
	rm -rf $(OBJDIR)$(APP_NAME)
