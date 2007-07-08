/* packet-stp.c
 * defines and structures for STP packet dissection
 * Copyright Andrew Tridgell, August 2003
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define TCP_PORT_STNL 1700


/*
  used to hold private parse info 
*/
struct stp_private {
	gboolean initialised;
	gboolean unicode;
};


typedef struct {
	guint32 count;
	guint16 *values;
} uint16_array_t;

struct stnl_header {
	unsigned char signature[4];
	guint16 msg_type;
	guint16 msg_length;
	guint64 msg_number;
	guint64 receiver_id;
	guint64 sender_id;
};


struct tank_header {
	unsigned char signature[4];
	guint16 version;
	unsigned int msg_type:4;
	unsigned int cmd_type:12;
	guint32 msg_length;
	guint64 client_id;
	guint8  padding[12];
	guint64 transaction_context;
};


struct tank_identify {
	const gchar *client_name;
	const gchar *client_platform;
	const gchar *client_locale;
	guint16 locale_flag;
	const gchar *client_version;
	guint16 client_charset;
	guint64 client_time;
	guint8  client_capabilities[16];
};

struct tank_identify_response {
	guint16 rc;
	const gchar *server_name;
	const gchar *platform_name;
	const gchar *software_version;
	guint64 server_time;
	guint8  server_capabilities[16];
	guint16 preferred_version;
	uint16_array_t supported_versions;
	guint8  admin_client;
};

typedef struct {
	guint32 cluster_id;
	guint32 container_id;
	guint32 epoch_id;
	guint64 object_id;
} uoid_t;

typedef struct {
	guint32 epoch;
	guint64 version;
} dvers_t;

typedef struct {
	guint32 epoch;
	guint64 version;
} svers_t;

typedef struct {
	guint64 seg_num;
} seg_update_t;

typedef struct {
	guint64 seg_num;
	guint32 read_count;
	guint32 write_count;
} seg_descr_t;

typedef struct {
	guint8 virt_block_num;
	guint8 block_count;
	guint64 disk_id;
	guint32 phys_block_num;
} extent_t;

typedef struct {
	guint32 nic_flags;
} net_interface_t;

typedef struct {
	guint64 timestamp;
} access_time_t;

typedef struct {
	guint16 name_length;
	guint8 obj_type;
	guint64 key;
} dir_entry_t;

typedef struct {
	guint32 offset;
	guint16 count;
	guint16 el_size;
	guint8 *elements;
} vector_t;

typedef struct {
	guint8 obj_type;
	guint64 create_time;
	guint64 change_time;
	guint64 access_time;
	guint64 modify_time;
	guint32 misc_attr;
	guint32 link_count;
	guint64 file_size;
	guint64 block_count;
	guint32 block_size;
	guint64 version;
} basic_objattr_t;

typedef struct {
	basic_objattr_t basic;
	guint32 userid;
	guint32 groupid;
	guint16 permissions;
	const char *symlink;
	const char *sdvalue;
} objattr_t;

typedef struct {
	guint8 mode;
	dvers_t version;
	objattr_t obj_attr;
	guint16 strategy;
} gdl_t;

typedef struct {
	guint8 mode;
	dvers_t version;
} gsl_t;

typedef struct {
	gboolean is_legacy;
	unsigned char n_addr[16];
} netaddr_t;

typedef struct {
	gboolean is_null;
	netaddr_t netaddr;
	guint32 netport;
} srvaddr_t;

struct tank_acquire_data_lock {
	uoid_t uoid;
	guint8 mode;
	gboolean opportunistic;
	dvers_t version;
	guint64 alt_id;
};

struct tank_acquire_data_lock_response {
	gdl_t granted;
};

struct tank_acquire_session_lock {
	uoid_t uoid;
	guint8 session_lock_mode;
	svers_t lock_version;
	guint8 data_lock_mode;
	guint64 alt_id;
};

struct tank_acquire_session_lock_response {
	gsl_t session_lock;
	gboolean not_strength_related;
	gdl_t data_lock;
};

struct tank_downgrade_session_lock {
	uoid_t uoid;
	guint8 lock_mode;
};

