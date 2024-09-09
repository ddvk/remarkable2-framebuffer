TEMPLATE = subdirs

include(version.pri)

SUBDIRS = \
	src/loader \   # relative paths
	src/server \
	src/client \
	src/xofb   \

