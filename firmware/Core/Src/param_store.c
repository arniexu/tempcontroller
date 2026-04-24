#include "param_store.h"

#include "app_config.h"

#include <stdint.h>
#include <string.h>

#if defined(USE_STDPERIPH_DRIVER)
#include "stm32f10x_flash.h"
#endif

#define PARAM_STORE_MAGIC         (0x50415241UL)
#define PARAM_STORE_LAYOUT_VER    (1UL)

#if defined(USE_STDPERIPH_DRIVER)
#define PARAM_STORE_PAGE_A_ADDR   (0x0800F800UL)
#define PARAM_STORE_PAGE_B_ADDR   (0x0800FC00UL)
#define PARAM_STORE_FLASH_ERASED  (0xFFFFFFFFUL)
#endif

typedef struct
{
    uint32_t magic;
    uint32_t layout_ver;
    uint32_t seq;
    uint32_t payload_len;
    app_params_t payload;
    uint32_t crc32;
} param_nv_record_t;

static app_params_t g_params;

#if !defined(USE_STDPERIPH_DRIVER)
static uint8_t g_nv_page_a[sizeof(param_nv_record_t)];
static uint8_t g_nv_page_b[sizeof(param_nv_record_t)];
static int g_nv_page_a_written = 0;
static int g_nv_page_b_written = 0;
#endif

static uint32_t crc32_update(uint32_t crc, uint8_t b)
{
    unsigned int i;

    crc ^= (uint32_t)b;
    for (i = 0U; i < 8U; ++i)
    {
        if ((crc & 1U) != 0U)
        {
            crc = (crc >> 1U) ^ 0xEDB88320UL;
        }
        else
        {
            crc >>= 1U;
        }
    }
    return crc;
}

static uint32_t crc32_calc(const void *buf, unsigned int len)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t crc = 0xFFFFFFFFUL;
    unsigned int i;

    for (i = 0U; i < len; ++i)
    {
        crc = crc32_update(crc, p[i]);
    }
    return ~crc;
}

static uint32_t param_record_crc(const param_nv_record_t *r)
{
    return crc32_calc(r, (unsigned int)(sizeof(param_nv_record_t) - sizeof(uint32_t)));
}

static int param_record_valid(const param_nv_record_t *r)
{
    if (r == 0)
    {
        return 0;
    }
    if (r->magic != PARAM_STORE_MAGIC)
    {
        return 0;
    }
    if (r->layout_ver != PARAM_STORE_LAYOUT_VER)
    {
        return 0;
    }
    if (r->payload_len != (uint32_t)sizeof(app_params_t))
    {
        return 0;
    }
    return (param_record_crc(r) == r->crc32) ? 1 : 0;
}

#if defined(USE_STDPERIPH_DRIVER)
static void read_record_from_flash(uint32_t addr, param_nv_record_t *out)
{
    const uint8_t *src = (const uint8_t *)addr;
    uint8_t *dst = (uint8_t *)out;
    unsigned int i;

    for (i = 0U; i < (unsigned int)sizeof(param_nv_record_t); ++i)
    {
        dst[i] = src[i];
    }
}

static int program_record_to_flash(uint32_t addr, const param_nv_record_t *rec)
{
    const uint32_t *w = (const uint32_t *)rec;
    unsigned int i;
    unsigned int word_count = (unsigned int)(sizeof(param_nv_record_t) / sizeof(uint32_t));

    FLASH_Unlock();

    if (FLASH_ErasePage(addr) != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return 0;
    }

    for (i = 0U; i < word_count; ++i)
    {
        if (FLASH_ProgramWord(addr + (i * 4U), w[i]) != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return 0;
        }
    }

    FLASH_Lock();
    return 1;
}

static int erase_page_if_written(uint32_t addr)
{
    const uint32_t *p = (const uint32_t *)addr;
    if (*p == PARAM_STORE_FLASH_ERASED)
    {
        return 1;
    }

    FLASH_Unlock();
    if (FLASH_ErasePage(addr) != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return 0;
    }
    FLASH_Lock();
    return 1;
}
#endif

