/* Copyright © 2023 Georgy E. All rights reserved. */

#include "w25qxx.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "main.h"
#include "utils.h"
#include "hal_defs.h"


typedef enum _flash_w25_command_t {
    FLASH_W25_CMD_WRITE_SR1       = ((uint8_t)0x01),
    FLASH_W25_CMD_PAGE_PROGRAMM   = ((uint8_t)0x02),
    FLASH_W25_CMD_READ            = ((uint8_t)0x03),
    FLASH_W25_CMD_WRITE_DISABLE   = ((uint8_t)0x04),
    FLASH_W25_CMD_READ_SR1        = ((uint8_t)0x05),
    FLASH_W25_CMD_WRITE_ENABLE    = ((uint8_t)0x06),
    FLASH_W25_CMD_ERASE_SECTOR    = ((uint8_t)0x20),
    FLASH_W25_CMD_WRITE_ENABLE_SR = ((uint8_t)0x50),
    FLASH_W25_CMD_ENABLE_RESET    = ((uint8_t)0x66),
    FLASH_W25_CMD_RESET           = ((uint8_t)0x99),
    FLASH_W25_CMD_JEDEC_ID        = ((uint8_t)0x9f)
} flash_w25_command_t;


typedef struct _flash_w25qxx_info_t {
    bool     initialized;
    bool     is_24bit_address;

    uint32_t page_size;
    uint32_t pages_count;

    uint32_t sector_size;
    uint32_t sectors_in_block;

    uint32_t block_size;
    uint32_t blocks_count;
} flash_w25qxx_info_t;


#define FLASH_W25_JEDEC_ID_SIZE       (sizeof(uint32_t))
#define FLASH_W25_SR1_BUSY            ((uint8_t)0b00000001)
#define FLASH_W25_SR1_WEL             ((uint8_t)0b00000010)
#define FLASH_W25_24BIT_ADDR_SIZE     ((uint16_t)512)
#define FLASH_W25_SR1_UNBLOCK_VALUE   ((uint8_t)0x00)
#define FLASH_W25_SR1_BLOCK_VALUE     ((uint8_t)0x0F)

#define FLASH_SPI_TIMEOUT_MS          ((uint32_t)10000)
#define FLASH_SPI_COMMAND_SIZE_MAX    ((uint8_t)10)



flash_status_t _flash_read_jdec_id(uint32_t* jdec_id);
flash_status_t _flash_read_SR1(uint8_t* SR1);
flash_status_t _flash_write_enable();
flash_status_t _flash_write_disable();
flash_status_t _flash_write(uint32_t addr, uint8_t* data, uint32_t len);
flash_status_t _flash_erase_sector(uint32_t addr);
flash_status_t _flash_set_protect_block(uint8_t value);

flash_status_t _flash_data_cmp(uint32_t addr, uint8_t* data, uint32_t len, bool* cmp_res);
flash_status_t _flash_erase_data(uint32_t addr, uint32_t len);

flash_status_t _flash_send_data(uint8_t* data, uint16_t len);
flash_status_t _flash_recieve_data(uint8_t* data, uint16_t len);
void           _flash_spi_cs_set();
void           _flash_spi_cs_reset();

bool           _flash_check_FREE();
bool           _flash_check_WEL();

uint32_t       _flash_get_storage_bytes_size();


#ifdef DEBUG
const char* FLASH_TAG = "FLS";
#endif


#define FLASH_W25_JDEC_ID_BLOCK_COUNT_MASK ((uint16_t)0x4011)
const uint16_t w25qxx_jdec_id_block_count[] = {
    2,   // w25q10
    4,   // w25q20
    8,   // w25q40
    16,  // w25q80
    32,  // w25q16
    64,  // w25q32
    128, // w25q64
    256, // w25q128
    512, // w25q256
    1024 // w25q512
};


