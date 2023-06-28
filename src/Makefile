
#  SPDX-License-Identifier: GPL-2.0-or-later

ifeq ($(V),1)
	VERBOSE=''
else
	VERBOSE=@
endif

BREW_SDK_DIR?=../brew
INCDIRS=-I $(BREW_SDK_DIR)/sdk/inc -I $(BREW_SDK_DIR)/inc
VPATH=$(BREW_SDK_DIR)/sdk/src

# used GCC 10
CC?=arm-none-eabi-gcc

CFLAGS=-c $(INCDIRS) -mlittle-endian -march=armv6 -mcpu=arm1136j-s -mapcs-frame -fno-builtin -ffunction-sections
CFLAGS+= -mthumb-interwork -std=c99 -O2 
CFLAGS+= -DDYNAMIC_APP -DBREW -DHAVE_CONFIG_H -DDEBUG
LDFLAGS+= -nostdlib -nodefaultlibs -nostartfiles -Wl,--emit-relocs -Wl,-Map,build/prboom.map -Wl,--cref

OBJDIR=build
OUTPUT=$(OBJDIR)/prboom

SRC_DOOM = \
	IAudioSource.o \
	r_demo.o \
	r_sky.o \
	doomdef.o \
	p_setup.o \
	m_menu.o \
	r_fps.o \
	g_game.o \
	m_argv.o \
	md5.o \
	p_enemy.o \
	p_ceilng.o \
	p_lights.o \
	brew.o \
	brew_blit.o \
	p_saveg.o \
	r_draw.o \
	z_zone.o \
	d_deh.o \
	lprintf.o \
	p_plats.o \
	i_sshot.o \
	p_pspr.o \
	mathfixed.o \
	p_tick.o \
	d_items.o \
	p_maputl.o \
	wi_stuff.o \
	v_video.o \
	m_misc.o \
	st_lib.o \
	p_floor.o \
	p_doors.o \
	m_random.o \
	p_checksum.o \
	p_user.o \
	w_wad.o \
	hu_lib.o \
	p_inter.o \
	s_sound.o \
	i_sound.o \
	m_cheat.o \
	p_map.o \
	p_sight.o \
	prboom.o \
	info.o \
	i_main.o \
	r_things.o \
	w_memcache.o \
	r_plane.o \
	p_telept.o \
	version.o \
	r_patch.o \
	p_switch.o \
	p_mobj.o \
	r_data.o \
	i_video.o \
	mmus2mid.o \
	am_map.o \
	p_genlin.o \
	r_main.o \
	hu_stuff.o \
	f_wipe.o \
	d_client.o \
	i_network.o \
	i_joy.o \
	tables.o \
	f_finale.o \
	i_system.o \
	sounds.o \
	r_segs.o \
	r_bsp.o \
	d_main.o \
	doomstat.o \
	r_filter.o \
	dstrings.o \
	st_stuff.o \
	z_bmalloc.o \
	m_bbox.o \
	p_spec.o

OBJS += $(addprefix $(OBJDIR)/, $(SRC_DOOM))

all: $(MKDIR) $(OUTPUT).mod

clean:
	rm -f "${OBJDIR}"/*

$(OUTPUT).mod: $(OUTPUT).elf
	@echo [Creating mod $@]
	wine $(BREW_SDK_DIR)/bin/elf2mod.exe $^

$(OUTPUT).elf: $(OBJDIR)/AEEModGen.o $(OBJDIR)/AEEAppGen.o $(OBJDIR)/AEEHIDButtons.o \
	$(OBJDIR)/AEEHIDThumbsticks.o $(OBJDIR)/EGL_1x.o $(OBJDIR)/GLES_1x.o $(OBJDIR)/GLES_ext.o $(OBJS)
	@echo [Linking $@]
	$(VERBOSE)$(CC) $(LDFLAGS) -T $(BREW_SDK_DIR)/bin/elf2mod.x -o $@ $^ -lgcc -lc 

$(MKDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o:	%.c
	@echo [Compiling $<]
	$(VERBOSE)$(CC) $(CFLAGS) -c $< -o $@

AEEModGen.o: AEEModGen.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/AEEModGen.o $(VPATH)/AEEModGen.c

AEEAppGen.o: AEEAppGen.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/AEEAppGen.o $(VPATH)/AEEAppGen.c

AEEHIDButtons.o: AEEHIDButtons.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/AEEHIDButtons.o $(VPATH)/AEEHIDButtons.c

AEEHIDThumbsticks.o: AEEHIDThumbsticks.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/AEEHIDThumbsticks.o $(VPATH)/AEEHIDThumbsticks.c

EGL_1x.o: EGL_1x.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/EGL_1x.o $(VPATH)/EGL_1x.c

GLES_1x.o: GLES_1x.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/GLES_1x.o $(VPATH)/GLES_1x.c

GLES_ext.o: GLES_ext.c
	$(VERBOSE)$(CC) $(CFLAGS) -o $(OBJDIR)/GLES_ext.o $(VPATH)/GLES_ext.c