struct tank_publish_cluster_info {
	guint32 container_id;
	vector_t primary_nic_list;
	vector_t secondary_nic_list;
	guint32 lease_period;
	guint16 num_retries;
	guint32 xmit_timeout;
	guint32 async_period;
};

struct tank_lookup_name {
	uoid_t parent_uoid;
	const gchar *name;
	gboolean case_insensitive;
	guint8 session_lock_mode;	
	guint8 data_lock_mode;	
	guint64 timestamp;
};

struct tank_lookup_name_response {
	uoid_t uoid;
	guint8 obj_type;
	srvaddr_t server;
	guint64 key;
	const gchar *name;
	gsl_t granted;
	gboolean not_strength_related;
	gdl_t gdl;
};

struct tank_change_name {
	uoid_t src_parent_uoid;
	const char *src_name;
	uoid_t dest_parent_uoid;
	const char *dest_name;
	gboolean dest_case_insensitive;
	guint64 timestamp;
	gboolean exclusive;
};

struct tank_change_name_response {
	basic_objattr_t src_parent_attrib;
	basic_objattr_t dest_parent_attrib;
	uoid_t src_child_uoid;
	guint8 src_child_type;
	basic_objattr_t src_child_attrib;
	gboolean dest_child_existed;
	uoid_t dest_child_uoid;
	basic_objattr_t dest_child_attrib;
	const char *dest_actual_name;
	guint64 dest_key;
};

struct tank_report_txn_status {
	guint32 rc;
	guint32 retry_delay;
};

struct tank_get_storage_capacity {
	uoid_t uoid;
};

struct tank_get_storage_capacity_response {
	guint64 total_blocks;
	guint64 free_blocks;
};

struct tank_create_file {
	uoid_t parent_uoid;
	const char *name;
	gboolean case_insensitive;
	guint64 timestamp;
	guint32 userid;
	guint32 groupid;
	guint16 permissions;
	guint32 misc_attr;
	guint16 quality_of_service;
	gboolean exclusive;
	guint8 session_lock_mode;
	guint8 data_lock_mode;
	const char *sdvalue;
};

struct tank_create_file_response {
	gboolean existed;
	uoid_t uoid;
	guint64 dir_key;
	srvaddr_t server;
	basic_objattr_t parent_attr;
	const char *actual_name;
	gsl_t session_lock;
	gboolean not_strength_related;
	gdl_t data_lock;
};

struct tank_create_dir {
	uoid_t parent_uoid;
	const char *name;
	gboolean case_insensitive;
	guint64 timestamp;
	guint32 userid;
	guint32 groupid;
	guint16 permissions;
	guint32 misc_attr;
	gboolean exclusive;
	guint8 session_lock_mode;
	const char *sdvalue;
};

struct tank_create_dir_response {
	gboolean existed;
	uoid_t uoid;
	guint64 dir_key;
	srvaddr_t server;
	basic_objattr_t parent_attr;
	guint64 dot_key;
	guint64 dot_dot_key;
	const char *actual_name;
	gsl_t session_lock;
	gboolean not_strength_related;
	gdl_t data_lock;
};

struct tank_renew_lease {
	guint64 timestamp;
};

struct tank_ping {
	guint64 timestamp;
};

struct tank_shutdown {
};

struct tank_notify_forward {
	srvaddr_t server;
	guint32 load_unit_id;
};

struct tank_create_hard_link {
	uoid_t parent_dir_uoid;
	const gchar *name;
	gboolean case_insensitive;
	gboolean exclusive;
	uoid_t extant_uoid;
	guint64 timestamp;
};

struct tank_create_hard_link_resp {
	guint64 key;
	gboolean existed;
	basic_objattr_t parent_attr;
	basic_objattr_t extant_attr;
	const gchar *actual_name;
};

struct tank_create_sym_link {
	uoid_t parent_dir_uoid;
	const gchar *name;
	const gchar *path_string;
	guint64 timestamp;
	guint32 owner_id;
	guint32 group_id;
	guint16 permissions;
	guint32 misc_attr;
	const gchar *stsd_val;
};

struct tank_create_sym_link_resp {
	uoid_t uoid;
	guint64 key;
	srvaddr_t server;
	basic_objattr_t parent_attr;
	gsl_t granted_session_lock;
};

