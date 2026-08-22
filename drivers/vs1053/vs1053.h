#ifndef DRIVERS_VS1053_VS1053_H
#define DRIVERS_VS1053_VS1053_H

#include <zephyr/device.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SCI register addresses */
#define VS1053_REG_MODE       0x00 // Mode
#define VS1053_REG_STATUS     0x01 // Device status
#define VS1053_REG_BASS       0x02 // Bass/treble enhancer
#define VS1053_REG_CLOCKF     0x03 // Clock frequency and multiplier
#define VS1053_REG_DECODETIME 0x04 // Elapsed decode time in seconds
#define VS1053_REG_AUDATA     0x05 // Misc. audio data
#define VS1053_REG_WRAM       0x06 // RAM read/write port
#define VS1053_REG_WRAMADDR   0x07 // RAM read/write base address
#define VS1053_REG_HDAT0      0x08 // Stream header data 0
#define VS1053_REG_HDAT1      0x09 // Stream header data 1
#define VS1053_REG_AIADDR     0x0A // Application start address
#define VS1053_REG_VOLUME     0x0B // Volume control
#define VS1053_REG_AICTRL0    0x0C // Application control register 0
#define VS1053_REG_AICTRL1    0x0D // Application control register 1
#define VS1053_REG_AICTRL2    0x0E // Application control register 2
#define VS1053_REG_AICTRL3    0x0F // Application control register 3

/* VS1053_REG_MODE bit definitions */
#define VS1053_MODE_SM_DIFF     BIT(0)  // Left channel polarity invert
#define VS1053_MODE_SM_LAYER12  BIT(1)  // Allow MPEG layers I and II
#define VS1053_MODE_SM_RESET    BIT(2)  // Soft reset
#define VS1053_MODE_SM_CANCEL   BIT(3)  // Cancel current decode
#define VS1053_MODE_SM_EARSPKLO BIT(4)  // EarSpeaker low setting
#define VS1053_MODE_SM_TESTS    BIT(5)  // Allow SDI tests
#define VS1053_MODE_SM_STREAM   BIT(6)  // Stream mode
#define VS1053_MODE_SM_SDINEW   BIT(11) // VS1002 native SPI mode
#define VS1053_MODE_SM_ADPCM    BIT(12) // PCM/ADPCM recording active
#define VS1053_MODE_SM_LINE1    BIT(14) // MIC/LINE1 selector
#define VS1053_MODE_SM_CLKRANGE BIT(15) // Input clock range select

/* WRAM peripheral register addresses */
#define VS1053_GPIO_DDR       0xC017 // GPIO direction
#define VS1053_GPIO_IDATA     0xC018 // GPIO input values
#define VS1053_GPIO_ODATA     0xC019 // GPIO output values
#define VS1053_INT_ENABLE     0xC01A // Interrupt enable
#define VS1053_PARA_PLAYSPEED 0x1E04 // Playback speed

// Maximum bytes per SDI transfer
#define VS1053_SDI_CHUNK_SIZE 32

/**
 * @brief Write a value to a VS1053 SCI register.
 *
 * @param dev  VS1053 device pointer.
 * @param addr Register address (VS1053_REG_*).
 * @param data 16-bit value to write.
 * @return 0 on success, negative errno on failure.
 */
int vs1053_sci_write(const struct device *dev, uint8_t addr, uint16_t data);

/**
 * @brief Read a VS1053 SCI register.
 *
 * @param dev  VS1053 device pointer.
 * @param addr Register address (VS1053_REG_*).
 * @param val  Output: register value.
 * @return 0 on success, negative errno on failure.
 */
int vs1053_sci_read(const struct device *dev, uint8_t addr, uint16_t *val);

/**
 * @brief Check if VS1053 FIFO is ready for more audio data.
 *
 * @param dev VS1053 device pointer.
 * @return true if DREQ is high, false otherwise.
 */
bool vs1053_ready_for_data(const struct device *dev);

/**
 * @brief Send compressed audio data to VS1053 SDI.
 *
 * Split the buffer into VS1053_SDI_CHUNK_SIZE byte chunks. Wait for DREQ
 * before each chunk, timing out after 100ms if DREQ does not update.
 *
 * @param dev VS1053 device pointer.
 * @param buf Pointer to audio data.
 * @param len Number of bytes to send.
 * @return 0 on success, -ETIMEDOUT if DREQ wait exceeds 100 ms,
 *         other negative errno on SPI failure.
 */
int vs1053_sdi_write(const struct device *dev, const uint8_t *buf, size_t len);

/**
 * @brief Set the output volume.
 *
 * Each byte is an independent attenuation value for one channel:
 * 0x00 = full volume, 0xFF = off.
 *
 * @param dev   VS1053 device pointer.
 * @param left  Left channel attenuation.
 * @param right Right channel attenuation.
 * @return 0 on success, negative errno on failure.
 */
int vs1053_set_volume(const struct device *dev, uint8_t left, uint8_t right);

/**
 * @brief Return the elapsed decode time in seconds.
 *
 * @param dev     VS1053 device pointer.
 * @param seconds Output: decoded seconds.
 * @return 0 on success, negative errno on failure.
 */
int vs1053_decode_time(const struct device *dev, uint16_t *seconds);

/**
 * @brief Perform a VS1053 soft reset.
 *
 * Sets VS1053_MODE_SM_RESET in the MODE register and waits for the
 * chip to complete the reset sequence.
 *
 * @param dev VS1053 device pointer.
 * @return 0 on success, negative errno on failure.
 */
int vs1053_soft_reset(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif
