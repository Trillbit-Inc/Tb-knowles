/*
 * nvm_util.h
 *
 * Created: 16-08-2022 17:54:52
 *  Author: bnrki
 */ 


#ifndef NVM_UTIL_H_
#define NVM_UTIL_H_

// multiple of NVM flash page size
#define LICENSE_MAX_SIZE	512

void nvm_util_init(void);
const char* nvm_util_get_lic(void);
int nvm_util_write_lic(char* buf, unsigned int size);

#endif /* NVM_UTIL_H_ */