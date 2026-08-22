/*
 * VLSI VS1053b SPI audio DSP codec driver
 *
 * The VS1053b exposes two SPI interfaces on the same bus:
 *
 *   Serial Control Interface
 *     CS: XCS (active low, managed by the SPI bus driver via DT)
 *     Speed:       ~250kHz
 *     Protocol:    opcode[1] + addr[1] + data[2] for writes;
 *                  opcode[1] + addr[1] + dummy[2] + read[2] for reads
 *
 *   Serial Data Interface
 *     CS:          XDCS (active low, driven by dcs_gpio directly)
 *     Speed:       ~7.3MHz @ 3.3V
 *     Protocol:    raw compressed audio
 *
 * DREQ goes high when the chip can accept VS1053_SDI_CHUNK_SIZE more
 * bytes through the SDI.
 */

#define DT_DRV_COMPAT vlsi_vs1053

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "vs1053.h"

LOG_MODULE_REGISTER(vs1053, CONFIG_LOG_DEFAULT_LEVEL);

// SPI command bytes
#define SCI_WRITE_CMD 0x02
#define SCI_READ_CMD  0x03

// Clock frequency: 3.5 x 12.288 MHz = ~43 MHz
#define CLOCKF_INIT 0x6000

// Initial volume: -10 dBFS per channel (0x00 = max, 0xFF = off)
#define VOLUME_INIT 20U

#define DREQ_TIMEOUT_MS 100

struct vs1053_config {
	struct spi_dt_spec spi;         // SPI
	struct gpio_dt_spec dreq_gpio;  // DREQ input
	struct gpio_dt_spec dcs_gpio;   // XDCS output
	struct gpio_dt_spec reset_gpio; // XRESET output
	uint32_t sdi_max_freq;          // SDI clock rate from DT
};

// Mutex for serial data transfer
struct vs1053_data {
	struct k_mutex lock;
};

/* ---- SCI ---- */

int vs1053_sci_write(const struct device *dev, uint8_t addr, uint16_t data)
{
	const struct vs1053_config *cfg = dev->config;
	uint8_t buf[4] = {
		SCI_WRITE_CMD,
		addr,
		(uint8_t)(data >> 8),
		(uint8_t)(data & 0xFF),
	};
	const struct spi_buf tx_buf = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&cfg->spi, &tx);
}

int vs1053_sci_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	const struct vs1053_config *cfg = dev->config;
	uint8_t tx_data[4] = {SCI_READ_CMD, addr, 0x00, 0x00};
	uint8_t rx_data[4] = {0};
	const struct spi_buf tx_buf = {.buf = tx_data, .len = sizeof(tx_data)};
	const struct spi_buf rx_buf = {.buf = rx_data, .len = sizeof(rx_data)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret == 0) {
		*val = ((uint16_t)rx_data[2] << 8) | rx_data[3];
	}
	return ret;
}

/* ---- SDI ---- */

bool vs1053_ready_for_data(const struct device *dev)
{
	const struct vs1053_config *cfg = dev->config;

	return gpio_pin_get_dt(&cfg->dreq_gpio) > 0;
}

// Send one chunk (<= VS1053_SDI_CHUNK_SIZE bytes) over the SDI.
static int sdi_write_chunk(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct vs1053_config *cfg = dev->config;
	const struct spi_buf tx_buf = {.buf = (void *)buf, .len = len};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	const struct spi_config sdi_cfg = {
		.frequency = cfg->sdi_max_freq,
		.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
	};
	int ret;

	gpio_pin_set_dt(&cfg->dcs_gpio, 1); // assert XDCS
	ret = spi_write(cfg->spi.bus, &sdi_cfg, &tx);
	gpio_pin_set_dt(&cfg->dcs_gpio, 0); // deassert XDCS

	return ret;
}

