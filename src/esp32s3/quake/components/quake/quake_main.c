#include "quake_main.h"
#include "esp_log.h"

static const char TAG[] = "quake";

void esp32_quake_main(int argc, char **argv, const char *pakName, uint32_t pakSize, const void *pakMmap);

void quake_main(const char *pakPath, uint32_t pakSize, const void *pakMmap)
{
    const char *argv[] = { "quake" };
    int argc = sizeof(argv) / sizeof(argv[0]);

    ESP_LOGI(TAG, "starting quake...");
    esp32_quake_main(argc, argv, pakPath, pakSize, pakMmap);
    ESP_LOGI(TAG, "exiting quake :(");
}