flash_w25qxx_info_t flash_w25qxx_info = {
    .initialized      = false,
    .is_24bit_address = false,

    .page_size        = FLASH_W25_PAGE_SIZE,
    .pages_count      = FLASH_W25_SECTOR_SIZE / FLASH_W25_PAGE_SIZE,

    .sector_size      = FLASH_W25_SECTOR_SIZE,
    .sectors_in_block = FLASH_W25_SETORS_IN_BLOCK,

    .block_size       = FLASH_W25_SETORS_IN_BLOCK * FLASH_W25_SECTOR_SIZE,
    .blocks_count     = 0
};


flash_status_t flash_w25qxx_init()
{
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash init: begin");
#endif

    _flash_spi_cs_reset();

    uint32_t jdec_id = 0;
    flash_status_t status = _flash_read_jdec_id(&jdec_id);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash init: error=%u (read JDEC ID)", status);
#endif
        return status;
    }

    flash_w25qxx_info.blocks_count = 0;
    uint16_t jdec_id_2b = (uint16_t)jdec_id;
    for (uint16_t i = 0; i < __arr_len(w25qxx_jdec_id_block_count); i++) {
        if ((uint16_t)(FLASH_W25_JDEC_ID_BLOCK_COUNT_MASK + i) == jdec_id_2b) {
            flash_w25qxx_info.blocks_count = w25qxx_jdec_id_block_count[i];
            break;
        }
    }

    if (!flash_w25qxx_info.blocks_count) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash init: error - unknown JDEC ID");
#endif
        return FLASH_ERROR;
    }


#if FLASH_BEDUG
    printTagLog(FLASH_TAG, "flash JDEC ID found: id=%08X, blocks_count=%lu", (unsigned int)jdec_id, flash_w25qxx_info.blocks_count);
#endif

    status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash init: error=%u (block FLASH error)", status);
#endif
        return status;
    }

    flash_w25qxx_info.initialized      = true;
    flash_w25qxx_info.is_24bit_address = (flash_w25qxx_info.blocks_count >= FLASH_W25_24BIT_ADDR_SIZE) ? true : false;

#if FLASH_BEDUG
    printTagLog(FLASH_TAG, "flash init: OK");
#endif

    return FLASH_OK;
}

flash_status_t flash_w25qxx_reset()
{
#if FLASH_BEDUG
    printTagLog(FLASH_TAG, "flash reset: begin");
#endif

    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error=%u (unset block protect)", status);
#endif
        status = FLASH_BUSY;
        goto do_block_protect;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_ENABLE_RESET, FLASH_W25_CMD_RESET };

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error=%u (send command)", status);
#endif
        status = FLASH_BUSY;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error (flash is busy)");
#endif
        return FLASH_BUSY;
    }

    _flash_spi_cs_reset();


    flash_status_t tmp_status = FLASH_OK;
do_block_protect:
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: error=%u (set block protected)", status);
#endif
        status = FLASH_BUSY;
    } else {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash reset: OK");
#endif
    }

    return status;
}

flash_status_t flash_w25qxx_read(uint32_t addr, uint8_t* data, uint32_t len)
{
#if FLASH_BEDUG
//	printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu: begin", addr, len);
#endif

    if (!flash_w25qxx_info.initialized) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu (flash was not initialized)", addr, len);
#endif
    	return FLASH_ERROR;
    }

    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu: error (unacceptable address)", addr, len);
#endif
        return FLASH_OOM;
    }

    _flash_spi_cs_set();


    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_READ;
    if (flash_w25qxx_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    flash_status_t status = FLASH_OK;
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu: error (FLASH busy)", addr, len);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu: error=%u (send command)", addr, len, status);
#endif
        goto do_spi_stop;
    }

    if (data && len) {
    	status = _flash_recieve_data(data, len);
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu: error=%u (recieve data)", addr, len, status);
#endif
        goto do_spi_stop;
    }

do_spi_stop:
    _flash_spi_cs_reset();

