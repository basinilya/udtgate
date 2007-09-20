lib_LTLIBRARIES = libminisocks.la
noinst_LIBRARIES = libudtgate.a

libudtgate_a_SOURCES =  socket_api.cpp utils.cpp logger.cpp  setproctitle.cpp\
			../include/socket_api.h ../include/utils.h \
            		../include/logger.h ../include/setproctitle.h worker.cpp \
			udtgate.cpp ../include/loopbuffer.h

libminisocks_la_SOURCES = minisocks.cpp


noinst_HEADERS = ../include/udtgate.h
