/*
 * debug.h
 *
 *  Created on: Jun 27, 2022
 *  Author: abdo daood < abdo454daood@gmail.com >
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

#ifdef DEBUG
#define DEBUG_PRINT(...)     printf(__VA_ARGS__); printf("\r\n");
#define DEBUG_PRINT_RAW(...) printf(__VA_ARGS__);
#define DEBUG_ERR_PRINT(...) printf("Error %s at %d : ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\r\n");
#else
	#define DEBUG_PRINT(...)
	#define DEBUG_PRINT_RAW(...)
	#define DEBUG_ERR_PRINT(...)
#endif

#endif /* INC_DEBUG_H_ */
