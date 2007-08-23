
bin_PROGRAMS     = test
test_SOURCES     = test.cpp

test_DEPENDENCIES = ../lib/libudtgate.a
test_LDADD        = ../lib/libudtgate.a

../lib/libudtgate.a:
	cd ../lib; make; cd ../relay;
