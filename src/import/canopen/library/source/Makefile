#
# CANopen Library Makefile
# $Id: Makefile,v 1.41 2012/05/09 13:12:30 aph Exp $
#
# CFLAGS and LIB_DIR is normally set from the calling Makefile
#
# INCDIR in CFLAGS must be set, so that at the very first step
# the cal_conf.h from the Application is read.
#
#
# Build the objects somewhere in the CANOPEN_OBJ_DIR

ifeq "$(COMPILER)" "BC50"
#########################################################################
.SUFFIXES: .obj
$(CANOPEN_OBJ_DIR)/%.obj : %.c
	$(CC) $(CFLAGS) $(CANOPEN_INC_DIRS) -o$@ -c $<

OBJE	= .obj
# some definitions to handle the borland linker commands
empty=
space= $(empty) $(empty)
plus= -+
endif

ifeq "$(COMPILER)" "MSVC"
#########################################################################
.SUFFIXES: .obj
$(CANOPEN_OBJ_DIR)/%.obj : %.c
	$(CC) $(CFLAGS) $(CANOPEN_INC_DIRS) /Fo$@ -c $<

OBJE	= .obj
# some definitions to handle the linker commands
empty=
space= $(empty) $(empty)
plus= -+
endif


ifeq "$(COMPILER)" "SDCC"
#########################################################################
.SUFFIXES: .rel
OBJE	= .rel
$(CANOPEN_OBJ_DIR)/%.rel : %.c
	$(CC) -c $(CFLAGS) $(CANOPEN_INC_DIRS) $<
	mv $(subst .c,.rel,$?) $(CANOPEN_OBJ_DIR)
	mv $(subst .c,.lst,$?) $(CANOPEN_OBJ_DIR)

endif



ifeq "$(COMPILER)" "KC166"
#########################################################################
empty=
klammer=)
space= $(empty) $(empty)
spaceend=$(space)$(klammer)
comma=,

KI=$(subst -I,,$(CANOPEN_INC_DIRS))
KINCDIRS=$(subst $(space),;,$(strip $(KI)))
.SUFFIXES: .obj
$(CANOPEN_OBJ_DIR)/%.obj : %.c
	@echo $(<F) > cdir
	@echo $(CFLAGS) >> cdir
	@echo "DF(CONFIG_ALIGNMENT=2)" >> cdir
	@echo "INCDIR($(KINCDIRS))" >> cdir
	$(CC) @cdir
	mv $(basename $(<F)).obj $(CANOPEN_OBJ_DIR)

OBJE	= .obj
endif

ifeq "$(COMPILER)" "KC51"
#########################################################################
empty=
klammer=)
space= $(empty) $(empty)
spaceend=$(space)$(klammer)
comma=,

KI=$(subst -I,,$(CANOPEN_INC_DIRS))
KINCDIRS=$(subst $(space),;,$(strip $(KI)))
.SUFFIXES: .obj
$(CANOPEN_OBJ_DIR)/%.obj : %.c
	@echo $(<F) > cdir
	@echo $(CFLAGS) >> cdir
	#@echo "DF(CONFIG_ALIGNMENT=1)" >> cdir
	@echo "INCDIR($(KINCDIRS))" >> cdir
	$(CC) @cdir
	mv $(basename $(<F)).obj $(CANOPEN_OBJ_DIR)

OBJE	= .obj
endif

ifeq "$(COMPILER)" "COSMIC"
#########################################################################
.SUFFIXES: .o
$(CANOPEN_OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $(CANOPEN_INC_DIRS) $<

#OBJE	= .o
endif

#########################################################################
# objects for all versions
#########################################################################

CO_FILES =	\
		access_o	\
		cmsmain	\
		nmt	\
		cmscodec	\
		sdoserv	\
		sdoclien	\
		sdomain	\
		sdo \
		emerg \
		lme \
		pdo	\
		set_com	\
		sync	\
		utility	\
		time	\
		timer	\
		nmterr	\
		nmt_s	\
		led	\

# slave only
SLAVE_OBJECTS =	\
		heartbt	\

# master only
MASTER_OBJECTS=	\
		guard	\
		nmt_m	\
		heartbt	\
		nmtstart	\
		cfg_man	\

#		sdomgr$(OBJE)
#		sdoreq$(OBJE)

ifeq "$(CANOPEN_LIB_TYPE)" "MASTER_SLAVE_DLL"
CO_FILES += 	\
	$(MASTER_OBJECTS) \
	$(SLAVE_OBJECTS) \
	sdoblock	\
	lss	\
	mpdo
endif

ifneq "$(findstring MASTER,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += $(MASTER_OBJECTS)
endif

ifneq "$(findstring SLAVE,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += $(SLAVE_OBJECTS)
endif

ifneq "$(findstring MPDO,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += mpdo
endif

ifneq "$(findstring SDOBLOCK,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += sdoblock
endif

ifneq "$(findstring LSS,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += lss
endif

ifneq "$(findstring FLYMA,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += flyma
endif

ifneq "$(findstring REDCY,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += redcy
endif

ifneq "$(findstring SAFETY,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += srdo
endif

ifneq "$(findstring SLAVEPLUS,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += slave_ut
endif

ifneq "$(findstring DYNMEM,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += dynmem
endif

ifneq "$(findstring DEBUG,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += 	\
	co_debug	\
	cansend
endif

ifneq "$(findstring SPDO,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += spdo
endif

ifneq "$(findstring USER,$(CANOPEN_LIB_TYPE))" ""
CO_FILES += $(subst USER,,$(filter USER%,$(CANOPEN_LIB_TYPE)))
endif

#
#
ifeq "$(CANOPEN_LIB_TYPE)" "TEST_MASTER"
CO_FILES = $(EXTM_OBJECTS) \
	  $(SLAVE_OBJECTS)	\
	  $(DEBUG_OBJECTS)
endif
#

#
# make libraries
# ############################################################################

# objects are defined local, but created somwhere,
# so the target prefix must be added here in the dependencies list

#LIB_NAME = $(CANOPEN_LIB_NAME)
#include $(TOOLS_DIR)/Makefile.$(TARGET)












ifeq "$(COMPILER)" "SDCC"
#########################################################################
$(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME): $(addprefix $(CANOPEN_LIB_DIR)/,$(OBJECTS))
	$(AR) -r $(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME) $?
endif


ifeq "$(COMPILER)" "COSMIC"
#########################################################################
$(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME): $(addprefix $(CANOPEN_LIB_DIR)/,$(OBJECTS))
endif

# the linker does not accept directory prefixes in his objects list
# so we must change to the object directory.
# But because the dependency list contains the full name
# the directory part is deletet using $(notdir ..)
# tlink Syntax:
# tlink libname +-file+-fiele .....

ifeq "$(COMPILER)" "BC50"
#########################################################################
$(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME): $(addprefix $(CANOPEN_LIB_DIR)/,$(OBJECTS))
	cd $(CANOPEN_LIB_DIR);\
	tlib $(CANOPEN_LIB_NAME) $(plus)$(subst $(space),$(plus),$(notdir $?))
endif

ifeq "$(COMPILER)" "MSVC"
#########################################################################
$(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME): $(addprefix $(CANOPEN_LIB_DIR)/,$(OBJECTS))
	cd $(CANOPEN_LIB_DIR);
	echo $(CANOPEN_LIB_NAME) $(plus)$(subst $(space),$(plus),$(notdir $?))
endif

# Keil C166 under windows
ifeq "$(COMPILER)" "KC166"
#########################################################################
$(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME): $(addprefix $(CANOPEN_LIB_DIR)/,$(OBJECTS))
	@echo "CANopen object files created, make lib now"
	@echo " Update $@ with: $(subst $(space),$(comma),$(notdir $(basename $?)))"
	if [ ! -e $@ ] ; \
	then $(AR) create $@;\
	fi
#	-$(AR) delete $@ \($(subst $(space),$(comma),$(basename $?))\)
#	$(AR) add $(subst $(space),$(comma),$?) to $@
	-@for m in $(notdir $(basename $?)); do \
		$(AR) delete $@ \($$m\); done
	-@for m in $?; do \
		$(AR) add $$m to $@; done
	# $(AR) list $@
endif

# Keil C166 under windows
ifeq "$(COMPILER)" "KC51"
#########################################################################
$(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME): $(addprefix $(CANOPEN_LIB_DIR)/,$(OBJECTS))
	@echo "CANopen object files created, make lib now"
	@echo " Update $@ with: $(subst $(space),$(comma),$(notdir $(basename $?)))"
	if [ ! -e $@ ] ; \
	then $(AR) create $@;\
	fi
#	-$(AR) delete $@ \($(subst $(space),$(comma),$(basename $?))\)
#	$(AR) add $(subst $(space),$(comma),$?) to $@
	-@for m in $(notdir $(basename $?)); do \
		$(AR) delete $@ \($$m\); done
	-@for m in $?; do \
		$(AR) add $$m to $@; done
	# $(AR) list $@
endif

#
#########################################################################
# statistc utilities
#
.PHONY:statistic
statistic:
	@$(SIZE) *.o  > stat
	@echo "-------------------------------------------------------">> stat
	@awk '{c+=$$1;d+=$$2;b+=$$3}END{print c "\t" d "\t" b}' \
				< stat | tee --append stat
	@echo        >> stat
	@$(SIZE) *.o
#
# alternativly Information between Lines
# MODULE INFORMATION:
# END OF MODULE INFORMATION
LR=sdo.lst
.PHONY:statistic51
statistic51:
	@echo	>> stat
	@date   >> stat
	@awk '/^DOS C51/{print $$1,$$2,$$3,$$4}' $(LR) >> stat
	@grep "COMPILER INVOKED" $(LR) >> stat
	@echo "MODULE INFORMATION:   STATIC OVERLAYABLE" >> stat
	@for file in *.lst ; do \
	    echo $$file >> .tmp_stat ;\
	    grep "CODE SIZE" $$file >> .tmp_stat ;\
	    grep "CONSTANT SIZE" $$file >> .tmp_stat ;\
	    grep "DATA SIZE" $$file >> .tmp_stat ;\
	done
	@echo "-----------------------------------------">> .tmp_stat
	@echo "                STATIC       OVERLAYABLE"
	@awk '/CODE SIZE/{cs+=$$4;cd+=$$5}\
	     /TANT SIZE/{coxs+=$$4;cod+=$$5}\
	     /XDATA SIZE/{xs+=$$4;xd+=$$5}\
	     / DATA SIZE/{ds+=$$4;dd+=$$5}\
	    END{print "CODE SIZE\t" cs "\t\t" cd "\n" \
	              "CONSTANT SIZE\t" coxs "\t\t" cod "\n" \
	              "XDATA SIZE\t" xs "\t\t" xd "\n" \
	              "DATA SIZE\t" ds "\t\t" dd}' \
	    < .tmp_stat | tee --append .tmp_stat
	@cat .tmp_stat >> stat
	@echo "====================================" >> stat
	@rm .tmp_stat


#
# Ressourcen-Statistic
# Aus dem *-lst file werden ausgewertet
#
# CODE SIZE		x
# NEAR-CONST SIZE
# FAR-CONST SIZE	x
# HUGE-CONST SIZE
# XHUGE-CONST SIZE
# NEAR-DATA SIZE	x
# FAR-DATA SIZE		x
# XHUGE-DATA SIZE
# IDATA-DATA SIZE
# SDATA-DATA SIZE
# BDATA-DATA SIZE
# HUGE-DATA SIZE
# BIT SIZE		x
# INIT'L SIZE		x
#
#
#
#

.PHONY:statistic67
statistic67:
	@echo	>> stat
	@date   >> stat
	@awk '/^DOS C166/{print $$1,$$2,$$3,$$4}' $(LR) >> stat
	@grep "COMPILER INVOKED" $(LR) >> stat
	@echo -e "\nMODULE INFORMATION:   INITIALIZED UNINITIALIZED" >> stat
	@for file in *.lst ; do \
	    awk '/* Revision/{print FILENAME,$$4,$$5;exit}' $$file>> .tmp_stat;\
	    grep "CODE SIZE\|-DATA SIZE\|BIT SIZE\|-CONST SIZE\|INIT.L SIZE" \
	    		$$file >> .tmp_stat;\
	done
	@echo "-----------------------------------------">> .tmp_stat
	@echo "MODULE INFORMATION:   INITIALIZED UNINITIALIZED" >> .tmp_stat
	@echo "MODULE INFORMATION:   INITIALIZED UNINITIALIZED"
	@awk '/CODE SIZE/{cs+=$$4;csu+=$$5}\
	     /FAR-CONST SIZE/{fcs+=$$4;fcsu+=$$5}\
	     /NEAR-DATA SIZE/{nds+=$$4;ndsu+=$$5}\
	     /FAR-DATA SIZE/{fds+=$$4;fdsu+=$$5}\
	     /INIT.L SIZE/{is+=$$4;isu+=$$5}\
	     /BIT SIZE/{bs+=$$4;bsu+=$$5}\
	    END{print "CODE SIZE\t\t" cs "\t\t" csu "\n" \
	              "FAR-CONST SIZE\t\t" fcs "\t\t" fcsu "\n" \
	              "NEAR-DATA SIZE\t\t" nds "\t\t" ndsu "\n"\
	              "FAR-DATA SIZE\t\t" fds "\t\t" fdsu "\n"\
	              "INITL SIZE\t\t" is "\t\t" isu "\n"\
	              "BIT SIZE\t\t" bs "\t\t" bsu}' \
	    < .tmp_stat | tee --append .tmp_stat
	@cat .tmp_stat | tr -d "\015" >> stat
	@echo "====================================" >> stat
	@rm .tmp_stat


.PHONY:co_help
co_help:
	@echo "Makefile for the CANopen Library"
	@sed -n '/^##/s/^## //p' Makefile

## Makefile for the CANopen Library
## This MAkefile is normaly called from othe Project Makefiles.
## Standalone targets are:
##
## make lint CANOPEN_LIB_TYPE=xxx
##   CANOPEN_LIB_TYPE=
## 	MASTER
## 	SLAVE
## 	MASTER_SLAVE
## 	EXT_MASTER
## 	EXT_SLAVE
##
## make timertest
## make ledtest

# lint should check all .c files of a possible CANOPEN_LIB_TYPE configuration.
# It must use a specific cal_conf.h file.

OBJE=.c
lint:
	@cp lint.cal_conf.h ../include/cal_conf.h
	splint -I../include $(OBJECTS) -I../../drivers/shar_inc -showsummary
	@rm ../include/cal_conf.h

#############################################################################
# make timertest application
TIMER_OBJECTS =  timertest.o timer.o
TIMER_CFLAGS  = -g -c -Wall -DCONFIG_USER_TIMER_EVENT=1 \
		-DTEST_INSERT_INDICATION=1    \
		-DTIMER_DEBUG_STANDALONE=1
		#-DTIMER_DEBUG=1 \


TIMER_INC_DIR = -I. \
		-I../include
CCX = gcc
timertest: $(TIMER_OBJECTS)
	$(CCX) -o timertest -v $(TIMER_OBJECTS)

#timer.o: timer.c
#	$(CCX) $(TIMER_CFLAGS) $(TIMER_INC_DIR) -o$@ $<

timertest.o: timertest.c
	$(CCX) $(TIMER_CFLAGS) $(TIMER_INC_DIR) -o$@ $<


#############################################################################
# make ledtest application

LED_OBJECTS = led.o ledtest.o
LED_CFLAGS  = -g -c -Wall -DDEBUG_STANDALONE \
              -DCONFIG_CO_LED -DCONFIG_CO_BOTH_LED

LED_INC_DIR = -I. -I../include
CCX = gcc

ledtest : $(LED_OBJECTS)
	$(CCX) -o ledtest -v $(LED_OBJECTS)

ledtest.o : ledtest.c
	$(CCX) $(LED_CFLAGS) $(LED_INC_DIR) -o $@ $<

#led.o : led.c
#	$(CCX) $(LED_CFLAGS) $(LED_INC_DIR) -o $@ $<

.PHONY: cleanled
cleanled:
	@rm $(LED_OBJECTS) ledtest

#############################################################################
# Canopen clean, try to clean from the main makefile
.PHONY: cclean
cclean:
	-rm $(OBJECTS) $(CANOPEN_LIB_DIR)/$(CANOPEN_LIB_NAME)

.PHONY: clean
co_clean:
	-rm --verbose *.o *.lst *.obj

.PHONY: ctags_lib
ctags_lib:
	ctags *.[ch] ../include/*.h
