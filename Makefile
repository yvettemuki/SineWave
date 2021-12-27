sources = \
	main.cpp \
	mesh.cpp \
	util.cpp \
	gl_core_3_3.c
libs = \
	-lGL \
	-lglut
outname = assignment3

all:
	g++ -std=c++11 $(sources) $(libs) -o $(outname)
clean:
	rm $(outname)