struct tank_remove_name {
	uoid_t parent_dir_uoid;
	const gchar *name;
	guint64 timestamp;
};

struct tank_remove_name_resp {
	basic_objattr_t parent_attr;
	uoid_t child_uoid;
	basic_objattr_t child_attr;
};

struct tank_set_basic_obj_attr {
	uoid_t uoid;
	guint32 which;
	guint64 create_time;
	guint64 change_time;
	guint64 access_time;
	guint64 modify_time;
	guint32 misc_attr;
	guint64 timestamp;
};

struct tank_set_basic_obj_attr_resp {
	basic_objattr_t obj_attr;
};

struct tank_set_access_ctl_attr {
	uoid_t uoid;
	guint32 which;
	guint32 owner_id;
	guint32 group_id;
	guint16 permissions;
	guint64 timestamp;
	const gchar *stsd_val;
};

struct tank_set_access_ctl_attr_resp {
	guint8 obj_attr;
};

struct tank_update_access_time {
	guint8 changes;
};

struct tank_publish_access_time {
	guint8 changes;
};


struct tank_read_dir {
	uoid_t dir_uoid;
	guint64 restart_key;
};

struct tank_read_dir_resp {
	basic_objattr_t parent_attr;
	guint8 end_reached;
	guint8 is_complete;
	guint8 entry_list;
};

struct tank_find_object {
	uoid_t uoid;
	guint8 session_lock_mode;
	guint8 data_lock_mode;
};

struct tank_find_object_resp {
	guint8 obj_type;
	srvaddr_t server;
	gsl_t granted_session_lock;
	guint8 not_strength_related;
	gdl_t granted_data_lock;
};

struct tank_acquire_session_lock_resp {
	gsl_t granted_session_lock;
	guint8 not_strength_related;
	gdl_t granted_data_lock;
};

struct tank_demand_session_lock {
	uoid_t uoid;
	guint8 lock_mode;
	guint8 demand_flag;
};

struct tank_deny_session_lock {
	uoid_t uoid;
};

struct tank_publish_lock_version {
	uoid_t uoid;
	guint8 lock_version;
};

struct tank_downgrade_data_lock {
	uoid_t uoid;
	guint8 lock_mode;
	guint8 access_time_valid;
	guint64 access_time;
};

struct tank_demand_data_lock {
	uoid_t uoid;
	guint8 lock_mode;
	guint8 opportunistic;
};

struct tank_deferred_downgrade_data_lock {
	uoid_t uoid;
	guint8 obj_attr;
};

struct tank_invalidate_directory {
	uoid_t uoid;
	const gchar *removed_name;
	basic_objattr_t dir_attr;
	guint8 unlinked;
	uoid_t unlinked_o_i_d;
};

struct tank_discard_directory {
	uoid_t uoid;
};

struct tank_invalidate_obj_attr {
	uoid_t uoid;
};

struct tank_discard_obj_attr {
	uoid_t uoid;
};

struct tank_publish_basic_obj_attr {
	uoid_t uoid;
	basic_objattr_t basic_attr;
};

struct tank_blk_disk_allocate {
	uoid_t uoid;
	guint64 block_offset;
	guint64 block_count;
	guint64 more_block_count;
};

struct tank_blk_disk_allocate_resp {
	basic_objattr_t obj_attr;
	guint8 seg_list;
};

struct tank_blk_disk_update {
	uoid_t uoid;
	guint8 harden;
	guint8 seg_update_list;
	guint8 new_lock_mode;
	guint32 ba_which;
	guint32 acl_which;
	guint64 create_time;
	guint64 change_time;
	guint64 access_time;
	guint64 modify_time;
	guint32 misc_attr;
	guint64 file_size;
	guint32 owner_id;
	guint32 group_id;
	guint16 permissions;
	const gchar *stsd_val;
};

struct tank_blk_disk_update_resp {
	basic_objattr_t obj_attr;
};

struct tank_blk_disk_get_segment {
	uoid_t uoid;
	guint64 seg_no;
	guint64 more_seg_count;
};

struct tank_blk_disk_get_segment_resp {
	guint8 seg_list;
};

struct tank_set_range_lock {
	uoid_t uoid;
	guint64 owner_id;
	guint64 start;
	guint64 length;
	guint8 mode;
	guint64 retry_id;
	guint64 alt_id;
	guint8 reassert_version;
	guint32 flags;
};

struct tank_set_range_lock_resp {
	guint8 owner_client;
	guint64 owner_id;
	guint64 start;
	guint64 length;
	guint8 mode;
	guint8 reassert_version;
	guint32 flags;
	guint32 rc;
};

struct tank_demand_range_lock {
	uoid_t uoid;
	guint64 txn_id;
	guint64 start;
	guint64 length;
	guint8 mode;
};

struct tank_demand_range_lock_resp {
	uoid_t uoid;
	guint64 txn_id;
	guint64 start;
	guint64 length;
	guint8 mode;
};

struct tank_relinquish_range_lock {
	uoid_t uoid;
	guint64 owner_id;
	guint64 start;
	guint64 length;
	guint8 mode;
};

struct tank_retry_range_lock {
	guint64 retry_id;
};

struct tank_compat_set_range_lock {
	uoid_t uoid;
	guint64 owner_id;
	guint8 lock_mode;
	guint64 range_start;
	guint64 range_length;
	guint8 wait_for_lock;
	guint64 retry_id;
};

struct tank_compat_check_range_lock {
	uoid_t uoid;
	guint64 owner_id;
	guint8 lock_mode;
	guint64 range_start;
	guint64 range_length;
};

struct tank_compat_check_range_lock_resp {
	guint8 net_addr;
	guint64 owner_id;
	guint8 lock_mode;
	guint64 range_start;
	guint64 range_length;
};

struct tank_release_all_range_locks {
	uoid_t uoid;
};

struct tank_verify_credentials {
	const gchar *credentials;
};

struct tank_publish_load_unit_info {
	guint32 load_unit_id;
	srvaddr_t server_addr;
	guint32 load_unit_state;
};

struct tank_publish_root_clt_info {
	guint8 root_clt_flag;
};

struct tank_publish_eviction_request {
	guint32 batch_size;
};

/*---------- Start FlexSAN data structures ----------*/

#define STP_SECTOR_SIZE 512

typedef struct                             /* stpLun */
{
  guint8  lun_id_format[16];               /* array of 16 chars */
  guint32 lun_id_length;
} lun_id_t;

/* The lun_or_vol_t adds 3 bytes of padding to align the union.
 * How do I tell the compiler to pack the values?
 */
typedef struct {
  guint8 is_volume;
  union 
  {
    lun_id_t lv_lun;
    guint64  lv_vol;
  } lv_device;
} lun_or_vol_t;

typedef struct                             /* stpLunInfo */
{
  guint8    state; 
  guint8    access; 
  guint8    vendor_id[8];
  guint8    product_id[16];
  guint8    product_ver[4];
  guint8    id_format[16]; /* Assuming length 8, Krishna's input needed. */
  guint32   blk_size; 
  guint64   capacity;
//  volume_t  volume; /* set to zeros if it is not a volume */
  guint64   vol_gid;
  guint64   vol_owner_id;
  guint64   vol_install_id;
  
  guint32   id_length; 
  guint32   dev_name_length; 
} lun_info_t;

typedef struct                             /* stpLun */
{
  guint64 gid;
} vol_id_t;

typedef struct                             /* stpVolInfo */
{
  guint8  vi_state;
  guint64 vi_globalDiskId;
  guint64 vi_ownerId;
  guint64 vi_installationId;
  guint64 vi_capacity;
} vol_info_t;

typedef struct                              /* stpCopyRequest */
{
  guint64 source_gid;
  guint32 source_start;
  guint64 dest_gid;
  guint32 dest_start;
  guint32 block_count;
} copy_data_request_t;

typedef struct                              /* stpCopyRequest */
{
  guint32 blocks_written;
  guint8  result;
} copy_result_t;

/*---------- Start FlexSAN messages ----------*/

struct tank_get_lun_list                   /* stpGetLunListMsg */
{
  guint8 discover;
};

