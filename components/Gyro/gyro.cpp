/*
 * Display.c
 *
 *  Created on: 14.08.2017
 *      Author: darek
 */
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "mpu6050.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "sdkconfig.h"

#define I2C_MPU_SDA_IO 4
#define I2C_MPU_SCL_IO 5
#define I2C_MPU_NUM              I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MPU_TX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define I2C_MPU_RX_BUF_DISABLE   0                /*!< I2C master do not need */

Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
uint16_t packetSize = 42;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU

static const char *TAG = "Drone's Gyro";

void MPU_initI2C() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MPU_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MPU_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.clk_stretch_tick = 3000; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MPU_NUM, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(I2C_MPU_NUM, &conf));
//    vTaskDelete(NULL);
}

void MPU_run(void*){
    MPU_initI2C();
    // Delay to enable drifting time
    //TODO: Set the right delay
    vTaskDelay(500/portTICK_PERIOD_MS);
    
	MPU6050 mpu = MPU6050();
	mpu.initialize();
	mpu.dmpInitialize();

	// This need to be setup individually
	mpu.setXGyroOffset(220);
	mpu.setYGyroOffset(76);
	mpu.setZGyroOffset(-85);
	mpu.setZAccelOffset(1788);

	mpu.setDMPEnabled(true);

	while(1){
	    mpuIntStatus = mpu.getIntStatus();
		// get current FIFO count
		fifoCount = mpu.getFIFOCount();

	    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
	        // reset so we can continue cleanly
	        mpu.resetFIFO();

	    // otherwise, check for DMP data ready interrupt frequently)
	    } else if (mpuIntStatus & 0x02) {
	        // wait for correct available data length, should be a VERY short wait
	        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

	        // read a packet from FIFO
            ESP_LOGI(TAG, "Running");
	        mpu.getFIFOBytes(fifoBuffer, packetSize);
	 		mpu.dmpGetQuaternion(&q, fifoBuffer);
			mpu.dmpGetGravity(&gravity, &q);
			mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
            ypr[0] = ypr[0] * 180/M_PI;
            ypr[1] = ypr[1] * 180/M_PI;
            ypr[2] = ypr[2] * 180/M_PI;
            
//			printf("YAW: %3.1f, ", ypr[0] * 180/M_PI);
//			printf("PITCH: %3.1f, ", ypr[1] * 180/M_PI);
//			printf("ROLL: %3.1f \n", ypr[2] * 180/M_PI);
	    }

	    //Best result is to match with DMP refresh rate
	    // Its last value in components/MPU6050/MPU6050_6Axis_MotionApps20.h file line 310
	    // Now its 0x13, which means DMP is refreshed with 10Hz rate
		vTaskDelay(10/portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}
