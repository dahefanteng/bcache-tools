/*
 * Author: Kent Overstreet <kmo@daterainc.com>
 *
 * GPLv2
 */

#ifndef _BCACHE_H
#define _BCACHE_H

#define BITMASK(name, type, field, offset, size)		\
static inline uint64_t name(const type *k)			\
{ return (k->field >> offset) & ~(((uint64_t) ~0) << size); }	\
								\
static inline void SET_##name(type *k, uint64_t v)		\
{								\
	k->field &= ~(~((uint64_t) ~0 << size) << offset);	\
	k->field |= v << offset;				\
}

static const char bcache_magic[] = {
	0xc6, 0x85, 0x73, 0xf6, 0x4e, 0x1a, 0x45, 0xca,
	0x82, 0x65, 0xf5, 0x7f, 0x48, 0xba, 0x6d, 0x81 };

/*
 * Version 0: Cache device
 * Version 1: Backing device
 * Version 2: Seed pointer into btree node checksum
 * Version 3: Cache device with new UUID format
 * Version 4: Backing device with data offset
 */
#define BCACHE_SB_VERSION_CDEV			0
#define BCACHE_SB_VERSION_BDEV			1
#define BCACHE_SB_VERSION_CDEV_WITH_UUID	3
#define BCACHE_SB_VERSION_BDEV_WITH_OFFSET	4
#define BCACHE_SB_MAX_VERSION			4

#define SB_SECTOR		8
#define SB_LABEL_SIZE		32
#define SB_JOURNAL_BUCKETS	256U
#define BDEV_DATA_START_DEFAULT	16	/* sectors */
#define SB_START		(SB_SECTOR * 512)


#define ATA_OP_IDENTIFY		0xec
#define ATA_OP_PIDENTIFY	0xa1

/*
 * Some useful ATA register bits
 */
enum {
	ATA_USING_LBA		= (1 << 6),
	ATA_STAT_DRQ		= (1 << 3),
	ATA_STAT_ERR		= (1 << 0),
};

/*
 * ATA PASS-THROUGH (16) CDB
 */
#define SG_ATA_16			0x85
#define SG_ATA_16_LEN			16

/*
 * ATA Protocols
 */
#define SG_ATA_PROTO_PIO_IN		(4 << 1)	/* PIO Data-in */

enum {
	/* No data is transferred */
	SG_CDB2_TLEN_NODATA	= 0 << 0,
	/* Transfer Length is found in the Feature field */
	SG_CDB2_TLEN_FEAT	= 1 << 0,
	/* Transfer Length is found in the Sector Count field */
	SG_CDB2_TLEN_NSECT	= 2 << 0,

	/* transfer units for Transfer Length are bytes */
	SG_CDB2_TLEN_BYTES	= 0 << 2,
	/* transfer units for Transfer Length are blocks */
	SG_CDB2_TLEN_SECTORS	= 1 << 2,

	/* data is transferred from the initiator to the target */
	SG_CDB2_TDIR_TO_DEV	= 0 << 3,
	/* indicate that data is transferred from the target to the initiator */
	SG_CDB2_TDIR_FROM_DEV	= 1 << 3,

	/* Check Condition */
	SG_CDB2_CHECK_COND	= 1 << 5,
};

/*
 *  SCSI Architecture Model (SAM) Status codes. Taken from SAM-6
 *  T10/BSR INCITS 546 dated January 5, 2018.
 */
#define SAM_STAT_GOOD		0x00
#define SG_CHECK_CONDITION	0x02
#define SG_DRIVER_SENSE		0x08

/*
 * This is a slightly modified SCSI sense "descriptor" format header.
 * The addition is to allow the 0x70 and 0x71 response codes. The idea
 * is to place the salient data from either "fixed" or "descriptor" sense
 * format into one structure to ease application processing.
 *
 * The original sense buffer should be kept around for those cases
 * in which more information is required (e.g. the LBA of a MEDIUM ERROR).
 */
struct scsi_sense_hdr {		/* See SPC-3 section 4.5 */
	uint8_t response_code;	/* permit: 0x0, 0x70, 0x71, 0x72, 0x73 */
	uint8_t sense_key;
	uint8_t asc;
	uint8_t ascq;
	uint8_t byte4;
	uint8_t byte5;
	uint8_t byte6;
	uint8_t additional_length;	/* always 0 for fixed sense format */
};

/*
 *  SENSE KEYS
 */

#define SG_NO_SENSE            0x00
#define SG_RECOVERED_ERROR     0x01

/* NVME Admin commands */
#define nvme_admin_identify	0x06

#define NVME_IDENTIFY_DATA_SIZE 4096

struct cache_sb {
	uint64_t		csum;
	uint64_t		offset;	/* sector where this sb was written */
	uint64_t		version;

	uint8_t			magic[16];

	uint8_t			uuid[16];
	union {
		uint8_t		set_uuid[16];
		uint64_t	set_magic;
	};
	uint8_t			label[SB_LABEL_SIZE];

	uint64_t		flags;
	uint64_t		seq;
	uint64_t		pad[8];

	union {
	struct {
		/* Cache devices */
		uint64_t	nbuckets;	/* device size */

		uint16_t	block_size;	/* sectors */
		uint16_t	bucket_size;	/* sectors */

		uint16_t	nr_in_set;
		uint16_t	nr_this_dev;
	};
	struct {
		/* Backing devices */
		uint64_t	data_offset;

		/*
		 * block_size from the cache device section is still used by
		 * backing devices, so don't add anything here until we fix
		 * things to not need it for backing devices anymore
		 */
	};
	};

	uint32_t		last_mount;	/* time_t */

	uint16_t		first_bucket;
	union {
		uint16_t	njournal_buckets;
		uint16_t	keys;
	};
	uint64_t		d[SB_JOURNAL_BUCKETS];	/* journal buckets */
};

static inline bool SB_IS_BDEV(const struct cache_sb *sb)
{
	return sb->version == BCACHE_SB_VERSION_BDEV
		|| sb->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET;
}

BITMASK(CACHE_SYNC,		struct cache_sb, flags, 0, 1);
BITMASK(CACHE_DISCARD,		struct cache_sb, flags, 1, 1);
BITMASK(CACHE_REPLACEMENT,	struct cache_sb, flags, 2, 3);
#define CACHE_REPLACEMENT_LRU	0U
#define CACHE_REPLACEMENT_FIFO	1U
#define CACHE_REPLACEMENT_RANDOM 2U

BITMASK(BDEV_CACHE_MODE,	struct cache_sb, flags, 0, 4);
#define CACHE_MODE_WRITETHROUGH	0U
#define CACHE_MODE_WRITEBACK	1U
#define CACHE_MODE_WRITEAROUND	2U
#define CACHE_MODE_NONE		3U
BITMASK(BDEV_STATE,		struct cache_sb, flags, 61, 2);
#define BDEV_STATE_NONE		0U
#define BDEV_STATE_CLEAN	1U
#define BDEV_STATE_DIRTY	2U
#define BDEV_STATE_STALE	3U

uint64_t crc64(const void *_data, size_t len);

#define node(i, j)		((void *) ((i)->d + (j)))
#define end(i)			node(i, (i)->keys)

#define csum_set(i)							\
	crc64(((void *) (i)) + 8, ((void *) end(i)) - (((void *) (i)) + 8))

#endif
