.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO

.text
	.global Start

Start:
	// DOUTSET = Set data output
	// DOUTCLR = Clear data output
	//
	// (PORT_SIZE * LED_PORT) = totale antallet portregistre
	ldr r0, =GPIO_BASE + (PORT_SIZE * LED_PORT) // Regner ut register-addr. Henter minnelokasjon og legger i r0
	ldr r1, =GPIO_PORT_DOUTCLR // Henter minnelokasjon 'GPIO_PORT_DOUTCLR' og legger i r1
	add r1, r0, r1 // r0 + r1 i r1
	ldr r2, =GPIO_PORT_DOUTSET // Henter minnelokasjon 'GPIO_PORT_DOUTSET' og legger i r1
	add r0, r0, r2 // r0 + r2 i r0
	// DIN = Data input
	ldr r2, =GPIO_BASE + (PORT_SIZE * BUTTON_PORT) + GPIO_PORT_DIN
	// Regner ut reg-addr.
	// ldr = Henter minnelokasjon og legger i r2

	// Nå: r0 inneholder doutset-lokasjon for LED, r1 inneholder doutclr-lokasjon for LED, r2 inneholder din-lokasjon for button

	ldr r3, =1 << LED_PIN
	ldr r4, =1 << BUTTON_PIN
	// Bitshifter 1 (pinnene lagres i binær, så pin 1 er 001=1, pin 2 er 010=2, pin 3 er 100=4 osv.)
	// Nå: r3 har pinen til LED, r4 har pinen til button

Loop:
	ldr r5, [r2] // Henter innhold i minnelokasjon r2 (din button) og legger i r5
	and r5, r5, r4 // Gjør et bitwise AND på r5 og r4, lagre i r5 (xxbxx and 00100 = 00(b and 1)00, b er input fra vår button)
	cmp r5, r4 // compare r5 og r4 (r5) (eq hvis button er inne)
	beq TurnOn // Branch if Equal (branch til TurnOn hvis button er inne)

TurnOff:
	str r3, [r0] // Lagre r3 (LED-pin) på minnelokasjon r0 (doutset).
	B Loop // branch to loop (endless loop!)

TurnOn:
	str r3, [r1] // Lagre r3 (LED-pin) på minnelokasjon r1 (doutclr).
	B Loop // branch to loop (endless loop!)


NOP // Behold denne på bunnen av fila

