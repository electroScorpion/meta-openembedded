SUMMARY = "PostgreSQL is a powerful, open source relational database system."
DESCRIPTION = "\
    PostgreSQL is an advanced Object-Relational database management system \
    (DBMS) that supports almost all SQL constructs (including \
    transactions, subselects and user-defined types and functions). The \
    postgresql package includes the client programs and libraries that \
    you'll need to access a PostgreSQL DBMS server.  These PostgreSQL \
    client programs are programs that directly manipulate the internal \
    structure of PostgreSQL databases on a PostgreSQL server. These client \
    programs can be located on the same machine with the PostgreSQL \
    server, or may be on a remote machine which accesses a PostgreSQL \
    server over a network connection. This package contains the docs \
    in HTML for the whole package, as well as command-line utilities for \
    managing PostgreSQL databases on a PostgreSQL server. \
    \
    If you want to manipulate a PostgreSQL database on a local or remote \
    PostgreSQL server, you need this package. You also need to install \
    this package if you're installing the postgresql-server package. \
"
HOMEPAGE = "http://www.postgresql.com"
LICENSE = "0BSD"
DEPENDS = "libnsl2 tzcode-native perl bison-native"

ARM_INSTRUCTION_SET = "arm"

SRC_URI = "https://ftp.postgresql.org/pub/source/v${PV}/${BP}.tar.bz2 \
    file://postgresql.init \
    file://postgresql-profile \
    file://postgresql.pam \
    file://postgresql-setup \
    file://postgresql.service \
"

LEAD_SONAME = "libpq.so"

# LDFLAGS for shared libraries
export LDFLAGS_SL = "${LDFLAGS}"
export LDFLAGS_EX_BE = "-Wl,--export-dynamic"

inherit autotools pkgconfig perlnative python3native python3targetconfig useradd update-rc.d systemd gettext perl-version multilib_header

CFLAGS += "-I${STAGING_INCDIR}/${PYTHON_DIR}"

SYSTEMD_SERVICE:${PN} = "postgresql.service"
SYSTEMD_AUTO_ENABLE:${PN} = "disable"

pkg_postinst:${PN} () {
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd sysvinit', 'true', 'false', d)}; then
        if [ -n "$D" ]; then
            OPTS="--root=$D"
        fi
        systemctl $OPTS mask postgresql-server.service
    fi
}

PACKAGECONFIG ??= " \
    ${@bb.utils.filter('DISTRO_FEATURES', 'pam systemd', d)} \
    openssl python uuid libxml perl zlib icu readline \
"
PACKAGECONFIG[tcl] = "--with-tcl --with-tclconfig=${STAGING_BINDIR_CROSS},--without-tcl,tcl tcl-native,"
PACKAGECONFIG[perl] = "--with-perl,--without-perl,perl,perl"
PACKAGECONFIG[python] = "--with-python,--without-python,python3,python3"
PACKAGECONFIG[gssapi] = "--with-gssapi,--without-gssapi,krb5"
PACKAGECONFIG[pam] = "--with-pam,--without-pam,libpam"
PACKAGECONFIG[ldap] = "--with-ldap,--without-ldap,openldap"
PACKAGECONFIG[systemd] = "--with-systemd,--without-systemd,systemd systemd-systemctl-native"
PACKAGECONFIG[uuid] = "--with-uuid=e2fs,--without-uuid,util-linux"
PACKAGECONFIG[libxml] = "--with-libxml,--without-libxml,libxml2,libxml2"
PACKAGECONFIG[libxslt] = "--with-libxslt,--without-libxslt,libxslt"
PACKAGECONFIG[zlib] = "--with-zlib,--without-zlib,zlib"
PACKAGECONFIG[lz4] = "--with-lz4,--without-lz4,lz4"
PACKAGECONFIG[openssl] = "--with-ssl=openssl,ac_cv_file__dev_urandom=yes,openssl"
PACKAGECONFIG[icu] = "--with-icu,--without-icu,icu,icu"
PACKAGECONFIG[readline] = "--with-readline,--without-readline,readline"

EXTRA_OECONF += "--disable-rpath \
    --datadir=${datadir}/${BPN} \
    --sysconfdir=${sysconfdir}/${BPN} \
"
EXTRA_OECONF:sh4 += "--disable-spinlocks"

DEBUG_OPTIMIZATION:remove:mips = " -Og"
DEBUG_OPTIMIZATION:append:mips = " -O"
BUILD_OPTIMIZATION:remove:mips = " -Og"
BUILD_OPTIMIZATION:append:mips = " -O"

