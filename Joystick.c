/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

/** \file
 *
 *  Main source file for the posts printer demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

/*
The following ButtonMap variable defines all possible buttons within the
original 13 bits of space, along with attempting to investigate the remaining
3 bits that are 'unused'. This is what led to finding that the 'Capture'
button was operational on the stick.
*/
uint16_t ButtonMap[16] = {
	0x01, //Y
	0x02, //B
	0x04, //A
	0x08, //X
	0x10, //L
	0x20, //R
	0x40, //ZL
	0x80, //ZR
	0x100, //Minus
	0x200, //Plus
	0x400, //L-stick
	0x800, //R-stick
	0x1000, //Unk
	0x2000, //Unk
	0x4000, //Unk
	0x8000, //Unk
};

// Main entry point.
int main(void) {
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	#ifdef ALERT_WHEN_DONE
	// Both PORTD and PORTB will be used for the optional LED flashing and buzzer.
	#warning LED and Buzzer functionality enabled. All pins on both PORTB and \
PORTD will toggle when printing is done.
	DDRD  = 0xFF; //Teensy uses PORTD
	PORTD =  0x0;
                  //We'll just flash all pins on both ports since the UNO R3
	DDRB  = 0xFF; //uses PORTB. Micro can use either or, but both give us 2 LEDs
	PORTB =  0x0; //The ATmega328P on the UNO will be resetting, so unplug it?
	#endif
	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}
typedef enum {
	SYNC_CONTROLLER,
	AIR_DODGE,
	MARIO_LEDGE_GETUP,
	MASH,
	RANDOM_DI,
	NOT_HIT,
	HIT,
	RESET,
	DONE
} State_t;
typedef enum {
	RUN,
	JUMP,
	ONLEDGE,
	GETUP,
	SHIELD
} Ledge_getup_state;
int report_count = 0;
State_t state = SYNC_CONTROLLER;
State_t second_state = MASH;
State_t third_state = RANDOM_DI;
int third_state_timeout=250;
int min_wait=4;
int dodge_count=0;
int jump_count=1;
// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			while(Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError);
			// At this point, we can react to this data.
			if(JoystickOutputData.Button != NULL || JoystickOutputData.HAT != NULL || JoystickOutputData.LX != NULL || JoystickOutputData.LY != NULL || JoystickOutputData.RX != NULL || JoystickOutputData.RY != NULL)
			{

			}
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		while(Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError);
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();
	}
}

typedef enum {
	SYNC_CONTROLLER,
	SYNC_POSITION,
	STOP_X,
	STOP_Y,
	MOVE_X,
	MOVE_Y,
	DONE
} State_t;
State_t state = SYNC_CONTROLLER;

#define ECHOES 2
int echoes = 0;
USB_JoystickReport_Input_t last_report;

int report_count = 0;
int xpos = 0;
int ypos = 0;
int portsval = 0;

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData) {

	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER;
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;
	switch(state)
	{
		case SYNC_CONTROLLER:
			if (report_count > 20)
			{
				changeState(second_state);
			}
			else if (report_count == 10)
			{
				ReportData->Button |= SWITCH_L | SWITCH_R;
			}
			report_count++;
			break;
		case RESET:
			if(report_count < 50){
				if(report_count > 10){
					ReportData->Button |= SWITCH_L | SWITCH_R | SWITCH_A;
				}
			}
			else{
				changeState(second_state);
			}
			report_count++;
			break;
		case NOT_HIT:
			if(report_count>20){
				//ReportData->Button |= SWITCH_A;
				//ReportData->LX = STICK_MAX;
			}

			if(report_count>150){
				if(report_count == 110 || report_count == 120 || report_count == 130){
					second_state=HIT;
				}
			}
			if(second_state==HIT){
				changeState(third_state);
				second_state=NOT_HIT;
			}
			report_count++;
			break;
		case AIR_DODGE:
			if(report_count>third_state_timeout){
				//changeState(RESET);
			}
			if(report_count == 2*(2*dodge_count+1)){
				ReportData->Button |= SWITCH_ZR;
				dodge_count++;
			}
			if(report_count == 4*jump_count){
				ReportData->Button |=SWITCH_X;
				jump_count++;
			}
			report_count++;
			break;
		case RANDOM_DI:
			if(report_count>third_state_timeout){
				changeState(RESET);
			}
			if(randbool){
				ReportData->LX = STICK_MAX;
			}
			else{
				ReportData->LX = STICK_MIN;
			}
			if(report_count % (min_wait) == 0){
				ReportData->Button |= SWITCH_ZR;
			}
			report_count++;
			break;
		case MASH:
			if(report_count>third_state_timeout){
				//changeState(RESET);
			}
			if(report_count % min_wait*2 < min_wait){
				ReportData->LX = STICK_MAX;
			}
			else if(report_count % min_wait*2 >= min_wait){
				ReportData->LX = STICK_MIN;
			}
			report_count++;
			break;
	}


}
