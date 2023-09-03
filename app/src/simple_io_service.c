#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "simple_io_service.h"

static struct bt_uuid_128 simple_io_service_uuid = BT_UUID_INIT_128(
	BT_UUID_SIMPLE_IO_SERVICE);

// 0x13e779a0, 0xbb72, 0x43a4, 0xa748, 0x9781b918258c (service)
// 0x4332aca6, 0x6d71, 0x4173, 0x9945, 0x6653b6c684a0 (rgb [WRITE])
// 0xb749d964, 0x4efb, 0x408a, 0x82ad, 0x7495e8af8d6d (id [WRITE])
// 0x030de9cf, 0xce4b, 0x44d0, 0x8aa2, 0x1db9185dc069 (button [NOTIFY])

static const struct bt_uuid_128 simple_io_rgb_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x4332aca6, 0x6d71, 0x4173, 0x9945, 0x6653b6c684a0));

static const struct bt_uuid_128 simple_io_id_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xb749d964, 0x4efb, 0x408a, 0x82ad, 0x7495e8af8d6d));

static const struct bt_uuid_128 simple_io_button_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x030de9cf, 0xce4b, 0x44d0, 0x8aa2, 0x1db9185dc069));

static struct bt_simple_io_cb *simple_io_cbs;

void bt_simple_io_register_cb(struct bt_simple_io_cb *cb)
{
    simple_io_cbs = cb;
}

static ssize_t write_rgb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, uint16_t len, uint16_t offset,	uint8_t flags)
{
	uint8_t val[3];

	printk("%s: len=%zu, offset=%u\n", __func__, len, offset);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len != sizeof(val)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	(void)memcpy(&val, buf, len);

    if (simple_io_cbs->set_rgb) {
        simple_io_cbs->set_rgb(val[0], val[1], val[2]);
    }

    return len;
}

static ssize_t write_id(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	char name[CONFIG_BT_DEVICE_NAME_MAX];
	int err;

	uint8_t val;

	printk("%s: len=%zu, offset=%u\n", __func__, len, offset);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len != sizeof(val)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	(void)memcpy(&val, buf, len);

	/* Set the BT Name to "Simple Web Zephyr <ID>" (used for easier identification in workshops ;)) */
	snprintk(name, sizeof(name), "Simple Web Zephyr %u", val);

	err = bt_set_name(name);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	return len;
}

static void button_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool button_notif = (value == BT_GATT_CCC_NOTIFY);

	printk("Button notifications %s\n", button_notif ? "enabled" : "disabled");
}

/* Simple IO Service Declaration */
BT_GATT_SERVICE_DEFINE(simple_io_svc,
	BT_GATT_PRIMARY_SERVICE(&simple_io_service_uuid),
	BT_GATT_CHARACTERISTIC(&simple_io_button_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
				   NULL, NULL, NULL),
	BT_GATT_CCC(button_ccc_cfg_changed,
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD("Button(s)", BT_GATT_PERM_READ),
    BT_GATT_CHARACTERISTIC(&simple_io_rgb_uuid.uuid,
				BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_WRITE,
                NULL, write_rgb, NULL),
    BT_GATT_CUD("RGB", BT_GATT_PERM_READ),
    BT_GATT_CHARACTERISTIC(&simple_io_id_uuid.uuid,
				BT_GATT_CHRC_WRITE,
                BT_GATT_PERM_WRITE,
                NULL, write_id, NULL),
    BT_GATT_CUD("Device ID", BT_GATT_PERM_READ),
);

int simple_io_button_notify(uint8_t pressed)
{
	int ret = bt_gatt_notify(NULL, &simple_io_svc.attrs[1], &pressed, sizeof(pressed));

	return ret == -ENOTCONN ? 0 : ret;
}

