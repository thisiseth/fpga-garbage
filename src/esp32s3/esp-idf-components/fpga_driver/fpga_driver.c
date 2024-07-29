#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gptimer.h"

#include "fpga_driver.h"
#include "fpga_api_gpu.h"
#include "fpga_api_io.h"
#include "fpga_qspi.h"

static const char TAG[] = "fpga_driver";

#define FPGA_DRIVER_TASK_PRIORITY  15
#define FPGA_DRIVER_TASK_STACKSIZE 4 * 1024
#define FPGA_DRIVER_TASK_NAME      "fpga_driver_main"

#define FPGA_DRIVER_TASK_PINNED_CORE 0

#define FPGA_DRIVER_TASK_TICK_US 1000

static volatile bool init = false;

static fpga_qspi_t qspi;
static gptimer_handle_t driver_timer = NULL;
static TaskHandle_t driver_task = NULL;

static bool driver_timer_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
static void driver_task_main(void *arg);

bool fpga_driver_init(fpga_driver_config_t *config)
{
    ESP_LOGI(TAG, "fpga driver init starting");

    if (init)
        return false;

    if (!fpga_qspi_init(&qspi, config->pinCsGpu, config->pinCsIo, config->pinSclk, config->pinD0, config->pinD1, config->pinD2, config->pinD3))
        return false;

    if (xTaskCreatePinnedToCore(driver_task_main, FPGA_DRIVER_TASK_NAME, FPGA_DRIVER_TASK_STACKSIZE, NULL, FPGA_DRIVER_TASK_PRIORITY, &driver_task, FPGA_DRIVER_TASK_PINNED_CORE) != pdPASS)
        return false;

    gptimer_config_t timer_config = 
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, //tick = 1us
        .flags.intr_shared = 0
    };

    if (gptimer_new_timer(&timer_config, &driver_timer) != ESP_OK)
        return false;

    gptimer_alarm_config_t alarm_config = 
    {
        .reload_count = 0, // counter will reload with 0 on alarm event
        .alarm_count = FPGA_DRIVER_TASK_TICK_US,
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };

    if (gptimer_set_alarm_action(driver_timer, &alarm_config) != ESP_OK)
        return false;

    gptimer_event_callbacks_t callbacks =  { .on_alarm = driver_timer_tick };

    if (gptimer_register_event_callbacks(driver_timer, &callbacks, NULL) != ESP_OK)
        return false;

    if (gptimer_enable(driver_timer) != ESP_OK)
        return false;

    if (gptimer_start(driver_timer) != ESP_OK)
        return false;

    ESP_LOGI(TAG, "fpga driver init successful");
    return init = true;
}

static IRAM_ATTR bool driver_timer_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    vTaskNotifyGiveFromISR(driver_task, &xHigherPriorityTaskWoken);

    return xHigherPriorityTaskWoken == pdTRUE;
}

#include "driver/gpio.h"

static IRAM_ATTR void driver_task_main(void *arg)
{
    ESP_LOGI(TAG, "fpga driver task started");

    /////////////
    gpio_set_direction(35, GPIO_MODE_OUTPUT);
    gpio_set_direction(37, GPIO_MODE_OUTPUT);

    int pink = 0;
    /////////////

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /////////////////
        gpio_set_level(35, pink = !pink);
        /////////////////
    }

    vTaskDelete(NULL);
}