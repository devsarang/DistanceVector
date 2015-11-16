################################################################################
SDIR =src
IDIR =inc
CC=g++
CFLAGS=-I$(IDIR) -g

ODIR=obj
LDIR =../lib

LIBS=-lm

_DEPS = RouterPacket.h Constants.h ServerRouter.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = ServerRouter.o sarangde_proj2.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -v -o $@ $< $(CFLAGS)
all: server

server : $(OBJ)
	g++ -v -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

