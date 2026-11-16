#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*============================================================================================================================================================================
Cấu hình ngoại vi
============================================================================================================================================================================*/

// SPI0 dự phòng. Chân CS do GPIO điều khiển và ở mức cao khi rảnh.
#define SPI_PORT      spi0
#define SPI_MISO_PIN  16
#define SPI_CS_PIN    17
#define SPI_SCK_PIN   18
#define SPI_MOSI_PIN  19
#define SPI_BAUD_RATE 1000000

// I2C0 dành cho OLED 1.
#define I2C0_PORT      i2c0
#define I2C0_SDA_PIN   4
#define I2C0_SCL_PIN   5
#define I2C0_BAUD_RATE 400000
#define OLED1_ADDRESS  0x3C

// I2C1 dành cho OLED 2.
#define I2C1_PORT      i2c1
#define I2C1_SDA_PIN   6
#define I2C1_SCL_PIN   7
#define I2C1_BAUD_RATE 400000

// UART1 dự phòng, cấu hình 8N1.
#define UART_PORT      uart1
#define UART_TX_PIN    8
#define UART_RX_PIN    9
#define UART_BAUD_RATE 115200

// Ba chân liên tiếp cho PIO đọc mã tác vụ và xung chốt.
#define TASK_D0_PIN     10
#define TASK_D1_PIN     11
#define TASK_STROBE_PIN 12

// LED báo lỗi dự phòng, nối qua điện trở xuống GND.
#define FALLBACK_LED_PIN 22

/*============================================================================================================================================================================
Cấu hình chẩn đoán
============================================================================================================================================================================*/

#define BLINK_PULSE_MS          100
#define BLINK_PAUSE_MS          1200
#define CORE1_LAUNCH_TIMEOUT_MS 1000
#define CORE1_RESULT_TIMEOUT_US 1000000
#define CORE1_TASK_TIMEOUT_US   3000000
#define CORE1_LAUNCH_MARKER     0x434F5231u

/*============================================================================================================================================================================
Mã trạng thái chẩn đoán
============================================================================================================================================================================*/

// Giá trị khác 0 là số nháy ngắn trong mỗi cụm.
typedef enum
{
    STATUS_OK = 0,
    ERROR_CYW43_INIT = 1,
    ERROR_DMA_NO_CHANNEL = 2,
    ERROR_DMA_TIMEOUT = 3,
    ERROR_DMA_DATA = 4,
    ERROR_CORE1_NO_RESULT = 5,
    ERROR_OLED_UI_TEST = 6,
    ERROR_TASK_PIO_INIT = 7
}
diagnostic_status_t;

#endif
