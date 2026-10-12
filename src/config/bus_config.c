#include "bus_config.h"

#include "../third_party/cjson/cJSON.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADDRESS_MAX_10BIT 0x3FF
#define ADDRESS_MAX_7BIT 0x7F

static void fail(char *err, size_t err_len, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

static void fail(char *err, size_t err_len, const char *format, ...) {
    if (!err || err_len == 0) {
        return;
    }
    va_list args;
    va_start(args, format);
    vsnprintf(err, err_len, format, args);
    va_end(args);
}

static const char *bus_name(VioBusKind kind) {
    return kind == VIO_BUS_SPI ? "spi" : "i2c";
}

static bool key_allowed(VioBusKind kind, const char *key) {
    static const char *const common[] = {"device", "device_scroll", "config", "model", NULL};
    static const char *const spi[] = {"cs_id", "mode", "max_hz", NULL};
    static const char *const i2c[] = {"address", "speed", "addr_width", NULL};
    const char *const *lists[] = {common, kind == VIO_BUS_SPI ? spi : i2c};
    for (size_t l = 0; l < 2; l++) {
        for (const char *const *k = lists[l]; *k; k++) {
            if (strcmp(*k, key) == 0) {
                return true;
            }
        }
    }
    return false;
}

/* cJSON keeps duplicate keys; the first one wins on lookup, so reject them. */
static const char *duplicate_key(const cJSON *object) {
    for (const cJSON *a = object->child; a; a = a->next) {
        for (const cJSON *b = a->next; b; b = b->next) {
            if (a->string && b->string && strcmp(a->string, b->string) == 0) {
                return a->string;
            }
        }
    }
    return NULL;
}

static bool is_integer(const cJSON *item) {
    return cJSON_IsNumber(item) && isfinite(item->valuedouble) &&
           item->valuedouble == floor(item->valuedouble);
}

/* An I2C address is an integer or a string such as "0x76". */
static bool parse_address(const cJSON *item, long *out) {
    if (is_integer(item)) {
        *out = (long)item->valuedouble;
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring[0] != '\0') {
        char *end = NULL;
        errno = 0;
        long value = strtol(item->valuestring, &end, 0);
        if (errno == 0 && *end == '\0') {
            *out = value;
            return true;
        }
    }
    return false;
}

static bool check_model(const cJSON *item) {
    if (cJSON_IsString(item)) {
        return true;
    }
    if (!cJSON_IsArray(item)) {
        return false;
    }
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, item) {
        if (!cJSON_IsString(entry)) {
            return false;
        }
    }
    return true;
}

static bool parse_slave(const cJSON *entry, int index, VioBusKind kind, VioSlaveConfig *slave,
                        char *err, size_t err_len) {
    if (!cJSON_IsObject(entry)) {
        fail(err, err_len, "slaves[%d]: must be an object", index);
        return false;
    }
    const char *dup = duplicate_key(entry);
    if (dup) {
        fail(err, err_len, "slaves[%d].%s: duplicate key", index, dup);
        return false;
    }
    for (const cJSON *item = entry->child; item; item = item->next) {
        if (!key_allowed(kind, item->string)) {
            fail(err, err_len, "slaves[%d].%s: unknown key for an %s slave", index, item->string,
                 bus_name(kind));
            return false;
        }
    }

    const cJSON *device = cJSON_GetObjectItemCaseSensitive(entry, "device");
    if (!device) {
        fail(err, err_len, "slaves[%d]: missing required key 'device'", index);
        return false;
    }
    if (!cJSON_IsString(device) || device->valuestring[0] == '\0') {
        fail(err, err_len, "slaves[%d].device: must be a non-empty string", index);
        return false;
    }

    const cJSON *scroll = cJSON_GetObjectItemCaseSensitive(entry, "device_scroll");
    if (!scroll) {
        fail(err, err_len, "slaves[%d]: missing required key 'device_scroll'", index);
        return false;
    }
    if (!cJSON_IsString(scroll) || scroll->valuestring[0] == '\0') {
        fail(err, err_len, "slaves[%d].device_scroll: must be a non-empty string", index);
        return false;
    }

    const cJSON *config = cJSON_GetObjectItemCaseSensitive(entry, "config");
    if (config && !cJSON_IsObject(config)) {
        fail(err, err_len, "slaves[%d].config: must be an object", index);
        return false;
    }

    const cJSON *model = cJSON_GetObjectItemCaseSensitive(entry, "model");
    if (model && !check_model(model)) {
        fail(err, err_len, "slaves[%d].model: must be a string or an array of strings", index);
        return false;
    }

    if (kind == VIO_BUS_SPI) {
        const cJSON *cs_id = cJSON_GetObjectItemCaseSensitive(entry, "cs_id");
        if (!cs_id) {
            fail(err, err_len, "slaves[%d]: missing required key 'cs_id'", index);
            return false;
        }
        if (!is_integer(cs_id) || cs_id->valuedouble < 0 || cs_id->valuedouble > 65535) {
            fail(err, err_len, "slaves[%d].cs_id: must be an integer from 0 to 65535", index);
            return false;
        }
        slave->cs_id = (int)cs_id->valuedouble;

        const cJSON *mode = cJSON_GetObjectItemCaseSensitive(entry, "mode");
        if (mode && (!is_integer(mode) || mode->valuedouble < 0 || mode->valuedouble > 3)) {
            fail(err, err_len, "slaves[%d].mode: must be an integer from 0 to 3", index);
            return false;
        }
        const cJSON *max_hz = cJSON_GetObjectItemCaseSensitive(entry, "max_hz");
        if (max_hz && (!cJSON_IsNumber(max_hz) || !(max_hz->valuedouble > 0))) {
            fail(err, err_len, "slaves[%d].max_hz: must be a positive number", index);
            return false;
        }
    } else {
        const cJSON *address = cJSON_GetObjectItemCaseSensitive(entry, "address");
        if (!address) {
            fail(err, err_len, "slaves[%d]: missing required key 'address'", index);
            return false;
        }
        long value = 0;
        if (!parse_address(address, &value) || value < 0 || value > ADDRESS_MAX_10BIT) {
            fail(err, err_len,
                 "slaves[%d].address: must be an integer or a string such as \"0x76\", "
                 "from 0 to 0x3FF",
                 index);
            return false;
        }

        const cJSON *width = cJSON_GetObjectItemCaseSensitive(entry, "addr_width");
        if (width) {
            if (!is_integer(width) || (width->valuedouble != 7 && width->valuedouble != 10)) {
                fail(err, err_len, "slaves[%d].addr_width: must be 7 or 10", index);
                return false;
            }
            if (width->valuedouble == 7 && value > ADDRESS_MAX_7BIT) {
                fail(err, err_len, "slaves[%d].address: 0x%lX does not fit in 7 bits", index,
                     value);
                return false;
            }
        }
        /* 7- and 10-bit addresses are separate address spaces. */
        slave->ten_bit = width ? width->valuedouble == 10 : value > ADDRESS_MAX_7BIT;
        const cJSON *speed = cJSON_GetObjectItemCaseSensitive(entry, "speed");
        if (speed && (!cJSON_IsNumber(speed) || !(speed->valuedouble > 0))) {
            fail(err, err_len, "slaves[%d].speed: must be a positive number", index);
            return false;
        }
        slave->address = (uint16_t)value;
    }

    slave->device = strdup(device->valuestring);
    slave->device_scroll = strdup(scroll->valuestring);
    slave->config_json = config ? cJSON_PrintUnformatted(config) : strdup("{}");
    if (!slave->device || !slave->device_scroll || !slave->config_json) {
        fail(err, err_len, "slaves[%d]: out of memory", index);
        return false;
    }
    return true;
}

