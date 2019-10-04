// labwork1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <iostream>
#include<conio.h>
#include<stdlib.h>
#include <windows.h>  //for Sleep function
extern "C" {
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <interface.h> 
}

xSemaphoreHandle sem_x_done;
xSemaphoreHandle sem_z_done;

xQueueHandle mbx_x;
xQueueHandle mbx_z;
xQueueHandle mbx_input;


int getBitValue(uInt8 value, uInt8 bit_n)
// given a byte value, returns the value of its bit n
{
	return(value & (1 << bit_n));
}

void setBitValue(uInt8* variable, int n_bit, int new_value_bit)
// given a byte value, set the n bit to value
{
	uInt8  mask_on = (uInt8)(1 << n_bit);
	uInt8  mask_off = ~mask_on;
	if (new_value_bit)* variable |= mask_on;
	else                *variable &= mask_off;
}

void moveXLeft()
{
	taskENTER_CRITICAL();
	uInt8 p = readDigitalU8(2); //  read port 2
	setBitValue(&p, 6, 1);     //  set bit 6 to high level
	setBitValue(&p, 7, 0);      //set bit 7 to low level
	writeDigitalU8(2, p); //  update port 2
	taskEXIT_CRITICAL();
}


void moveXRight()
{
	taskENTER_CRITICAL();
	uInt8 p = readDigitalU8(2); //read port 2
	setBitValue(&p, 6, 0);    //  set bit 6 to  low level
	setBitValue(&p, 7, 1);      //set bit 7 to high level
	writeDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}

void moveYInside()
{
	taskENTER_CRITICAL();
	uInt8 p = readDigitalU8(2);
	setBitValue(&p, 5, 1);    //  set bit 5 to  high level
	setBitValue(&p, 4, 0);      //set bit 4 to low level
	writeDigitalU8(2, p);
	taskEXIT_CRITICAL();
}

void moveYOutside()
{
	taskENTER_CRITICAL();
	uInt8 p = readDigitalU8(2); //read port 2
	setBitValue(&p, 5, 0);    //  set bit 5 to  low level
	setBitValue(&p, 4, 1);      //set bit 4 to high level
	writeDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}

void moveZUp()
{
	taskENTER_CRITICAL();
	uInt8 p = readDigitalU8(2); //read port 2
	setBitValue(&p, 3, 1);    //  set bit 3 to  high level
	setBitValue(&p, 2, 0);      //set bit 2 to low level
	writeDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}

void moveZDown()
{
	taskENTER_CRITICAL();
	uInt8 p = readDigitalU8(2); //read port 2
	setBitValue(&p, 3, 0);    //  set bit 3 to  low level
	setBitValue(&p, 2, 1);      //set bit 2 to high level
	writeDigitalU8(2, p); //update port 2
	taskEXIT_CRITICAL();
}

void stopX()
{
	taskENTER_CRITICAL();
	uInt8 pX = readDigitalU8(2); //read port 2
	setBitValue(&pX, 6, 0);   //  set bit 6 to  low level
	setBitValue(&pX, 7, 0);   //set bit 7 to low level
	writeDigitalU8(2, pX); //update port 2
	taskEXIT_CRITICAL();
}


void stopY()
{
	taskENTER_CRITICAL();
	uInt8 pY = readDigitalU8(2); //read port 2
	setBitValue(&pY, 4, 0);   //  set bit 6 to  low level
	setBitValue(&pY, 5, 0);   //set bit 7 to low level
	writeDigitalU8(2, pY); //update port 2
	taskEXIT_CRITICAL();
}

void stopZ()
{
	taskENTER_CRITICAL();
	uInt8 pZ = readDigitalU8(2); //read port 2
	setBitValue(&pZ, 2, 0);   //  set bit 6 to  low level
	setBitValue(&pZ, 3, 0);   //set bit 7 to low level
	writeDigitalU8(2, pZ); //update port 2
	taskEXIT_CRITICAL();
}