#if FLASH_BEDUG
//    if (status == FLASH_OK) {
//		printTagLog(FLASH_TAG, "flash read addr=%lu len=%lu: OK", addr, len);
//    }
//    if (status == FLASH_OK && len) {
//		util_debug_hex_dump(data, addr, len);
//    }
#endif

    return status;
}

flash_status_t flash_w25qxx_write(uint32_t addr, uint8_t* data, uint32_t len)
{
	/* Check input data BEGIN */
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu: begin", addr, len);
//	util_debug_hex_dump(data, addr, len);
#endif

    if (!flash_w25qxx_info.initialized) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu (flash was not initialized)", addr, len);
#endif
        return FLASH_ERROR;
    }

    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error (unacceptable address)", addr, len);
#endif
        return FLASH_OOM;
    }
	/* Check input data END */


    /* Compare old flashed data BEGIN */
    bool compare_status = false;
    flash_status_t status = _flash_data_cmp(addr, data, len, &compare_status);
	if (status != FLASH_OK) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (compare data)", addr, len, status);
#endif
		return status;
	}

	if (!compare_status) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu: ABORT (already written)", addr, len);
#endif
        return FLASH_OK;
	}
    /* Compare old flashed data END */


	/* Erase data BEGIN */
	status = _flash_erase_data(addr, len);
	if (status != FLASH_OK) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (erase old data)", addr, len, status);
#endif
		return status;
	}
	/* Erase data END */


    /* Write data BEGIN */
    uint32_t cur_len = 0;
    while (cur_len < len) {
    	uint32_t write_len = FLASH_W25_PAGE_SIZE;
    	if (cur_len + write_len > len) {
    		write_len = len - cur_len;
    	}
    	status = _flash_write(addr + cur_len, data + cur_len, write_len);
    	if (status != FLASH_OK) {
#if FLASH_BEDUG
        	printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (write)", addr + cur_len, write_len, status);
#endif
			return status;
    	}

    	uint8_t page_buf[FLASH_W25_PAGE_SIZE] = {};
		status = flash_w25qxx_read(addr + cur_len, page_buf, write_len);
    	if (status != FLASH_OK) {
#if FLASH_BEDUG
        	printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (read written page after write)", addr + cur_len, write_len, status);
#endif
			return status;
    	}

		if (memcmp(page_buf, data + cur_len, write_len)) {
#if FLASH_BEDUG
        	printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error (compare written page with read)", addr + cur_len, write_len);
			printTagLog(FLASH_TAG, "Needed page:");
			util_debug_hex_dump(data + cur_len, addr + cur_len, write_len);
			printTagLog(FLASH_TAG, "Readed page:");
			util_debug_hex_dump(page_buf, addr + cur_len, write_len);
#endif
			return FLASH_ERROR;
    	}

    	cur_len += write_len;
    }
    /* Write data END */

#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu: OK", addr, len);
#endif

    return status;
}

flash_status_t _flash_write(uint32_t addr, uint8_t* data, uint32_t len)
{
	if (len > flash_w25qxx_info.page_size) {
#if FLASH_BEDUG
		printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error (unacceptable data length)", addr, len);
#endif
		return FLASH_ERROR;
	}

    if (addr + len > _flash_get_storage_bytes_size()) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error (unacceptable address)", addr, len);
#endif
        return FLASH_OOM;
    }

    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (unset block protect)", addr, len, status);
#endif
        goto do_block_protect;
    }
    status = _flash_write_enable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (write enable)", addr, len, status);