struct tank_get_lun_info                   /* stpGetLunInfoMsg */
{
  guint8 discover;
  guint8 lun_list;                         /* stpList of stpLun */
};

struct tank_get_lun_info_resp              /* stpGetLunInfoListRespMsg */
{
  guint8 more_flag;
  guint8 lun_list;                         /* stpList of stpLunInfo */
};

struct tank_get_vol_list                   /* stpGetVolListMsg */
{
  guint8 discover;
};

struct tank_get_vol_info                   /* stpGetVolInfoMsg */
{
  guint8 discover;
  guint8 vol_list;                         /* stpVector of GID */
};

struct tank_get_vol_info_resp              /* stpGetVolInfoListRespMsg */
{
  guint8 more_flag;
  guint8 vol_list;                         /* stpVector of stpVolInfo */
};

struct tank_read_sector {                  /* stpReadSectorMsg */
  guint64      offset;
  lun_or_vol_t device_name;
};

struct tank_read_sector_resp {             /* stpReadSectorRespMsg */
  guint32 result_code;
  guint8  payload[ STP_SECTOR_SIZE ];
};

struct tank_write_sector {                 /* stpWriteSectorMsg */
  guint64      offset;
  guint8       payload[ STP_SECTOR_SIZE ];
  lun_or_vol_t device_name;
};

struct tank_write_sector_resp {            /* stpWriteSectorRespMsg */
guint32 result_code;
};

struct tank_copy_data                      /* stpCopyDataMsg */
{
  guint8 copy_data_list;                   /* stpVector of stpCopyRequest */
};

struct tank_copy_data_resp                 /* stpCopyDataRespMsg */
{
  guint8 result;                           /* stpVector of stpCopyResult */
};

/*----------- End FlexSAN structures ----------*/
struct tank_null {
};

struct tank_null_resp {
};


#define TANK_CMD_IDENTIFY                       0x01
#define TANK_CMD_RENEW_LEASE                    0x02
#define TANK_CMD_SHUTDOWN                       0x03
#define TANK_CMD_PING                           0x04
#define TANK_CMD_IDENTIFY_RESPONSE              0x05

#define TANK_CMD_REPORT_TXN_STATUS              0x07
#define TANK_CMD_NOTIFY_FORWARD                 0x08

#define TANK_CMD_CREATE_FILE                    0x10
#define TANK_CMD_CREATE_FILE_RESPONSE           0x11
#define TANK_CMD_CREATE_DIR                     0x12
#define TANK_CMD_CREATE_DIR_RESPONSE            0x13
#define TANK_CMD_CREATE_HARD_LINK               0x14
#define TANK_CMD_CREATE_HARD_LINK_RESP          0x15
#define TANK_CMD_CREATE_SYM_LINK                0x16
#define TANK_CMD_CREATE_SYM_LINK_RESP           0x17
#define TANK_CMD_LOOKUP_NAME                    0x18
#define TANK_CMD_LOOKUP_NAME_RESPONSE           0x19
#define TANK_CMD_CHANGE_NAME                    0x1a
#define TANK_CMD_CHANGE_NAME_RESPONSE           0x1b
#define TANK_CMD_REMOVE_NAME                    0x1c
#define TANK_CMD_REMOVE_NAME_RESP               0x1d

#define TANK_CMD_SET_BASIC_OBJ_ATTR             0x30
#define TANK_CMD_SET_BASIC_OBJ_ATTR_RESP        0x31
#define TANK_CMD_SET_ACCESS_CTL_ATTR            0x32
#define TANK_CMD_SET_ACCESS_CTL_ATTR_RESP       0x33
#define TANK_CMD_READ_DIR                       0x34
#define TANK_CMD_READ_DIR_RESP                  0x35
#define TANK_CMD_FIND_OBJECT                    0x36
#define TANK_CMD_FIND_OBJECT_RESP               0x37
#define TANK_CMD_UPDATE_ACCESS_TIME             0x38
#define TANK_CMD_PUBLISH_ACCESS_TIME            0x39

