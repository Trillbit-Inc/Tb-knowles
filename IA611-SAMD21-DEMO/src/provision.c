#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "trill_host.h"
#include "provision.h"

#define CMD_PREFIX      "tbc " // Trailing space is intentional. Used for comparison.
#define RESP_PREFIX     "tbr"

/**
 * @brief Serial command description
 * 
 * incoming format: tbc <cmd> [<args>]\n
 * min length 6
 * 
 */

#define MINIMUM_CMD_LINE_LENGTH 6

typedef enum
{
    PROV_CMD_GET_VERSION=1,
    PROV_CMD_GET_MCU_ID,
    PROV_CMD_LIC_SHORT=6,
} prov_cmd_t;

typedef enum 
{
    // Responses codes
    PROV_RESP_OK = 0,
    PROV_RESP_UNKNOWN_CMD = -100,
    PROV_RESP_LIC_TOO_LARGE,
    PROV_RESP_LIC_CSUM_ERROR,
    PROV_RESP_INVALID_ARGS,
} prov_resp_t;

static int process_line(prov_parameters_t* params,
            char* lic_buf,
            unsigned int lic_buf_size);
static int get_line(prov_parameters_t* params);
static int resp_version(prov_parameters_t* params);
static int resp_mcu_id(prov_parameters_t* params);
static int resp_error(prov_parameters_t* params, int resp_code);
static int resp_lic_short(prov_parameters_t* params, const char* args,
            char* lic_buf);

static int calc_check_sum_buf(const char* buf, int len)
{
    // 8-bit sum.
    unsigned char sum = 0;
    
    for (int i = 0; i < len; i++)
    {
        sum += buf[i];
    }

    return (int) sum;
}

static int send_resp(prov_parameters_t* params, int len)
{
    return params->app_uart_tx_fn(
            params->uart_drv_obj,
            params->uart_buffer,
            len);
}

int prov_init(prov_parameters_t* params)
{
    if (!params)
    {
        return PROV_ERR_INVALID_PARAMETERS;
    }

    if ((!params->app_uart_rx_fn) ||
        (!params->app_uart_tx_fn))
    {
        return PROV_ERR_INVALID_PARAMETERS;
    }

    memset(params->uart_buffer, 0, PROV_UART_BUFFER_SIZE);

    return 0;
}

int prov_run(prov_parameters_t* params, char* lic_buf, unsigned int* lic_buf_size)
{
    int ret;

    if ((!params) ||
        (!lic_buf) ||
        (!lic_buf_size))
    {
        return PROV_ERR_INVALID_PARAMETERS;
    }

    if (*lic_buf_size < TRILL_HOST_LICENSE_BUFFER_SIZE)
    {
        return PROV_ERR_BUFFER_TOO_SMALL;
    }

    // enter command processing loop.
    while (1)
    {
        ret = get_line(params);
        if (ret < 0)
        {
            return ret;
        }
        
        ret = process_line(params,
                lic_buf,
                *lic_buf_size);
        if (ret < 0)
        {
            return ret;
        }

        if (ret > 0)
        {
            *lic_buf_size = ret;
            return 0;
        }
    }
}

static int get_line(prov_parameters_t* params)
{
    int ret;
    int i = 0;

    do 
    {
        ret = params->app_uart_rx_fn(
            params->uart_drv_obj,
            &params->uart_buffer[i],
            1);

        if (ret < 0)
        {
            return ret;
        }

        i++;

        if (i >= PROV_UART_BUFFER_SIZE)
        {
            return PROV_ERR_BUFFER_TOO_SMALL;
        }

    } while (params->uart_buffer[i-1] != '\n');

    params->uart_buffer[i] = 0;
    
    return i;
}