#endif
        goto do_block_protect;
    }
    if (!util_wait_event(_flash_check_WEL, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (WEL bit wait time exceeded)", addr, len, status);
#endif
		status = FLASH_BUSY;
		goto do_block_protect;
    }

    uint8_t counter = 0;
    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };

    spi_cmd[counter++] = FLASH_W25_CMD_PAGE_PROGRAMM;
    if (flash_w25qxx_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error (FLASH busy)", addr, len);
#endif
        return FLASH_ERROR;
    }

    _flash_spi_cs_set();

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (send command)", addr, len, (unsigned int)status);
#endif
        _flash_spi_cs_reset();
        return FLASH_ERROR;
    }

    status = _flash_send_data(data, len);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
    	printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (wait write data timeout)", addr, len, (unsigned int)status);
#endif
        _flash_spi_cs_reset();
        return FLASH_ERROR;
    }

do_block_protect:

    _flash_spi_cs_reset();

    status = _flash_write_disable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (write is not disabled)", addr, len, status);
#endif
    }

    status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "flash write addr=%lu len=%lu error=%u (set block protected)", addr, len, status);
#endif
        return status;
    }

    return FLASH_OK;
}

uint32_t flash_w25qxx_get_pages_count()
{
#if FLASH_TEST
	return FLASH_TEST_PAGES_COUNT;
#endif
    flash_status_t status = FLASH_OK;
    if (!flash_w25qxx_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get pages count: initializing error");
#endif
        return 0;
    }
    return flash_w25qxx_info.pages_count * flash_w25qxx_info.sectors_in_block * flash_w25qxx_info.blocks_count;
}

uint32_t flash_w25qxx_get_blocks_count()
{
#if FLASH_TEST
	return 1;
#endif
    flash_status_t status = FLASH_OK;
    if (!flash_w25qxx_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get blocks count: initializing error");
#endif
        return 0;
    }
    return flash_w25qxx_info.blocks_count;
}

uint32_t flash_w25qxx_get_block_size()
{
    flash_status_t status = FLASH_OK;
    if (!flash_w25qxx_info.initialized) {
        status = flash_w25qxx_init();
    }
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get block size: initializing error");
#endif
        return 0;
    }
    return flash_w25qxx_info.block_size;
}

flash_status_t _flash_data_cmp(uint32_t addr, uint8_t* data, uint32_t len, bool* cmp_res)
{
	*cmp_res = false;

	uint32_t cur_len = 0;
	while (cur_len < len) {
		uint32_t needed_len = FLASH_W25_PAGE_SIZE;
		if (cur_len + needed_len > len) {
			needed_len = len - cur_len;
		}

		uint8_t read_data[FLASH_W25_PAGE_SIZE] = {};
		flash_status_t status = flash_w25qxx_read(addr + cur_len, read_data, needed_len);
		if (status != FLASH_OK) {
#if FLASH_DEBUG
	        printTagLog(FLASH_TAG, "flash compare addr=%lu len=%lu error=%u (read)", addr + cur_len, needed_len, status);
#endif
	        return status;
		}

		if (memcmp(read_data, data + cur_len, needed_len)) {
			*cmp_res = true;
			break;
		}

		cur_len += needed_len;
	}

	return FLASH_OK;
}