DEBUG_OPTIMIZATION:remove:mipsel = " -Og"
DEBUG_OPTIMIZATION:append:mipsel = " -O"
BUILD_OPTIMIZATION:remove:mipsel = " -Og"
BUILD_OPTIMIZATION:append:mipsel = " -O"

PACKAGES_DYNAMIC += "^${PN}-plperl \
    ^${PN}-pltcl \
    ^${PN}-plpython \
"

python populate_packages:prepend() {

    def fill_more(name):
        if name is None or name.strip() == "":
            return

        fpack=d.getVar('PACKAGES', False) or ""
        fpack="${PN}-" + name + " " + fpack
        d.setVar('PACKAGES', fpack)

    conf=(d.getVar('PACKAGECONFIG') or "").split()
    pack=d.getVar('PACKAGES', False) or ""
    bb.debug(1, "PACKAGECONFIG=%s" % conf)
    bb.debug(1, "PACKAGES1=%s" % pack )

    if "perl" in conf :
        fill_more("plperl")

    if "tcl" in conf:
        fill_more("pltcl")

    if "python" in conf:
        fill_more("plpython")

    pack=d.getVar('PACKAGES') or ""
    bb.debug(1, "PACKAGES2=%s" % pack)

}

# Same as the function in cpan-base.bbclass (but without the perl RDEPENDS)
def is_target(d):
    if not bb.data.inherits_class('native', d):
        return "yes"
    return "no"

# This will make native perl use target settings (for include dirs etc.)
export PERLCONFIGTARGET = "${@is_target(d)}"
export PERL_ARCHLIB = "${STAGING_LIBDIR}${PERL_OWN_DIR}/perl5/${@get_perl_version(d)}/${@get_perl_arch(d)}"

EXTRA_AUTORECONF += "-I config"

do_configure() {
    # do_configure
    autotools_do_configure

    # do_configure:append
    # workaround perl package related bugs
    sed -i -e "s:-L/usr/local/lib:-L=/usr/local/lib:g" \
        ${B}/src/Makefile.global
    LIBPNA="\${STAGING_LIBDIR_NATIVE}/perl-native"
    LIBNA="\${STAGING_LIBDIR_NATIVE}"
    BLIBNA="\${STAGING_BASE_LIBDIR_NATIVE}"
    sed -i -e "/^perl_archlibexp/s:${LIBPNA}:${STAGING_LIBDIR}:g" \
        ${B}/src/Makefile.global
    sed -i -e "/^perl_privlibexp/s:${libdir}:${STAGING_LIBDIR}:g" \
        ${B}/src/Makefile.global
    # remove the rpath, replace with correct lib path
    sed -i \
        -e "/^perl_embed_ldflags/s:-Wl,-rpath,${LIBNA}::g" \
        -e "/^perl_embed_ldflags/s:-Wl,-rpath,${BLIBNA}::g" \
        -e "/^perl_embed_ldflags/s:-Wl,-rpath-link,${LIBNA}::g" \
        -e "/^perl_embed_ldflags/s:-Wl,-rpath-link,${BLIBNA}::g" \
        -e "/^perl_embed_ldflags/s:${LIBPNA}:${STAGING_LIBDIR}:g" \
        -e "/^perl_embed_ldflags/s:${LIBNA}:${STAGING_LIBDIR}:g"  \
        -e "/^perl_embed_ldflags/s:${BLIBNA}:${STAGING_BASELIBDIR}:g" \
        -e "/^TCLSH/s:=.*:= ${bindir}/tclsh:g" \
    ${B}/src/Makefile.global

    if ${@bb.utils.contains('PACKAGECONFIG', 'perl', 'true', 'false', d)}; then
        # workaround perl package's libperl.so problem
        # we are using perlnative so this perl should have same version
        perl_version=`perl -v 2>/dev/null | \
            sed -n 's/This is perl.*v[a-z ]*\([0-9]\.[0-9][0-9.]*\).*$/\1/p'`
        if [ ! -h "${STAGING_LIBDIR}/perl/$perl_version/CORE/libperl.so" -a \
             ! -h "${STAGING_LIBDIR}/libperl.so" ]; then
            ln -sf ../../../libperl.so.5 \
                ${STAGING_LIBDIR}/perl/$perl_version/CORE/libperl.so
        fi
    fi
}