int getXPos()
{
	uInt8 p = readDigitalU8(0);
	if (!getBitValue(p, 2))
		return 1;
	if (!getBitValue(p, 1))
		return 2;
	if (!getBitValue(p, 0))
		return 3;
	return(-1);
}

int getZPos()
{
	uInt8 p0 = readDigitalU8(0);
	uInt8 p1 = readDigitalU8(1);
	if (!getBitValue(p1, 3))
		return 1;
	if (!getBitValue(p1, 1))
		return 2;
	if (!getBitValue(p0, 7))
		return 3;
	return(-1);
}

int getYPos()
{
	uInt8 p = readDigitalU8(0);

	if (!getBitValue(p, 5))
		return 1;
	if (!getBitValue(p, 4))
		return 2;
	if (!getBitValue(p, 3))
		return 3;
	return(-1);

}



void gotoX(int x_dest) {
	int x_act = getXPos();
	if (x_act != x_dest) {
		if (x_act < x_dest)
			moveXRight();
		else if (x_act > x_dest)
			moveXLeft();
		while (getXPos() != x_dest) {
			Sleep(1);
		}
		stopX();
	}
}


void gotoY(int y_dest) {
	int y_act = getYPos();
	if (y_act != y_dest) {
		if (y_act < y_dest)
			moveYInside();
		else if (y_act > y_dest)
			moveYOutside();
		while (getYPos() != y_dest) {
			Sleep(1);
		}
		stopY();
	}
}



void gotoZ(int z_dest) {
	int z_act = getZPos();
	if (z_act != z_dest) {
		if (z_act < z_dest)
			moveZUp();
		else if (z_act > z_dest)
			moveZDown();
		while (getZPos() != z_dest) {
			Sleep(1);
		}
		stopZ();
	}
}

/*********************************/
/*************TASKS***************/
/*********************************/


void goto_x_task(void* param)
{
	while (true)
	{
		int x;
		xQueueReceive(mbx_x, &x, portMAX_DELAY);
		gotoX(x);
		xSemaphoreGive(sem_x_done);
	}
}

void goto_z_task(void* param)
{

	while (true)
	{
		int z;
		xQueueReceive(mbx_z, &z, portMAX_DELAY);
		gotoZ(z);
		xSemaphoreGive(sem_z_done);
	}
}

void gotoXZ(int x, int z)
{

	// parallel execution
	// send request for each goto_x_task and goto_z_task
	// which execute gotoX and gotoZ on their own.  
	xQueueSend(mbx_x, &x, portMAX_DELAY);
	xQueueSend(mbx_z, &z, portMAX_DELAY);

	//wait for goto_x completion, synchronization
	xSemaphoreTake(sem_x_done, portMAX_DELAY);

	//wait for goto_z completion, synchronization           
	xSemaphoreTake(sem_z_done, portMAX_DELAY);
}


void putPartInCell(){

	uInt8 p0 = readDigitalU8(0);
	uInt8 p1 = readDigitalU8(1);
	int z_pos = getZPos();
	if (z_pos == 1) {
		while (getBitValue(p1, 2) != 0) {

			moveZUp();
		}
		stopZ();
		gotoY(3);
		gotoZ(1);
		gotoY(2);
	}

	if (z_pos == 2) {
		while (getBitValue(p1, 1) != 0) {

			moveZUp();
		}
		stopZ();
		gotoY(3);
		gotoZ(2);
		gotoY(2);
	}

	if (z_pos == 3) {
		while (getBitValue(p1, 2) != 0) {

			moveZUp();
		}
		stopZ();
		gotoY(3);
		gotoZ(3);
		gotoY(2);

		
		//stop Y();
	}
}

