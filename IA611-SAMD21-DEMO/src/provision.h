#ifndef _PROVISION_H_
#define _PROVISION_H_

/**
 * @brief Provision module error codes.
 */
enum {
    PROV_ERR_CODE_BASE = -2000,
    PROV_ERR_INVALID_PARAMETERS,
    PROV_ERR_BUFFER_TOO_SMALL,
    PROV_ERR_INVALID_DATA,
    PROV_ERR_FAILED_TO_GET_DEV_ID
};

/**
 * @brief Internal buffer used for UART data exchange.
 * 
 */
#define PROV_UART_BUFFER_SIZE	512

/**
 * @brief Peripheral access functions.
 * Provision module will access peripherals through these user application functions.
 * 
 * @param xxx_drv_obj Peripheral driver object.
 * @param data Data buffer to hold read/write contents.
 * @param size Size of the data buffer to read/write.
 * 
 * @return int User application Peripheral access functions should return negative error codes for any errors 
 * else 0 on read/write success.
 */
typedef int (*app_uart_tx_t)(void* uart_drv_obj, const char* data, unsigned int size);
typedef int (*app_uart_rx_t)(void* uart_drv_obj, char* data, unsigned int size);


/**
 * @brief Initialization parameters for Host provision module.
 * User application should set the entire structure contents to zeros before setting 
 * individual members.
 * 
 */
typedef struct {
    // Provision module will pass this driver objects to peripheral access functions.
    void* uart_drv_obj;
    
    // Blocking uart tx function.
    app_uart_tx_t app_uart_tx_fn;
    
    // Blocking uart rx function.
    app_uart_rx_t app_uart_rx_fn;
	
	// Internal members - Begin here.
    char uart_buffer[PROV_UART_BUFFER_SIZE];
} prov_parameters_t;


/**
 * @brief Initialize provision parameters to prepare for UART communication.
 * 
 * @param params 
 * @return int 0 on success else negtive error code.
 */
int prov_init(prov_parameters_t* params);

/**
 * @brief Communicate with program/script running on comptuer over UART.
 * This function may take long time to return. Can be called in a separate task.
 * On success, user application should:
 * 1. Store the license in non-volatile storage.
 * 2. If trill host sdk init was already called with failed return, then:
 *      a. Call trill_host_deinit.
 *      b. Call trill_host_init with new license.
 * 
 * License provisioning process can be repeated any number of times.
 * 
 * @param params Provision parameters from init call.
 * @param data On success, received license is stored here.
 * @param size Size of the license data buffer. Size should be atleast TRILL_HOST_LICENSE_BUFFER_SIZE bytes.
 *          On success, updated to actual license data size received.
 * @return int 0 on success, else negative error code.
 */
int prov_run(prov_parameters_t* params, char* lic_buf, unsigned int* lic_buf_size);

#endif //_PROVISION_H_
