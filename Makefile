
ECHO_PREFIX=Makefile: 
ARDUINO_ROOT=/home/denis/arduino/arduino-1.8.5
ARDUINO=$(ARDUINO_ROOT)/arduino
TARGET=dlar
TTY=/dev/ttyACM*
TTY_BAUD=9600


all: install tty


install:
	@echo "$(ECHO_PREFIX)Compiling and installing ..."
	@echo

	$(ARDUINO) --upload "$(TARGET)" --port $(TTY)


tty:
	stty -F $(TTY) $(TTY_BAUD) -parenb -parodd cs8 -hupcl -cstopb cread clocal -crtscts -ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr -icrnl -ixon -ixoff -iuclc -ixany -imaxbel -iutf8 -opost -olcuc -ocrnl -onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0 -isig -icanon -iexten -echo -echoe -echok -echonl noflsh -xcase -tostop -echoprt -echoctl -echoke
	@echo "$(ECHO_PREFIX)SERIAL OUTPUT FOLLOWS:"
	@echo

	@bash -c "cat < $(TTY)"
