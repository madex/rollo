unsigned long pa, pb, pc, pd;
#define GPIO_PORTA_DATA_R pa
#define GPIO_PORTB_DATA_R pb
#define GPIO_PORTC_DATA_R pc
#define GPIO_PORTD_DATA_R pd
#define GPIO_PIN_0   1
#define GPIO_PIN_1   2
#define GPIO_PIN_2   4
#define GPIO_PIN_3   8
#define GPIO_PIN_4  16
#define GPIO_PIN_5  32
#define GPIO_PIN_6  64
#define GPIO_PIN_7 128
#define UARTprintf printf
#define UARTgetc   getchar
#define SYSCTL_SYSDIV_2_5 1
#define false 0
#define true 1
#define SYSCTL_USE_PLL     1
#define SYSCTL_XTAL_16MHZ  2
#define SYSCTL_PERIPH_GPIOA  1
#define SYSCTL_PERIPH_GPIOB  2
#define SYSCTL_PERIPH_GPIOD  8
#define GPIO_PORTA_BASE 234
#define GPIO_PORTD_BASE 234
#define GPIO_PORTB_BASE 234
#define __asm myAsm
static inline void ROM_SysCtlClockSet(unsigned long i) {}

static inline void ROM_SysTickPeriodSet(unsigned long i) {
}
static inline void ROM_SysTickEnable() {
}
static inline void ROM_SysTickIntEnable() {
}
static inline void ROM_IntMasterEnable() {
}
static inline void SysCtlPeripheralEnable(unsigned long i) {
}
static inline void GPIOPinTypeUART(unsigned long i, unsigned long j) {
}
static inline void UARTStdioInit(char i) {
}
static inline void GPIOPinTypeGPIOInput(unsigned long i, unsigned long j) {
}
static inline void GPIOPinTypeGPIOOutput(unsigned long i, unsigned long j) {
}

static inline void SysTickIntRegister(void *ptr) {
}
static inline void UARTEchoSet(unsigned char b) {
}

static inline void myAsm(char *str) {
}

static inline char UARTRxBytesAvail() {
    return 1;
}
