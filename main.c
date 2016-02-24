#include <msp432.h>
#include <string.h>
#include "driverlib.h"
#include "msprf24.h"
#include "nrf_userconfig.h"
#include "stdint.h"
#include "globals.h"

int InitFunction(void);
uint8_t flag = 0;

void main()
{
	int err = NONE;
	char addr[5], i = '0';
	char buf[32];
	uint8_t status;

	err = InitFunction();

	WDTCTL = WDTHOLD | WDTPW;
//        DCOCTL = CALDCO_1MHZ;
//        BCSCTL1 = CALBC1_1MHZ;
//        BCSCTL2 = DIVS_0;  // SMCLK = DCOCLK/1
	// SPI (USI) uses SMCLK, prefer SMCLK=DCO (no clock division)

	/* Initial values for nRF24L01+ library config variables */
	rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
	rf_addr_width      = 5;
	rf_speed_power     = RF24_SPEED_MIN | RF24_POWER_MIN;
	rf_channel         = 120;
	msprf24_init();  // All RX pipes closed by default
	msprf24_open_pipe(0, 1);  // Open pipe#0 with Enhanced ShockBurst enabled for receiving Auto-ACKs
	msprf24_set_pipe_packetsize(0, 15);  // Dynamic payload length enabled (size=0)

	// Transmit to 'rad01' (0x72 0x61 0x64 0x30 0x31)
	msprf24_standby();
	memcpy(addr, "\xDE\xAD\xBE\xEF\x01", 5);
//	addr[0] = 'r'; addr[1] = 'a'; addr[2] = 'd'; addr[3] = '0'; addr[4] = '2';
	w_tx_addr(addr);
	w_rx_addr(0, addr);  // Pipe 0 receives auto-ack's, autoacks are sent back to the TX addr so the PTX node
						 // needs to listen to the TX addr on pipe#0 to receive them.
	buf[0] = '1';
	buf[1] = '\0';
	w_tx_payload(2, buf);
	msprf24_activate_tx();

	if (rf_irq & RF24_IRQ_FLAGGED) {
			msprf24_get_irq_reason();  // this updates rf_irq
			if (rf_irq & RF24_IRQ_TX)
					status = 1;
			if (rf_irq & RF24_IRQ_TXFAILED)
					status = 0;
			msprf24_irq_clear(RF24_IRQ_MASK);  // Clear any/all of them
	}
	memset(buf, 0, sizeof(buf));

	while(1)
	{
		if(flag)
		{
			flag = 0;
			if (buf[0] == '1')
				buf[0] = '0';
			else
				buf[0] = '1';

			buf[1] = '\0';

			GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
			w_tx_payload(strlen(buf), buf);
			msprf24_activate_tx();
			GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
		}
	}
}
//------------------------------------------------------------------------------
// GPIO ISR
void gpio_isr(void)
{
    uint32_t status;

    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P6);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P6, status);

    msprf24_get_irq_reason();  // this updates rf_irq
	if (rf_irq & RF24_IRQ_TX)
	{
		status = 1;
		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
		GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
	}
	if (rf_irq & RF24_IRQ_TXFAILED)
	{
		status = 0;
		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
	}
	msprf24_irq_clear(RF24_IRQ_MASK);  // Clear any/all of them
}
//-----------------------------------------------------------------------
void systick_isr(void)
{
	flag = 1;
}
//------------------------------------------------------------------------------
int InitFunction(void)
{
	int err = NONE;

	MAP_WDT_A_holdTimer();
	//	Configuring SysTick to trigger at 3000 (MCLK is 3MHz so this will make it toggle every 0.001s)
//	MAP_SysTick_enableModule();
//	MAP_SysTick_setPeriod(300000000);
//	MAP_SysTick_enableInterrupt();
	MAP_GPIO_setAsInputPin(GPIO_PORT_P6, GPIO_PIN1);


	/* Configuring P6.7 as an input. P1.0 as output and enabling interrupts */
//	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
	//Setting RGB LED as output
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
	GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

	//Set P6.1 to be the pin interupt
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P6, GPIO_PIN1);
	MAP_GPIO_enableInterrupt(GPIO_PORT_P6, GPIO_PIN1);

	MAP_Interrupt_enableInterrupt(INT_PORT6);
	MAP_Interrupt_enableMaster();

	return err;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