static bool check_unique(const VioBusConfig *config, VioBusKind kind, char *err, size_t err_len) {
    for (int i = 0; i < config->count; i++) {
        for (int j = 0; j < i; j++) {
            const VioSlaveConfig *a = &config->slaves[j];
            const VioSlaveConfig *b = &config->slaves[i];
            if (kind == VIO_BUS_SPI && a->cs_id == b->cs_id) {
                fail(err, err_len, "slaves[%d].cs_id: duplicate chip select %d (also used by slaves[%d])",
                     i, b->cs_id, j);
                return false;
            }
            if (kind == VIO_BUS_I2C && a->address == b->address && a->ten_bit == b->ten_bit) {
                fail(err, err_len,
                     "slaves[%d].address: duplicate %d-bit address 0x%X (also used by slaves[%d])", i,
                     b->ten_bit ? 10 : 7, (unsigned)b->address, j);
                return false;
            }
        }
    }
    return true;
}

bool vio_bus_config_parse(const char *config_json, VioBusKind kind, VioBusConfig *out,
                          char *err, size_t err_len) {
    memset(out, 0, sizeof(*out));
    if (!config_json) {
        fail(err, err_len, "config: missing (NULL)");
        return false;
    }

    const char *end = NULL;
    cJSON *root = cJSON_ParseWithOpts(config_json, &end, 1);
    if (!root) {
        fail(err, err_len, "config: invalid JSON at offset %ld", end ? (long)(end - config_json) : 0L);
        return false;
    }

    bool ok = false;
    if (!cJSON_IsObject(root)) {
        fail(err, err_len, "config: must be a JSON object");
        goto done;
    }
    int slaves_keys = 0;
    for (const cJSON *item = root->child; item; item = item->next) {
        slaves_keys += item->string && strcmp(item->string, "slaves") == 0;
    }
    if (slaves_keys > 1) {
        fail(err, err_len, "slaves: duplicate key");
        goto done;
    }

    const cJSON *slaves = cJSON_GetObjectItemCaseSensitive(root, "slaves");
    if (!slaves) {
        ok = true; /* a bus without slaves */
        goto done;
    }
    if (!cJSON_IsArray(slaves)) {
        fail(err, err_len, "slaves: must be an array");
        goto done;
    }

    int count = cJSON_GetArraySize(slaves);
    if (count > 0) {
        out->slaves = calloc((size_t)count, sizeof(VioSlaveConfig));
        if (!out->slaves) {
            fail(err, err_len, "slaves: out of memory");
            goto done;
        }
    }
    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, slaves) {
        int index = out->count++;
        if (!parse_slave(entry, index, kind, &out->slaves[index], err, err_len)) {
            goto done;
        }
    }
    ok = check_unique(out, kind, err, err_len);

done:
    cJSON_Delete(root);
    if (!ok) {
        vio_bus_config_free(out);
    }
    return ok;
}

void vio_bus_config_free(VioBusConfig *config) {
    if (!config) {
        return;
    }
    for (int i = 0; i < config->count; i++) {
        free(config->slaves[i].device);
        free(config->slaves[i].device_scroll);
        free(config->slaves[i].config_json);
    }
    free(config->slaves);
    config->slaves = NULL;
    config->count = 0;
}
