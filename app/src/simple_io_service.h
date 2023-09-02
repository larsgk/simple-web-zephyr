#ifndef BT_SIMPLE_IO_SERVICE_H_
#define BT_SIMPLE_IO_SERVICE_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generated from https://www.uuidtools.com/

// 13e779a0-bb72-43a4-a748-9781b918258c (service)
// 4332aca6-6d71-4173-9945-6653b6c684a0 (rgb [WRITE])
// b749d964-4efb-408a-82ad-7495e8af8d6d (id [WRITE])
// 030de9cf-ce4b-44d0-8aa2-1db9185dc069 (button [NOTIFY])


#define BT_UUID_SIMPLE_IO_SERVICE \
	BT_UUID_128_ENCODE(0x13e779a0, 0xbb72, 0x43a4, 0xa748, 0x9781b918258c)

typedef void (*simple_io_set_rgb_cb)(uint8_t r, uint8_t g, uint8_t b);
// typedef void (*simple_io_name_cb)(int16_t vx, int16_t vy);

struct bt_simple_io_cb {
    simple_io_set_rgb_cb    set_rgb;
};

void bt_simple_io_register_cb(struct bt_simple_io_cb *cb);
int simple_io_button_notify(uint8_t pressed);

#ifdef __cplusplus
}
#endif

#endif /* BT_SIMPLE_IO_SERVICE_H_ */
