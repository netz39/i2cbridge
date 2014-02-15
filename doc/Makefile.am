
man_MANS = i2cbridge.1 i2cbridge_iclient.1 i2cbridge_uclient.1
EXTRA_DIST = i2cbridge.1.in i2cbridge_iclient.1.in i2cbridge_uclient.1.in
CLEANFILES = i2cbridge.1 i2cbridge_iclient.1 i2cbridge_uclient.1

SUBST = " \
    s|%PACKAGE%|@PACKAGE@|g; \
    s|%PACKAGE_NAME%|@PACKAGE_NAME@|g; \
    s|%PACKAGE_VERSION%|@PACKAGE_VERSION@|g; \
    s|%PACKAGE_URL%|@PACKAGE_URL@|g; \
    s|%pkgdatadir%|$(pkgdatadir)|g; \
    s|%includedir%|$(includedir)|g; \
    s|%DATE%|$(shell date "+%d %b %Y")|g; \
    s^%PORT%^$(shell grep -i @PACKAGE@_port $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
    s^%PWD%^$(shell grep -i @PACKAGE@_pwd $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)@PACKAGE@^g; \
    s^%UNAME%^$(shell grep -i @PACKAGE@_unix $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
    s^%ERROR_OK%^$(shell grep -i @PACKAGE@_ERROR_OK $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
    s^%ERROR_INTERNAL%^$(shell grep -i @PACKAGE@_ERROR_INTERNAL $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
    s^%ERROR_COMMAND%^$(shell grep -i @PACKAGE@_ERROR_COMMAND $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
    s^%ERROR_ADDRESS%^$(shell grep -i @PACKAGE@_ERROR_ADDRESS $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
    s^%ERROR_I2C%^$(shell grep -i @PACKAGE@_ERROR_I2C $(top_srcdir)/src/@PACKAGE@.h|tr -s " "|cut -d" " -f3)^g; \
"

% : %.in
	sed -e $(SUBST) $< >$@
