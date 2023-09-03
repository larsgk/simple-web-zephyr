# Simple Web Zephyr

If you haven't done it yet, first go to [The Zephyr getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) and install all dependencies (I'd recommend following the path with the virtual python environment).

This repo contains a stand alone Zephyr application that can be fetched and initialized like this:

```shell
west init -m git@github.com:larsgk/simple-web-zephyr.git --mr main my-workspace
```

Then use west to fetch dependencies:

```shell
cd my-workspace
west update
```

Go to the app folder:

```shell
cd simple-web-zephyr
```

There are a few scripts for building and flashing the nRF52840 Dongle - run them in this order:

```shell
build_application.sh
create_flash_package.sh
flash_dongle.sh
```

Note: You'll need to get the dongle in DFU mode by pressing the small side buttton with a nail. The dongle should then start fading a red light in and out.