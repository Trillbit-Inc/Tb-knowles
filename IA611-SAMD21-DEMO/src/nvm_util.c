#include <asf.h>
#include <nvm.h>

#include "nvm_util.h"

// For SAMD21 Xplained Pro Board
// Total rows = 4069 / 4 = 1024 Rows
// License Buffer = 512 bytes = 512 bytes / 64 bytes per page / 4 pages per row = 2 Rows
#define LICENSE_PAGE_ADDRESS	(1022 * NVMCTRL_ROW_PAGES * NVMCTRL_PAGE_SIZE)
#define LICENSE_ROW1			LICENSE_PAGE_ADDRESS
#define LICENSE_ROW2			(1023 * NVMCTRL_ROW_PAGES * NVMCTRL_PAGE_SIZE)
#define N_ROWS					((LICENSE_MAX_SIZE / NVMCTRL_PAGE_SIZE) / NVMCTRL_ROW_PAGES)

static unsigned long rows[] = {
		LICENSE_ROW1,
		LICENSE_ROW2
	};

void nvm_util_init(void)
{
//! [setup_1]
	struct nvm_config config_nvm;
//! [setup_1]

//! [setup_2]
	nvm_get_config_defaults(&config_nvm);
//! [setup_2]

//! [setup_3]
	config_nvm.manual_page_write = false;
//! [setup_3]

//! [setup_4]
	nvm_set_config(&config_nvm);
//! [setup_4]
}
//! [setup]

const char* nvm_util_get_lic(void)
{
	return (const char*) (FLASH_ADDR + LICENSE_PAGE_ADDRESS);
}

int nvm_util_write_lic(char* buf, unsigned int size)
{
	if ((!size) ||
		(size > LICENSE_MAX_SIZE))
	{
		return -1;
	}
		
	
	enum status_code error_code;
	
	for (int i = 0; i < N_ROWS; i++)
	{
		do
		{
			error_code = nvm_erase_row(rows[i]);
		} while (error_code == STATUS_BUSY);
	
		if (error_code != STATUS_OK)
			return -1;
	}
	
	unsigned int n_pages = size / NVMCTRL_PAGE_SIZE;
	if ((size % NVMCTRL_PAGE_SIZE) != 0)
	{
		n_pages++;
	}
	
	for (unsigned int i = 0; (i < n_pages) && (size > 0); i++)
	{
		int write_size = NVMCTRL_PAGE_SIZE;
		
		if (size < NVMCTRL_PAGE_SIZE)
		{
			write_size = size;
		}
		
		do
		{
			error_code = nvm_write_buffer(
					LICENSE_PAGE_ADDRESS + (i*NVMCTRL_PAGE_SIZE),
					(const unsigned char*)&buf[(i*NVMCTRL_PAGE_SIZE)],
					write_size);
		} while (error_code == STATUS_BUSY);
		
		if (error_code != STATUS_OK)
			return -1;
		
		size -= write_size;
	}
	
	return 0;
}

