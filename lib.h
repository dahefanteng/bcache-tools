/* SPDX-License-Identifier: GPL-2.0 */
// Author: Shaoxiong Li <dahefanteng@gmail.com>

#include "list.h"

struct dev {
	char		name[40];
	char		*magic;
	uint64_t	first_sector;
	uint64_t	csum;
	uint64_t	version;
	char		label[SB_LABEL_SIZE + 1];
	char		uuid[40];
	uint16_t	sectors_per_block;
	uint16_t	sectors_per_bucket;
	char		cset[40];
	char		state[40];
	char		bname[40];
	char		attachuuid[40];
	struct	list_head	dev_list;
};

struct bdev {
	struct dev	base;
	uint16_t	first_sector;
	uint8_t		cache_mode;
	uint8_t		cache_state;
};

//typedef int bool;
struct cdev {
	struct dev	base;
	uint16_t	first_sector;
	uint64_t	cache_sectors;
	uint64_t	total_sectors;
	bool		ordered;
	bool		discard;
	uint16_t	pos;
	unsigned int	replacement;
};


int list_bdevs(struct list_head *head);
int detail_dev(char *devname, struct bdev *bd, struct cdev *cd, int *type);
int register_dev(char *devname);
int stop_backdev(char *devname);
int unregister_cset(char *cset);
int attach_backdev(char *cset, char *devname);
int detach_backdev(char *devname);
int set_backdev_cachemode(char *devname, char *cachemode);
int set_label(char *devname, char *label);
int cset_to_devname(struct list_head *head, char *cset, char *devname);


#define DEVLEN sizeof(struct dev)

#define BCACHE_NO_SUPPORT		"N/A"

#define BCACHE_BASIC_STATE_ACTIVE	"active"
#define BCACHE_BASIC_STATE_INACTIVE	"inactive"

#define BCACHE_ATTACH_ALONE		"Alone"
#define BCACHE_BNAME_NOT_EXIST		"Non-Exist"
#define DEV_PREFIX_LEN			5