flash_status_t _flash_erase_data(uint32_t addr, uint32_t len)
{
	uint32_t end_sector_idx = ((addr + len) / FLASH_W25_SECTOR_SIZE) + 1;

	for (uint32_t cur_sector_idx = addr / FLASH_W25_SECTOR_SIZE; cur_sector_idx < end_sector_idx; cur_sector_idx++) {
		uint32_t sector_addr = cur_sector_idx * FLASH_W25_SECTOR_SIZE;
		uint8_t  sector_buf[FLASH_W25_SECTOR_SIZE] = {};

		/* Read target sector BEGIN */
		flash_status_t status = flash_w25qxx_read(sector_addr, sector_buf, sizeof(sector_buf));
		if (status != FLASH_OK) {
#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash erase data addr=%lu len=%lu error (unable to read sector: block_addr=%lu sector_addr=%lu len=%lu)",
				addr,
				len,
				sector_addr / flash_w25qxx_info.block_size,
				(sector_addr % flash_w25qxx_info.block_size) / flash_w25qxx_info.sector_size,
				FLASH_W25_SECTOR_SIZE
			);
#endif
			return status;
		}
		/* Read target sector END */


		/* Check target sector need erase BEGIN */
		uint32_t erase_len = len;
		uint32_t addr_in_sector = (addr % FLASH_W25_SECTOR_SIZE);
		if (addr_in_sector + erase_len > sector_addr + FLASH_W25_SECTOR_SIZE) {
			erase_len = FLASH_W25_SECTOR_SIZE - addr_in_sector;
		}

		bool need_erase_sector = false;
		for (uint32_t i = addr_in_sector; i < FLASH_W25_SECTOR_SIZE; i++) {
			if (sector_buf[i] != 0xFF) {
				need_erase_sector = true;
				break;
			}
		}
		if (!need_erase_sector) {
			continue;
		}
		/* Check target sector need erase END */


		/* Erase sector BEGIN */
		memset(sector_buf + addr_in_sector, 0xFF, erase_len);

		status = _flash_erase_sector(sector_addr);
		if (status != FLASH_OK) {
#if FLASH_BEDUG
			printTagLog(
				FLASH_TAG,
				"flash erase data addr=%lu len=%lu error (unable to erase sector: block_addr=%lu sector_addr=%lu len=%lu)",
				addr,
				len,
				sector_addr / flash_w25qxx_info.block_size,
				(sector_addr % flash_w25qxx_info.block_size) / flash_w25qxx_info.sector_size,
				FLASH_W25_SECTOR_SIZE
			);
#endif
			return status;
		}
		if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
			printTagLog(FLASH_TAG, "flash erase data addr=%lu len=%lu error (flash is busy)", addr, len);
#endif
			return FLASH_BUSY;
		}
		/* Erase sector END */


		/* Return old data BEGIN */
		uint32_t write_len = 0;
		while (write_len < FLASH_W25_SECTOR_SIZE) {
			status = _flash_write(sector_addr + write_len, sector_buf + write_len, FLASH_W25_PAGE_SIZE);
			if (status != FLASH_OK) {
#if FLASH_BEDUG
				printTagLog(
					FLASH_TAG,
					"flash erase data addr=%lu len=%lu error (unable to write old data page addr=%lu)",
					addr,
					len,
					sector_addr + write_len
				);
#endif
				return status;
			}

			write_len += FLASH_W25_PAGE_SIZE;
		}
		/* Return old data END */
	}

	return FLASH_OK;
}

flash_status_t _flash_read_jdec_id(uint32_t* jdec_id)
{
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "get JEDEC ID: begin");
#endif

    bool cs_selected = (HAL_GPIO_ReadPin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin) == GPIO_PIN_RESET);
    if (!cs_selected) {
        _flash_spi_cs_set();
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get JEDEC ID error (FLASH busy)");
#endif
        goto do_spi_stop;
    }

    uint8_t spi_cmd[] = { FLASH_W25_CMD_JEDEC_ID };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get JDEC ID error=%u (send command)", status);
#endif
        goto do_spi_stop;
    }

    uint8_t data[FLASH_W25_JEDEC_ID_SIZE] = { 0 };
    status = _flash_recieve_data(data, sizeof(data));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "get JDEC ID error=%u (recieve data)", status);
#endif
        goto do_spi_stop;
    }

    *jdec_id = ((((uint32_t)data[0]) << 16) | (((uint32_t)data[1]) << 8) | ((uint32_t)data[2]));

do_spi_stop:
    if (!cs_selected) {
        _flash_spi_cs_reset();
    }

    return status;
}

