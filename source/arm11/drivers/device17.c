#include "arm11/drivers/device17.h"
#include "arm11/drivers/i2c.h"

static Device17 device;

void Device17_Poll(void)
{
    I2C_readArray(I2C_DEV_N3DS_HID, &device, sizeof device);
}

Device17* Device17_GetDevice(void)
{
    return &device;
}