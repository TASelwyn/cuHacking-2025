ARTIFACT = qnx_plantr

#Build architecture/variant string, possible values: x86, armv7le, etc...
PLATFORM = aarch64le

#Build profile, possible values: release, debug, profile, coverage
BUILD_PROFILE ?= debug

CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#Compiler definitions

CC = qcc -Vgcc_nto$(PLATFORM)
CXX = q++ -Vgcc_nto$(PLATFORM)_cxx
LD = $(CC)

#User defined include/preprocessor flags and libraries
INCLUDES += -I libs/system/gpio
INCLUDES += -I libs/rpi_gpio/public
INCLUDES += -I libs/rpi_i2c/public
INCLUDES += -I libs/rpi_spi/public
INCLUDES += -I libs/rpi_ws281x/public

LIBS += -L libs/rpi_gpio/build/$(CONFIG_NAME)
LIBS += -lrpi_gpio
LIBS += -L libs/rpi_i2c/build/$(CONFIG_NAME)
LIBS += -lrpi_i2c
LIBS += -L libs/rpi_spi/build/$(CONFIG_NAME)
LIBS += -lrpi_spi
LIBS += -L libs/rpi_ws281x/build/$(CONFIG_NAME)
LIBS += -lrpi_ws281x
LIBS += -lm

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
SRCS = $(call rwildcard, src, c)

#Object files list
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o, $(basename $(SRCS))))

#Compiling rule
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

#Linking rule
LDFLAGS = -lcurl -ljson
$(TARGET):$(OBJS) librpi_i2c.a librpi_gpio.a librpi_spi.a librpi_ws281x.a
	$(LD) -o $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

#Rules section for default compilation and linking
all: $(TARGET)
librpi_gpio.a:
	$(MAKE) -C libs/rpi_gpio
librpi_i2c.a:
	$(MAKE) -C libs/rpi_i2c	
librpi_spi.a:
	$(MAKE) -C libs/rpi_spi
librpi_ws281x.a:
	$(MAKE) -C libs/rpi_ws281x

clean:
	rm -fr $(OUTPUT_DIR)

rebuild: clean all

#Inclusion of dependencies (object files to source and includes)
-include $(OBJS:%.o=%.d)