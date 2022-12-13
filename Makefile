TARGET = dpmutil

OBJECTS = dpmutil.o I2CHAL.o PlatformMCU.o syzygy.o ZmodADC.o ZmodDAC.o ZmodDigitizer.o main.o

CC = gcc
LD = gcc
RM = rm -f

CFLAGS = -Wall

all: $(TARGET)

%.o: %.c
	${CC} -c ${CFLAGS} $< -o $@

$(TARGET): $(OBJECTS)
	$(LD) $(OBJECTS) -o $@

clean:
	$(RM) *.o $(TARGET)
