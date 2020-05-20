

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "am_mcu_apollo.h"

/**
 * @brief Storage information.
 *
 * The alignment requirements are specified in number of bytes.
 */
typedef struct {
    /** Required address and size alignment for read operations. */
    uint32_t read_align;
    /** Required address and size alignment for write operations. */
    uint32_t write_align;
    /** Required address and size alignment for erase operations. */
    uint32_t erase_align;
    /** True if flash can be read using memcpy. */
    bool is_memory_mapped;
}am_storage_info_t;

/**
 * @brief Storage information.
 */
static const am_storage_info_t storage_info =
{
    .read_align = 1,
    .write_align = sizeof(uint32_t),
    .erase_align = AM_HAL_FLASH_PAGE_SIZE,
    .is_memory_mapped = true
};

static bool is_aligned(uint32_t addr, size_t size, size_t align)
{
    return ((addr % align == 0) && (size % align == 0));
}

/**
 * @brief Reads data from persistent storage.
 *
 * @param[in] address The address to start reading from.
 * @param[in] size The number of bytes to read.
 * @param[in,out] data The returned data. This memory must be allocated by
 * the caller.
 * @return :: 0 for success ; -1 for failed
 */
int8_t storage_read(uint32_t address, size_t size, void *data)
{
    if (!is_aligned(address, size, storage_info.read_align)) {
        return -1;
    }

    memcpy(data, (const void *)address, size);

    return 0;
}
/**
 * @brief Writes data to persistent storage.
 *
 * @param[in] address The address to start writing to.
 * @param[in] data The data to be written.
 * @param[in] size The number of bytes to write.
 * @return :: 0 for success ; -1 for failed
 */
int8_t storage_write(uint32_t address, const void *data, size_t size)
{
    uint32_t i32ReturnCode;
    uint32_t ui32Critical;
    int8_t res = 0;

    if (!is_aligned(address, size, storage_info.write_align)) {
        return -1;
    }

    if ((uint32_t)data%4 != 0) {
        return -1;
    }

    ui32Critical = am_hal_interrupt_master_disable();
    i32ReturnCode = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,
                                          (uint32_t *)data,
                                          (uint32_t *)address,
                                          size/4);
    am_hal_interrupt_master_set(ui32Critical);
    if (i32ReturnCode)
        res = -1;

    return res;
}

/**
 * @brief Erases data from persistent storage.
 *
 * @param[in] address The address to start erasing from.
 * @param[in] size The number of bytes to erase.
 * @return :: 0 for success ; -1 for failed
 */
int8_t storage_erase(uint32_t address, size_t size)
{
    uint32_t i32ReturnCode;
    uint32_t ui32Critical;
    uint8_t i;
    int8_t res = 0;

    if (!is_aligned(address, size, storage_info.erase_align)) {
        return -1;
    }

    for (i = 0; i < (size/storage_info.erase_align); i++) {
        ui32Critical = am_hal_interrupt_master_disable();
        i32ReturnCode = am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                                                AM_HAL_FLASH_ADDR2INST((address+(i*storage_info.erase_align))),
                                                AM_HAL_FLASH_ADDR2PAGE((address+(i*storage_info.erase_align))) );
        am_hal_interrupt_master_set(ui32Critical);
        if (i32ReturnCode) {
            res = -1;
            break;
        }
    }

    return res;
}