#define TANK_CMD_ACQUIRE_SESSION_LOCK           0x40
#define TANK_CMD_ACQUIRE_SESSION_LOCK_RESPONSE  0x41
#define TANK_CMD_DOWNGRADE_SESSION_LOCK         0x42		
#define TANK_CMD_DEMAND_SESSION_LOCK            0x43
#define TANK_CMD_DENY_SESSION_LOCK              0x44
#define TANK_CMD_PUBLISH_LOCK_VERSION           0x45

#define TANK_CMD_ACQUIRE_DATA_LOCK              0x50
#define TANK_CMD_ACQUIRE_DATA_LOCK_RESPONSE     0x51
#define TANK_CMD_DOWNGRADE_DATA_LOCK            0x52
#define TANK_CMD_DEMAND_DATA_LOCK               0x53
#define TANK_CMD_DEFERRED_DOWNGRADE_DATA_LOCK   0x54
#define TANK_CMD_INVALIDATE_DIRECTORY           0x55
#define TANK_CMD_DISCARD_DIRECTORY              0x56
#define TANK_CMD_INVALIDATE_OBJ_ATTR            0x57
#define TANK_CMD_DISCARD_OBJ_ATTR               0x58
#define TANK_CMD_PUBLISH_BASIC_OBJ_ATTR         0x59

#define TANK_CMD_BLK_DISK_ALLOCATE              0x60
#define TANK_CMD_BLK_DISK_ALLOCATE_RESP         0x61
#define TANK_CMD_BLK_DISK_UPDATE                0x62
#define TANK_CMD_BLK_DISK_UPDATE_RESP           0x63
#define TANK_CMD_BLK_DISK_GET_SEGMENT           0x64
#define TANK_CMD_BLK_DISK_GET_SEGMENT_RESP      0x65

#define TANK_CMD_COMPAT_SET_RANGE_LOCK          0x70
#define TANK_CMD_COMPAT_CHECK_RANGE_LOCK        0x71
#define TANK_CMD_COMPAT_CHECK_RANGE_LOCK_RESP   0x72
#define TANK_CMD_RELEASE_ALL_RANGE_LOCKS        0x73
#define TANK_CMD_RETRY_RANGE_LOCK               0x74
#define TANK_CMD_SET_RANGE_LOCK                 0x75
#define TANK_CMD_SET_RANGE_LOCK_RESP            0x76
#define TANK_CMD_DEMAND_RANGE_LOCK              0x77
#define TANK_CMD_DEMAND_RANGE_LOCK_RESP         0x78
#define TANK_CMD_RELINQUISH_RANGE_LOCK          0x79

#define TANK_CMD_PUBLISH_CLUSTER_INFO           0x83
#define TANK_CMD_PUBLISH_LOAD_UNIT_INFO         0x84
#define TANK_CMD_GET_STORAGE_CAPACITY           0x85
#define TANK_CMD_GET_STORAGE_CAPACITY_RESPONSE  0x86
#define TANK_CMD_PUBLISH_ROOT_CLT_INFO          0x87
#define TANK_CMD_PUBLISH_EVICTION_REQUEST       0x88

/*---------- Start FlexSAN messages ----------*/

#define TANK_CMD_GET_LUN_LIST                   0x90
#define TANK_CMD_GET_LUN_LIST_RESP              0x91
#define TANK_CMD_GET_VOL_LIST                   0x92
#define TANK_CMD_GET_VOL_LIST_RESP              0x93
#define TANK_CMD_GET_LUN_INFO                   0x94
#define TANK_CMD_GET_LUN_INFO_RESP              0x95
#define TANK_CMD_GET_VOL_INFO                   0x96
#define TANK_CMD_GET_VOL_INFO_RESP              0x97
#define TANK_CMD_READ_SECTOR                    0x98
#define TANK_CMD_READ_SECTOR_RESP               0x99
#define TANK_CMD_WRITE_SECTOR                   0x9a
#define TANK_CMD_WRITE_SECTOR_RESP              0x9b
#define TANK_CMD_COPY_DATA                      0x9c
#define TANK_CMD_COPY_DATA_RESP                 0x9d
#define TANK_CMD_CANCEL                         0x9e
#define TANK_CMD_CANCEL_RESP                    0x9f
#define TANK_CMD_NULL                           0xfd
#define TANK_CMD_NULL_RESP                      0xfe

/*---------- End FlexSAN messages ----------*/
