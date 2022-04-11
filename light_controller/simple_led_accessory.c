/*
 * simple_led_accessory.c
 * Define the accessory in pure C language using the Macro in characteristics.h
 *
*  Created on: 2022-01-20
 *      Author: Ye Zhang
 */

#include <Arduino.h>
#include <homekit/types.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <stdio.h>
#include <port.h>

//const char * buildTime = __DATE__ " " __TIME__ " GMT";

#define ACCESSORY_NAME  ("ESP8266_LED")
#define ACCESSORY_SN  ("SN_0123456")
#define ACCESSORY_MANUFACTURER ("西北偏北工作室")
#define ACCESSORY_MODEL  ("XBPB004")

#define LED_RGB_SCALE 255       // this is the scaling factor used for color conversion
#define R_GPIO 14
#define G_GPIO 12
#define B_GPIO 16

int rgb[3] = {0,0,0};

// Global variables
float led_hue = 0;              // hue is scaled 0 to 360
float led_saturation = 0;      // saturation is scaled 0 to 100
float led_brightness = 0;     // brightness is scaled 0 to 100

bool led_power = false; //true or flase

void color(int red, int green, int blue) {
	if (red < 0) { red = 0; }
	if (red > 255) { red = 255; }

	if (green < 0) { green = 0; }
	if (green > 255) { green = 255; }
	
	if (blue < 0) { blue = 0; }
	if (blue > 255) { blue = 255; }
    
    if (blue == 0)
    {
        digitalWrite(B_GPIO,HIGH);
    }else{
        int _b = (int)( 255 - blue + 0.5f);
        analogWrite(B_GPIO, _b);
    }

    if (green == 0)
    {
        digitalWrite(G_GPIO,HIGH);
    }else{
        int _g = (int)( 255 - green + 0.5f);
        analogWrite(G_GPIO, _g);
    }

    if (red == 0)
    {
        digitalWrite(R_GPIO,HIGH);
    }else{
        int _r = (int)( 255 - red + 0.5f);
        analogWrite(R_GPIO, _r);
    }
    
}

static void hsi2rgb(float h, float s, float i, int* _rgb) {
    int r, g, b;

    while (h < 0) { h += 360.0F; };     // cycle h around to 0-360 degrees
    while (h >= 360) { h -= 360.0F; };
    h = 3.14159F*h / 180.0F;            // convert to radians.
    s /= 100.0F;                        // from percentage to ratio
    i /= 100.0F;                        // from percentage to ratio
    s = s > 0 ? (s < 1 ? s : 1) : 0;    // clamp s and i to interval [0,1]
    i = i > 0 ? (i < 1 ? i : 1) : 0;    // clamp s and i to interval [0,1]
    i = i * sqrt(i);                    // shape intensity to have finer granularity near 0

    if (h < 2.09439) {
        r = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        g = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        b = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else if (h < 4.188787) {
        h = h - 2.09439;
        g = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        b = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        r = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else {
        h = h - 4.188787;
        b = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        r = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        g = LED_RGB_SCALE * i / 3 * (1 - s);
    }

    _rgb[0] = (uint8_t) r;
    _rgb[1] = (uint8_t) g;
    _rgb[2] = (uint8_t) b;
}

void led_update() {
	if (led_power) {
        hsi2rgb(led_hue, led_saturation, led_brightness, rgb);
		color(rgb[0], rgb[1], rgb[2]);
	} else {
		printf("OFF\n");
		digitalWrite(R_GPIO,HIGH);
        digitalWrite(G_GPIO,HIGH);
        digitalWrite(B_GPIO,HIGH);

	}
}

homekit_value_t led_on_get() {
	return HOMEKIT_BOOL(led_power);
}

void led_on_set(homekit_value_t value) {
	if (value.format != homekit_format_bool) {
		printf("Invalid on-value format: %d\n", value.format);
		return;
	}
	led_power = value.bool_value;
	if (led_power) {
		if (led_brightness < 1) {
			color(0, 0, 0);
            return;
		}
	}
	led_update();
}


homekit_value_t led_brightness_get() {
    return HOMEKIT_INT(led_brightness);
}
void led_brightness_set(homekit_value_t value) {
    if (value.format != homekit_format_int) {
        // printf("Invalid brightness-value format: %d\n", value.format);
        return;
    }
    led_brightness = value.int_value;
    led_update();
}

homekit_value_t led_hue_get() {
    return HOMEKIT_FLOAT(led_hue);
}

void led_hue_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        // printf("Invalid hue-value format: %d\n", value.format);
        return;
    }
    led_hue = value.float_value;
    led_update();
}

homekit_value_t led_saturation_get() {
    return HOMEKIT_FLOAT(led_saturation);
}

void led_saturation_set(homekit_value_t value) {
    if (value.format != homekit_format_float) {
        // printf("Invalid sat-value format: %d\n", value.format);
        return;
    }
    led_saturation = value.float_value;
    led_update();
}


homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t led_on = HOMEKIT_CHARACTERISTIC_(ON, false,
		//.callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback),
		.getter=led_on_get,
		.setter=led_on_set
);




void accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
}

homekit_accessory_t *accessories[] =
		{
				HOMEKIT_ACCESSORY(
						.id = 1,
						.category = homekit_accessory_category_lightbulb,
						.services=(homekit_service_t*[]){
						HOMEKIT_SERVICE(ACCESSORY_INFORMATION,.characteristics=(homekit_characteristic_t*[]){
						    &name,
						    HOMEKIT_CHARACTERISTIC(MANUFACTURER, ACCESSORY_MANUFACTURER),
						    &serial_number,
						    HOMEKIT_CHARACTERISTIC(MODEL, ACCESSORY_MODEL),
						    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1.0"),
						    HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
						    NULL
						}),
						HOMEKIT_SERVICE(LIGHTBULB, .primary=true,.characteristics=(homekit_characteristic_t*[]){
						    HOMEKIT_CHARACTERISTIC(NAME, "灯带控制器"),
						    &led_on,
						    HOMEKIT_CHARACTERISTIC(BRIGHTNESS, 100,.getter = led_brightness_get,.setter = led_brightness_set),
                            HOMEKIT_CHARACTERISTIC(HUE, 0,.getter = led_hue_get,.setter = led_hue_set),
                            HOMEKIT_CHARACTERISTIC(SATURATION, 0,.getter = led_saturation_get,.setter = led_saturation_set),   
						    NULL
						}),
						NULL
						}),
				NULL
		};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = "771-55-104",
		//.on_event = on_homekit_event,
		.setupId = "KD9K"
};
