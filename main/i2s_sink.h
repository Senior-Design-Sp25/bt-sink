
#ifndef __I2S_SINK_H__
#define __I2S_SINK_H__

/* Define I2S GPIO pins */
#define I2S_GPIO_BCLK (25) 	/* 26 I2S Bit Clock GPIO pin */ 
#define I2S_GPIO_WS (26)	/* 22 I2S Word Select (LRCLK) GPIO pin */
#define I2S_GPIO_D_OUT (27)	/* 25 I2S Data Out GPIO pin */

/* initialize i2s and create ring buffer */
void init_i2s(void);

/* data handling task for i2s */
void i2s_data_handler(void *arg);

#endif /* __I2S_SINK_H__ */