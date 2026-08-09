/*
 * Binary protocol (host <-> device over bulk endpoints):
 *
 * Host -> Device commands:
 *   0x01               GET_TIMEZONE  (no payload)
 *   0x02 <h> <m>       SET_TIMEZONE  (int8 hours, int8 minutes)
 *   0x03               GET_ALARMS    (no payload)
 *   0x04 <h> <m> <s> <e>  ADD_ALARM (hour, minute, sound_id, enabled)
 *   0x05 <i>           DELETE_ALARM  (uint8 index)
 *
 * Device -> Host responses:
 *   0x01 <h> <m>           TIMEZONE  (int8 hours, int8 minutes)
 *   0x03 <n> [h m s e]...  ALARMS    (count, then 4 bytes per alarm)
 *   0xFF                   ACK
 *   0xFE                   NAK
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/msos_desc.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(timechime_usb, LOG_LEVEL_INF);

#include "webusb.h"
#include "msosv2.h"
#include "alarm.h"
#include "time.h"

#define TIMECHIME_USB_VID 0x1E2A
#define TIMECHIME_USB_PID 0x000A

#define CMD_GET_TIMEZONE 0x01
#define CMD_SET_TIMEZONE 0x02
#define CMD_GET_ALARMS   0x03
#define CMD_ADD_ALARM    0x04
#define CMD_DELETE_ALARM 0x05

#define RESP_TIMEZONE 0x01
#define RESP_ALARMS   0x03
#define RESP_ACK      0xFF
#define RESP_NAK      0xFE

#define TCUSB_BUF_SIZE 256

#define TCUSB_ENABLED 0

// USB device config.

USBD_DEVICE_DEFINE(timechime_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), TIMECHIME_USB_VID,
		   TIMECHIME_USB_PID);

USBD_DESC_LANG_DEFINE(timechime_lang);
USBD_DESC_MANUFACTURER_DEFINE(timechime_mfr, "LenaScript");
USBD_DESC_PRODUCT_DEFINE(timechime_product, "Timechime");

USBD_DESC_CONFIG_DEFINE(timechime_fs_cfg_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(timechime_fs_config, 0, 250, &timechime_fs_cfg_desc);

// Bulk USB setup.

NET_BUF_POOL_FIXED_DEFINE(tcusb_pool, 1, 0, sizeof(struct udc_buf_info), NULL);
UDC_STATIC_BUF_DEFINE(tcusb_buf, TCUSB_BUF_SIZE);

struct tcusb_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_desc_header nil_desc;
};

struct tcusb_data {
	struct tcusb_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	atomic_t state;
};

static uint8_t tcusb_ep_out(struct usbd_class_data *const c_data)
{
	struct tcusb_data *d = usbd_class_get_private(c_data);

	return d->desc->if0_out_ep.bEndpointAddress;
}

static uint8_t tcusb_ep_in(struct usbd_class_data *const c_data)
{
	struct tcusb_data *d = usbd_class_get_private(c_data);

	return d->desc->if0_in_ep.bEndpointAddress;
}

static struct net_buf *tcusb_buf_alloc(struct usbd_class_data *const c_data, uint8_t ep)
{
	struct net_buf *buf;
	struct udc_buf_info *bi;

	buf = net_buf_alloc_with_data(&tcusb_pool, tcusb_buf, TCUSB_BUF_SIZE, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	net_buf_reset(buf);
	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

static size_t tcusb_build_response(const uint8_t *cmd, size_t cmd_len, uint8_t *resp,
				   size_t resp_max)
{
	if (cmd_len == 0 || resp_max == 0) {
		return 0;
	}

	switch (cmd[0]) {
	case CMD_GET_TIMEZONE: {
		int8_t hours, minutes;

		timechime_time_get_timezone_offset(&hours, &minutes);
		resp[0] = RESP_TIMEZONE;
		resp[1] = (uint8_t)hours;
		resp[2] = (uint8_t)minutes;
		return 3;
	}
	case CMD_SET_TIMEZONE: {
		if (cmd_len < 3) {
			resp[0] = RESP_NAK;
			return 1;
		}
		timechime_time_set_timezone_offset((int8_t)cmd[1], (int8_t)cmd[2]);
		resp[0] = RESP_ACK;
		return 1;
	}
	case CMD_GET_ALARMS: {
		uint8_t count = timechime_alarm_get_count();
		size_t needed = 2U + (size_t)count * 4U;

		if (needed > resp_max) {
			resp[0] = RESP_NAK;
			return 1;
		}

		resp[0] = RESP_ALARMS;
		resp[1] = count;
		size_t off = 2;

		for (uint8_t i = 0; i < count; i++) {
			timechime_alarm_t *alarm;

			if (!timechime_alarm_get(i, &alarm)) {
				break;
			}
			resp[off++] = alarm->hour;
			resp[off++] = alarm->minute;
			resp[off++] = alarm->sound_id;
			resp[off++] = alarm->enabled ? 1U : 0U;
		}
		return off;
	}
	case CMD_ADD_ALARM: {
		if (cmd_len < 5) {
			resp[0] = RESP_NAK;
			return 1;
		}
		bool ok = timechime_alarm_new(cmd[1], cmd[2], cmd[3], cmd[4] != 0);

		resp[0] = ok ? RESP_ACK : RESP_NAK;
		return 1;
	}
	case CMD_DELETE_ALARM: {
		if (cmd_len < 2) {
			resp[0] = RESP_NAK;
			return 1;
		}
		bool ok = timechime_alarm_delete(cmd[1]);

		resp[0] = ok ? RESP_ACK : RESP_NAK;
		return 1;
	}
	default:
		resp[0] = RESP_NAK;
		return 1;
	}
}

static int tcusb_request_handler(struct usbd_class_data *c_data, struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct tcusb_data *data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = (struct udc_buf_info *)net_buf_user_data(buf);

	if (!atomic_test_bit(&data->state, TCUSB_ENABLED) || err) {
		usbd_ep_buf_free(uds_ctx, buf);
		return 0;
	}

	uint8_t ep = bi->ep;

	memset(bi, 0, sizeof(struct udc_buf_info));

	if (ep == tcusb_ep_out(c_data)) {
		// Command received.
		static uint8_t resp[TCUSB_BUF_SIZE];
		size_t resp_len = tcusb_build_response(buf->data, buf->len, resp, sizeof(resp));

		net_buf_reset(buf);
		net_buf_add_mem(buf, resp, MIN(resp_len, net_buf_tailroom(buf)));
		bi->ep = tcusb_ep_in(c_data);
	} else {
		// IN transfer complete, queue next OUT receive.
		net_buf_reset(buf);
		bi->ep = tcusb_ep_out(c_data);
	}

	if (usbd_ep_enqueue(c_data, buf)) {
		LOG_ERR("Failed to enqueue buffer");
		usbd_ep_buf_free(uds_ctx, buf);
	}

	return 0;
}

static void *tcusb_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	struct tcusb_data *data = usbd_class_get_private(c_data);

	return data->fs_desc;
}

static void tcusb_enable(struct usbd_class_data *const c_data)
{
	struct tcusb_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;

	if (atomic_test_and_set_bit(&data->state, TCUSB_ENABLED)) {
		return;
	}

	buf = tcusb_buf_alloc(c_data, tcusb_ep_out(c_data));
	if (!buf) {
		LOG_ERR("Failed to allocate RX buffer");
		return;
	}

	if (usbd_ep_enqueue(c_data, buf)) {
		LOG_ERR("Failed to enqueue RX buffer");
		usbd_ep_buf_free(uds_ctx, buf);
	}
}

static void tcusb_disable(struct usbd_class_data *const c_data)
{
	struct tcusb_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, TCUSB_ENABLED);
}

static int tcusb_init(struct usbd_class_data *c_data)
{
	return 0;
}

static struct usbd_class_api tcusb_api = {
	.request = tcusb_request_handler,
	.get_desc = tcusb_get_desc,
	.enable = tcusb_enable,
	.disable = tcusb_disable,
	.init = tcusb_init,
};

static struct tcusb_desc tcusb_desc_0 = {
	.if0 =
		{
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = USB_BCC_VENDOR,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
	.if0_out_ep =
		{
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = 0x01,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(64U),
			.bInterval = 0x00,
		},
	.if0_in_ep =
		{
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = 0x81,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(64U),
			.bInterval = 0x00,
		},
	.nil_desc =
		{
			.bLength = 0,
			.bDescriptorType = 0,
		},
};

static const struct usb_desc_header *tcusb_fs_desc_0[] = {
	(struct usb_desc_header *)&tcusb_desc_0.if0,
	(struct usb_desc_header *)&tcusb_desc_0.if0_out_ep,
	(struct usb_desc_header *)&tcusb_desc_0.if0_in_ep,
	(struct usb_desc_header *)&tcusb_desc_0.nil_desc,
};

static struct tcusb_data tcusb_data_0 = {
	.desc = &tcusb_desc_0,
	.fs_desc = tcusb_fs_desc_0,
};

USBD_DEFINE_CLASS(tcusb_0, &tcusb_api, &tcusb_data_0, NULL);

static void usbd_msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_INF("USBD: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			usbd_enable(usbd_ctx);
		} else if (msg->type == USBD_MSG_VBUS_REMOVED) {
			usbd_disable(usbd_ctx);
		}
	}
}

void timechime_usb_init(void)
{
	int ret;

	ret = usbd_add_descriptor(&timechime_usbd, &timechime_lang);
	if (ret) {
		LOG_ERR("Failed to add language descriptor (%d)", ret);
		return;
	}

	ret = usbd_add_descriptor(&timechime_usbd, &timechime_mfr);
	if (ret) {
		LOG_ERR("Failed to add manufacturer descriptor (%d)", ret);
		return;
	}

	ret = usbd_add_descriptor(&timechime_usbd, &timechime_product);
	if (ret) {
		LOG_ERR("Failed to add product descriptor (%d)", ret);
		return;
	}

	ret = usbd_add_descriptor(&timechime_usbd, &bos_vreq_webusb);
	if (ret) {
		LOG_ERR("Failed to add WebUSB BOS descriptor (%d)", ret);
		return;
	}

	ret = usbd_add_descriptor(&timechime_usbd, &bos_vreq_msosv2);
	if (ret) {
		LOG_ERR("Failed to add MSOSv2 BOS descriptor (%d)", ret);
		return;
	}

	ret = usbd_add_configuration(&timechime_usbd, USBD_SPEED_FS, &timechime_fs_config);
	if (ret) {
		LOG_ERR("Failed to add FS configuration (%d)", ret);
		return;
	}

	ret = usbd_register_class(&timechime_usbd, "tcusb_0", USBD_SPEED_FS, 1);
	if (ret) {
		LOG_ERR("Failed to register USB class (%d)", ret);
		return;
	}

	usbd_device_set_code_triple(&timechime_usbd, USBD_SPEED_FS, 0, 0, 0);

	ret = usbd_msg_register_cb(&timechime_usbd, usbd_msg_cb);
	if (ret) {
		LOG_ERR("Failed to register message callback (%d)", ret);
		return;
	}

	ret = usbd_init(&timechime_usbd);
	if (ret) {
		LOG_ERR("Failed to initialize USB device (%d)", ret);
		return;
	}

	if (!usbd_can_detect_vbus(&timechime_usbd)) {
		ret = usbd_enable(&timechime_usbd);
		if (ret) {
			LOG_ERR("Failed to enable USB device (%d)", ret);
		}
	}
}