flash_status_t _flash_read_SR1(uint8_t* SR1)
{
    bool cs_selected = (HAL_GPIO_ReadPin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin) == GPIO_PIN_RESET);
    if (!cs_selected) {
        _flash_spi_cs_set();
    }

    uint8_t spi_cmd[] = { FLASH_W25_CMD_READ_SR1 };

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&FLASH_SPI, spi_cmd, sizeof(spi_cmd), FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
       return FLASH_BUSY;
    }

    if (status != HAL_OK) {
       return FLASH_ERROR;
    }

    status = HAL_SPI_Receive(&FLASH_SPI, SR1, sizeof(uint8_t), FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
        return FLASH_BUSY;
    }

    if (status != HAL_OK) {
        return FLASH_ERROR;
    }

    _flash_spi_cs_reset();

    if (cs_selected) {
        _flash_spi_cs_set();
    }

    return FLASH_OK;
}

flash_status_t _flash_write_enable()
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "write enable error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_WRITE_ENABLE };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
#if FLASH_BEDUG
    if (status != FLASH_OK) {
        printTagLog(FLASH_TAG, "write enable error=%u", status);
    }
#endif

    _flash_spi_cs_reset();

    return status;
}

flash_status_t _flash_write_disable()
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "write disable error (FLASH busy)");
#endif
        return FLASH_BUSY;
    }

    _flash_spi_cs_set();

    uint8_t spi_cmd[] = { FLASH_W25_CMD_WRITE_DISABLE };
    flash_status_t status = _flash_send_data(spi_cmd, sizeof(spi_cmd));
#if FLASH_BEDUG
    if (status != FLASH_OK) {
        printTagLog(FLASH_TAG, "write disable error=%u", status);
    }
#endif

    _flash_spi_cs_reset();

    return status;
}

flash_status_t _flash_erase_sector(uint32_t addr)
{
#if FLASH_BEDUG
	printTagLog(FLASH_TAG, "flash erase sector addr=%lu: begin", addr);
#endif

    if (addr % flash_w25qxx_info.sector_size > 0) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error (unacceptable address)", addr);
#endif
        return FLASH_ERROR;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error (flash is busy)", addr);
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd[FLASH_SPI_COMMAND_SIZE_MAX] = { 0 };
    uint8_t counter = 0;
    spi_cmd[counter++] = FLASH_W25_CMD_ERASE_SECTOR;
    if (flash_w25qxx_info.is_24bit_address) {
        spi_cmd[counter++] = (addr >> 24) & 0xFF;
    }
    spi_cmd[counter++] = (addr >> 16) & 0xFF;
    spi_cmd[counter++] = (addr >> 8) & 0xFF;
    spi_cmd[counter++] = addr & 0xFF;

    flash_status_t status = _flash_set_protect_block(FLASH_W25_SR1_UNBLOCK_VALUE);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (unset block protect)", addr, status);
#endif
        goto do_spi_stop;
    }

    status = _flash_write_enable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (write is not enabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    if (!util_wait_event(_flash_check_WEL, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (WEL bit wait time exceeded)", addr, FLASH_BUSY);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (BUSY bit wait time exceeded)", addr, FLASH_BUSY);
#endif
        status = FLASH_BUSY;
        goto do_spi_stop;
    }

    _flash_spi_cs_set();

    status = _flash_send_data(spi_cmd, counter);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (write is not enabled)", addr, status);
#endif
        goto do_spi_stop;
    }

    _flash_spi_cs_reset();

    status = _flash_write_disable();
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (write is not disabled)", addr, status);
#endif
        goto do_spi_stop;
    }

do_spi_stop:
    _flash_spi_cs_reset();

    if (status != FLASH_OK) {
        goto do_block_protect;
    }

    uint8_t cmpr_buf[FLASH_W25_SECTOR_SIZE] = { 0 };
    uint8_t read_buf[FLASH_W25_SECTOR_SIZE] = { 0 };
    memset(cmpr_buf, 0xFF, sizeof(cmpr_buf));
    memset(read_buf, 0xFF, sizeof(read_buf));

    status = flash_w25qxx_read(addr, read_buf, flash_w25qxx_info.sector_size);
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (check target sector)", addr, status);
#endif
        goto do_block_protect;
    }

    if (memcmp(cmpr_buf, read_buf, flash_w25qxx_info.sector_size)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "erase sector addr=%lu error - sector is not erased", addr);
