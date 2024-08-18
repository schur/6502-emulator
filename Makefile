CC      = gcc
CFLAGS  = -Wall -O2
LIBS	= -lstdc++ -lm -lGL -lX11 -lpng
DEPS	= olcPixelGameEngine.h
OBJ		= 6502_demo.o Bus.o olc6502.o 
OUT		= 6502_demo

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
clean: 
	rm -f *.o *~ core $(OUT) 
