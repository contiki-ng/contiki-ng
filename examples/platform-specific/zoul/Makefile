CONTIKI_PROJECT = test-tsl256x test-sht25 test-servo
CONTIKI_PROJECT += test-bmp085-bmp180 test-motion test-rotation-sensor
CONTIKI_PROJECT += test-grove-light-sensor test-grove-loudness-sensor
CONTIKI_PROJECT += test-weather-meter test-grove-gyro test-lcd test-iaq
CONTIKI_PROJECT += test-pm10-sensor test-vac-sensor test-aac-sensor
CONTIKI_PROJECT += test-zonik test-dht22 test-ac-dimmer
CONTIKI_PROJECT += test-bme280 test-vbat

CONTIKI_TARGET_SOURCEFILES += tsl256x.c sht25.c bmpx8x.c motion-sensor.c
CONTIKI_TARGET_SOURCEFILES += adc-sensors.c weather-meter.c grove-gyro.c
CONTIKI_TARGET_SOURCEFILES += rgb-bl-lcd.c pm10-sensor.c iaq.c zonik.c relay.c
CONTIKI_TARGET_SOURCEFILES += dht22.c servo.c ac-dimmer.c bme280-arch.c

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = zoul

CONTIKI = ../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_DRIVERS_DIR)/bme280

include $(CONTIKI)/Makefile.include
