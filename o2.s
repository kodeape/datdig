.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO
.include "sys-tick_constants.s" // Register-adresser og konstanter for SysTick

.text
	.global Start

.global SysTick_Handler
.thumb_func
	SysTick_Handler:
		push {lr}
		bl IncrementTenths
		pop {lr}

		bx lr // returner fra interrupt

IncrementTenths:
	ldr r0, = tenths
	ldr r1, = #1
	ldr r2, [r0]
	add r1, r2
	ldr r2, = #10
	cmp r1, r2
	beq IncrementSeconds
	str r1, [r0]

	mov pc, lr

IncrementSeconds:
	ldr r1, = #0
	str r1, [r0]
	ldr r0, = seconds
	ldr r1, = #1
	ldr r2, [r0]
	add r1, r2
	ldr r2, = #60
	cmp r1, r2
	beq IncrementMinutes
	str r1, [r0]

	push {r0, lr}
	bl ToggleLED
	pop {r0, lr}

	mov pc, lr

IncrementMinutes:
	ldr r1, = #0
	str r1, [r0]
	ldr r0, = minutes
	ldr r1, = #1
	ldr r2, [r0]
	add r1, r2
	str r1, [r0]

	mov pc, lr

ToggleLED:
	ldr r0, [r0]
	ldr r1, = #1
	and r0, r1
	cmp r0, r1 // eq dersom siste bit i sekunder-tallet en 1 (altså antall sekunder er et oddetall)
	bne TurnOff
	b TurnOn

TurnOff:
	ldr r0, = GPIO_BASE + (PORT_SIZE * LED_PORT) + GPIO_PORT_DOUTCLR
	ldr r1, = #1 << LED_PIN
	str r1, [r0]

	mov pc, lr

TurnOn:
	ldr r0, = GPIO_BASE + (PORT_SIZE * LED_PORT) + GPIO_PORT_DOUTSET
	ldr r1, = #1 << LED_PIN
	str r1, [r0]

	mov pc, lr

.global GPIO_ODD_IRQHandler
.thumb_func
	GPIO_ODD_IRQHandler:
		ldr r0, = SYSTICK_BASE + SYSTICK_CTRL
		ldr r1, = #1
		ldr r2, [r0]
		and r1, r2
		cmp r1, #1
		beq StopTimerCall

		StartTimer: // trenger ikke egentlig denne labelen men det ser pent ut
		ldr r1, = #0b111
		str r1, [r0]
		b ClearIFCall

		StopTimerCall:
		push {lr}
		bl StopTimer
		pop {lr}

		ClearIFCall:
		push {lr}
		bl ClearIF
		pop {lr}

		bx lr // returner fra interrupt

StopTimer:
	ldr r0, = SYSTICK_BASE + SYSTICK_CTRL
	ldr r1, = #0b110
	str r1, [r0]

	mov pc, lr

ResetTimer:
	push {lr}
	bl StopTimer
	pop {lr} // stoppet timern

	ldr r0, = tenths
	ldr r1, = #0
	str r1, [r0] // tenths = 0
	ldr r0, = seconds
	ldr r1, = #0
	str r1, [r0] // seconds = 0

	ldr r0, = minutes
	ldr r1, = #0
	str r1, [r0] // minutes = 0

	mov pc, lr

ClearIF:
	ldr r0, = GPIO_BASE + GPIO_IFC
	ldr r1, = #1
	lsl r1, #9
	str r1, [r0]

	mov pc, lr

Start:
	// ---- systick setup ----
	push {lr}
	bl ResetTimer
	pop {lr} // timer av og tenths, seconds og minutes satt til 0

	ldr r0, = SYSTICK_BASE
	ldr r1, = SYSTICK_LOAD
	add r1, r0
	ldr r2, = FREQUENCY/10
	str r2, [r1] // interrupter 10 ganger i sekundet når systick er aktiv


	// ---- button interruption setup ----
	// les 2.2.3 for å forstå noe som helst her
	ldr r0, = GPIO_BASE
	ldr r1, = GPIO_EXTIPSELH
	add r1, r0
	ldr r4, [r1]
	ldr r2, = #1
	lsl r2, #4 // r2: 10000  -  port B = 1 (se gpio_constants.s) og pin9 har bit 3 til 7 (første bit er bit 0)
	orr r2, r4 // r2: 1xxxx  -  x-ene er hva enn som lå på pin8 fra før, vi ønsker ikke endre noe annet enn pin9
	str r2, [r1] // pin9 (som button ligger på) gir interrupt på port B (som button ligger i)

	ldr r1, = GPIO_EXTIFALL
	add r1, r0
	ldr r4, [r1]
	ldr r2, = #1
	lsl r2, #9 // r2: 1000000000  -  bit 9 representerer pin9
	orr r3, r2, r4 // r2: 1xxxxxxxxx  -  x-ene er hva enn som lå på pin0 - pin8 fra før, vi ønsker ikke endre på noe annet enn pin9
	str r3, [r1] // pin9 gir interrupt på fall fra 1 til 0 (når button trykkes ned)

	push {r0, r2, lr} // ClearIF bruker ikke egentlig r2, jeg pusher og popper den av god vane
	bl ClearIF
	pop {r0, r2, lr} // if cleara så vi kan ta imot button interrupts

	ldr r1, = GPIO_IEN
	add r1, r0
	ldr r4, [r1]
	orr r3, r2, r4
	str r3, [r1] // pin9 enabla for interrupts

Loop: // evig loop for å ha noe som alltid kjører
 	b Loop

NOP // Behold denne på bunnen av fila
