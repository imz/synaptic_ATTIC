@INTLTOOL_DESKTOP_RULE@

desktopdir       = $(datadir)/applications
desktop_in_files = synaptic.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

kdeappdir       = $(datadir)/applications
kdeapp_in_files = synaptic-kde.desktop.in
kdeapp_DATA     = $(kdeapp_in_files:.desktop.in=.desktop)

EXTRA_DIST = $(desktop_in_files) \
		$(desktop_DATA) \
		$(kdeapp_in_files) \
		$(kdeapp_DATA) 
