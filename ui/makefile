TOPDIR = ..
include $(TOPDIR)/makefile.config

OBJECTS = \
 ui.$(OBJ) mouse.$(OBJ) window.$(OBJ) uidraw.$(OBJ) button.$(OBJ) \
 gadget.$(OBJ) listbox.$(OBJ) message.$(OBJ) popup.$(OBJ) scroll.$(OBJ) \
 radio.$(OBJ) checkbox.$(OBJ) inputbox.$(OBJ) userbox.$(OBJ) menu.$(OBJ) \
 keytrap.$(OBJ) icon.$(OBJ) keypress.$(OBJ) \
 menubar.$(OBJ) keypad.$(OBJ)

ifdef ENV_LINUX
OBJECTS += lfile.$(OBJ)
else
OBJECTS += file.$(OBJ)
endif

OUTPUT = $(LIBDIR)/ui.$(LIB)

include $(TOPDIR)/makefile.lib