int vs1053_sdi_write(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct vs1053_data *data = dev->data;
	size_t offset = 0;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	while (offset < len) {
		size_t chunk = MIN(len - offset, (size_t)VS1053_SDI_CHUNK_SIZE);
		int64_t deadline = k_uptime_get() + DREQ_TIMEOUT_MS;

		while (!vs1053_ready_for_data(dev)) {
			if (k_uptime_get() >= deadline) {
				ret = -ETIMEDOUT;
				goto out;
			}
			k_yield();
		}

		ret = sdi_write_chunk(dev, buf + offset, chunk);
		if (ret < 0) {
			goto out;
		}
		offset += chunk;
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

/* ---- Helpers ---- */

int vs1053_set_volume(const struct device *dev, uint8_t left, uint8_t right)
{
	return vs1053_sci_write(dev, VS1053_REG_VOLUME, ((uint16_t)left << 8) | right);
}

int vs1053_decode_time(const struct device *dev, uint16_t *seconds)
{
	return vs1053_sci_read(dev, VS1053_REG_DECODETIME, seconds);
}

int vs1053_soft_reset(const struct device *dev)
{
	int ret;

	ret = vs1053_sci_write(dev, VS1053_REG_MODE, VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_RESET);
	if (ret < 0) {
		return ret;
	}

	// Wait 100ms for reset to complete
	int64_t deadline = k_uptime_get() + 100;

	while (!vs1053_ready_for_data(dev)) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_msleep(1);
	}

	return 0;
}

/* ---- Driver init ---- */

static int vs1053_init(const struct device *dev)
{
	const struct vs1053_config *cfg = dev->config;
	struct vs1053_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->dreq_gpio)) {
		LOG_ERR("DREQ GPIO not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->dcs_gpio)) {
		LOG_ERR("DCS GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->dreq_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure DREQ GPIO: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->dcs_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure DCS GPIO: %d", ret);
		return ret;
	}

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("RESET GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure RESET GPIO: %d", ret);
			return ret;
		}

		// Assert and deassert reset
		gpio_pin_set_dt(&cfg->reset_gpio, 1);
		k_msleep(10);
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(10);
	}

	ret = vs1053_soft_reset(dev);
	if (ret < 0) {
		LOG_ERR("Soft reset failed: %d", ret);
		return ret;
	}

	// Boost internal clock
	ret = vs1053_sci_write(dev, VS1053_REG_CLOCKF, CLOCKF_INIT);
	if (ret < 0) {
		return ret;
	}

	ret = vs1053_set_volume(dev, VOLUME_INIT, VOLUME_INIT);
	if (ret < 0) {
		return ret;
	}

	// Check status version field to confirm chip is online
	uint16_t status;

	ret = vs1053_sci_read(dev, VS1053_REG_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to read STATUS register: %d", ret);
		return ret;
	}
	uint8_t version = (status >> 4) & 0x0F;

	if (version != 4) {
		LOG_WRN("Unexpected VS10xx version %u (expected 4 for VS1053)", version);
	} else {
		LOG_DBG("VS1053 detected (version=%u)", version);
	}

	return 0;
}

/* ---- Init macro ---- */

#define VS1053_INIT(inst)                                                                          \
	static struct vs1053_data vs1053_data_##inst;                                              \
                                                                                                   \
	static const struct vs1053_config vs1053_cfg_##inst = {                                    \
		.spi = SPI_DT_SPEC_INST_GET(                                                       \
			inst, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),         \
		.dreq_gpio = GPIO_DT_SPEC_INST_GET(inst, dreq_gpios),                              \
		.dcs_gpio = GPIO_DT_SPEC_INST_GET(inst, dcs_gpios),                                \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.sdi_max_freq = DT_INST_PROP(inst, sdi_max_frequency),                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vs1053_init, NULL, &vs1053_data_##inst, &vs1053_cfg_##inst,    \
			      POST_KERNEL, CONFIG_AUDIO_VS1053_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(VS1053_INIT)
