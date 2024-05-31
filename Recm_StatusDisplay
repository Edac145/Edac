#include <xc.h>
#define FCY 10000000UL // 10 MHz Instruction Rate
#include <libpic30.h>
#include <inttypes.h>
#include <string.h>
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/system/clock.h"
#include "mcc_generated_files/system/pins.h"
#include "mcc_generated_files/uart/uart2.h"
#include "mcc_generated_files/uart/uart1.h"
#include "mcc_generated_files/secondary_core/sec_core1.h"

#include <stdio.h>

//#define FCY CLOCK_PeripheralFrequencyGet()
const struct SEC_CORE_INTERFACE *secondaryCore = &MSIInterface;

typedef struct {
    bool output_pins[5];
    bool input_pins[3];
    unsigned int uart_status;
} MainCoreStatus;

typedef struct {
    bool output_pins[12];
    bool input_pins[8];
    int analog_inputs[3];
    int pwm_status[4];
    unsigned int uart_status;
    unsigned int spi_status;
} SecondaryCoreStatus;

MainCoreStatus getMainCoreStatus();
void encodeMainCoreStatus(char *buffer, MainCoreStatus status);
void decodeSecondaryCoreStatus(SecondaryCoreStatus *status, int *buffer);

bool getOutputPinStatus(int pin);
bool getInputPinStatus(int pin);
unsigned int getUARTStatus(); // Use uint32_t for baud rate
void sendStatusViaUART(const char *buffer);
void encodeSecondaryCoreStatus(char *buffer, SecondaryCoreStatus status);

int main(void) {
    char mainCoreBuffer[50]; // Adjust size based on protocol
    char secondaryCoreBuffer[100];
    SYSTEM_Initialize();
    SEC_CORE1_Initialize();
    SEC_CORE1_Program();
    SEC_CORE1_Start();

    while(1) {
        IO_RD11_Toggle();
        MainCoreStatus mainStatus = getMainCoreStatus();
        encodeMainCoreStatus(mainCoreBuffer, mainStatus);
        sendStatusViaUART(mainCoreBuffer);
        __delay_ms(2000);

        // Wait for interrupt from secondary core
        while (!secondaryCore->IsInterruptRequested());
        secondaryCore->InterruptRequestAcknowledge();
        while (secondaryCore->IsInterruptRequested());
        secondaryCore->InterruptRequestAcknowledgeComplete();
        
        // Mailbox read
        int dataReceive[30]; // Adjust size to match the size of the SecondaryCoreStatus
        secondaryCore->ProtocolRead(MSI1_ProtocolB, (unsigned int*)dataReceive);
        
        // Decode the received data to the status structure
        SecondaryCoreStatus secStatus;
        decodeSecondaryCoreStatus(&secStatus, dataReceive);
        encodeSecondaryCoreStatus(secondaryCoreBuffer, secStatus);
        
        sendStatusViaUART(secondaryCoreBuffer);
        __delay_ms(2000);

    }    
}

bool getInputPinStatus(int pin) {
    switch(pin) {
        case 0: return IO_RD8_GetValue();
        case 1: return IO_RD9_GetValue();
        case 2: return IO_RD10_GetValue();
        default: return 0;
    }
}

bool getOutputPinStatus(int pin) {
    switch(pin) {
        case 0: return IO_RD11_GetValue();
        case 1: return IO_RD12_GetValue();
        case 2: return IO_RD13_GetValue();
        case 3: return IO_RD14_GetValue();
        case 4: return IO_RD15_GetValue();
        default: return 0;
    }
}

unsigned int getUARTStatus() {
    return UART2_BaudRateGet(); // Use the provided function to get the baud rate
}

MainCoreStatus getMainCoreStatus() {
    MainCoreStatus status;
    for (int i = 0; i < 5; i++) {
        status.output_pins[i] = getOutputPinStatus(i);
    }
    for (int i = 0; i < 3; i++) {
        status.input_pins[i] = getInputPinStatus(i);
    }
    status.uart_status = getUARTStatus();
    return status;
}

void encodeMainCoreStatus(char *buffer, MainCoreStatus status) {
    sprintf(buffer, "AA,01,%d,%d,%d,%d,%d,%d,%d,%d,%u\n", 
            status.output_pins[0], status.output_pins[1], status.output_pins[2], status.output_pins[3], status.output_pins[4], 
            status.input_pins[0], status.input_pins[1], status.input_pins[2], status.uart_status);
}

void decodeSecondaryCoreStatus(SecondaryCoreStatus *status, int *buffer) {
    int idx = 2; // Skip header and core identifier
    for (int i = 0; i < 12; i++) {
        status->output_pins[i] = buffer[idx++];
    }
    for (int i = 0; i < 8; i++) {
        status->input_pins[i] = buffer[idx++];
    }
    for (int i = 0; i < 3; i++) {
        status->analog_inputs[i] = buffer[idx++];
    }
    for (int i = 0; i < 4; i++) {
        status->pwm_status[i] = buffer[idx++];
    }
    status->uart_status = buffer[idx++];
    status->spi_status = buffer[idx++];
}

void sendStatusViaUART(const char *buffer) {
    while(*buffer) {
        UART1_Write(*buffer++);
    }
}

void encodeSecondaryCoreStatus(char *buffer, SecondaryCoreStatus status) {
    sprintf(buffer, "AA,02,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u\n", 
            status.output_pins[0], status.output_pins[1], status.output_pins[2], status.output_pins[3], status.output_pins[4], status.output_pins[5],
            status.output_pins[6], status.output_pins[7], status.output_pins[8], status.output_pins[9], status.output_pins[10], status.output_pins[11],
            status.input_pins[0], status.input_pins[1], status.input_pins[2], status.input_pins[3], status.input_pins[4], status.input_pins[5],
            status.input_pins[6], status.input_pins[7], status.analog_inputs[0], status.analog_inputs[1], status.analog_inputs[2], 
            status.pwm_status[0], status.pwm_status[1], status.pwm_status[2], status.pwm_status[3], status.uart_status, status.spi_status);
}