void vTaskHorizontal(void* pvParameters)
{
	while (TRUE)
	{
		//go right
		uInt8 aa = readDigitalU8(2);
		writeDigitalU8(2, (aa & (0xff - 0x40)) | 0x80);

		// wait till last sensor
		while (readDigitalU8(0) & 0x01) {
			taskYIELD();
		}
		// go left		
		aa = readDigitalU8(2);
		vTaskDelay(10); // to simulate some latency  
		writeDigitalU8(2, (aa & (0xff - 0x80)) | 0x40);

		// wait till last sensor
		while ((readDigitalU8(0) & 0x04)) {
			taskYIELD();
		}
	}
}

void vTaskVertical(void* pvParameters)
{
	while (TRUE)
	{
		//go up
		uInt8 aa = readDigitalU8(2);
		writeDigitalU8(2, (aa & (0xff - 0x04)) | 0x08);

		// wait till last sensor		
		while ((readDigitalU8(0) & 0x40)) { vTaskDelay(1); }

		// go left		
		aa = readDigitalU8(2);
		vTaskDelay(10); // to simulate some latency
		writeDigitalU8(2, (aa & (0xff - 0x08)) | 0x04);

		// wait till last sensor		
		while ((readDigitalU8(1) & 0x08)) { vTaskDelay(1); }
	}
}

void show_menu()
{
	printf("\nCalibration keys: q, w, a, s, z, x (stops at the nearest sensor)");
	printf("\n(1) ....... Put a good in a specific x, z position");
	printf("\n(2) ....... Retrieve a good from a specific x, z position");
	//….
}


/*******************STORAGE******************/
void task_storage_services(void* param)
{
	int cmd = -1;

	while (true) {
		show_menu();
		// get selected option from keyboard
		printf("\n\nEnter option=");
		xQueueReceive(mbx_input, &cmd, portMAX_DELAY);

		if (cmd == 'q')
			moveXLeft();
		if (cmd == 'w')
			moveXRight();
		if (cmd == 'a')
			stopX();
		if (cmd == '1')
		{
			int x, z; // you can use scanf or else you like
			printf("\nX=");
			xQueueReceive(mbx_input, &x, portMAX_DELAY);

			x = x - '0'; //convert from ascii code to number
			printf("\nZ=");
			xQueueReceive(mbx_input, &z, portMAX_DELAY);
			z = z - '0'; //convert from ascii code to number

			if (x >= 1 && x <= 3 && z >= 1 && z <= 3)
				gotoXZ(x, z);  // it freezes the services,  
			else
				printf("\nWrong (x,z) coordinates, are you sleeping?... ");
		}
		if (cmd == 't') // hidden option
		{
			writeDigitalU8(2, 0); //stop all motors;
			vTaskEndScheduler(); // terminates application
		}
	}   // end while
}

/****************MAILBOXES*****************/
void receive_instructions_task(void* ignore) {
	int c = 0;

	while (true) {
		c = _getwch();  // this function is new.
		putchar(c);
		xQueueSend(mbx_input, &c, portMAX_DELAY);
	}
}


int main(int argc, char** argv) {

	printf("\nwaiting for hardware simulator...");
	printf("\nReminding: gotoXZ requires kit calibration first...");
	createDigitalInput(0);
	createDigitalInput(1);
	createDigitalOutput(2);
	writeDigitalU8(2, 0);
	printf("\ngot access to simulator...");

	sem_x_done = xSemaphoreCreateCounting(1000, 0);// try modeling adequate    
	sem_z_done = xSemaphoreCreateCounting(1000, 0);// capacity for the semaphores 

	mbx_x = xQueueCreate(10, sizeof(int));
	mbx_z = xQueueCreate(10, sizeof(int));
	mbx_input = xQueueCreate(10, sizeof(int));

	xTaskCreate(goto_x_task, "goto_x_task", 100, NULL, 0, NULL);
	xTaskCreate(goto_z_task, "goto_z_task", 100, NULL, 0, NULL);
	xTaskCreate(task_storage_services, "task_storage_services", 100, NULL, 0, NULL);
	xTaskCreate(receive_instructions_task, "receive_instructions_task", 100, NULL, 0, NULL);

	vTaskStartScheduler();




	return 0;
}

