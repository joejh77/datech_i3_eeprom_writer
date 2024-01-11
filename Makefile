#soft-fp version
CC=/home/${USER}/workspace/project/oasis/buildroot-oasis/output/firstview_nor/host/bin/arm-linux-gnueabi-gcc
CXX=/home/${USER}/workspace/project/oasis/buildroot-oasis/output/firstview_nor/host/bin/arm-linux-gnueabi-g++
STRIP=/home/${USER}/workspace/project/oasis/buildroot-oasis/output/firstview_nor/host/bin/arm-linux-gnueabi-strip
#CXX=/home/${USER}/workspace/project/Oasis/tools/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++
#STRIP=/home/${USER}/workspace/Oasis/tools/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-strip
CXXFLAGS=-g -Wall -std=c++11 -march=armv7-a -mfloat-abi=softfp -mfpu=neon -DNDEBUG -fPIC


CXXFLAGS+=-O0 -fstack-protector-strong
#CXXFLAGS+=-O3

INCLUDES=-I../datech_i3_app/include
LDFLAGS=

LDLIBS=

.PHONY: clean
all: clean eeprom_writer

stripped: eeprom_writer
	$(STRIP) --strip-unneeded eeprom_writer

eeprom_writer:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c eeprom_writer.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c ../datech_i3_app/SB_System.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c ../datech_i3_app/sysfs.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c ../datech_i3_app/datools.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c ../datech_i3_app/bb_micom.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) *.o $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) *.o eeprom_writer
 
