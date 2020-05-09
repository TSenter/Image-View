CC=gcc
CFLAGS=-g
DEPS = formats/png.h
OBJ = main.o formats/png.o

img_view: $(OBJ)
	$(CC) -o $@ $^ utils/version.h $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
