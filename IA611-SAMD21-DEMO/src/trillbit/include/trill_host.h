#ifndef _TRILL_HOST_H_
#define _TRILL_HOST_H_

#include <stddef.h>

// Version format: Major.Minor.Fixes
#define TRILL_HOST_SDK_VERSION          "1.0.1"
#define TRILL_IA61x_ALGO_ID             0xE0
#define TRILL_BLOCK_HEADER_LEN          4
#define TRILL_BLOCK_PAYLOAD_LEN_INDEX   TRILL_BLOCK_HEADER_LEN
#define TRILL_BLOCK_PAYLOAD_INDEX       (TRILL_BLOCK_PAYLOAD_LEN_INDEX+1)
/**
 * @brief Trill Host SDK error codes.
 */
enum {
    TRILL_HOST_ERR_CODE_BASE = -1000,
    TRILL_HOST_ERR_INVALID_PARAMETERS,
    TRILL_HOST_ERR_INVALID_SDK_LICENSE,
    TRILL_HOST_ERR_LICENSE_NOT_FOUND,
    TRILL_HOST_ERR_OUT_OF_MEMORY,
    TRILL_HOST_ERR_BUFFER_TOO_SMALL,
    TRILL_HOST_ERR_INVALID_DATA,
    TRILL_HOST_ERR_FAILED_TO_GET_DEV_ID,
    TRILL_HOST_ERR_AUTH_FAILED,
};

#define TRILL_HOST_LICENSE_BUFFER_SIZE      (400)
#define TRILL_HOST_MAX_PAYLOAD_SIZE         (256)

enum {
    TRILL_KW_PAYLOAD_AVAILABLE = 1,
    TRILL_KW_HOST_AUTH_NEEDED,
    TRILL_KW_HOST_AUTH_PASS
};

typedef void* (*trill_mem_alloc_t) (size_t size);
typedef void (*trill_mem_free_t) (void* ptr);

/**
 * @brief Initialization parameters for host init call.
 * User application should set the entire structure contents to zeros before setting 
 * individual members.
 * 
 */
typedef struct {
    
    // Pointer to SDK License string (ASCII) as delivered. Should be NULL terminated.
    // Once the init function returns. User can reuse the buffer supplied.
    const char* sdk_license;
    
    // User application memory allocator. Parameters are similar to libc malloc.
    trill_mem_alloc_t mem_alloc_fn;
    // User application memory allocator. Parameters are similar to libc free.
    trill_mem_free_t mem_free_fn;
    
} trill_host_init_parameters_t;

/**
 * @brief Opaque handle for user application.
 * 
 */
typedef void* trill_host_handle_t;

/**
 * @brief Initialize Trill Host SDK with given parameters.
 * 
 * @param params Mandatory. Refer to trill_host_init_parameters_t
 * @param handle Pass this opaque handle to other trill host SDK calls.
 * @return int Returns 0 if License is correct and init was successful else negative error code.
 */
int trill_host_init(trill_host_init_parameters_t* params, trill_host_handle_t* handle);

/**
 * @brief Handles the Authentication request from IA61x. Pass the data received from RDB command
 * as is to this function.
 * 
 * @param handle Trill host handle as received from init call.
 * @param data Data received from RDB command to IA61x.
 * @param size Size of data buffer in bytes.
 * @return int 0 on success else negative error code.
 */
int trill_host_handle_auth(trill_host_handle_t handle, unsigned char* data, unsigned int size);

/**
 * @brief Returns the Unique Identifier of MCU or Board as seen by the Host SDK.
 * Initializing the Host before calling this function is not require.
 * 
 * @return const char* Returns a NULL terminated Unique Identifier of MCU or Board.
 */
const char* trill_host_get_id(void);

/**
 * @brief Shutdown the IA611 SDK.
 * Free up any allocated resources.
 * User application can Reset or Load different DSP algorithm or Power off
 * the IA611 chip after this call.
 * 
 * @param handle Trill host handle as received from init call.
 * @return int Returns 0 on success else negative error code.
 */
int trill_host_deinit(trill_host_handle_t handle);

#endif //_TRILL_HOST_H_
