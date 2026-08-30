#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "usb/cdc_acm_host.h"
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "ring_buffer.h"

#define USB_HOST_PRIORITY (20)
#define RX_TASK_PRIORITY (10)
#define RX_TASK_STACK_SIZE (4096)
#define RX_POLL_MS (20)
#define TX_TIMEOUT_MS (1000)
#define MESSAGE_BUFFER 128
#define CURRENT_VERSION 0.5
#define CALCULATOR_VERSION 0.2

static const char *TAG = "USB-CDC";
static const char *LOGTAG = "LOG";
static const char *ERRORTAG = "Error";
static SemaphoreHandle_t device_disconnected_sem;
static bool device_connected_handled = false;
static RingBuffer usb_rx_buf;
static char line_buf[MESSAGE_BUFFER];
static size_t line_len = 0;

uint8_t peerMACAdresses[][6] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};
size_t peerMACLength = sizeof(peerMACAdresses) / sizeof(peerMACAdresses[0]);
cdc_acm_dev_hdl_t cdc_dev = NULL;

int error(char* message);
int sendESPNOWMessage(char* message);
esp_err_t sendUSBMessage(cdc_acm_dev_hdl_t dev_hdl, const char* message, size_t data_len);
void processUSBRxBuffer(void);

static bool handleUSBRx(const uint8_t *data, size_t rx_size, void *arg) {
    for (size_t i = 0; i < rx_size; i++) {
        if (!rb_push(&usb_rx_buf, data[i])) {
            ESP_LOGW(TAG, "RX ring buffer full, dropping byte!");
        }
    }
    return true;
}

void processUSBRxBuffer(void) {
    uint8_t byte;
    while (rb_pop(&usb_rx_buf, &byte)) {
        if (byte == '\n' || byte == '\r') {
            if (line_len > 0) {
                line_buf[line_len] = '\0';
                ESP_LOGI(LOGTAG, "%s", line_buf);
                sendESPNOWMessage(line_buf);
                line_len = 0;
            }
        } else if (line_len < MESSAGE_BUFFER - 1) {
            line_buf[line_len++] = (char)byte;
        } else {
            ESP_LOGW(TAG, "Line too long, dropping character");
        }
    }
}

static void RxTask(void *arg) {
    while (1) {
        processUSBRxBuffer();
        vTaskDelay(pdMS_TO_TICKS(RX_POLL_MS));
    }
}

static void handleUSBEvent(const cdc_acm_host_dev_event_data_t *event, void *user_ctx) {
    switch (event->type) {
    case CDC_ACM_HOST_ERROR:
        ESP_LOGE(TAG, "CDC-ACM error has occurred, err_no = %i", event->data.error);
        break;
    case CDC_ACM_HOST_DEVICE_DISCONNECTED:
        ESP_LOGI(TAG, "Device suddenly disconnected");
        device_connected_handled = false;
        ESP_ERROR_CHECK(cdc_acm_host_close(event->data.cdc_hdl));
        xSemaphoreGive(device_disconnected_sem);
        break;
    case CDC_ACM_HOST_SERIAL_STATE:
        ESP_LOGI(TAG, "Serial state notif 0x%04X", event->data.serial_state.val);
        break;
    default:
        ESP_LOGW(TAG, "Unsupported CDC event: %d (possibly suspend/resume)", event->type);
        break;
    }
}

static void USBTask(void *arg) {
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            ESP_LOGI(TAG, "USB: All devices freed");
        }
    }
}

static void espnowRecieve(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
    ESP_LOGI(LOGTAG, "Successfully Recieved Data!");
    char safe_buf[MESSAGE_BUFFER];
    size_t copy_len = (data_len < MESSAGE_BUFFER - 1) ? data_len : MESSAGE_BUFFER - 1;
    memcpy(safe_buf, data, copy_len);
    safe_buf[copy_len] = '\0';
    ESP_LOGI(LOGTAG, "%s", safe_buf);
    sendUSBMessage(cdc_dev, safe_buf, copy_len);
}

void espnowSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(LOGTAG, "Successful");
    } else {
        ESP_LOGE(ERRORTAG, "Did not send ESP-NOW Message");
    }
}

void app_main(void) {
    device_disconnected_sem = xSemaphoreCreateBinary();
    assert(device_disconnected_sem);

    rb_init(&usb_rx_buf);

    ESP_LOGI(TAG, "Installing USB Host");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LOWMED,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));

    BaseType_t task_created = xTaskCreate(USBTask, "usb_lib", 4096, NULL, USB_HOST_PRIORITY, NULL);
    assert(task_created == pdTRUE);

    BaseType_t rx_task_created = xTaskCreate(RxTask, "rx_task", RX_TASK_STACK_SIZE, NULL, RX_TASK_PRIORITY, NULL);
    assert(rx_task_created == pdTRUE);

    ESP_LOGI(TAG, "Installing CDC-ACM driver");
    ESP_ERROR_CHECK(cdc_acm_host_install(NULL));

    ESP_LOGI(TAG, "Initializing ESP-NOW");
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    uint8_t channel = 6;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnowRecieve));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnowSent));

    for (int i = 0; i < peerMACLength; i++) {
        const uint8_t *selectedMAC = peerMACAdresses[i];
        esp_now_peer_info_t peer_info;
        memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
        memcpy(peer_info.peer_addr, selectedMAC, 6);
        peer_info.channel = 0;
        peer_info.ifidx = WIFI_IF_STA;
        ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    }

    const cdc_acm_host_device_config_t dev_config = {
        .connection_timeout_ms = 1000,
        .out_buffer_size = 512,
        .in_buffer_size = 512,
        .user_arg = NULL,
        .event_cb = handleUSBEvent,
        .data_cb = handleUSBRx
    };

    ESP_LOGI(TAG, "Opening CDC ACM device...");

    while (true) {
        esp_err_t err = cdc_acm_host_open(CDC_HOST_ANY_VID, CDC_HOST_ANY_PID, 0, &dev_config, &cdc_dev);
        if (ESP_OK != err) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        if (!device_connected_handled) {
            ESP_LOGI(TAG, "A USB device has been connected (first time)");
            device_connected_handled = true;
            //sendUSBMessage(cdc_dev, "//action:Info", sizeof("//action:Info"));
        }
        xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
    }
}

int error(char* message) {
    ESP_LOGE(ERRORTAG, "%s", message);
    return 1;
}

int sendESPNOWMessage(char* message) {
    for (int i = 0; i < peerMACLength; i++) {
        const uint8_t *selectedMAC = peerMACAdresses[i];
        esp_err_t err = esp_now_send(selectedMAC, (const uint8_t*)message, strlen(message));
        if (err != ESP_OK) {
            ESP_LOGE(ERRORTAG, "esp_now_send failed: %s", esp_err_to_name(err));
        }
    }
    return 1;
}

esp_err_t sendUSBMessage(cdc_acm_dev_hdl_t dev_hdl, const char* message, size_t messageLength) {
    esp_err_t err = cdc_acm_host_data_tx_blocking(dev_hdl, (const uint8_t*)message, messageLength, TX_TIMEOUT_MS);
    return err;
}
