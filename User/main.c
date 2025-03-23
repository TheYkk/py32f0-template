#include "py32f0xx_bsp_printf.h"
#include "py32f0xx_hal_spi.h"
#include "py32f0xx_bsp_clock.h"
#include "py32f0xx_bsp_printf.h"
#include "py32f0xx_hal.h"

SPI_HandleTypeDef spi1Handle;

void APP_ErrorHandler(void);

static void APP_SPI_Config(void);

// Helper function to handle CS pin
void FLASH_CS_Low(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // Assuming PA4 is CS
}

void FLASH_CS_High(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // Assuming PA4 is CS
}

// Function to write enable the flash (WREN command)
HAL_StatusTypeDef FLASH_WriteEnable(void)
{
  uint8_t cmd = 0x06; // WREN command code [cite: 39]
  FLASH_CS_Low();
  HAL_StatusTypeDef status = HAL_SPI_Transmit(&spi1Handle, &cmd, 1, HAL_MAX_DELAY);
  FLASH_CS_High();
  return status;
}

// Function to read the status register
HAL_StatusTypeDef FLASH_ReadStatusRegister(uint8_t *status)
{
  uint8_t cmd = 0x05; // RDSR command code [cite: 41, 42]
  FLASH_CS_Low();
  HAL_StatusTypeDef status1 = HAL_SPI_Transmit(&spi1Handle, &cmd, 1, HAL_MAX_DELAY);
  HAL_StatusTypeDef status2 = HAL_SPI_Receive(&spi1Handle, status, 1, HAL_MAX_DELAY);
  FLASH_CS_High();
  return status1 == HAL_OK && status2 == HAL_OK ? HAL_OK : HAL_ERROR;
}

// Function to wait for the flash to be ready (WIP bit check)
void FLASH_WaitForReady(void)
{
  uint8_t status;
  do
  {
    FLASH_ReadStatusRegister(&status);
  } while (status & 0x01); // WIP bit is bit 0 [cite: 363, 364, 365]
}

// Function to write a page of data to the flash
HAL_StatusTypeDef FLASH_PageProgram(uint32_t address, uint8_t *data, uint16_t len)
{
  if (len > 256)
    return HAL_ERROR; // Page size is 256 bytes [cite: 324]

  HAL_StatusTypeDef status = HAL_OK;

  FLASH_WriteEnable(); // Enable writing [cite: 171]
  FLASH_WaitForReady();

  uint8_t cmd = 0x02; // PP command code [cite: 40]
  FLASH_CS_Low();

  status |= HAL_SPI_Transmit(&spi1Handle, &cmd, 1, HAL_MAX_DELAY);

  // Send the 24-bit address [cite: 45, 46]
  status |= HAL_SPI_Transmit(&spi1Handle, (uint8_t *)&address + 2, 1, HAL_MAX_DELAY); // MSB
  status |= HAL_SPI_Transmit(&spi1Handle, (uint8_t *)&address + 1, 1, HAL_MAX_DELAY);
  status |= HAL_SPI_Transmit(&spi1Handle, (uint8_t *)&address, 1, HAL_MAX_DELAY); // LSB

  // Send the data
  status |= HAL_SPI_Transmit(&spi1Handle, data, len, HAL_MAX_DELAY);

  FLASH_CS_High();

  return status;
}

// Function to read data from the flash
HAL_StatusTypeDef FLASH_ReadData(uint32_t address, uint8_t *data, uint16_t len)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t cmd = 0x03; // READ LOW POWER
  // uint8_t cmd = 0x0B; // READ FAST command code [cite: 39]

  FLASH_CS_Low();

  status |= HAL_SPI_Transmit(&spi1Handle, &cmd, 1, HAL_MAX_DELAY);

  // Send the 24-bit address [cite: 91, 92, 93]
  status |= HAL_SPI_Transmit(&spi1Handle, (uint8_t *)&address + 2, 1, HAL_MAX_DELAY); // MSB
  status |= HAL_SPI_Transmit(&spi1Handle, (uint8_t *)&address + 1, 1, HAL_MAX_DELAY);
  status |= HAL_SPI_Transmit(&spi1Handle, (uint8_t *)&address, 1, HAL_MAX_DELAY); // LSB

  // Receive the data
  status |= HAL_SPI_Receive(&spi1Handle, data, len, HAL_MAX_DELAY);

  FLASH_CS_High();

  return status;
}
// Replace your current ID reading code with this:
void ReadFlashID(uint8_t *id_buffer)
{
  uint8_t cmd = 0x9F; // RDID command

  FLASH_CS_Low();

  // Send only the command byte
  HAL_SPI_Transmit(&spi1Handle, &cmd, 1, HAL_MAX_DELAY);

  // Then receive the ID bytes separately
  HAL_SPI_Receive(&spi1Handle, id_buffer, 3, HAL_MAX_DELAY);

  FLASH_CS_High();
}
int main(void)
{
  HAL_Init();
  BSP_USART_Config();
  HAL_Delay(4000);
  GPIO_InitTypeDef GPIO_CSInitStruct;
  GPIO_CSInitStruct.Pin = GPIO_PIN_4;
  GPIO_CSInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_CSInitStruct.Pull = GPIO_NOPULL;
  GPIO_CSInitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_CSInitStruct);
  FLASH_CS_High(); // Initialize CS pin to high (inactive)

  APP_SPI_Config();

  // Read Flash ID
  uint8_t id_buffer[3] = {0};
  ReadFlashID(id_buffer);
  printf("Flash ID (separate): %02X %02X %02X\r\n",
         id_buffer[0], id_buffer[1], id_buffer[2]);
  // Sample Write and Read
  uint32_t test_address = 0x000100;
  uint8_t write_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  uint8_t read_data[16] = {0};

  // Write data to flash
  // if (FLASH_PageProgram(test_address, write_data, sizeof(write_data)) != HAL_OK)
  // {
  //   printf("Write failed\r\n");
  // }
  // FLASH_WaitForReady(); // Wait for write to complete

  // Read data from flash
  if (FLASH_ReadData(test_address, read_data, sizeof(read_data)) == HAL_OK)
  {
    printf("Read Data: ");
    for (int i = 0; i < sizeof(read_data); i++)
    {
      printf("%02X ", read_data[i]);
    }
    printf("\r\n");
  }
  else
  {
    printf("Read failed\r\n");
  }
  GPIO_InitTypeDef GPIO_InitStruct;

  // PA0 - LED
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  while (1)
  {
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0); // Toggle LED on PA0
    printf("echo\r\n");
  }
}

static void APP_SPI_Config(void)
{
  spi1Handle.Instance = SPI1;
  spi1Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // Adjust as needed
  spi1Handle.Init.Direction = SPI_DIRECTION_2LINES;            // Use 2 lines for full duplex [cite: 357, 358]
  spi1Handle.Init.CLKPolarity = SPI_POLARITY_LOW;              // Check datasheet for correct polarity [cite: 355, 356]
  spi1Handle.Init.CLKPhase = SPI_PHASE_1EDGE;                  // Check datasheet for correct phase [cite: 355, 356]
  spi1Handle.Init.DataSize = SPI_DATASIZE_8BIT;
  spi1Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
  spi1Handle.Init.NSS = SPI_NSS_SOFT;
  spi1Handle.Init.Mode = SPI_MODE_MASTER;
  /* SPI initialization */
  if (HAL_SPI_Init(&spi1Handle) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

void APP_ErrorHandler(void)
{
  while (1)
    ;
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Export assert error source and line number
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  while (1)
    ;
}
#endif /* USE_FULL_ASSERT */