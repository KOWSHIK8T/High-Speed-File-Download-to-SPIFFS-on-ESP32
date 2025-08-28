#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"

#define WIFI_SSID "Sai"
#define WIFI_PASSWORD "Saikowshik"
#define DOWNLOAD_URL "https://dl.espressif.com/dl/misc/2MB.bin"

#define TARGET_DOWNLOAD_BYTES (1024 * 1024)

#define STREAM_BUFFER_SIZE (48 * 1024) //32kb pipe b/w tasks
#define HTTP_RECV_BUFFER_SIZE (24 * 1024) //16kb buffer for http client
#define FILE_BUFFER_SIZE (16 * 1024) //8kb buffer for file write
#define CHUNK_WRITE_SIZE (4 * 1024) //write to file in 4kb 

static const char *TAG = "HIGH_SPEED_DOWNLOADER";

//global state
typedef struct{
    StreamBufferHandle_t stream_buffer;
    volatile bool producer_done;
    volatile bool consumer_done;
    volatile size_t total_bytes_written;
}app_context_t;

static app_context_t g_ctx = {0};

static void wifi_event_handler(void* arg,esp_event_base_t event_base, int32_t event_id, void* event_data){
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
    esp_wifi_connect();
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ESP_LOGI(TAG,"Got IP address, Starting download test.");
        //singal that we are connected (can use a semaphore)
    }
}

void wifi_init(void)
{
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {.ssid = WIFI_SSID, .password = WIFI_PASSWORD },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    //disable wifi power save mode for max performance
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

void spiffs_init(void){
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_vfs_spiffs_register(&conf);
}

void producer_task(void *pvParameters){
    ESP_LOGI(TAG, "producer task started on core %d", xPortGetCoreID());

    esp_http_client_config_t config = {
        .url = DOWNLOAD_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 2000,
        .buffer_size = HTTP_RECV_BUFFER_SIZE,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_open(client, 0);
    esp_http_client_fetch_headers(client);
    char *recv_buffer = malloc(HTTP_RECV_BUFFER_SIZE);
    size_t total_downloaded = 0;
    int read_len = 0;

    while(total_downloaded < TARGET_DOWNLOAD_BYTES){
        size_t bytes_to_read = (TARGET_DOWNLOAD_BYTES - total_downloaded < HTTP_RECV_BUFFER_SIZE) ? (TARGET_DOWNLOAD_BYTES - total_downloaded) : HTTP_RECV_BUFFER_SIZE;
        read_len = esp_http_client_read (client, recv_buffer, bytes_to_read);
        if(read_len <= 0){
            break;
        }
        xStreamBufferSend(g_ctx.stream_buffer, recv_buffer, read_len, portMAX_DELAY);
        total_downloaded += read_len;

    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(recv_buffer);

    g_ctx.producer_done = true;
    ESP_LOGI(TAG, "Producer finished, downloaded %d bytes.", total_downloaded);
    vTaskDelete(NULL);
}

void consumer_task(void *pvParameters){
    ESP_LOGI(TAG, "consumer task started on core %d", xPortGetCoreID());

    char* chunk_buffer = malloc(CHUNK_WRITE_SIZE);
    char* file_buffer = malloc(FILE_BUFFER_SIZE);

    FILE* f = fopen("/spiffs/download.bin", "wb");
    if(f == NULL){
        ESP_LOGE(TAG, "FAiled to open file for writing!");
        vTaskDelete(NULL);
        return;
    }    

    setvbuf(f, file_buffer, _IOFBF, FILE_BUFFER_SIZE);

    while(!g_ctx.producer_done || xStreamBufferBytesAvailable(g_ctx.stream_buffer) > 0){
        size_t received_bytes = xStreamBufferReceive(g_ctx.stream_buffer, chunk_buffer, CHUNK_WRITE_SIZE, pdMS_TO_TICKS(100));

        if(received_bytes > 0){
            fwrite(chunk_buffer, 1, received_bytes, f);
                g_ctx.total_bytes_written += received_bytes;
        }
    }
    
    fclose(f);
    free(chunk_buffer);
    free(file_buffer);

    g_ctx.consumer_done = true;
    ESP_LOGI(TAG, "consumer finished . wrote %d bytes.",g_ctx.total_bytes_written);
    vTaskDelete(NULL);
}

void app_main(void){
    //initialize
    wifi_init();
    spiffs_init();
    vTaskDelay(pdMS_TO_TICKS(5000));

    g_ctx.stream_buffer = xStreamBufferCreate(STREAM_BUFFER_SIZE, 1); //pipe
    
    ESP_LOGI(TAG, "starting download of %d bytes to SPIFFS ", TARGET_DOWNLOAD_BYTES);
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);

    xTaskCreatePinnedToCore(producer_task, "Producer", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(consumer_task, "Consumer", 8192, NULL, 5, NULL, 1);

    while(!g_ctx.producer_done || !g_ctx.consumer_done){
        vTaskDelay(pdMS_TO_TICKS(250));
        ESP_LOGI(TAG, "written %d / %d bytes", g_ctx.total_bytes_written, TARGET_DOWNLOAD_BYTES);
    }

    gettimeofday(&tv_end, NULL);

    float duration_s = (tv_end.tv_sec - tv_start.tv_sec) + ((tv_end.tv_usec - tv_start.tv_usec) / 1000000.0);
    float speed_kbps = 0;
    
    if(duration_s > 0){
        speed_kbps = (g_ctx.total_bytes_written/1024.0)/duration_s;
    }

    ESP_LOGI(TAG, "DOWNLOAD COMPLETED :)");
    ESP_LOGI(TAG, "wrote %zu bytes in %.2f seconds", g_ctx.total_bytes_written, duration_s);
    ESP_LOGI(TAG, "Average throughput : %.2f KBps", speed_kbps);

    vStreamBufferDelete(g_ctx.stream_buffer);
}
