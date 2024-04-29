#include "arm11/drivers/n3ds_exthid.h"
#include "arm11/drivers/i2c.h"

static N3DS_EXTHID device;

void N3DS_EXTHID_Poll(void)
{
    I2C_readArray(I2C_DEV_N3DS_HID, &device, sizeof device);
}

N3DS_EXTHID* N3DS_EXTHID_GetDevice(void)
{
    return &device;
}