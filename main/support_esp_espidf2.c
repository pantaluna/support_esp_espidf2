#include "mjd.h"

/*
 * Logging
 */
static const char TAG[] = "myapp";

/*
 * FreeRTOS settings
 */
#define MYAPP_RTOS_TASK_STACK_SIZE (8192)
#define MYAPP_RTOS_TASK_PRIORITY_NORMAL (RTOS_TASK_PRIORITY_NORMAL)

/*
 * Settings
 */
static const char *MY_UPLOAD_QUEUE_NVS_PARTITION = "nvsmeteohub";
static const char *MY_UPLOAD_QUEUE_NVS_NAMESPACE = "uploadqueue";
static const char *MY_UPLOAD_QUEUE_FMT_NVS_KEY_RECORD = "record%05u";
static const char *MY_UPLOAD_QUEUE_NVS_KEY_LAST_RECORD_ID = "lastrecordid";

/*
 * Structs
 */
typedef struct {
    uint8_t data[860];
} data_record_t;

/*
 * Helpers
 */
static esp_err_t _nvs_flash_init() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval = ESP_OK;

    ESP_LOGI(TAG, "***SECTION: NVS init***");

    f_retval = nvs_flash_init_partition(MY_UPLOAD_QUEUE_NVS_PARTITION);
    if (f_retval == ESP_ERR_NVS_NO_FREE_PAGES || f_retval == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGD(TAG, "  OK. WARNING nvs_flash_init_partition() err %i (%s)", f_retval, esp_err_to_name(f_retval));

        f_retval = nvs_flash_erase_partition(MY_UPLOAD_QUEUE_NVS_PARTITION);
        if (f_retval != ESP_OK) {
            ESP_LOGE(TAG, "  ABORT. nvs_flash_erase_partition(MY_UPLOAD_QUEUE_NVS_PARTITION) err %i (%s)", f_retval, esp_err_to_name(f_retval));
            // GOTO
            goto label_cleanup;
        }

        f_retval = nvs_flash_init_partition(MY_UPLOAD_QUEUE_NVS_PARTITION);
    }
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  ABORT. nvs_flash_init_partition(MY_UPLOAD_QUEUE_NVS_PARTITION) err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto label_cleanup;
    }
    // LABEL
    label_cleanup: ;

    return f_retval;
}

static esp_err_t _log_nvs_stats(const char *param_ptr_partition_name, size_t *param_ptr_free_entries) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval = ESP_OK;

    nvs_stats_t stats;

    f_retval = nvs_get_stats(param_ptr_partition_name, &stats);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  nvs_get_stats() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }
    ESP_LOGI(TAG, "NVS STATS partition %s: used_entries=%zu  free_entries=%zu  total_entries=%zu", param_ptr_partition_name, stats.used_entries,
            stats.free_entries, stats.total_entries);

    *param_ptr_free_entries = stats.free_entries;

    // LABEL
    cleanup: ;

    return f_retval;
}

static void _log_data_record(data_record_t *param_ptr_data) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    ESP_LOGI(TAG, "  [sizeof(struct): %u bytes]", sizeof(*param_ptr_data));
}

static esp_err_t _save_data_record_to_flash(data_record_t *param_ptr_data_record) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval = ESP_OK;

    size_t nvs_free_entries = 0;

    nvs_handle handle;
    uint32_t last_record_id = 0;
    char nvs_key[16] = "";

    ESP_LOGI(TAG, "\n\n***SECTION: Save (1) data record to flash***");

    mjd_log_memory_statistics();

    ESP_LOGD(TAG, "exec nvs_open_from_partition()");
    f_retval = nvs_open_from_partition(MY_UPLOAD_QUEUE_NVS_PARTITION, MY_UPLOAD_QUEUE_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  nvs_open_from_partition() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    f_retval = _log_nvs_stats(MY_UPLOAD_QUEUE_NVS_PARTITION, &nvs_free_entries);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  _log_nvs_stats() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    f_retval = nvs_get_u32(handle, MY_UPLOAD_QUEUE_NVS_KEY_LAST_RECORD_ID, &last_record_id);
    if (f_retval == ESP_OK) {
        ESP_LOGD(TAG, "NVS key=%s val=%i", MY_UPLOAD_QUEUE_NVS_KEY_LAST_RECORD_ID, last_record_id);
    } else if (f_retval == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "  OK WARNING nvs_get_u32() err %i (%s)", f_retval, esp_err_to_name(f_retval));
    } else {
        ESP_LOGE(TAG, "  nvs_get_u32() ERROR err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    last_record_id++;
    sprintf(nvs_key, MY_UPLOAD_QUEUE_FMT_NVS_KEY_RECORD, last_record_id);
    ESP_LOGI(TAG, "%s(): Storing the data record in the NVS nvs_key=%s ...", __FUNCTION__, nvs_key);

    // NVS set blob ***Hangs when 91% of partition namespace space is used***
    f_retval = nvs_set_blob(handle, nvs_key, param_ptr_data_record, sizeof(*param_ptr_data_record));
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  nvs_set_blob() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    f_retval = nvs_set_u32(handle, MY_UPLOAD_QUEUE_NVS_KEY_LAST_RECORD_ID, last_record_id);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  nvs_set_u32() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    ESP_LOGD(TAG, "exec nvs_commit()");
    f_retval = nvs_commit(handle);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  nvs_commit() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    f_retval = _log_nvs_stats(MY_UPLOAD_QUEUE_NVS_PARTITION, &nvs_free_entries);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "  _log_nvs_stats() err %i (%s)", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    // LABEL
    cleanup: ;

    // NVS Close (void)
    nvs_close(handle);

    return f_retval;
}

/*
 * TASK
 */
void main_task(void *pvParameter) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval;

    data_record_t data_record =
        { 0 };

    _log_data_record(&data_record);

    mjd_log_memory_statistics();

    f_retval = _nvs_flash_init();
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "ABORT. _nvs_flash_init() | err %i %s", f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto label_restart;
    }

    /*
     * LOOP hangs @ item setblob key=record00509
     */
    for (uint32_t u = 0; u < 550; u++) {
        _save_data_record_to_flash(&data_record);
    }

    /*
     * LABEL
     */
    label_restart: ;

    vTaskDelete(NULL);
}

/*
 * MAIN
 */
void app_main() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    /* Init */
    nvs_flash_init();
    mjd_log_chip_info();
    mjd_log_memory_statistics();

    /*
     * Main task
     */
    BaseType_t xReturned;
    xReturned = xTaskCreatePinnedToCore(&main_task, "main_task (name)", MYAPP_RTOS_TASK_STACK_SIZE, NULL,
    MYAPP_RTOS_TASK_PRIORITY_NORMAL, NULL,
    APP_CPU_NUM);
    if (xReturned == pdPASS) {
        ESP_LOGI(TAG, "OK Task main_task has been created, and is running right now");
    }

    /*
     * END
     */
    ESP_LOGI(TAG, "END %s()", __FUNCTION__);
}