static int process_line(prov_parameters_t* params,
            char* lic_buf,
            unsigned int lic_buf_size)
{
    int cmd;
    int ret;
    char* saveptr = NULL;

    if (strlen(params->uart_buffer) < MINIMUM_CMD_LINE_LENGTH)
    {
        return 0; // unknown command. ignore.
    }

    ret = strlen(CMD_PREFIX);
    
    if (strncmp(params->uart_buffer, CMD_PREFIX, ret) != 0)
    {
        return 0; // unknown command. ignore.
    }

    // prefix: can't be null, already compared.
    const char* token = strtok_r(params->uart_buffer, " ", &saveptr);
    // cmd
    token = strtok_r(NULL, " ", &saveptr);
    if (!token)
    {
        return 0; //unknown command. ignore.
    }

    cmd = atoi(token);

    // further args or NULL.
    token = token + strlen(token) + 1;
    
    switch (cmd)
    {
    case PROV_CMD_GET_VERSION:
        ret = resp_version(params);
        break;
    case PROV_CMD_GET_MCU_ID:
        ret = resp_mcu_id(params);
        break;
    case PROV_CMD_LIC_SHORT:
        ret = resp_lic_short(params, token,  lic_buf);
        break;
    default:
        ret = resp_error(params, PROV_RESP_UNKNOWN_CMD);
        break;
    }

    return ret;
}

static int resp_version(prov_parameters_t* params)
{
    int ret;
    
    ret = snprintf(params->uart_buffer, PROV_UART_BUFFER_SIZE, 
            "%s %d %s\n", 
            RESP_PREFIX, 
            PROV_RESP_OK, 
            TRILL_HOST_SDK_VERSION);

    if (ret >= PROV_UART_BUFFER_SIZE)
    {
        return PROV_ERR_BUFFER_TOO_SMALL;
    }

    return send_resp(params, ret);
}

static int resp_error(prov_parameters_t* params, int resp_code)
{
    int ret;
    ret = snprintf(params->uart_buffer, PROV_UART_BUFFER_SIZE, 
            "%s %d\n", 
            RESP_PREFIX, 
            resp_code);

    if (ret >= PROV_UART_BUFFER_SIZE)
    {
        return PROV_ERR_BUFFER_TOO_SMALL;
    }

    return send_resp(params, ret);
}

static int resp_mcu_id(prov_parameters_t* params)
{
    int ret;
    
    const char* dev_id = trill_host_get_id();
    if (!dev_id)
    {
        return PROV_ERR_FAILED_TO_GET_DEV_ID;
    }

    int csum = calc_check_sum_buf(dev_id, strlen(dev_id));

    ret = snprintf(params->uart_buffer, PROV_UART_BUFFER_SIZE, 
            "%s %d %s %d\n", 
            RESP_PREFIX, 
            PROV_RESP_OK, 
            dev_id,
            csum);

    if (ret >= PROV_UART_BUFFER_SIZE)
    {
        return PROV_ERR_BUFFER_TOO_SMALL;
    }

    return send_resp(params, ret);
}

static int resp_lic_short(prov_parameters_t* params, const char* args,
            char* lic_buf)
{
    int ret;
    int rx_csum;
    
    if (!args)
    {
        return resp_error(params, PROV_RESP_INVALID_ARGS);
    }

    ret = sscanf(args, "%s %d", (char*)lic_buf, &rx_csum);
    if (ret == EOF)
    {
        return resp_error(params, PROV_RESP_INVALID_ARGS);
    }
    else if (ret != 2)
    {
        return resp_error(params, PROV_RESP_INVALID_ARGS);
    }

    int lic_len = strlen(lic_buf);
    int csum = calc_check_sum_buf(lic_buf, lic_len);
    int failed = PROV_RESP_LIC_CSUM_ERROR;

    if (csum == rx_csum)
    {
        failed = 0;
    }

    lic_len++; //include NULL terminating character.
    
    ret = snprintf(params->uart_buffer, PROV_UART_BUFFER_SIZE, 
            "%s %d\n", 
            RESP_PREFIX, 
            failed);

    if (ret >= PROV_UART_BUFFER_SIZE)
    {
        return PROV_ERR_BUFFER_TOO_SMALL;
    }

    send_resp(params, ret);
    
    if (failed)
    {
        return 0;
    }

    return lic_len;
}
