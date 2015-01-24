CC=gcc
CFLAGS=-ansi -O2
OUTPUT=otp
PREFIX=/usr

otp: otp.c

install: otp
	cp $(OUTPUT) $(PREFIX)/bin/
	cp otp.1 $(PREFIX)/share/man/man1/

.c:
	$(CC) $< $(CFLAGS) -o $(OUTPUT) 

clean:
	rm -f otp

uninstall:
	rm $(PREFIX)/bin/$(OUTPUT)
	rm $(PREFIX)/share/man/man1/otp.1

