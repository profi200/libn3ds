#include "arm11/drivers/wifi/wifi.h"
#include "arm11/drivers/cfg11.h"
#include "arm11/drivers/gpio.h"
#include "arm11/drivers/mcu.h"
#include "arm11/drivers/timer.h"
#include "arm11/drivers/wifi/sdio.h"



u32 WIFI_init(void)
{
	getCfg11Regs()->wifi_power = WIFI_POWER_ON; // Actually for the MP regs. Not power.
	GPIO_write(GPIO_2_2, 0);  // Select DSi/3DS WiFi (SDIO).
	GPIO_write(GPIO_3_12, 1); // Deassert Atheros reset pin.
	MCU_setWifiLedState(1);   // Enable WiFi LED.
	TIMER_sleepMs(2);         // The chipset needs very close to 2 ms after reset. TODO: How much does the spec allow?

	SDIO_init();

	// TODO: WiFi init here.

	return 0;
}

void WIFI_deinit(void)
{
	// TODO: WiFi deinit here.

	MCU_setWifiLedState(0);
	GPIO_write(GPIO_3_12, 0);
	//GPIO_write(GPIO_2_2, 0); // Not needed.
	getCfg11Regs()->wifi_power = 0;
}
