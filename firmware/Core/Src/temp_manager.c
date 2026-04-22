#include "temp_manager.h"

#include "app_config.h"
#include "bsp_ds18b20.h"
#include "debug_log.h"

typedef struct
{
    float samples[APP_TEMP_FILTER_WINDOW];
    unsigned int head;
    unsigned int count;
    unsigned int consecutive_fail;
    bool has_last;
    float last_valid;
} sensor_ctx_t;

static temp_snapshot_t g_temp;
static sensor_ctx_t g_sensor[APP_TEMP_SENSOR_COUNT];
static bool g_prev_degraded = false;
static bool g_prev_fault = false;
static uint8_t g_prev_mask = 0x00U;

static bool temp_in_range(float t)
{
    return (t >= APP_TEMP_VALID_MIN_C) && (t <= APP_TEMP_VALID_MAX_C);
}

static bool step_is_reasonable(sensor_ctx_t *ctx, float t)
{
    float delta;

    if ((!ctx->has_last) || (APP_TEMP_MAX_SINGLE_STEP_C <= 0.0f))
    {
        return true;
    }

    delta = t - ctx->last_valid;
    if (delta < 0.0f)
    {
        delta = -delta;
    }

    return delta <= APP_TEMP_MAX_SINGLE_STEP_C;
}

static float sensor_filtered_value(const sensor_ctx_t *ctx)
{
    float sum = 0.0f;
    unsigned int i;

    if (ctx->count == 0U)
    {
        return 0.0f;
    }

    for (i = 0U; i < ctx->count; ++i)
    {
        sum += ctx->samples[i];
    }

    return sum / (float)ctx->count;
}

static void sensor_push_sample(sensor_ctx_t *ctx, float t)
{
    ctx->samples[ctx->head] = t;
    ctx->head = (ctx->head + 1U) % APP_TEMP_FILTER_WINDOW;

    if (ctx->count < APP_TEMP_FILTER_WINDOW)
    {
        ctx->count++;
    }

    ctx->last_valid = t;
    ctx->has_last = true;
    ctx->consecutive_fail = 0U;
}

void temp_manager_init(void)
{
    unsigned int i;

    bsp_ds18b20_init();

    for (i = 0U; i < APP_TEMP_SENSOR_COUNT; ++i)
    {
        g_sensor[i].head = 0U;
        g_sensor[i].count = 0U;
        g_sensor[i].consecutive_fail = 0U;
        g_sensor[i].has_last = false;
        g_sensor[i].last_valid = 0.0f;
    }

    g_temp.t1 = 25.0f;
    g_temp.t2 = 25.0f;
    g_temp.t3 = 25.0f;
    g_temp.t_ctrl = 25.0f;
    g_temp.valid_mask = 0x00U;
    g_temp.sensor_degraded = false;
    g_temp.sensor_fault = false;

    g_prev_degraded = false;
    g_prev_fault = false;
    g_prev_mask = 0x00U;
}

void temp_manager_update(void)
{
    float reading;
    float filtered[APP_TEMP_SENSOR_COUNT];
    float ctrl_sum = 0.0f;
    unsigned int valid_count = 0U;
    unsigned int i;

    g_temp.valid_mask = 0x00U;

    for (i = 0U; i < APP_TEMP_SENSOR_COUNT; ++i)
    {
        filtered[i] = 0.0f;

        if (bsp_ds18b20_read_c((uint8_t)i, &reading) && temp_in_range(reading) && step_is_reasonable(&g_sensor[i], reading))
        {
            sensor_push_sample(&g_sensor[i], reading);
            filtered[i] = sensor_filtered_value(&g_sensor[i]);
            g_temp.valid_mask |= (uint8_t)(1U << i);
        }
        else
        {
            if (g_sensor[i].consecutive_fail < 255U)
            {
                g_sensor[i].consecutive_fail++;
            }

            if (g_sensor[i].count > 0U)
            {
                filtered[i] = sensor_filtered_value(&g_sensor[i]);
            }
        }
    }

    g_temp.t1 = filtered[0U];
    g_temp.t2 = filtered[1U];
    g_temp.t3 = filtered[2U];

    for (i = 0U; i < APP_TEMP_SENSOR_COUNT; ++i)
    {
        if (g_sensor[i].consecutive_fail < APP_TEMP_MAX_CONSEC_FAIL)
        {
            ctrl_sum += filtered[i];
            valid_count++;
        }
    }

    if (valid_count > 0U)
    {
        g_temp.t_ctrl = ctrl_sum / (float)valid_count;
    }

    g_temp.sensor_degraded = (valid_count == 2U);
    g_temp.sensor_fault = (valid_count <= 1U);

    if (g_temp.valid_mask != g_prev_mask)
    {
        debug_log_info("TEMP", "valid mask 0x%02X -> 0x%02X", (unsigned int)g_prev_mask, (unsigned int)g_temp.valid_mask);
        g_prev_mask = g_temp.valid_mask;
    }

    if (g_temp.sensor_degraded != g_prev_degraded)
    {
        if (g_temp.sensor_degraded)
        {
            debug_log_warn("TEMP", "degraded mode enabled");
        }
        else
        {
            debug_log_info("TEMP", "degraded mode cleared");
        }
        g_prev_degraded = g_temp.sensor_degraded;
    }

    if (g_temp.sensor_fault != g_prev_fault)
    {
        if (g_temp.sensor_fault)
        {
            debug_log_error("TEMP", "sensor fault, valid_count=%u", valid_count);
        }
        else
        {
            debug_log_info("TEMP", "sensor fault cleared");
        }
        g_prev_fault = g_temp.sensor_fault;
    }
}

const temp_snapshot_t *temp_manager_get_snapshot(void)
{
    return &g_temp;
}
