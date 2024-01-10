#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#if defined(CONFIG_GPIO)
#include <zephyr/drivers/gpio.h>
#endif

/* Enable USB for the nRF52840 Dongle */
#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#endif

#include "rgb_led.h"
#include "simple_io_service.h"

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SIMPLE_IO_SERVICE),
};

enum breathing_light_mask_values {
	BREATHING_LIGHT_OFF = 0,
	BREATHING_LIGHT_RED = 0x1,
	BREATHING_LIGHT_GREEN = 0x2,
    BREATHING_LIGHT_YELLOW = 0x3,
	BREATHING_LIGHT_BLUE = 0x4,
	BREATHING_LIGHT_MAGENTA = 0x5,
    BREATHING_LIGHT_CYAN = 0x6,
    BREATHING_LIGHT_WHITE = 0x7,
};

int8_t breathing_light = BREATHING_LIGHT_BLUE; // blue

static void set_rgb_cb(uint8_t r, uint8_t g, uint8_t b) {
	breathing_light = BREATHING_LIGHT_OFF; // Disable breathing light when setting RGB color

	printk("%s: Set RGB (r, g, b) = (%u, %u, %u)\n", __func__, r, g, b);

	rgb_led_set(r, g, b);
}

struct bt_simple_io_cb io_cbs = {
	.set_rgb = set_rgb_cb,
};

static void bt_ready(void)
{
	int err;

	printk("%s: Bluetooth initialized\n", __func__);

    if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_simple_io_register_cb(&io_cbs);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("%s: Advertising failed to start (err %d)\n", __func__, err);
		return;
	}

	printk("%s: Advertising successfully started\n", __func__);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("%s: Connection failed (err 0x%02x)\n", __func__, err);
		breathing_light = BREATHING_LIGHT_RED;
	} else {
		printk("%s: Connected\n", __func__);
		breathing_light = BREATHING_LIGHT_GREEN;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("%s: Disconnected (reason 0x%02x)\n", __func__, reason);
	breathing_light = BREATHING_LIGHT_BLUE;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

#if defined(CONFIG_GPIO)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios,
							      {0});
static struct gpio_callback button_cb_data;

static struct k_work button_work;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	k_work_submit(&button_work);
}

void handle_button_event(struct k_work *work)
{
    int val = gpio_pin_get_dt(&button);

	printk("%s: Button = %d\n", __func__, val);

/* Enable the following line to change RGB color on button press/release */
#if 0
	breathing_light = BREATHING_LIGHT_OFF; // Disable breathing light when setting RGB color
    if (val) {
        rgb_led_set(255,0,0);
    } else {
        rgb_led_set(0,255,0);
    }
#endif
    simple_io_button_notify(val);
}
#endif

/* Breathing light step duration*/
#define SLEEP_MS 10

int main(void)
{
	int err;
	int8_t state = 0;
	int32_t val = 0;

	/* Enable USB for the nRF52840 Dongle */
#if defined(CONFIG_BOARD_NRF52840DONGLE_NRF52840)
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;

	if (usb_enable(NULL)) {
		return 0;
	}

	// Uncomment the following to wait until connected
	// while (!dtr) {
	// 	uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
	// 	k_sleep(K_MSEC(100));
	// }
#endif

    /* Check that the RGB PWM devices are present*/
    printk("Initialize RGB LED...\n");
    err = rgb_led_init();
    if (err) {
        printk("Error setting up RGB light!\n");
        return 0;
    }

#if defined(CONFIG_GPIO)
    printk("Initialize Button...\n");
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	err = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (err != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       err, button.port->name, button.pin);
		return 0;
	}

	err = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (err != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			err, button.port->name, button.pin);
		return 0;
	}

	k_work_init(&button_work, handle_button_event);

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);
#endif

	// Enable BT
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();

    /* Loop for the breathing/fading lights */
	while (1) {
		k_msleep(SLEEP_MS);
		if (breathing_light != BREATHING_LIGHT_OFF) {
			rgb_led_set(
				(breathing_light & 0x1) ? val : 0,
				(breathing_light & 0x2) ? val : 0,
				(breathing_light & 0x4) ? val : 0);

			val += (state & 0x10) == 0 ? 2 : -2;

			if (val >= 0xFE) {
				state ^= 0x10;
			} else if (val < 0x01) {
				state ^= 0x10;
			}
		} 
	}
}
