SUMMARY = "Access, organize and share your photos on GNOME"
SECTION = "x11/gnome"
LICENSE = "GPLv3"
LIC_FILES_CHKSUM = "file://COPYING;md5=1ebbd3e34237af26da5dc08a4e440464"

DEPENDS = " \
    glib-2.0-native \
    gdk-pixbuf-native \
    librsvg-native \
    gtk+3 \
    babl \
    gegl \
    geocode-glib \
    gnome-online-accounts \
    grilo \
    gsettings-desktop-schemas \
    libdazzle \
    libgdata \
    gfbgraph \
    tracker \
    libhandy \
"

GNOMEBASEBUILDCLASS = "meson"

inherit gnomebase gettext upstream-version-is-even gnome-help features_check

def gnome_verdir(v):
    return oe.utils.trim_version(v, 1)

REQUIRED_DISTRO_FEATURES = "x11"

SRC_URI[archive.sha256sum] = "e02d73e138af8b2868b5cad7faa1bdd278aeade3b6c3c92836511a4e6f3af1af"

FILES_${PN} += " \
    ${datadir}/dbus-1 \
    ${datadir}/metainfo \
    ${datadir}/gnome-shell \
"

RRECOMMENDS_${PN} = "grilo-plugins"