static int load_from_nv(app_params_t *params, uint32_t *seq_out)
{
    param_nv_record_t a;
    param_nv_record_t b;
    const param_nv_record_t *best = 0;

#if defined(USE_STDPERIPH_DRIVER)
    read_record_from_flash(PARAM_STORE_PAGE_A_ADDR, &a);
    read_record_from_flash(PARAM_STORE_PAGE_B_ADDR, &b);
#else
    memset(&a, 0xFF, sizeof(a));
    memset(&b, 0xFF, sizeof(b));
    if (g_nv_page_a_written)
    {
        memcpy(&a, g_nv_page_a, sizeof(a));
    }
    if (g_nv_page_b_written)
    {
        memcpy(&b, g_nv_page_b, sizeof(b));
    }
#endif

    if (param_record_valid(&a))
    {
        best = &a;
    }
    if (param_record_valid(&b))
    {
        if ((best == 0) || (b.seq > best->seq))
        {
            best = &b;
        }
    }

    if (best == 0)
    {
        return 0;
    }

    if (params != 0)
    {
        *params = best->payload;
    }
    if (seq_out != 0)
    {
        *seq_out = best->seq;
    }
    return 1;
}

static void save_to_nv(const app_params_t *params)
{
    param_nv_record_t a;
    param_nv_record_t b;
    param_nv_record_t rec;
    uint32_t next_seq = 1UL;
    int a_valid;
    int b_valid;

#if defined(USE_STDPERIPH_DRIVER)
    read_record_from_flash(PARAM_STORE_PAGE_A_ADDR, &a);
    read_record_from_flash(PARAM_STORE_PAGE_B_ADDR, &b);
#else
    memset(&a, 0xFF, sizeof(a));
    memset(&b, 0xFF, sizeof(b));
    if (g_nv_page_a_written)
    {
        memcpy(&a, g_nv_page_a, sizeof(a));
    }
    if (g_nv_page_b_written)
    {
        memcpy(&b, g_nv_page_b, sizeof(b));
    }
#endif

    a_valid = param_record_valid(&a);
    b_valid = param_record_valid(&b);

    if (a_valid && b_valid)
    {
        next_seq = ((a.seq > b.seq) ? a.seq : b.seq) + 1UL;
    }
    else if (a_valid)
    {
        next_seq = a.seq + 1UL;
    }
    else if (b_valid)
    {
        next_seq = b.seq + 1UL;
    }

    rec.magic = PARAM_STORE_MAGIC;
    rec.layout_ver = PARAM_STORE_LAYOUT_VER;
    rec.seq = next_seq;
    rec.payload_len = (uint32_t)sizeof(app_params_t);
    rec.payload = *params;
    rec.crc32 = param_record_crc(&rec);

#if defined(USE_STDPERIPH_DRIVER)
    if (!a_valid || (b_valid && (b.seq > a.seq)))
    {
        if (program_record_to_flash(PARAM_STORE_PAGE_A_ADDR, &rec))
        {
            (void)erase_page_if_written(PARAM_STORE_PAGE_B_ADDR);
        }
    }
    else
    {
        if (program_record_to_flash(PARAM_STORE_PAGE_B_ADDR, &rec))
        {
            (void)erase_page_if_written(PARAM_STORE_PAGE_A_ADDR);
        }
    }
#else
    if (!a_valid || (b_valid && (b.seq > a.seq)))
    {
        memcpy(g_nv_page_a, &rec, sizeof(rec));
        g_nv_page_a_written = 1;
        g_nv_page_b_written = 0;
    }
    else
    {
        memcpy(g_nv_page_b, &rec, sizeof(rec));
        g_nv_page_b_written = 1;
        g_nv_page_a_written = 0;
    }
#endif
}

void param_store_init(void)
{
    if (!load_from_nv(&g_params, 0))
    {
        param_store_load_defaults(&g_params);
        save_to_nv(&g_params);
    }
}

void param_store_load_defaults(app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    params->set_temp_c = APP_TEMP_DEFAULT_SETPOINT_C;
    params->alarm_threshold_c = APP_TEMP_ALARM_THRESHOLD_C;
    params->kp = 8.0f;
    params->ki = 0.3f;
    params->kd = 15.0f;
    params->schedule_enabled = 0U;
    params->schedule_start_min = 8U * 60U;
    params->schedule_end_min = 10U * 60U;
    params->log_period_s = 1U;
}

void param_store_load(app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    *params = g_params;
}

void param_store_save(const app_params_t *params)
{
    if (params == 0)
    {
        return;
    }

    g_params = *params;
    g_params.schedule_enabled = (g_params.schedule_enabled != 0U) ? 1U : 0U;
    g_params.schedule_start_min %= 1440U;
    g_params.schedule_end_min %= 1440U;
    if (g_params.log_period_s == 0U)
    {
        g_params.log_period_s = 1U;
    }
    save_to_nv(&g_params);
}

const app_params_t *param_store_get(void)
{
    return &g_params;
}

app_params_t *param_store_get_mutable(void)
{
    return &g_params;
}
