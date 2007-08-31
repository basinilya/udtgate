bin_PROGRAMS     = ugw_testconn test

ugw_testconn_SOURCES     = testconn.cpp
test_SOURCES             = test.cpp

test_DEPENDENCIES = ../lib/libudtgate.a
test_LDADD        = ../lib/libudtgate.a

ugw_testconn_DEPENDENCIES = ../lib/libudtgate.a
ugw_testconn_LDADD        = ../lib/libudtgate.a

../lib/libudtgate.a:
	cd ../lib; make; cd ../relay;
