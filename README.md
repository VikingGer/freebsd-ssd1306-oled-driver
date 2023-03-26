# freebsd-ssd1306-oled-driver
Driver to show FreeBSD system stats on a ssd1306 128x32 pixel oled display

The main goal in developing this driver was to run OPNSense on a 52Pi CM4 router board and display some system statistics.
Parts of the code were written with ChatGPT (support was provided by the OpenAI GPT-3 API) and using available code from:

Ref:
  1. https://github.com/610t/FreeBSD_I2C
  2. https://github.com/NateLol/luci-app-oled
  3. https://github.com/geerlingguy/raspberry-pi-pcie-devices/issues/337

Feel free to optimize the code.

## How to setup the display:
- Compile the src like: clang -o ssd1306 ssd1306.c
- Copy the resulting binary to /usr/local/bin/
> chmod +x /usr/local/bin/ssd1306
### The driver should be started as daemon. 
- Therefore copy "ssd1306_oled" into /usr/local/etc/rc.d/
> chmod +x /usr/local/etc/rc.d/ssd1306_oled

> echo 'ssd1306_oled_enable="YES"' > /etc/rc.conf.d/ssd1306_oled

## Interact:
> service ssd1306_oled start
> 
> service ssd1306_oled stop

## Raspi config.txt:
dtparam=i2c_arm=on

gpio=2,3=a0

https://forums.freebsd.org/threads/rpi4-i2c-devices.77825/#post-510324