#endif
        status = FLASH_ERROR;
        goto do_block_protect;
    }

    flash_status_t tmp_status = FLASH_OK;
do_block_protect:
    tmp_status = _flash_set_protect_block(FLASH_W25_SR1_BLOCK_VALUE);
    if (status == FLASH_OK) {
        status = tmp_status;
    } else {
        return status;
    }

#if FLASH_BEDUG
    if (status == FLASH_OK) {
    	printTagLog(FLASH_TAG, "flash erase sector addr=%lu: OK", addr);
    } else {
        printTagLog(FLASH_TAG, "erase sector addr=%lu error=%u (set block protected)", addr, status);
        status = FLASH_BUSY;
    }
#endif

    return status;
}

flash_status_t _flash_set_protect_block(uint8_t value)
{
    if (!util_wait_event(_flash_check_FREE, FLASH_SPI_TIMEOUT_MS)) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "set protect block value=%02X error (FLASH busy)", value);
#endif
        return FLASH_BUSY;
    }

    uint8_t spi_cmd_01[] = { FLASH_W25_CMD_WRITE_ENABLE_SR };

    _flash_spi_cs_set();

    flash_status_t status = _flash_send_data(spi_cmd_01, sizeof(spi_cmd_01));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "set protect block value=%02X error=%u (enable write SR1)", value, status);
#endif
        goto do_spi_stop;
    }

    _flash_spi_cs_reset();


    uint8_t spi_cmd_02[] = { FLASH_W25_CMD_WRITE_SR1, ((value & 0x0F) << 2) };

    _flash_spi_cs_set();

    status = _flash_send_data(spi_cmd_02, sizeof(spi_cmd_02));
    if (status != FLASH_OK) {
#if FLASH_BEDUG
        printTagLog(FLASH_TAG, "set protect block value=%02X error=%u (write SR1)", value, status);
#endif
        goto do_spi_stop;
    }

    goto do_spi_stop;

do_spi_stop:
    _flash_spi_cs_reset();

    return status;
}


flash_status_t _flash_send_data(uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&FLASH_SPI, data, len, FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
        return FLASH_BUSY;
    }
    if (status != HAL_OK) {
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

flash_status_t _flash_recieve_data(uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status = HAL_SPI_Receive(&FLASH_SPI, data, len, FLASH_SPI_TIMEOUT_MS);
    if (status == HAL_BUSY) {
        return FLASH_BUSY;
    }
    if (status != HAL_OK) {
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

void _flash_spi_cs_set()
{
    HAL_GPIO_WritePin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin, GPIO_PIN_RESET);
}

void _flash_spi_cs_reset()
{
    HAL_GPIO_WritePin(FLASH_SPI_CS_GPIO_Port, FLASH_SPI_CS_Pin, GPIO_PIN_SET);
}

bool _flash_check_FREE()
{
    uint8_t SR1 = 0x00;
    flash_status_t status = _flash_read_SR1(&SR1);
    if (status != FLASH_OK) {
        return false;
    }

    return !(SR1 & FLASH_W25_SR1_BUSY);
}

bool _flash_check_WEL()
{
    uint8_t SR1 = 0x00;
    flash_status_t status = _flash_read_SR1(&SR1);
    if (status != FLASH_OK) {
        return false;
    }

    return SR1 & FLASH_W25_SR1_WEL;
}

uint32_t _flash_get_storage_bytes_size()
{
#if FLASH_TEST
	return flash_w25qxx_get_pages_count() * FLASH_W25_PAGE_SIZE;
#endif
    return flash_w25qxx_info.blocks_count * flash_w25qxx_info.block_size;
}