do_compile:append() {
    oe_runmake -C contrib all

    for f in `find ${B} -name "*.[c|h]"`;
    do
        sed -i -e 's,${B}/../,,' $f
    done
}

# server needs to configure user and group
usernum = "28"
groupnum = "28"
USERADD_PACKAGES = "${PN}"
USERADD_PARAM:${PN} = "-M -g postgres -o -r -d ${localstatedir}/lib/${BPN} \
    -s /bin/sh -c 'PostgreSQL Server' -u ${usernum} postgres"
GROUPADD_PARAM:${PN} = "-g ${groupnum} -o -r postgres"

INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME = "${BPN}-server"
INITSCRIPT_PARAMS = "start 64 . stop 36 0 1 2 3 4 5 6 ."

do_install:append() {
    # install contrib
    oe_runmake DESTDIR=${D} -C contrib install
    oe_multilib_header pg_config.h pg_config_ext.h ecpg_config.h postgresql/server/pg_config.h postgresql/server/pg_config_ext.h
    # install tutorial
    install -d -m 0755 ${D}${libdir}/${BPN}/tutorial
    install ${B}/src/tutorial/* ${D}${libdir}/${BPN}/tutorial

    # install COPYRIGHT README HISTORY
    install -d -m 0755 ${D}${docdir}/${BPN}
    for i in ${B}/COPYRIGHT ${B}/README ${B}/HISTORY ${B}/doc/KNOWN_BUGS ${B}/doc/MISSING_FEATURES ${B}/doc/README* ${B}/doc/bug.template; do
        [ -f $i ] && install $i ${D}${docdir}/${BPN}
    done

    # install dirs and server init
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${UNPACKDIR}/${BPN}.init ${D}${sysconfdir}/init.d/${BPN}-server
    sed -i -e "s/^PGVERSION=.*$/PGVERSION=${PV}/g" ${D}${sysconfdir}/init.d/${BPN}-server
    install -m 0755 ${UNPACKDIR}/${BPN}-setup ${D}${bindir}/${BPN}-setup
    install -d -m 700 ${D}${localstatedir}/lib/${BPN}/data
    install -d -m 700 ${D}${localstatedir}/lib/${BPN}/backups
    install -m 644 ${UNPACKDIR}/${BPN}-profile ${D}${localstatedir}/lib/${BPN}/.profile
    chown -R postgres:postgres ${D}${localstatedir}/lib/${BPN}
    # multiple server config directory
    install -d -m 700 ${D}${sysconfdir}/default/${BPN}

    if ${@bb.utils.contains('DISTRO_FEATURES', 'pam', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/pam.d
        install -m 644 ${UNPACKDIR}/postgresql.pam ${D}${sysconfdir}/pam.d/postgresql
    fi

    # Install systemd unit files
    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${UNPACKDIR}/postgresql.service ${D}${systemd_unitdir}/system
    sed -i -e 's,@BINDIR@,${bindir},g' \
        ${D}${systemd_unitdir}/system/postgresql.service
    # Remove the build path
    if [ -f ${D}${libdir}/${BPN}/pgxs/src/Makefile.global ]; then
        sed -i -e 's#${RECIPE_SYSROOT}##g' \
               -e 's#${RECIPE_SYSROOT_NATIVE}##g' \
               -e 's#${WORKDIR}##g' \
               -e 's#${TMPDIR}##g' \
             ${D}${libdir}/${BPN}/pgxs/src/Makefile.global
    fi
}

SSTATE_SCAN_FILES += "Makefile.global"
SSTATE_SCAN_FILES:remove = "*_config"

postgresql_fix_sources () {
	for f in ${PKGD}${TARGET_DBGSRC_DIR}/src/pl/plperl/Util.c \
			${PKGD}${TARGET_DBGSRC_DIR}/src/pl/plperl/SPI.c; do
		if [ -e $f ]; then
			sed -i -e 's#${B}/../${BP}#${TARGET_DBGSRC_DIR}#g' $f
		fi
	done
}
PACKAGESPLITFUNCS =+ "postgresql_fix_sources"

PACKAGES =+ "${PN}-client ${PN}-server-dev ${PN}-timezone \
    libecpg-compat libecpg-compat-dev \
    libecpg libecpg-dev libecpg-staticdev libecpg-doc \
    libpq libpq-dev libpq-staticdev \
    libpgtypes libpgtypes-staticdev libpgtypes-dev \
    ${PN}-contrib \
"

RPROVIDES:${PN}-dbg += "libecpg-compat-dbg \
                        libecpg-dbg \
                        libpq-dbg \
                        libpgtypes-dbg \
                        ${PN}-contrib-dbg \
                        ${PN}-pltcl-dbg \
                        ${PN}-plpython-dbg \
                        ${PN}-plperl-dbg \
                       "

FILES:${PN} += "${sysconfdir}/init.d/${BPN}-server \
    ${localstatedir}/lib/${BPN}/data ${localstatedir}/lib/${BPN}/backups \
    ${localstatedir}/lib/${BPN}/.profile ${sysconfdir}/default/${BPN} \
    ${libdir}/${BPN}/dict_snowball.so ${libdir}/${BPN}/plpgsql.so \
    ${libdir}/${BPN}/euc2004_sjis2004.so \
    ${libdir}/${BPN}/libpqwalreceiver.so \
    ${libdir}/${BPN}/*_and_*.so \
    ${@'${sysconfdir}/pam.d/postgresql' \
       if 'pam' == d.getVar('enable_pam') \
       else ''} \
"

FILES:${PN}-client = "${bindir}/clusterdb \
    ${bindir}/createdb \
    ${bindir}/createuser \
    ${bindir}/dropdb \
    ${bindir}/dropuser \
    ${bindir}/pg_dump \
    ${bindir}/pg_dumpall \
    ${bindir}/pg_restore \
    ${bindir}/psql \
    ${bindir}/reindexdb \
    ${bindir}/vacuumdb \
    ${bindir}/vacuumlo \
    ${datadir}/${BPN}/psqlrc.sample \
"
FILES:${PN}-client-doc = "${mandir}/man1/clusterdb.* \
    ${mandir}/man1/createdb.*   ${mandir}/man1/createlang.* \
    ${mandir}/man1/createuser.* ${mandir}/man1/dropdb.* \
    ${mandir}/man1/droplang.*   ${mandir}/man1/dropuser.* \
    ${mandir}/man1/pg_dump.*    ${mandir}/man1/pg_dumpall.* \
    ${mandir}/man1/pg_restore.* ${mandir}/man1/psql.* \
    ${mandir}/man1/reindexdb.*  ${mandir}/man1/vacuumdb.* \
    ${mandir}/man7/* \
"
FILES:${PN}-doc += "${docdir}/${BPN}/html ${libdir}/${BPN}/tutorial/ \
    ${mandir}/man1/initdb.* ${mandir}/man1/pg_controldata.* \
    ${mandir}/man1/pg_ctl.* ${mandir}/man1/pg_resetxlog.* \
    ${mandir}/man1/postgres.* ${mandir}/man1/postmaster.* \
"
FILES:${PN}-timezone = "${datadir}/${BPN}/timezone \
    ${datadir}/${BPN}/timezonesets \
"
RDEPENDS:${PN} += "${PN}-timezone"
FILES:${PN}-server-dev = "${includedir}/${BPN}/server \
                          ${libdir}/${BPN}/pgxs \
"

FILES:libecpg = "${libdir}/libecpg*${SOLIBS}"
FILES:libecpg-dev = "${libdir}/libecpg*${SOLIBSDEV} \
    ${libdir}/libpgtypes*${SOLIBSDEV} \
    ${includedir}/ecpg*.h ${includedir}/${BPN}/ecpg*.h \
    ${includedir}/pgtypes*.h ${includedir}/${BPN}/informix \
    ${includedir}/sql3types.h ${includedir}/sqlca.h \
"
FILES:libecpg-doc = "${mandir}/man1/ecpg.*"
FILES:libecpg-staticdev = "${libdir}/libecpg*.a"
SECTION:libecpg-staticdev = "devel"
RDEPENDS:libecpg-staticdev = "libecpg-dev (= ${EXTENDPKGV})"

FILES:libpq = "${libdir}/libpq*${SOLIBS}"
FILES:libpq-dev = "${libdir}/libpq*${SOLIBSDEV} \
    ${includedir} \
"
FILES:libpq-staticdev = "${libdir}/libpq*.a ${libdir}/libpgport.a"
SECTION:libpq-staticdev = "devel"
RDEPENDS:libpq-staticdev = "libpq-dev (= ${EXTENDPKGV})"

FILES:libecpg-compat = "${libdir}/libecpg_compat*${SOLIBS}"
FILES:libecpg-compat-dev = "${libdir}/libecpg_compat*${SOLIBS}"
FILES:libpgtypes = "${libdir}/libpgtypes*${SOLIBS}"
FILES:libpgtypes-staticdev = "${libdir}/libpgtypes*.a"
FILES:libpgtypes-dev = "${libdir}/libpgtypes*${SOLIBS} ${includedir}/pgtypes*.h"

FILES:${PN}-contrib = " ${bindir}/oid2name ${bindir}/pg_standby \
    ${bindir}/pgbench \
    ${S}/contrib/spi/*.example \
    ${libdir}/${BPN}/_int.so ${libdir}/${BPN}/adminpack.so \
    ${libdir}/${BPN}/autoinc.so ${libdir}/${BPN}/auto_explain.so \
    ${libdir}/${BPN}/auth_delay.so ${libdir}/${BPN}/btree_gin.so \
    ${libdir}/${BPN}/btree_gist.so ${libdir}/${BPN}/.so \
    ${libdir}/${BPN}/chkpass.so ${libdir}/${BPN}/citext.so \
    ${libdir}/${BPN}/cube.so ${libdir}/${BPN}/dblink.so \
    ${libdir}/${BPN}/dict_int.so ${libdir}/${BPN}/dict_xsyn.so \
    ${libdir}/${BPN}/dummy_seclabel.so ${libdir}/${BPN}/earthdistance.so \
    ${libdir}/${BPN}/file_fdw.so ${libdir}/${BPN}/fuzzystrmatch.so \
    ${libdir}/${BPN}/hstore.so ${libdir}/${BPN}/insert_username.so \
    ${libdir}/${BPN}/isn.so ${libdir}/${BPN}/lo.so \
    ${libdir}/${BPN}/ltree.so ${libdir}/${BPN}/moddatetime.so \
    ${libdir}/${BPN}/pageinspect.so ${libdir}/${BPN}/pg_buffercache.so \
    ${libdir}/${BPN}/pg_freespacemap.so ${libdir}/${BPN}/pg_trgm.so \
    ${libdir}/${BPN}/pgcrypto.so ${libdir}/${BPN}/pgrowlocks.so \
    ${libdir}/${BPN}/pgstattuple.so ${libdir}/${BPN}/pg_stat_statements.so \
    ${libdir}/${BPN}/refint.so ${libdir}/${BPN}/seg.so \
    ${libdir}/${BPN}/sslinfo.so \
    ${libdir}/${BPN}/tablefunc.so \
    ${libdir}/${BPN}/test_parser.so ${libdir}/${BPN}/timetravel.so \
    ${libdir}/${BPN}/uuid-ossp.so \
    ${libdir}/${BPN}/pgxml.so ${libdir}/${BPN}/passwordcheck.so \
    ${libdir}/${BPN}/pg_upgrade_support.so ${libdir}/${BPN}/.so \
    ${libdir}/${BPN}/unaccent.so \
"
DESCRIPTION:${PN}-contrib = "The postgresql-contrib package contains \
    contributed packages that are included in the PostgreSQL distribution."

FILES:${PN}-pltcl = "${libdir}/${BPN}/pltcl.so ${bindir}/pltcl_delmod \
    ${binddir}/pltcl_listmod ${bindir}/pltcl_loadmod  \
    ${datadir}/${BPN}/unknown.pltcl"
SUMMARY:${PN}-pltcl = "The Tcl procedural language for PostgreSQL"
DESCRIPTION:${PN}-pltcl = "PostgreSQL is an advanced Object-Relational \
    database management system.  The postgresql-pltcl package contains the PL/Tcl \
    procedural language for the backend."

FILES:${PN}-plperl = "${libdir}/${BPN}/plperl.so"
SUMMARY:${PN}-plperl = "The Perl procedural language for PostgreSQL"
DESCRIPTION:${PN}-plperl = "PostgreSQL is an advanced Object-Relational \
    database management system.  The postgresql-plperl package contains the \
    PL/Perl procedural language for the backend."

# In version 8, it will be plpython.so
# In version 9, it might be plpython{2,3}.so depending on python2 or 3
FILES:${PN}-plpython = "${libdir}/${BPN}/plpython*.so"
SUMMARY:${PN}-plpython = "The Python procedural language for PostgreSQL"
DESCRIPTION:${PN}-plpython = "PostgreSQL is an advanced Object-Relational \
    database management system.  The postgresql-plpython package contains \
    the PL/Python procedural language for the backend."
