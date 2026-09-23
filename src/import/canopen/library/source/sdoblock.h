/*
 * sdoblock - defines for sdo block transfer usage
 *
 * Copyright (c) 2001-2017 port GmbH Halle/Saale
 *------------------------------------------------------------------
 */

/*
DESCRIPTION

The file contains definitions of structures and data types for sdo block usage

*/

#ifndef __SDOBLOCK_H
# define __SDOBLOCK_H

#define CO_SDOBLOCK_CONT_FLAG	0x80
#define CO_SDOBLOCK_NO_MORE_BLKS	0x80	/* no more blocks to download */

#define CO_SDOBLOCK_USE_CRC	0x04	/* generate CRC by block transfer */
#define CO_SDOBLOCK_SIZE_VALID	0x02	/* block size valid */

/* defines for client block subcommands */
#ifndef CO_SDO_CCS_MASK
#  define CO_SDO_CCS_MASK	0xe0 	/* client command specifier */
#endif /* CO_SDO_CCS_MASK */

#define CO_SDOBLK_CCS_DOWN	0xc0	/* client command specifier download */
#define CO_SDOBLK_CCS_UP	0xa0	/* client command specifier upload */

#define CO_SDOBLK_CS_MASK	0x03	/* client subcommand mask */

#define CO_SDOBLK_CS_INIT	0x00	/* init up/download */
#define CO_SDOBLK_CS_DL		0x01	/* block download */
#define CO_SDOBLK_CS_UP_END	0x01	/* end upload */
#define CO_SDOBLK_CS_UP_RESP	0x02	/* upload response */
#define CO_SDOBLK_CS_UP_START	0x03	/* start upload */

/* defines for server block subcommands */
#define CO_SDOBLK_SCS_MASK	0xe0	/* server command specifier */

#define CO_SDOBLK_SCS_DOWN	0xa0	/* server command specifier download */
#define CO_SDOBLK_SCS_UP	0xc0	/* server command specifier upload */

#define CO_SDOBLK_SS_MASK	0x03	/* server subcommand mask */

#define CO_SDOBLK_SS_INIT	0x00	/* init up/download response */
#define CO_SDOBLK_SS_END	0x01	/* end block up/download response */
#define CO_SDOBLK_SS_BLK_DL	0x02	/* block download response */


/* external data declarations */


/* function prototypes */

void	sdoContBlockTrans(CO_LINE_PARA_DECL);


#endif		/*  __SDOBLOCK_H */

/* end of source */

