bin_PROGRAMS=wsnet1

AM_CPPFLAGS=-I$(top_srcdir)/include -I$(top_srcdir)/../.. 

TRACER=$(top_srcdir)/../../libtracer/tracer.c \
	$(top_srcdir)/../../libtracer/tracer_vcd.c \
	$(top_srcdir)/../../libtracer/tracer_bin.c

LOGGER=$(top_srcdir)/../../liblogger/logger.c

wsnet1_SOURCES=			\
	command_line.c		\
	core.c			\
	log.c			\
	nodes.c			\
	packets.c		\
	sim_conf.c		\
	simulation.c		\
	worldsens_srv.c		\
	worldsens_wsnet.c 	\
	$(TRACER) $(LOGGER)

wsnet1_CFLAGS=-DWSNET1
