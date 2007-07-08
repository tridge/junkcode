/* packet-stp.c
 * Routines for STP packet dissection
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <time.h>
#include <string.h>
#include <glib.h>
#include <ctype.h>
#include <epan/packet.h>
#include <epan/conversation.h>
#include "packet-stp.h"
#include <epan/strutil.h>
#include "prefs.h"

typedef void (*pull_element_t)(tvbuff_t *tvb, packet_info *pinfo,
			       proto_tree *parent_tree, struct tank_header *hdr,
			       int *var_offset, int length, int idx);

static int proto_stnl = -1;

static gint ett_stnl = -1;
static gint ett_tank = -1;
static gint ett_identify = -1;
static gint ett_identify_response = -1;
static gint ett_acquire_data_lock = -1;
static gint ett_acquire_data_lock_response = -1;
static gint ett_uoid = -1;
static gint ett_dvers = -1;
static gint ett_svers = -1;
static gint ett_gdl = -1;
static gint ett_gsl = -1;
static gint ett_srvaddr = -1;
static gint ett_netaddr = -1;
static gint ett_objattr = -1;
static gint ett_basic_objattr = -1;
static gint ett_extent = -1;
static gint ett_extent_list = -1;
static gint ett_seg_update = -1;
static gint ett_seg_descr = -1;
static gint ett_net_interface = -1;
static gint ett_access_time = -1;
static gint ett_dir_entry = -1;
static gint ett_vector = -1;
static gint ett_list = -1;
static gint ett_publish_cluster_info = -1;
static gint ett_report_txn_status = -1;
static gint ett_lookup_name = -1;
static gint ett_lookup_name_response = -1;
static gint ett_change_name = -1;
static gint ett_change_name_response = -1;
static gint ett_unknown = -1;
static gint ett_acquire_session_lock = -1;
static gint ett_acquire_session_lock_response = -1;
static gint ett_downgrade_session_lock = -1;
static gint ett_get_storage_capacity = -1;
static gint ett_get_storage_capacity_response = -1;
static gint ett_create_file = -1;
static gint ett_create_file_response = -1;
static gint ett_create_dir = -1;
static gint ett_create_dir_response = -1;
static gint ett_renew_lease = -1;
static gint ett_ping = -1;
static gint ett_shutdown = -1;
static gint ett_notify_forward = -1;
static gint ett_create_hard_link = -1;
static gint ett_create_hard_link_resp = -1;
static gint ett_create_sym_link = -1;
static gint ett_create_sym_link_resp = -1;
static gint ett_remove_name = -1;
static gint ett_remove_name_resp = -1;
static gint ett_set_basic_obj_attr = -1;
static gint ett_set_basic_obj_attr_resp = -1;
static gint ett_set_access_ctl_attr = -1;
static gint ett_set_access_ctl_attr_resp = -1;
static gint ett_update_access_time = -1;
static gint ett_publish_access_time = -1;
static gint ett_read_dir = -1;
static gint ett_read_dir_resp = -1;
static gint ett_find_object = -1;
static gint ett_find_object_resp = -1;
static gint ett_demand_session_lock = -1;
static gint ett_deny_session_lock = -1;
static gint ett_publish_lock_version = -1;
static gint ett_downgrade_data_lock = -1;
static gint ett_demand_data_lock = -1;
static gint ett_deferred_downgrade_data_lock = -1;
static gint ett_invalidate_directory = -1;
static gint ett_discard_directory = -1;
static gint ett_invalidate_obj_attr = -1;
static gint ett_discard_obj_attr = -1;
static gint ett_publish_basic_obj_attr = -1;
static gint ett_blk_disk_allocate = -1;
static gint ett_blk_disk_allocate_resp = -1;
static gint ett_blk_disk_update = -1;
static gint ett_blk_disk_update_resp = -1;
static gint ett_blk_disk_get_segment = -1;
static gint ett_blk_disk_get_segment_resp = -1;
static gint ett_set_range_lock = -1;
static gint ett_set_range_lock_resp = -1;
static gint ett_demand_range_lock = -1;
static gint ett_demand_range_lock_resp = -1;
static gint ett_relinquish_range_lock = -1;
static gint ett_retry_range_lock = -1;
static gint ett_compat_set_range_lock = -1;
static gint ett_compat_check_range_lock = -1;
static gint ett_compat_check_range_lock_resp = -1;
static gint ett_release_all_range_locks = -1;
static gint ett_publish_load_unit_info = -1;
static gint ett_publish_root_clt_info = -1;
static gint ett_publish_eviction_request = -1;

/*---------- Start FlexSAN global data ----------*/

static gint ett_lun_id = -1;
static gint ett_lun_or_vol = -1;
static gint ett_lun_info = -1;
static gint ett_vol_id = -1;
static gint ett_vol_info = -1;
static gint ett_copy_data_request = -1;
static gint ett_copy_result = -1;

/*---------- Start FlexSAN global messages ----------*/

static gint ett_get_lun_list = -1;
static gint ett_get_lun_list_resp = -1;
static gint ett_get_vol_list = -1;
static gint ett_get_vol_list_resp = -1;
static gint ett_get_lun_info = -1;
static gint ett_get_lun_info_resp = -1;
static gint ett_get_vol_info = -1;
static gint ett_get_vol_info_resp = -1;
static gint ett_read_sector = -1;
static gint ett_read_sector_resp = -1;
static gint ett_write_sector = -1;
static gint ett_write_sector_resp = -1;
static gint ett_copy_data = -1;
static gint ett_copy_data_resp = -1;
static gint ett_null = -1;
static gint ett_null_resp = -1;

/*---------- End FlexSAN messages ----------*/

/* how do we create a stream specific private data area? */
static struct stp_private stp_private;

static const gchar *tank_cmd_type(guint16 cmd_type);
static int dissect_tank_srvaddr(tvbuff_t *tvb, packet_info *pinfo, 
				proto_tree *parent_tree, int offset, 
				struct tank_header *hdr _U_);
static int dissect_tank_uoid(tvbuff_t *tvb, packet_info *pinfo, 
			     proto_tree *parent_tree, int offset, 
			     struct tank_header *hdr _U_,
			     const char *label);

#define HF_NAME(sname, field) ("tank." #sname "." #field)

#define DECLARE_FIELD(sname, field, name, type, base) \
	{ NULL, { name, HF_NAME(sname, field), type, base, NULL, 0, name, HFILL }}

static hf_register_info hf_tank_stnl[] = {
	DECLARE_FIELD(stnl, msg_type, "Message Type", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(stnl, msg_length, "Message Length", FT_UINT16, BASE_DEC),
	DECLARE_FIELD(stnl, msg_type, "Message Type", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(stnl, msg_number, "Message Number", FT_UINT64, BASE_DEC),
	DECLARE_FIELD(stnl, receiver_id, "Receiver ID", FT_UINT64, BASE_DEC),
	DECLARE_FIELD(stnl, sender_id, "Sender ID", FT_UINT64, BASE_DEC),
};

static hf_register_info hf_tank_header[] = {
	DECLARE_FIELD(header, version, "Protocol version", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(header, msg_type, "Message Type", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(header, cmd_type, "Command Type", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(header, msg_length, "Message Length", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(header, client_id, "Client ID", FT_UINT64, BASE_DEC),
	DECLARE_FIELD(header, padding, "padding", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(header, transaction_context, "Transaction Context", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_identify[] = {
	DECLARE_FIELD(identify, client_name, "Client Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify, client_platform, "Client Platform", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify, client_locale, "Client Locale", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify, locale_flag, "Locale Flag", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(identify, client_version, "Client Version", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify, client_charset, "Client Charset", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(identify, client_time, "Client Time", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(identify, client_capabilities, "Client Capabilities", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_identify_response[] = {
	DECLARE_FIELD(identify_response, rc, "Response Code", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(identify_response, server_name, "Server Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify_response, platform_name, "Platform Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify_response, software_version, "Software Version", FT_STRING, BASE_NONE),
	DECLARE_FIELD(identify_response, server_time, "Server Time", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(identify_response, capabilities, "Server Capabilities", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(identify_response, preferred_version, "Preferred Version", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(identify_response, supported_versions, "Supported Versions", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(identify_response, admin_client, "Admin Client", FT_UINT8, BASE_DEC),
};

static hf_register_info hf_tank_uoid[] = {
	DECLARE_FIELD(uoid, cluster_id, "Cluster ID", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(uoid, container_id, "Container ID", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(uoid, epoch_id, "Epoch ID", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(uoid, object_id, "Object ID", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_dvers[] = {
	DECLARE_FIELD(dvers, epoch, "Epoch", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(dvers, version, "Version", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_svers[] = {
	DECLARE_FIELD(svers, epoch, "Epoch", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(svers, version, "Version", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_acquire_data_lock[] = {
	DECLARE_FIELD(acquire_data_lock, uoid, "Unique Object ID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_data_lock, mode, "Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_data_lock, opportunistic, "Opportunistic", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(acquire_data_lock, version, "Lock Version", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_data_lock, alt_id, "Alternate ID", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_acquire_data_lock_response[] = {
	DECLARE_FIELD(acquire_data_lock_response, granted_lock, "Granted Lock", FT_BYTES, BASE_HEX),
};


static hf_register_info hf_tank_acquire_session_lock[] = {
	DECLARE_FIELD(acquire_session_lock, uoid, "Unique Object ID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_session_lock, session_lock_mode, "Session Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_session_lock, lock_version, "Lock Version", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_session_lock, data_lock_mode, "Data Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_session_lock, alt_id, "Alternate ID", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_acquire_session_lock_response[] = {
	DECLARE_FIELD(acquire_session_lock_response, session_lock, "Session Lock", FT_BYTES, BASE_HEX),
	DECLARE_FIELD(acquire_session_lock_response, not_strength_related, "Not Strength Related", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(acquire_session_lock_response, data_lock, "Data Lock", FT_BYTES, BASE_HEX),
};

static hf_register_info hf_tank_downgrade_session_lock[] = {
	DECLARE_FIELD(downgrade_session_lock, uoid, "Unique Object ID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(downgrade_session_lock, lock_mode, "Desired Lock Mode", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_extent[] = {
	DECLARE_FIELD(extent, virt_block_num, "Virtual Block Number", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(extent, block_count, "Block Count", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(extent, disk_id, "Global Disk ID", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(extent, phys_block_num, "Physcal Block Number", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_seg_update[] = {
	DECLARE_FIELD(seg_update, seg_num, "Segment Number", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(seg_update, read_state, "Read State ", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(seg_update, write_state, "Write State", FT_UINT8, BASE_HEX),

};

static hf_register_info hf_tank_seg_descr[] = {
	DECLARE_FIELD(seg_descr, seg_num, "Segment Number", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(seg_descr, read_count, "Read Count", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(seg_descr, read_state, "Read State ", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(seg_descr, write_count, "Write Count", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(seg_descr, write_state, "Write State", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_net_interface[] = {
	DECLARE_FIELD(net_interface, net_addr, "Server", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(net_interface, nic_flags, "NIC Flags", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_access_time[] = {
	DECLARE_FIELD(access_time, uoid, "Unique OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(access_time, timestamp, "Access Time", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_dir_entry[] = {
	DECLARE_FIELD(dir_entry, name_length, "Name Length", FT_UINT16, BASE_DEC),
	DECLARE_FIELD(dir_entry, uoid, "Unique OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(dir_entry, obj_type, "Object Type", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(dir_entry, server, "Server Address", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(dir_entry, key, "Object Key", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(dir_entry, name, "Name", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_vector[] = {
	DECLARE_FIELD(vector, offset, "Offset", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(vector, count, "Count", FT_UINT16, BASE_DEC),
	DECLARE_FIELD(vector, el_size, "Element Size", FT_UINT16, BASE_DEC),
};

static hf_register_info hf_tank_list[] = {
	DECLARE_FIELD(list, offset, "Offset", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(list, l_size, "List Size", FT_UINT16, BASE_DEC),
	DECLARE_FIELD(list, count, "Count", FT_UINT16, BASE_DEC),
};

static hf_register_info hf_tank_publish_cluster_info[] = {
	DECLARE_FIELD(publish_cluster_info, container_id, "Container ID", FT_UINT16, BASE_DEC),
	DECLARE_FIELD(publish_cluster_info, primary_nic_list, "Primary NICs", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(publish_cluster_info, secondary_nic_list, "Secondary NICs", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(publish_cluster_info, lease_period, "Lease Period", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(publish_cluster_info, num_retries, "Num Retries", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(publish_cluster_info, xmit_timeout, "Transmit Timeout", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(publish_cluster_info, async_period, "Async Period", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_report_txn_status[] = {
	DECLARE_FIELD(report_txn_status, rc, "Result Code", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(report_txn_status, retry_delay, "Retry Delay", FT_UINT32, BASE_DEC),
};

static hf_register_info hf_tank_basic_objattr[] = {
	DECLARE_FIELD(basic_objattr, obj_type, "Object Type", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(basic_objattr, create_time, "Create Time", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(basic_objattr, change_time, "Change Time", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(basic_objattr, access_time, "Access Time", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(basic_objattr, modify_time, "Modify Time", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(basic_objattr, misc_attr,   "Miscellaneous Attribs", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(basic_objattr, link_count,  "Link Count", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(basic_objattr, file_size,   "File Size", FT_UINT64, BASE_DEC),
	DECLARE_FIELD(basic_objattr, block_count, "Block Count", FT_UINT64, BASE_DEC),
	DECLARE_FIELD(basic_objattr, block_size,  "Block Size", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(basic_objattr, version,  "Version", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_objattr[] = {
	DECLARE_FIELD(objattr, userid,  "User ID", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(objattr, groupid,  "Group ID", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(objattr, permissions,  "Permissions", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(objattr, symlink,  "Symlink Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(objattr, sdvalue,  "SD Value", FT_STRING, BASE_HEX),
};

static hf_register_info hf_tank_gdl[] = {
	DECLARE_FIELD(gdl, mode, "Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(gdl, version, "Lock Version", FT_BYTES, BASE_HEX),
	DECLARE_FIELD(gdl, obj_attr, "Object Attributes", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(gdl, strategy, "Strategy", FT_UINT16, BASE_HEX),
};

static hf_register_info hf_tank_gsl[] = {
	DECLARE_FIELD(gsl, mode, "Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(gsl, version, "Lock Version", FT_BYTES, BASE_HEX),
};

static hf_register_info hf_tank_srvaddr[] = {
	DECLARE_FIELD(srvaddr, is_null, "Is Null", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(srvaddr, netaddr, "Network Address", FT_BYTES, BASE_HEX),
	DECLARE_FIELD(srvaddr, netport, "Network Port", FT_UINT32, BASE_DEC),
};

static hf_register_info hf_tank_netaddr[] = {
	DECLARE_FIELD(netaddr, is_legacy, "Legacy", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(netaddr, n_addr, "Address", FT_STRING, BASE_HEX),
};

static hf_register_info hf_tank_lookup_name[] = {
	DECLARE_FIELD(lookup_name, parent_uoid, "Parent OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name, name, "Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(lookup_name, case_insensitive, "Case Insensitive", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(lookup_name, session_lock_mode, "Session Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name, data_lock_mode, "Data Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name, timestamp, "Timestamp", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_lookup_name_response[] = {
	DECLARE_FIELD(lookup_name_response, uoid, "Unique OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name_response, obj_type, "Object Type", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name_response, server, "Server Address", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name_response, key, "Object Key", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(lookup_name_response, name, "Actual Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(lookup_name_response, granted, "Granted Session Lock", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(lookup_name_response, not_strength_related, "NotStrengthRelated", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(lookup_name_response, gdl, "Granted Data Lock", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_change_name[] = {
	DECLARE_FIELD(change_name, src_parent_uoid, "Source Parent OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name, src_name, "Source Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(change_name, dest_parent_uoid, "Dest Parent OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name, dest_name, "Dest Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(change_name, dest_case_insensitive, "Dest Case Insensitive", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(change_name, timestamp, "Timestamp", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(change_name, exclusive, "Exclusive", FT_UINT8, BASE_DEC),
};

static hf_register_info hf_tank_change_name_response[] = {
	DECLARE_FIELD(change_name_response, src_parent_attrib, "Source Parent Attrib", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, dest_parent_attrib, "Dest Parent Attrib", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, src_child_uoid, "Source Child OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, src_child_type, "Source Child Type", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, src_child_attrib, "Source Child Attrib", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, dest_child_existed, "Dest Child Existed", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(change_name_response, dest_child_uoid, "Dest Child OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, dest_child_attrib, "Dest Child Attrib", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(change_name_response, dest_actual_name, "Dest Actual Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(change_name_response, dest_key, "Dest Key", FT_UINT64, BASE_HEX),	
};

static hf_register_info hf_tank_get_storage_capacity[] = {
	DECLARE_FIELD(get_storage_capacity, uoid, "Unique OID", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_get_storage_capacity_response[] = {
	DECLARE_FIELD(get_storage_capacity_response, total_blocks, "Total Blocks", FT_UINT64, BASE_DEC),
	DECLARE_FIELD(get_storage_capacity_response, free_blocks, " Free Blocks", FT_UINT64, BASE_DEC),
};

static hf_register_info hf_tank_create_file[] = {
	DECLARE_FIELD(create_file, parent_uoid, "Parent OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file, name, "Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(create_file, case_insensitive, "Case Insensitive", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_file, timestamp, "Timestamp", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(create_file, userid, "User ID", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(create_file, groupid, "Group ID", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(create_file, permissions, "Permissions", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(create_file, misc_attr, "Miscellaneous Attributes", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(create_file, quality_of_service, "Quality of Service", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(create_file, exclusive, "Exclusive", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_file, session_lock_mode, "Session Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file, data_lock_mode, "Data Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file, sdvalue, "SD Value", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_create_file_response[] = {
	DECLARE_FIELD(create_file_response, existed, "Existed", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_file_response, uoid, "Unique OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file_response, dir_key, "Directory Key", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(create_file_response, server, "Server", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file_response, parent_attr, "Parrent Attributes", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file_response, actual_name, "Actual Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(create_file_response, session_lock, "Session Lock", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_file_response, not_strength_related, "Not Strength Related", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_file_response, data_lock, "Data Lock", FT_UINT8, BASE_HEX),	
};

static hf_register_info hf_tank_create_dir[] = {
	DECLARE_FIELD(create_dir, parent_uoid, "Parent OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir, name, "Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(create_dir, case_insensitive, "Case Insensitive", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_dir, timestamp, "Timestamp", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(create_dir, userid, "User ID", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(create_dir, groupid, "Group ID", FT_UINT32, BASE_DEC),
	DECLARE_FIELD(create_dir, permissions, "Permissions", FT_UINT16, BASE_HEX),
	DECLARE_FIELD(create_dir, misc_attr, "Miscellaneous Attributes", FT_UINT32, BASE_HEX),
	DECLARE_FIELD(create_dir, exclusive, "Exclusive", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_dir, session_lock_mode, "Session Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir, data_lock_mode, "Data Lock Mode", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir, sdvalue, "SD Value", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_create_dir_response[] = {
	DECLARE_FIELD(create_dir_response, existed, "Existed", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_dir_response, uoid, "Unique OID", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir_response, dir_key, "Directory Key", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(create_dir_response, server, "Server", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir_response, parent_attr, "Parrent Attributes", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir_response, dot_key, "Dot Key", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(create_dir_response, dot_dot_key, "DotDot Key", FT_UINT64, BASE_HEX),
	DECLARE_FIELD(create_dir_response, actual_name, "Actual Name", FT_STRING, BASE_NONE),
	DECLARE_FIELD(create_dir_response, session_lock, "Session Lock", FT_UINT8, BASE_HEX),
	DECLARE_FIELD(create_dir_response, not_strength_related, "Not Strength Related", FT_UINT8, BASE_DEC),
	DECLARE_FIELD(create_dir_response, data_lock, "Data Lock", FT_UINT8, BASE_HEX),	
};

static hf_register_info hf_tank_renew_lease[] = {
  DECLARE_FIELD(renew_lease, timestamp, "Timestamp", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_ping[] = {
  DECLARE_FIELD(ping, timestamp, "Client Timestamp", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_shutdown[] = {
};

static hf_register_info hf_tank_notify_forward[] = {
  DECLARE_FIELD(notify_forward, server, "Forwarding Server Address", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(notify_forward, load_unit_id, "Load Unit Being Moved", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_create_hard_link[] = {
  DECLARE_FIELD(create_hard_link, parent_dir_uoid, "Parent OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_hard_link, name, "Link Name.", FT_STRING, BASE_NONE),
  DECLARE_FIELD(create_hard_link, case_insensitive, "Case Insensitive", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(create_hard_link, exclusive, "Exclusive Name Creation", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(create_hard_link, extant_uoid, "Link Target OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_hard_link, timestamp, "Client Timestamp", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_create_hard_link_resp[] = {
  DECLARE_FIELD(create_hard_link_resp, key, "Assigned Search Key", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(create_hard_link_resp, existed, "Target Name Already Existed", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(create_hard_link_resp, parent_attr, "Updated Parent Basic Attributes", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_hard_link_resp, extant_attr, "Updated Target Basic Attributes", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_hard_link_resp, actual_name, "Actual Name", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_create_sym_link[] = {
  DECLARE_FIELD(create_sym_link, parent_dir_uoid, "Parent OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_sym_link, name, "Link Name", FT_STRING, BASE_NONE),
  DECLARE_FIELD(create_sym_link, path_string, "Link Value", FT_STRING, BASE_NONE),
  DECLARE_FIELD(create_sym_link, timestamp, "Client Timestamp", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(create_sym_link, owner_id, "Owner ID", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(create_sym_link, group_id, "Group ID", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(create_sym_link, permissions, "Permission Bits", FT_UINT16, BASE_HEX),
  DECLARE_FIELD(create_sym_link, misc_attr, "Misc Object Attributes", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(create_sym_link, stsd_val, "STSD Value", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_create_sym_link_resp[] = {
  DECLARE_FIELD(create_sym_link_resp, uoid, "Unique OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_sym_link_resp, key, "Assigned Search Key.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(create_sym_link_resp, server, "Net Address Of Server For Object", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_sym_link_resp, parent_attr, "Updated Parent Basic Attributes", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(create_sym_link_resp, granted_session_lock, "Granted Session Lock State", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_remove_name[] = {
  DECLARE_FIELD(remove_name, parent_dir_uoid, "Parent OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(remove_name, name, "Name Remove", FT_STRING, BASE_NONE),
  DECLARE_FIELD(remove_name, timestamp, "Client Timestamp", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_remove_name_resp[] = {
  DECLARE_FIELD(remove_name_resp, parent_attr, "Updated Parent Attributes", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(remove_name_resp, child_uoid, "Child UOID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(remove_name_resp, child_attr, "Updated Child Attributes", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_set_basic_obj_attr[] = {
  DECLARE_FIELD(set_basic_obj_attr, uoid, "Unique OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, which, "Flags: Basic Attributes To Set", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, create_time, "Creation Time", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, change_time, "Change Time", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, access_time, "Access Time", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, modify_time, "Modification Time", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, misc_attr, "New Misc Attributes", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(set_basic_obj_attr, timestamp, "Client Timestamp", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_set_basic_obj_attr_resp[] = {
  DECLARE_FIELD(set_basic_obj_attr_resp, obj_attr, "Updated Basic Attributes", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_set_access_ctl_attr[] = {
  DECLARE_FIELD(set_access_ctl_attr, uoid, "Unique OID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_access_ctl_attr, which, "Flags: Control Attributes To Set.", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(set_access_ctl_attr, owner_id, "New Owner ID", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(set_access_ctl_attr, group_id, "New Group ID", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(set_access_ctl_attr, permissions, "Permission Bits", FT_UINT16, BASE_HEX),
  DECLARE_FIELD(set_access_ctl_attr, timestamp, "Client Timestamp", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_access_ctl_attr, stsd_val, "New STSD Value", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_set_access_ctl_attr_resp[] = {
  DECLARE_FIELD(set_access_ctl_attr_resp, obj_attr, "Updated Control Attributes.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_update_access_time[] = {
  DECLARE_FIELD(update_access_time, changes, "List of Access Time Updates", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_publish_access_time[] = {
  DECLARE_FIELD(publish_access_time, changes, "List of Access Time Updates", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_read_dir[] = {
  DECLARE_FIELD(read_dir, dir_uoid, "Directory's unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(read_dir, restart_key, "Search key to be used to restart directory scan.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_read_dir_resp[] = {
  DECLARE_FIELD(read_dir_resp, parent_attr, "Updated basic attributes for the parent directory object.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(read_dir_resp, end_reached, "True if the end of the set of directory entries has been reached.", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(read_dir_resp, is_complete, "True if this (single) response message contains the complete set of directory entries.", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(read_dir_resp, entry_list, "The next set of directory entries returned. A list of varying-length structures described by stpDirectoryEntry.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_find_object[] = {
  DECLARE_FIELD(find_object, uoid, "Unique object id to be locked.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(find_object, session_lock_mode, "Requested session lock mode.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(find_object, data_lock_mode, "Requested data lock mode. (opportunistic).", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_find_object_resp[] = {
  DECLARE_FIELD(find_object_resp, obj_type, "Object type code.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(find_object_resp, server, "Network address of server that serves this object.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(find_object_resp, granted_session_lock, "Granted session lock state.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(find_object_resp, not_strength_related, "Set to stpBoolean_True if the client already holds the session lock and the requested lock was not strength-related to this lock. The currently- held lock is indicated in folr_grantedSessionLock.", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(find_object_resp, granted_data_lock, "Granted data lock (if any)", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_demand_session_lock[] = {
  DECLARE_FIELD(demand_session_lock, uoid, "Unique object identifier.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(demand_session_lock, lock_mode, "Required lock mode.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(demand_session_lock, demand_flag, "Demand flags", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_deny_session_lock[] = {
  DECLARE_FIELD(deny_session_lock, uoid, "Unique object identifier.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_publish_lock_version[] = {
  DECLARE_FIELD(publish_lock_version, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(publish_lock_version, lock_version, "New version of the session lock.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_downgrade_data_lock[] = {
  DECLARE_FIELD(downgrade_data_lock, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(downgrade_data_lock, lock_mode, "Desired new lock mode; if set to stpDataLockMode_None then client is actually releasing the lock.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(downgrade_data_lock, access_time_valid, "Set to stpBoolean_True if dl_accessTime contains an access time.", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(downgrade_data_lock, access_time, "Estimated last access timestamp.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_demand_data_lock[] = {
  DECLARE_FIELD(demand_data_lock, uoid, "Unique object identifier.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(demand_data_lock, lock_mode, "Required lock mode.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(demand_data_lock, opportunistic, "If true, then demand may be deferred. Otherwise client has to downgrade lock.", FT_UINT8, BASE_DEC),
};

static hf_register_info hf_tank_deferred_downgrade_data_lock[] = {
  DECLARE_FIELD(deferred_downgrade_data_lock, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(deferred_downgrade_data_lock, obj_attr, "Current attributes of object except Symlink Value and ACL.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_invalidate_directory[] = {
  DECLARE_FIELD(invalidate_directory, uoid, "Unique object identifier of dir.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(invalidate_directory, removed_name, "Name to be removed from the cache if a directory entry has been removed.  The empty string if no name is being removed.", FT_STRING, BASE_NONE),
  DECLARE_FIELD(invalidate_directory, dir_attr, "Updated basic object attributes for the directory.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(invalidate_directory, unlinked, "If name was removed, did an object become unlinked (link count became zero)?", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(invalidate_directory, unlinked_oid, "OID of object that has become unlinked (link count zero).", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_discard_directory[] = {
  DECLARE_FIELD(discard_directory, uoid, "Unique object identifier of dir.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_invalidate_obj_attr[] = {
  DECLARE_FIELD(invalidate_obj_attr, uoid, "Unique object identifier.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_discard_obj_attr[] = {
  DECLARE_FIELD(discard_obj_attr, uoid, "Unique object identifier.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_publish_basic_obj_attr[] = {
  DECLARE_FIELD(publish_basic_obj_attr, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(publish_basic_obj_attr, basic_attr, "Basic object attributes.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_blk_disk_allocate[] = {
  DECLARE_FIELD(blk_disk_allocate, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(blk_disk_allocate, block_offset, "Block offset of virtual block range to be allocated.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_allocate, block_count, "Number of contiguous blocks in virtual block range.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_allocate, more_block_count, "Hint additional number of blocks to be allocated.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_blk_disk_allocate_resp[] = {
  DECLARE_FIELD(blk_disk_allocate_resp, obj_attr, "Updated basic attributes of the object.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(blk_disk_allocate_resp, seg_list, "List of updated segment descriptors. A list of varying- length structures, each described by stpBlkDiskSegmentDescr.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_blk_disk_update[] = {
  DECLARE_FIELD(blk_disk_update, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, harden, "True if client requests the update to be made persistent as part of this transaction.", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(blk_disk_update, seg_update_list, "Segment Updates", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, new_lock_mode, "If set to a weaker mode than stpDataLockMode_Exclusive then the associated data lock will be downgraded as a side-effect of a successful update.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, ba_which, "Indicates which basic attributes are to be set.", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, acl_which, "Indicates which access control attributes are to be set.", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, create_time, "New creation timestamp value.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, change_time, "New last change timestamp value.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, access_time, "New last access timestamp value.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, modify_time, "New last modification timestamp value.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, misc_attr, "New misc. object attributes.", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, file_size, "New value for file size.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, owner_id, "New owner id value to be set.", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(blk_disk_update, group_id, "New group id value to be set.", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(blk_disk_update, permissions, "New permissions value.", FT_UINT16, BASE_HEX),
  DECLARE_FIELD(blk_disk_update, stsd_val, "New stsdVal to be set.", FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_blk_disk_update_resp[] = {
  DECLARE_FIELD(blk_disk_update_resp, obj_attr, "Updated basic object attributes.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_blk_disk_get_segment[] = {
  DECLARE_FIELD(blk_disk_get_segment, uoid, "Unique object id.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(blk_disk_get_segment, seg_no, "Segment number for required segment descriptor.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(blk_disk_get_segment, more_seg_count, "Hint additional number of segment descriptors (beginning at gs_segNo) to be returned.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_blk_disk_get_segment_resp[] = {
  DECLARE_FIELD(blk_disk_get_segment_resp, seg_list, "Returned list of updated segment descriptors. A list of varying- length structures, each described by stpBlkDiskSegmentDescr.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_set_range_lock[] = {
  DECLARE_FIELD(set_range_lock, uoid, "object ID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_range_lock, owner_id, "process ID", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock, start, "start of range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock, length, "length of range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock, mode, "requested lock mode", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_range_lock, retry_id, "thread waiting for lock", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock, alt_id, "previous client ID", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock, reassert_version, "lk version previously held", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_range_lock, flags, "request flags", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_set_range_lock_resp[] = {
  DECLARE_FIELD(set_range_lock_resp, owner_client, "this or conflicting client", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, owner_id, "this or conflicting PID", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, start, "start of range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, length, "length of range (not zero)", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, mode, "granted lock mode", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, reassert_version, "needed for reassertion", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, flags, "indicates what was granted", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(set_range_lock_resp, rc, "result code (see above)", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_demand_range_lock[] = {
  DECLARE_FIELD(demand_range_lock, uoid, "object ID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(demand_range_lock, txn_id, "pass back to server in response", FT_UINT64, BASE_DEC),
  DECLARE_FIELD(demand_range_lock, start, "start of demanded range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(demand_range_lock, length, "length of demanded range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(demand_range_lock, mode, "mode with which must be compatible", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_demand_range_lock_resp[] = {
  DECLARE_FIELD(demand_range_lock_resp, uoid, "object ID", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(demand_range_lock_resp, txn_id, "identifies demand", FT_UINT64, BASE_DEC),
  DECLARE_FIELD(demand_range_lock_resp, start, "start of response range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(demand_range_lock_resp, length, "length of response range", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(demand_range_lock_resp, mode, "mode with which client is compat.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_relinquish_range_lock[] = {
  DECLARE_FIELD(relinquish_range_lock, uoid, "", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(relinquish_range_lock, owner_id, "", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(relinquish_range_lock, start, "", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(relinquish_range_lock, length, "", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(relinquish_range_lock, mode, "", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_retry_range_lock[] = {
  DECLARE_FIELD(retry_range_lock, retry_id, "Retry identifier.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_compat_set_range_lock[] = {
  DECLARE_FIELD(compat_set_range_lock, uoid, "Unique object id of file.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(compat_set_range_lock, owner_id, "Local owner of lock at client.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_set_range_lock, lock_mode, "New lock mode to be set.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(compat_set_range_lock, range_start, "Offset in file of first byte of range.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_set_range_lock, range_length, "Length of range to be locked or unlocked; zero represents largest possible range.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_set_range_lock, wait_for_lock, "Set to stpBoolean_True if txn intends to wait for the lock to become available. Ignored if lock is being released.", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(compat_set_range_lock, retry_id, "Identifier to be registered if the lock is not immediately available and if \"waitForLock\" was specified. Ignored if not waiting for lock or if lock is being released.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_compat_check_range_lock[] = {
  DECLARE_FIELD(compat_check_range_lock, uoid, "Unique object id of file.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock, owner_id, "Local owner of lock at client.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock, lock_mode, "Desired lock mode.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock, range_start, "Offset in file of first byte of range.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock, range_length, "Length of range; zero represents largest possible range.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_compat_check_range_lock_resp[] = {
  DECLARE_FIELD(compat_check_range_lock_resp, net_addr, "Network address of client that holds the lock.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock_resp, owner_id, "Local owner at holding client.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock_resp, lock_mode, "Held lock mode.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock_resp, range_start, "Offset in file of first byte of conflicting range.", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(compat_check_range_lock_resp, range_length, "Length of conflicting range; zero represents maximum possible range.", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_release_all_range_locks[] = {
  DECLARE_FIELD(release_all_range_locks, uoid, "Unique object id of file.", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_publish_load_unit_info[] = {
  DECLARE_FIELD(publish_load_unit_info, load_unit_id, "Load unit affected.", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(publish_load_unit_info, server_addr, "Server serving load unit, if and only if state is online.", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(publish_load_unit_info, load_unit_state, "Offline or online.", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_publish_root_clt_info[] = {
  DECLARE_FIELD(publish_root_clt_info, root_clt_flag, "Specifies whether this clt is a root/admin client", FT_UINT8, BASE_DEC),
};

static hf_register_info hf_tank_publish_eviction_request[] = {
  DECLARE_FIELD(publish_eviction_request, batch_size, "Number of objects that the server would like the client to evict.", FT_UINT32, BASE_DEC),
};

/*---------- Start FlexSAN data GUI mappings ----------*/

/* Map the lun_t to ett_lun to the gui elements */
static hf_register_info hf_tank_lun_id[] = {
  DECLARE_FIELD(lun_id, lun_id_format, "Lun ID Format", FT_STRING, BASE_NONE),
  DECLARE_FIELD(lun_id, lun_id_length, "Lun ID Length", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(lun_id, lun_id_value,  "Lun ID Value",  FT_STRING, BASE_NONE),
};

static hf_register_info hf_tank_lun_or_vol[] = {
  DECLARE_FIELD(lun_or_vol, is_volume, "Is Volume?", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(lun_or_vol, lv_device.lv_vol, "Volume GID 1", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_lun_info[] = {
  DECLARE_FIELD(lun_info, state, "State", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(lun_info, access, "Access", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(lun_info, vendor_id, "Vendor Id", FT_STRING, BASE_NONE),
  DECLARE_FIELD(lun_info, product_id, "Product ID", FT_STRING, BASE_NONE),
  DECLARE_FIELD(lun_info, product_version, "Product Version", FT_STRING, BASE_NONE),
  DECLARE_FIELD(lun_info, id_format, "ID Format", FT_STRING, BASE_NONE),
  DECLARE_FIELD(lun_info, blk_size, "Block Size", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(lun_info, capacity, "Capacity", FT_UINT64, BASE_DEC),

//  DECLARE_FIELD(lun_info, volume, "", ),
  DECLARE_FIELD(lun_info, vol_gid, "Volume GID 2", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(lun_info, vol_owner_id, "Volume Owner Id", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(lun_info, vol_install_id, "Volume Installation Id", FT_UINT64, BASE_HEX),

  DECLARE_FIELD(lun_info, id_length, "Lun Id Length", FT_UINT32, BASE_DEC),
  DECLARE_FIELD(lun_info, local_name_length, "Local Name Length", FT_UINT32, BASE_DEC),
};

static hf_register_info hf_tank_vol_id[] = {
  DECLARE_FIELD(vol_id, gid, "Volume GID", FT_UINT64, BASE_HEX),
};

static hf_register_info hf_tank_vol_info[] = {
  DECLARE_FIELD(lun_info, vi_state, "State", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(lun_info, vi_gid, "Volume GID", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(lun_info, vi_owner_id, "Owner Id", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(lun_info, vi_install_id, "Installation Id", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(lun_info, vi_capacity, "Capacity (in bytes)", FT_UINT64, BASE_DEC),
};

  static hf_register_info hf_tank_copy_data_request[] = {
  DECLARE_FIELD(copy_data_request, source_gid,    "Source disk GID", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(copy_data_request, source_start,  "Source start block", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(copy_data_request, dest_gid,      "Destination disk GID", FT_UINT64, BASE_HEX),
  DECLARE_FIELD(copy_data_request, dest_start,    "Destination start block", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(copy_data_request, block_count,   "Block count", FT_UINT32, BASE_HEX),
};

static hf_register_info hf_tank_copy_result[] = {
  DECLARE_FIELD(copy_result, blocks_written, "Blocks written", FT_UINT32, BASE_HEX),
  DECLARE_FIELD(copy_result, result, "Result", FT_UINT8, BASE_HEX),
};

/*---------- Start FlexSAN message GUI mappings ----------*/

static hf_register_info hf_tank_get_lun_list[] = {
  DECLARE_FIELD(get_lun_list, discover, "Discover (rescan) flag", FT_UINT8, BASE_DEC),
};

static hf_register_info hf_tank_get_lun_info[] = {
  DECLARE_FIELD(get_lun_info, discover, "Discover (rescan) flag", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(get_lun_info, lun_list, "List of stpLun structures", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_get_lun_info_resp[] = {
  DECLARE_FIELD(get_lun_info_resp, more, "More data flag", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(get_lun_info_resp, lun_list, "List of stpLunInfo structures", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_get_vol_list[] = {
  DECLARE_FIELD(get_vol_list, discover, "Discover (rescan) flag", FT_UINT8, BASE_DEC),
};

static hf_register_info hf_tank_get_vol_info[] = {
  DECLARE_FIELD(get_vol_info, discover, "Discover (rescan) flag", FT_UINT8, BASE_DEC),
  DECLARE_FIELD(get_vol_info, vol_list, "List of stpVol structures", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_get_vol_info_resp[] = {
  DECLARE_FIELD(get_vol_info_resp, more, "More data flag", FT_UINT8, BASE_HEX),
  DECLARE_FIELD(get_vol_info_resp, vol_list, "Vector of stpVolInfo structures", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_read_sector[] = {
  DECLARE_FIELD(read_sector, offset, "Reserved sector number", FT_UINT32, BASE_DEC),
//  DECLARE_FIELD(read_sector, device_name, "", ,),
};

static hf_register_info hf_tank_read_sector_resp[] = {
  DECLARE_FIELD(read_sector_resp, result_code, "Result", FT_UINT32, BASE_DEC),
//  DECLARE_FIELD(read_sector_resp, payload_[_ stp_sector_size_ _], "", char, bogus),
};

static hf_register_info hf_tank_write_sector[] = {
  DECLARE_FIELD(write_sector, offset, "Reserved sector number", FT_UINT32, BASE_DEC),
//  DECLARE_FIELD(write_sector, payload_[_ stp_sector_size_ _], "", char, bogus),
//  DECLARE_FIELD(write_sector, device_name, "", stpLunOrVolName, bogus),
};

static hf_register_info hf_tank_write_sector_resp[] = {
  DECLARE_FIELD(write_sector_resp, result_code, "Result (0 == success)", FT_UINT32, BASE_DEC),
};

static hf_register_info hf_tank_copy_data[] = {
  DECLARE_FIELD(copy_data, copy_data_list, "Copy Data requests", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_copy_data_resp[] = {
  DECLARE_FIELD(copy_data_resp, copy_result_list, "Copy Data results", FT_UINT8, BASE_HEX),
};

static hf_register_info hf_tank_null[] = {
};

static hf_register_info hf_tank_null_resp[] = {
};

/*---------- End FlexSAN message GUI mappings ----------*/

/*
  find a field handle given the registered field info
*/
static int hf_id(hf_register_info *hf, const char *name)
{
	int i;
	for (i=0;hf[i].p_id;i++) {
		if (strcmp(name, hf[i].hfinfo.abbrev) == 0) {
			return *(hf[i].p_id);
		}
	}
	fprintf(stderr, __FILE__ " - ERROR: No hf_id for %s !\n", name);
	return -1;
}

#define HF(sname, field) hf_id(hf_tank_ ## sname, HF_NAME(sname, field))

#define PULL_NAMESTRING(sname, field) do { \
	pkt.field = pull_namestring(sinfo, tree, HF(sname, field), tvb, offset, &var_offset); \
	offset += 8; \
} while (0)

#define PULL_UNICODE(sname, field) do { \
	pkt.field = pull_ascii(sinfo, tree, HF(sname, field), tvb, offset, &var_offset); \
	offset += 8; \
} while (0)

#define PULL_ASCII(sname, field) do { \
	pkt.field = pull_ascii(sinfo, tree, HF(sname, field), tvb, offset, &var_offset); \
	offset += 8; \
} while (0)

#define PULL_ARBSTRING(sname, field) do { \
	pkt.field = pull_arbstring(tree, HF(sname, field), tvb, offset, &var_offset); \
	offset += 8; \
} while (0)

#define PULL_UINT16_ARRAY(sname, field) do { \
	pkt.field = pull_uint16_array(tree, HF(sname, field), tvb, offset, &var_offset); \
	offset += 8; \
} while (0)

#define PULL_TIME(sname, field) do { \
	pkt.field = tvb_get_ntoh64(tvb, offset); \
	proto_tree_add_item(tree, HF(sname, field), tvb, offset, 8, FALSE); \
	offset += 8; \
} while (0)

#define PULL_UINT8(sname, field) do { \
	pkt.field = tvb_get_guint8(tvb, offset); \
	proto_tree_add_uint(tree, HF(sname, field), tvb, offset, 1, pkt.field); \
	offset += 1; \
} while (0)

#define PULL_UINT16(sname, field) do { \
	pkt.field = tvb_get_ntohs(tvb, offset); \
	proto_tree_add_uint(tree, HF(sname, field), tvb, offset, 2, pkt.field); \
	offset += 2; \
} while (0)

#define PULL_UINT32(sname, field) do { \
	pkt.field = tvb_get_ntohl(tvb, offset); \
	proto_tree_add_uint(tree, HF(sname, field), tvb, offset, 4, pkt.field); \
	offset += 4; \
} while (0)

#define PULL_UINT64(sname, field) do { \
	pkt.field = tvb_get_ntoh64(tvb, offset); \
	proto_tree_add_item(tree, HF(sname, field), tvb, offset, 8, FALSE); \
	offset += 8; \
} while (0)

#define PULL_CHAR_ARRAY(sname, field, name, length) do { \
	proto_tree_add_text(tree, tvb, offset, length, name ": %s", \
			    tvb_bytes_to_str(tvb, offset, length)); \
	offset += length; \
} while (0)

#define PULL_UOID(sname, label) do { \
	offset = dissect_tank_uoid(tvb, pinfo, tree, offset, hdr, label); \
} while (0)

#define PULL_DVERS(sname) do { \
	offset = dissect_tank_dvers(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_SVERS(sname) do { \
	offset = dissect_tank_svers(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_GDL(sname) do { \
	offset = dissect_tank_gdl(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_GSL(sname) do { \
	offset = dissect_tank_gsl(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_SRVADDR(sname) do { \
	offset = dissect_tank_srvaddr(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_NETADDR(sname) do { \
	offset = dissect_tank_netaddr(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_OBJATTR(sname) do { \
	offset = dissect_tank_objattr(tvb, pinfo, tree, offset, hdr); \
} while (0)

#define PULL_BASIC_OBJATTR(sname, label) do { \
	offset = dissect_tank_basic_objattr(tvb, pinfo, tree, offset, hdr, label); \
} while (0)

#define PULL_VECTOR(sname, type, label) do { \
	offset = pull_vector(tvb, pinfo, tree, offset, hdr, &var_offset, label, pull_element_ ## type); \
} while (0)

#define PULL_LIST(sname, type, label) do { \
	offset = pull_list(tvb, pinfo, tree, offset, hdr, &var_offset, label, pull_element_ ## type); \
} while (0)


/*---------- Start FlexSAN pull macros ----------*/

#define PULL_STRING(sname, type, length ) do { \
        p = tvb_get_string(tvb, offset, length); \
	proto_tree_add_string(tree, HF(sname, type), tvb, offset, length, p ); \
        offset += length; \
} while (0)

#define PULL_LUN_ID(sname, type, label) do { \
	offset = pull_lun_id( tvb, pinfo, tree, hdr, offset, label ); \
} while (0)

#define PULL_LUN_OR_VOL(sname, type, label) do { \
	offset = pull_lun_or_vol( tvb, pinfo, tree, hdr, offset, label ); \
} while (0)

#define PULL_VOL_ID(sname, type, label) do { \
	offset = pull_vol_id( tvb, pinfo, tree, hdr, offset, label ); \
} while (0)

/*---------- End FlexSAN pull macros ----------*/

/*
  pull a 64bit integer from the wire in network byte order
*/
static guint64 tvb_get_ntoh64(tvbuff_t *tvb, int offset)
{
	guint64 v;
	v = tvb_get_ntohl(tvb, offset);
	v <<= 32;
	v |= tvb_get_ntohl(tvb, offset+4);
	return v;
}

/*
  the STNL message types
 */
static const value_string stnl_msg_types[] = {
	{ 0x01, "Reliable Message" },
	{ 0x02, "ACK" },
	{ 0x03, "NACK" },
	{ 0x04, "Unreliable Message" },
	{ 0x00, NULL}
};

static const gchar *stnl_msg_type(guint16 msg_type)
{
	int i;
	for (i=0;stnl_msg_types[i].strptr;i++) {
		if (stnl_msg_types[i].value == msg_type) {
			return stnl_msg_types[i].strptr;
		}
	}
	return "INVALID-MSG-TYPE";
}

/*
  the TANK message types
 */
static const value_string tank_msg_types[] = {
	{ 0x8, "Transaction" },
	{ 0x4, "Response" },
	{ 0x2, "Demand"},
	{ 0x1, "Immediate"},
	{ 0x0, "Control"},
	{ 0, NULL}
};

static const gchar *tank_msg_type(guint16 msg_type)
{
	int i;
	for (i=0;tank_msg_types[i].strptr;i++) {
		if (tank_msg_types[i].value == msg_type) {
			return tank_msg_types[i].strptr;
		}
	}
	return "INVALID-MSG-TYPE";
}



/*
  pull a unicode string from a packet
*/
static const guint8 *pull_unicode(struct stp_private *sinfo _U_, 
				  proto_tree *tree, int hid, tvbuff_t *tvb, 
				  int offset, int *var_offset)
{
	int s_ofs, s_len;
	const guint8 *p = "";

	s_ofs = tvb_get_ntohl(tvb, offset);
	s_len = tvb_get_ntohl(tvb, offset+4);

	if (!tvb_bytes_exist(tvb, s_ofs, s_len)) {
		p = "ERR: INVALID STRING";
		proto_tree_add_text(tree, tvb, offset, 8, p);
		return p;
	}

	if (s_len > 0) {
		p = tvb_fake_unicode(tvb, s_ofs, s_len, FALSE);
		s_len *= 2;
	}

	proto_tree_add_string(tree, hid, tvb, s_ofs, s_len, p);

	if (s_len > 0 && s_ofs + s_len > (*var_offset)) {
		(*var_offset) = s_ofs + s_len;
	}

	return p;
}


/*
  pull an ascii string from a packet
*/
static const guint8 *pull_ascii(struct stp_private *sinfo _U_, 
				proto_tree *tree, int hid, tvbuff_t *tvb, 
				int offset, int *var_offset)
{
	int s_ofs, s_len;
	const guint8 *p = "";

	s_ofs = tvb_get_ntohl(tvb, offset);
	s_len = tvb_get_ntohl(tvb, offset+4);

	if (!tvb_bytes_exist(tvb, s_ofs, s_len)) {
		p = "ERR: INVALID STRING";
		proto_tree_add_text(tree, tvb, offset, 8, p);
		return p;
	}

	if (s_len > 0) {
		p = tvb_get_string(tvb, s_ofs, s_len);
	}

	if (s_len > 0 && p[0] == 0) {
		/* probably unicode */
		return pull_unicode(sinfo, tree, hid, tvb, offset, var_offset);
	}

	proto_tree_add_string(tree, hid, tvb, s_ofs, s_len, p);

	if (s_len > 0 && s_ofs + s_len > (*var_offset)) {
		(*var_offset) = s_ofs + s_len;
	}

	return p;
}


/*
  pull an ascii or unicode string from a packet
*/
static const guint8 *pull_namestring(struct stp_private *sinfo, 
				     proto_tree *tree, int hid, tvbuff_t *tvb, 
				     int offset, int *var_offset)
{
//	if (sinfo->unicode) {
//		return pull_unicode(sinfo, tree, hid, tvb, offset, var_offset);
//	}
	return pull_ascii(sinfo, tree, hid, tvb, offset, var_offset);
}


/*
  pull an opaque string from a packet
*/
static const guint8 *pull_arbstring(proto_tree *tree, int hid, tvbuff_t *tvb, 
				    int offset, int *var_offset)
{
	int s_ofs, s_len;
	const guint8 *p;

	s_ofs = tvb_get_ntohl(tvb, offset);
	s_len = tvb_get_ntohl(tvb, offset+4);

	p = tvb_get_string(tvb, s_ofs, s_len);
	proto_tree_add_string(tree, hid, tvb, s_ofs, s_len, p);

	if (s_ofs + s_len > (*var_offset)) {
		(*var_offset) = s_ofs + s_len;
	}

	return p;
}

/*
  pull an array of uint16s from a packet
*/
static uint16_array_t pull_uint16_array(proto_tree *tree, int hid, tvbuff_t *tvb, 
				       int offset, int *var_offset)
{
	int s_ofs, s_len, i;
	uint16_array_t a;

	s_ofs = tvb_get_ntohl(tvb, offset);
	s_len = tvb_get_ntohl(tvb, offset+4);

	tvb_ensure_bytes_exist(tvb, *var_offset, 2*s_len);

	a.count = s_len;
	a.values = g_malloc(2*s_len);

	for (i=0;i<s_len;i++) {
		a.values[i] = tvb_get_ntohs(tvb, *var_offset);
		proto_tree_add_uint(tree, hid, tvb, *var_offset, 2, a.values[i]);
		(*var_offset) += 2;
	}

	return a;
}


/*
  pull a vector element of type segment update from a packet
*/
static void pull_element_seg_update(tvbuff_t *tvb, packet_info *pinfo,
				    proto_tree *parent_tree,
				    struct tank_header *hdr, int *var_offset,
				    int length, int idx)
{
	seg_update_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, length,
				   "Segment Update %d", idx);
	tree = proto_item_add_subtree(item, ett_seg_update);

	PULL_UINT64(seg_update, seg_num);
	PULL_CHAR_ARRAY(seg_update, read_state, "Read State", 32);
	PULL_CHAR_ARRAY(seg_update, write_state, "Write State", 32);

	(*var_offset) += length;
}


/*
  extract the extent array from a segment descriptor
*/
static int get_extents(tvbuff_t *tvb, proto_tree *parent_tree,
		       int offset, char *label, int count)
{
	extent_t pkt;
	proto_item *item = NULL;
	proto_tree *subtree = NULL;
	proto_tree *tree = NULL;
	int i;

	item = proto_tree_add_text(parent_tree, tvb, offset, count * 14,
				   "%ss[%d]", label, count);
	subtree = proto_item_add_subtree(item, ett_extent_list);

	for ( i = 0; i < count; i++) {
		item = proto_tree_add_text(subtree, tvb, offset,
					   14, "%s %d", label, i);
		tree = proto_item_add_subtree(item, ett_extent);
		PULL_UINT8(extent, virt_block_num);
		PULL_UINT8(extent, block_count);
		PULL_UINT64(extent, disk_id);
		PULL_UINT32(extent, phys_block_num);
	}
	return offset;
}


/*
  pull a list element of type segment descriptor from a packet
*/
static void pull_element_seg_descr(tvbuff_t *tvb, packet_info *pinfo,
				   proto_tree *parent_tree,
				   struct tank_header *hdr, int *var_offset,
				   int length, int idx)
{
	seg_descr_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;
	int rd_count, wrt_count, datasize;

	rd_count = tvb_get_ntohl(tvb, offset+8);
	wrt_count = tvb_get_ntohl(tvb, offset+44);
	datasize  = 80 + (rd_count + wrt_count) * 14;
	item = proto_tree_add_text(parent_tree, tvb, offset, datasize,
				   "Segment Descriptor %d", idx);
	tree = proto_item_add_subtree(item, ett_seg_descr);

	PULL_UINT64(seg_descr, seg_num);
	PULL_UINT32(seg_descr, read_count);
	PULL_CHAR_ARRAY(seg_descr, read_state, "Read State", 32);
	PULL_UINT32(seg_descr, write_count);
	PULL_CHAR_ARRAY(seg_descr, write_state, "Write State", 32);

	offset = get_extents(tvb, tree, *var_offset, "Read Extent", rd_count);
	offset = get_extents(tvb, tree, *var_offset, "Write Extent",
			     wrt_count);

	(*var_offset) = offset;
}


/*
  pull a vector element of type network interface from a packet
*/
static void pull_element_net_interface(tvbuff_t *tvb, packet_info *pinfo,
				       proto_tree *parent_tree,
				       struct tank_header *hdr, int *var_offset,
				       int length, int idx)
{
	net_interface_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;

	item = proto_tree_add_text(parent_tree, tvb, (*var_offset), length,
				   "Network Interface %d", idx);
	tree = proto_item_add_subtree(item, ett_net_interface);

	PULL_SRVADDR(net_interface);
	PULL_UINT32(net_interface, nic_flags);

	(*var_offset) += length;
}


/*
  pull a vector element of type access time from a packet
*/
static void pull_element_access_time(tvbuff_t *tvb, packet_info *pinfo,
				     proto_tree *parent_tree,
				     struct tank_header *hdr, int *var_offset,
				     int length, int idx)
{
	access_time_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;

	item = proto_tree_add_text(parent_tree, tvb, (*var_offset), length,
				   "Access Time Update %d", idx);
	tree = proto_item_add_subtree(item, ett_access_time);

	PULL_UOID(access_time, "Unique OID");
	PULL_TIME(access_time, timestamp);

	(*var_offset) += length;
}


/*
  pull a list element of type dir entry from a packet
*/
static void pull_element_dir_entry(tvbuff_t *tvb, packet_info *pinfo,
				   proto_tree *parent_tree,
				   struct tank_header *hdr, int *var_offset,
				   int length, int idx)
{
	dir_entry_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;
	const guint8 *p = "";
	int namelen;

	namelen = tvb_get_ntohs(tvb, offset) * 2;
	item = proto_tree_add_text(parent_tree, tvb, (*var_offset),
				   54 + namelen, "Directory Entry %d", idx);
	tree = proto_item_add_subtree(item, ett_dir_entry);

	PULL_UINT16(dir_entry, name_length);
	PULL_UOID(dir_entry, "Unique OID");
	PULL_UINT8(dir_entry, obj_type);
	PULL_SRVADDR(dir_entry);
	PULL_UINT64(dir_entry, key);

	if (!tvb_bytes_exist(tvb, offset, pkt.name_length)) {
		p = "ERR: INVALID STRING";
		proto_tree_add_text(tree, tvb, offset, pkt.name_length, p);
	} else if (pkt.name_length > 0) {
		p = tvb_fake_unicode(tvb, offset, pkt.name_length, FALSE);
		proto_tree_add_string(tree, HF(dir_entry, name), tvb,
				      offset, namelen, p);
	}
	(*var_offset) = offset + namelen;
}


/*
  pull an arbitrary vector from a packet
*/
static int pull_vector(tvbuff_t *tvb, packet_info *pinfo,
			proto_tree *parent_tree, int offset, 
			struct tank_header *hdr, int *var_offset,
			char *label, pull_element_t pull_element)
{
	int v_offset, count, el_size, i;
	proto_item *item = NULL;
	proto_tree *tree = NULL;

	v_offset = tvb_get_ntohl(tvb, offset);
	count = tvb_get_ntohs(tvb, offset+4);
	el_size = tvb_get_ntohs(tvb, offset+6);
	*var_offset = v_offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 8,
			           "Vector[%d]: (%s)", count, label);
	tree = proto_item_add_subtree(item, ett_vector);

	proto_tree_add_uint(tree, hf_id(hf_tank_vector, "tank.vector.offset"), 
			    tvb, offset, 4, v_offset);
	proto_tree_add_uint(tree, hf_id(hf_tank_vector, "tank.vector.count"), 
			    tvb, offset+4, 2, count);
	proto_tree_add_uint(tree, hf_id(hf_tank_vector, "tank.vector.el_size"), 
			    tvb, offset+6, 2, el_size);

	for (i = 0; i < count; i++) {
		(*pull_element)(tvb, pinfo, tree, hdr, var_offset, el_size, i);
	}
	return offset + 8;

}


/*
  pull an arbitrary list from a packet
*/
static int pull_list(tvbuff_t *tvb, packet_info *pinfo,
		     proto_tree *parent_tree, int offset, 
		     struct tank_header *hdr, int *var_offset,
		     char *label, pull_element_t pull_element)
{
	int l_offset, l_size, count, i;
	proto_item *item = NULL;
	proto_tree *tree = NULL;

	l_offset = tvb_get_ntohl(tvb, offset);
	l_size = tvb_get_ntohl(tvb, offset+4);
	count = tvb_get_ntohl(tvb, offset+8);
	*var_offset = l_offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 12,
			           "List[%d]: (%s)", count, label);
	tree = proto_item_add_subtree(item, ett_list);

	proto_tree_add_uint(tree, hf_id(hf_tank_list, "tank.list.offset"), 
			    tvb, offset, 4, l_offset);
	proto_tree_add_uint(tree, hf_id(hf_tank_list, "tank.list.l_size"), 
			    tvb, offset+4, 4, l_size);
	proto_tree_add_uint(tree, hf_id(hf_tank_list, "tank.list.count"), 
			    tvb, offset+8, 4, count);

	for (i = 0; i < count; i++) {
		(*pull_element)(tvb, pinfo, tree, hdr, var_offset, 0, i);
	}
	return offset + 12;
}

/*---------- Start FlexSAN pull element functions ----------*/

static int pull_lun_id(tvbuff_t *tvb, packet_info *pinfo,
                       proto_tree *parent_tree,
                       struct tank_header *hdr, int offset,
                       char *label )
{
        lun_id_t      pkt;
	proto_item   *item = NULL;
	proto_tree   *tree = NULL;
	const guint8 *p;

        int id_length = tvb_get_ntohl( tvb, offset + 16 );
        int length = sizeof(lun_id_t) + id_length;

        item = proto_tree_add_text(parent_tree, tvb, offset,
                                   length, label );
        
	tree = proto_item_add_subtree(item, ett_lun_id );

        PULL_STRING( lun_id, lun_id_format, 16 );
        PULL_UINT32( lun_id, lun_id_length );
        PULL_STRING( lun_id, lun_id_value, id_length );

        return offset;
}

static void pull_element_lun_id(tvbuff_t *tvb, packet_info *pinfo,
                                proto_tree *parent_tree,
                                struct tank_header *hdr, int *var_offset,
                                int length, int idx)
{
        char label[32];
        int  offset = *var_offset;
        proto_tree *tree = parent_tree;
  
        sprintf( label, "stpLun[ %d ]", idx );

        PULL_LUN_ID( lun_id, value, label );

        (*var_offset) = offset;
}

static int pull_lun_or_vol(tvbuff_t *tvb, packet_info *pinfo,
                            proto_tree *parent_tree,
                            struct tank_header *hdr, int offset,
                           char *label )
{
        lun_or_vol_t  pkt;
	proto_item   *item = NULL;
	proto_tree   *tree = NULL;
        guint8        is_volume = tvb_get_guint8( tvb, offset );
        int           length = sizeof( lun_or_vol_t ) - 3;
        char          disp_label[64];
        
        if ( is_volume == 0 )
        {
          int id_length = tvb_get_ntohl( tvb, offset + 17 );          
          length += id_length; /* Need to get id_length */
        }
        
        item = proto_tree_add_text(parent_tree, tvb, offset,
                                   length, label);
        
	tree = proto_item_add_subtree(item, ett_lun_or_vol );

        PULL_UINT8( lun_or_vol, is_volume );

        if ( is_volume == 1 )
        {
          PULL_UINT64( lun_or_vol, lv_device.lv_vol );
          /* Adjust offset because of union */
          offset += sizeof( lun_id_t ) - 8;
        }
        else
        {
          PULL_LUN_ID( lun_or_vol, lv_lun, "lv_lun" );
        }
        
        return offset;
}

static void pull_element_lun_or_vol(tvbuff_t *tvb, packet_info *pinfo,
                                    proto_tree *parent_tree,
                                    struct tank_header *hdr, int *var_offset,
                                    int length, int idx)
{
        char label[32];
        int offset = *var_offset;
        proto_tree *tree = parent_tree;

        sprintf( label, "stpLunOrVolName[ %d ]", idx );

        PULL_LUN_OR_VOL( lun_id_or_vol, lv_device, label );

        (*var_offset) = offset;
}

static int pull_vol_id(tvbuff_t *tvb, packet_info *pinfo,
                       proto_tree *parent_tree,
                       struct tank_header *hdr, int offset,
                       char *label )
{
        vol_id_t      pkt;
	proto_item   *item = NULL;
	proto_tree   *tree = NULL;
	const guint8 *p;

        int length = sizeof(vol_id_t);

        item = proto_tree_add_text(parent_tree, tvb, offset,
                                   length, label );
        
	tree = proto_item_add_subtree(item, ett_vol_id );

        /* Pull the length of the volId */
        PULL_UINT64( vol_id, gid );
                     
        return offset;
}

static void pull_element_vol_id(tvbuff_t *tvb, packet_info *pinfo,
                                proto_tree *parent_tree,
                                struct tank_header *hdr, int *var_offset,
                                int length, int idx)
{
        char label[32];
        int  offset = *var_offset;
        proto_tree *tree = parent_tree;

        /* Need to deal with variable length aspect */

        sprintf( label, "stpVol[ %d ]", idx );

        PULL_VOL_ID( vol_id, value, label );
        
        (*var_offset) = offset;
}

/*
  pull a copy request from it's stpVector
*/

static void pull_element_lun_info(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree,
                                  struct tank_header *hdr, int *var_offset,
                                  int length, int idx)
{
        lun_info_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;
	const guint8 *p;

        /* TODO: pull id and name lengths and add to length */

	item = proto_tree_add_text(parent_tree, tvb, (*var_offset), length,
				   "Lun info %d", idx);
	tree = proto_item_add_subtree(item, ett_lun_info);

        PULL_UINT8(  lun_info, state );
        PULL_UINT8(  lun_info, access );
        PULL_STRING( lun_info, vendor_id, 8 );
        PULL_STRING( lun_info, product_id, 16 );
        PULL_STRING( lun_info, product_ver, 4 );
        PULL_STRING( lun_info, id_format, 16 );
        PULL_UINT32( lun_info, blk_size);
        PULL_UINT64( lun_info, capacity);
        PULL_UINT64( lun_info, vol_gid);
        PULL_UINT64( lun_info, vol_owner_id);
        PULL_UINT64( lun_info, vol_install_id);
        PULL_UINT32( lun_info, id_length );
        PULL_UINT32( lun_info, dev_name_length );

	(*var_offset) = offset;
}

/*
  pull a copy request from it's stpVector
*/

static void pull_element_vol_info(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree,
                                  struct tank_header *hdr, int *var_offset,
                                  int length, int idx)
{
        vol_info_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;
	const guint8 *p;

	item = proto_tree_add_text(parent_tree, tvb, (*var_offset), length,
				   "Vol info %d", idx);
	tree = proto_item_add_subtree(item, ett_vol_info);

        PULL_UINT8(  vol_info, vi_state );
        PULL_UINT64( vol_info, vi_globalDiskId);
        PULL_UINT64( vol_info, vi_ownerId);
        PULL_UINT64( vol_info, vi_installationId);
        PULL_UINT64( vol_info, vi_capacity);

	(*var_offset) = offset;
}

/*
  pull a copy request from it's stpVector

  if PULL_VECTOR, length is set to the value in stpVector.v_size.
  if PULL_LIST, length is set to 0.
*/

static void pull_element_copy_data_request(tvbuff_t *tvb, packet_info *pinfo,
                                           proto_tree *parent_tree,
                                           struct tank_header *hdr, int *var_offset,
                                           int length, int idx)
{
        copy_data_request_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;

	item = proto_tree_add_text(parent_tree, tvb, (*var_offset), length,
				   "Copy Data Request %d", idx);
	tree = proto_item_add_subtree(item, ett_copy_data_request);

        PULL_UINT64( copy_data_request, source_gid );
        PULL_UINT32( copy_data_request, source_start );
        PULL_UINT64( copy_data_request, dest_gid );
        PULL_UINT32( copy_data_request, dest_start );
        PULL_UINT32( copy_data_request, block_count );

	(*var_offset) += length;
}

static void pull_element_copy_result(tvbuff_t *tvb, packet_info *pinfo,
                                            proto_tree *parent_tree,
                                            struct tank_header *hdr, int *var_offset,
                                            int length, int idx)
{
        copy_result_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int offset = *var_offset;

	item = proto_tree_add_text(parent_tree, tvb, (*var_offset), length,
				   "Copy Data Request %d", idx);
	tree = proto_item_add_subtree(item, ett_copy_result );

        PULL_UINT32( copy_result, blocks_written );
        PULL_UINT8(  copy_result, result );

	(*var_offset) += length;
}

/*---------- End FlexSAN data pull element functions ----------*/

/*
  dissect an Identify request
*/
static int dissect_tank_identify(tvbuff_t *tvb, packet_info *pinfo, 
				  proto_tree *parent_tree, int offset, 
				  struct tank_header *hdr)
{
	struct tank_identify pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset,
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_identify);

	PULL_ASCII(identify, client_name);
	PULL_ASCII(identify, client_platform);
	PULL_ARBSTRING(identify, client_locale);
	PULL_UINT16(identify, locale_flag);
	PULL_ASCII(identify, client_version);
	PULL_UINT16(identify, client_charset);
	PULL_TIME(identify, client_time);
	PULL_CHAR_ARRAY(identify, client_capabilities, "Client Capabilities", 16);

	sinfo->unicode = pkt.locale_flag ? TRUE : FALSE;
	fprintf(stderr, "Unicode now %s\n", sinfo->unicode?"Enabled":"Disabled");

	return MAX(var_offset, offset);
}

/*
  dissect an Identify response
*/
static int dissect_tank_identify_response(tvbuff_t *tvb, packet_info *pinfo, 
					  proto_tree *parent_tree, int offset, 
					  struct tank_header *hdr)
{
	struct tank_identify_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_identify_response);

	PULL_UINT16(identify_response, rc);
	PULL_UNICODE(identify_response, server_name);
	PULL_UNICODE(identify_response, platform_name);
	PULL_UNICODE(identify_response, software_version);
	PULL_TIME(identify_response, server_time);
	PULL_CHAR_ARRAY(identify_response, server_capabilities, "Server Capabilities", 16);
	PULL_UINT16(identify_response, preferred_version);
	PULL_UINT16_ARRAY(identify_response, supported_versions);

	return MAX(var_offset, offset);
}


/*
  dissect a uoid_t
*/
static int dissect_tank_uoid(tvbuff_t *tvb, packet_info *pinfo, 
			     proto_tree *parent_tree, int offset, 
			     struct tank_header *hdr _U_,
			     const char *label)
{
	uoid_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 20, label);
	tree = proto_item_add_subtree(item, ett_uoid);

	PULL_UINT32(uoid, cluster_id);
	PULL_UINT32(uoid, container_id);
	PULL_UINT32(uoid, epoch_id);
	PULL_UINT64(uoid, object_id);

	return offset;
}

/*
  dissect a dvers_t
*/
static int dissect_tank_dvers(tvbuff_t *tvb, packet_info *pinfo, 
			      proto_tree *parent_tree, int offset, 
			      struct tank_header *hdr _U_)
{
	dvers_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 12, "Data Lock Version");
	tree = proto_item_add_subtree(item, ett_dvers);

	PULL_UINT32(dvers, epoch);
	PULL_UINT64(dvers, version);

	return offset;
}

/*
  dissect a svers_t
*/
static int dissect_tank_svers(tvbuff_t *tvb, packet_info *pinfo, 
			      proto_tree *parent_tree, int offset, 
			      struct tank_header *hdr _U_)
{
	svers_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 12, "Session Lock Version");
	tree = proto_item_add_subtree(item, ett_svers);

	PULL_UINT32(svers, epoch);
	PULL_UINT64(svers, version);

	return offset;
}

/*
  dissect a basic_objattr_t
*/
static int dissect_tank_basic_objattr(tvbuff_t *tvb, packet_info *pinfo, 
				      proto_tree *parent_tree, int offset, 
				      struct tank_header *hdr _U_,
				      const char *label)
{
	basic_objattr_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 69, label);
	tree = proto_item_add_subtree(item, ett_basic_objattr);

	PULL_UINT8(basic_objattr, obj_type);
	PULL_TIME(basic_objattr, create_time);
	PULL_TIME(basic_objattr, change_time);
	PULL_TIME(basic_objattr, access_time);
	PULL_TIME(basic_objattr, modify_time);
	PULL_UINT32(basic_objattr, misc_attr);
	PULL_UINT32(basic_objattr, link_count);
	PULL_UINT64(basic_objattr, file_size);
	PULL_UINT64(basic_objattr, block_count);
	PULL_UINT32(basic_objattr, block_size);
	PULL_UINT64(basic_objattr, version);

	return MAX(var_offset, offset);
}

/*
  dissect a objattr_t
*/
static int dissect_tank_objattr(tvbuff_t *tvb, packet_info *pinfo, 
				proto_tree *parent_tree, int offset, 
				struct tank_header *hdr _U_)
{
	objattr_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 95, "Object Attributes");
	tree = proto_item_add_subtree(item, ett_objattr);

	PULL_BASIC_OBJATTR(objattr, "Basic Object Attributes");
	PULL_UINT32(objattr, userid);
	PULL_UINT32(objattr, groupid);
	PULL_UINT16(objattr, permissions);
	PULL_NAMESTRING(objattr, symlink);
	PULL_ARBSTRING(objattr, sdvalue);

	return MAX(var_offset, offset);
}

/*
  dissect a gdl_t
*/
static int dissect_tank_gdl(tvbuff_t *tvb, packet_info *pinfo, 
			    proto_tree *parent_tree, int offset, 
			    struct tank_header *hdr _U_)
{
	gdl_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 110, "Granted Data Lock");
	tree = proto_item_add_subtree(item, ett_gdl);

	PULL_UINT8(gdl, mode);
	PULL_DVERS(gdl);
	PULL_OBJATTR(gdl);
	PULL_UINT16(gdl, strategy);

	return offset;
}

/*
  dissect a gsl_t
*/
static int dissect_tank_gsl(tvbuff_t *tvb, packet_info *pinfo, 
			    proto_tree *parent_tree, int offset, 
			    struct tank_header *hdr _U_)
{
	gsl_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 13, "Granted Session Lock");
	tree = proto_item_add_subtree(item, ett_gsl);

	PULL_UINT8(gsl, mode);
	PULL_DVERS(gsl);

	return offset;
}

/*
  dissect a netaddr_t
*/
static int dissect_tank_netaddr(tvbuff_t *tvb, packet_info *pinfo, 
				proto_tree *parent_tree, int offset, 
				struct tank_header *hdr _U_)
{
	netaddr_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 17, "Network Address");
	tree = proto_item_add_subtree(item, ett_netaddr);

	PULL_UINT8(netaddr, is_legacy);
	PULL_CHAR_ARRAY(netaddr, n_addr, "Address", 16);

	return offset;
}


/*
  dissect a srvaddr_t
*/
static int dissect_tank_srvaddr(tvbuff_t *tvb, packet_info *pinfo, 
				proto_tree *parent_tree, int offset, 
				struct tank_header *hdr _U_)
{
	srvaddr_t pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 22, "Server Address");
	tree = proto_item_add_subtree(item, ett_srvaddr);

	PULL_UINT8(srvaddr, is_null);
	PULL_NETADDR(srvaddr);
	PULL_UINT32(srvaddr, netport);

	return offset;
}

/*
  dissect an acquire data lock request
*/
static int dissect_tank_acquire_data_lock(tvbuff_t *tvb, packet_info *pinfo, 
					  proto_tree *parent_tree, int offset, 
					  struct tank_header *hdr)
{
	struct tank_acquire_data_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_acquire_data_lock);

	PULL_UOID(acquire_data_lock, "Unique OID");
	PULL_UINT8(acquire_data_lock, mode);
	PULL_UINT8(acquire_data_lock, opportunistic);
	PULL_DVERS(acquire_data_lock);
	PULL_UINT64(acquire_data_lock, alt_id);

	return offset;
}


/*
  dissect an acquire data lock response
*/
static int dissect_tank_acquire_data_lock_response(tvbuff_t *tvb, packet_info *pinfo, 
						   proto_tree *parent_tree, int offset, 
						   struct tank_header *hdr)
{
	struct tank_acquire_data_lock_response pkt _U_;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_acquire_data_lock_response);

	PULL_GDL(acquire_data_lock_response);

	return offset;
}


/*
  dissect an acquire session lock request
*/
static int dissect_tank_acquire_session_lock(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_acquire_session_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_acquire_session_lock);

	PULL_UOID(acquire_session_lock, "Unique OID");
	PULL_UINT8(acquire_session_lock, session_lock_mode);
	PULL_SVERS(acquire_session_lock);
	PULL_UINT8(acquire_session_lock, data_lock_mode);
	PULL_UINT64(acquire_session_lock, alt_id);

	return offset;
}

/*
  dissect an acquire session lock response
*/
static int dissect_tank_acquire_session_lock_response(tvbuff_t *tvb, packet_info *pinfo, 
						      proto_tree *parent_tree, int offset, 
						      struct tank_header *hdr)
{
	struct tank_acquire_session_lock_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_acquire_session_lock_response);

	PULL_GSL(acquire_session_lock_response);
	PULL_UINT8(acquire_session_lock_response, not_strength_related);
	PULL_GDL(acquire_session_lock_response);

	return offset;
}

/*
  dissect an downgrade session lock request
*/
static int dissect_tank_downgrade_session_lock(tvbuff_t *tvb,
		                               packet_info *pinfo, 
					       proto_tree *parent_tree,
					       int offset, 
					       struct tank_header *hdr)
{
	struct tank_downgrade_session_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset,
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_downgrade_session_lock);

	PULL_UOID(downgrade_session_lock, "Unique OID");
	PULL_UINT8(downgrade_session_lock, lock_mode);

	return offset;
}

/*
  dissect a publish cluster info request
*/
static int dissect_tank_publish_cluster_info(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_publish_cluster_info pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_cluster_info);

	PULL_UINT32(publish_cluster_info, container_id);
	PULL_VECTOR(publish_cluster_info, net_interface, "Primary NICs");
	PULL_VECTOR(publish_cluster_info, net_interface, "Secondary NICs");
	PULL_UINT32(publish_cluster_info, lease_period);
	PULL_UINT16(publish_cluster_info, num_retries);
	PULL_UINT32(publish_cluster_info, xmit_timeout);
	PULL_UINT32(publish_cluster_info, async_period);

	return MAX(var_offset, offset);
}

/*
  dissect a report transaction status
*/
static int dissect_tank_report_txn_status(tvbuff_t *tvb, packet_info *pinfo, 
					  proto_tree *parent_tree, int offset, 
					  struct tank_header *hdr)
{
	struct tank_report_txn_status pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_report_txn_status);

	PULL_UINT32(report_txn_status, rc);
	PULL_UINT32(report_txn_status, retry_delay);

	return offset;
}

/*
  dissect a lookup name request
*/
static int dissect_tank_lookup_name(tvbuff_t *tvb, packet_info *pinfo, 
				    proto_tree *parent_tree, int offset, 
				    struct tank_header *hdr)
{
	struct tank_lookup_name pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_lookup_name);

	PULL_UOID(lookup_name, "Unique OID");
	PULL_ASCII(lookup_name, name);
	PULL_UINT8(lookup_name, case_insensitive);
	PULL_UINT8(lookup_name, session_lock_mode);
	PULL_UINT8(lookup_name, data_lock_mode);
	PULL_TIME(lookup_name, timestamp);

	return MAX(var_offset, offset);
}


/*
  dissect a lookup name response
*/
static int dissect_tank_lookup_name_response(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_lookup_name_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_lookup_name_response);

	PULL_UOID(lookup_name_response, "Unique OID");
	PULL_UINT8(lookup_name_response, obj_type);
	PULL_SRVADDR(lookup_name_response);
	PULL_UINT64(lookup_name_response, key);
	PULL_UNICODE(lookup_name_response, name);
	PULL_GSL(lookup_name_response);
	PULL_UINT8(lookup_name_response, not_strength_related);
	PULL_GDL(lookup_name_response);

	return MAX(var_offset, offset);
}


/*
  dissect a change name request
*/
static int dissect_tank_change_name(tvbuff_t *tvb, packet_info *pinfo, 
				    proto_tree *parent_tree, int offset, 
				    struct tank_header *hdr)
{
	struct tank_change_name pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_change_name);

	PULL_UOID(change_name, "Source OID");
	PULL_NAMESTRING(change_name, src_name);
	PULL_UOID(change_name, "Dest OID");
	PULL_NAMESTRING(change_name, dest_name);
	PULL_UINT8(change_name, dest_case_insensitive);
	PULL_UINT64(change_name, timestamp);
	PULL_UINT8(change_name, exclusive);

	return MAX(var_offset, offset);
}

/*
  dissect a change name response
*/
static int dissect_tank_change_name_response(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_change_name_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_change_name_response);

	PULL_BASIC_OBJATTR(change_name_response, "Source Parent Attrib");
	PULL_BASIC_OBJATTR(change_name_response, "Dest Parent Attrib");
	PULL_UOID(change_name_response, "Source Child OID");
	PULL_UINT8(change_name_response, src_child_type);
	PULL_BASIC_OBJATTR(change_name_response, "Source Child Attrib");
	PULL_UINT8(change_name_response, dest_child_existed);
	PULL_UOID(change_name_response, "Dest Child OID");
	PULL_BASIC_OBJATTR(change_name_response, "Dest Child Attrib");
	PULL_NAMESTRING(change_name_response, dest_actual_name);
	PULL_UINT64(change_name_response, dest_key);

	return MAX(var_offset, offset);
}


/*
  dissect a get storage capacity request
*/
static int dissect_tank_get_storage_capacity(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_get_storage_capacity pkt _U_;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_storage_capacity);

	PULL_UOID(get_storage_capacity, "Unique OID");

	return offset;
}

/*
  dissect a get storage capacity response
*/
static int dissect_tank_get_storage_capacity_response(tvbuff_t *tvb, packet_info *pinfo, 
						      proto_tree *parent_tree, int offset, 
						      struct tank_header *hdr)
{
	struct tank_get_storage_capacity_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_storage_capacity_response);

	PULL_UINT64(get_storage_capacity_response, total_blocks);
	PULL_UINT64(get_storage_capacity_response, free_blocks);

	return offset;
}


/*
  dissect a create file request
*/
static int dissect_tank_create_file(tvbuff_t *tvb, packet_info *pinfo, 
				    proto_tree *parent_tree, int offset, 
				    struct tank_header *hdr)
{
	struct tank_create_file pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_file);

	PULL_UOID(create_file, "Parent OID");
	PULL_ASCII(create_file, name);
	PULL_UINT8(create_file, case_insensitive);
	PULL_UINT64(create_file, timestamp);
	PULL_UINT32(create_file, userid);
	PULL_UINT32(create_file, groupid);
	PULL_UINT16(create_file, permissions);
	PULL_UINT32(create_file, misc_attr);
	PULL_UINT16(create_file, quality_of_service);
	PULL_UINT8(create_file, exclusive);
	PULL_UINT8(create_file, session_lock_mode);
	PULL_UINT8(create_file, data_lock_mode);
	PULL_ARBSTRING(create_file, sdvalue);

	return MAX(var_offset, offset);
}


/*
  dissect a create file response
*/
static int dissect_tank_create_file_response(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_create_file_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_file_response);

	PULL_UINT8(create_file_response, existed);
	PULL_UOID(create_file_response, "Unique OID");
	PULL_UINT64(create_file_response, dir_key);
	PULL_SRVADDR(create_file_response);
	PULL_BASIC_OBJATTR(create_file_response, "Parent Attributes");
	PULL_NAMESTRING(create_file_response, actual_name);
	PULL_GSL(create_file_response);
	PULL_UINT8(create_file_response, not_strength_related);
	PULL_GDL(create_file_response);

	return MAX(var_offset, offset);
}


/*
  dissect a create dir request
*/
static int dissect_tank_create_dir(tvbuff_t *tvb, packet_info *pinfo, 
				   proto_tree *parent_tree, int offset, 
				   struct tank_header *hdr)
{
	struct tank_create_dir pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_dir);

	PULL_UOID(create_dir, "Parent OID");
	PULL_ASCII(create_dir, name);
	PULL_UINT8(create_dir, case_insensitive);
	PULL_UINT64(create_dir, timestamp);
	PULL_UINT32(create_dir, userid);
	PULL_UINT32(create_dir, groupid);
	PULL_UINT16(create_dir, permissions);
	PULL_UINT32(create_dir, misc_attr);
	PULL_UINT8(create_dir, exclusive);
	PULL_UINT8(create_dir, session_lock_mode);
	PULL_ARBSTRING(create_dir, sdvalue);

	return MAX(var_offset, offset);
}


/*
  dissect a create dir response
*/
static int dissect_tank_create_dir_response(tvbuff_t *tvb, packet_info *pinfo, 
					     proto_tree *parent_tree, int offset, 
					     struct tank_header *hdr)
{
	struct tank_create_dir_response pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	int var_offset = offset;

	item = proto_tree_add_text(parent_tree, tvb, offset, 
				   hdr->msg_length - offset, tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_dir_response);

	PULL_UINT8(create_dir_response, existed);
	PULL_UOID(create_dir_response, "Unique OID");
	PULL_UINT64(create_dir_response, dir_key);
	PULL_SRVADDR(create_dir_response);
	PULL_BASIC_OBJATTR(create_dir_response, "Parent Attributes");
	PULL_UINT64(create_dir_response, dot_key);
	PULL_UINT64(create_dir_response, dot_dot_key);
	PULL_NAMESTRING(create_dir_response, actual_name);
	PULL_GSL(create_dir_response);
	PULL_UINT8(create_dir_response, not_strength_related);
	PULL_GDL(create_dir_response);

	return MAX(var_offset, offset);
}


/*
  dissect an unknown request
*/
static int dissect_tank_unknown(tvbuff_t *tvb, packet_info *pinfo, 
				proto_tree *parent_tree, int offset, 
				struct tank_header *hdr)
{
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int remaining;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset, hdr->msg_length - offset, "Unknown 0x%x", 
				   hdr->cmd_type);
	tree = proto_item_add_subtree(item, ett_unknown);

	remaining = tvb_reported_length(tvb) - offset;
	proto_tree_add_text(tree, tvb, offset, remaining,
			    "Unknown Data[%d]: %s", 
			    remaining,
			    tvb_bytes_to_str(tvb, offset, remaining));
	offset += remaining;

	return offset;
}


/*
  dissect Renew Lease message
*/
static int dissect_tank_renew_lease(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_renew_lease pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_renew_lease);

	PULL_TIME(renew_lease, timestamp);

	return offset;
}


/*
  dissect Ping message
*/
static int dissect_tank_ping(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_ping pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_ping);

	PULL_TIME(ping, timestamp);

	return offset;
}


/*
  dissect Shutdown message
*/
static int dissect_tank_shutdown(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	(void) proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	return offset;
}


/*
  dissect Notify Forward message
*/
static int dissect_tank_notify_forward(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_notify_forward pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_notify_forward);

	PULL_SRVADDR(notify_forward);
	PULL_UINT32(notify_forward, load_unit_id);

	return offset;
}


/*
  dissect Create Hard Link message
*/
static int dissect_tank_create_hard_link(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_create_hard_link pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_hard_link);

	PULL_UOID(create_hard_link, "Parent OID");
	PULL_ASCII(create_hard_link, name);
	PULL_UINT8(create_hard_link, case_insensitive);
	PULL_UINT8(create_hard_link, exclusive);
	PULL_UOID(create_hard_link, "Target OID");
	PULL_TIME(create_hard_link, timestamp);

	return MAX(var_offset, offset);
}


/*
  dissect Create Hard Link Resp message
*/
static int dissect_tank_create_hard_link_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_create_hard_link_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_hard_link_resp);

	PULL_UINT64(create_hard_link_resp, key);
	PULL_UINT8(create_hard_link_resp, existed);
	PULL_BASIC_OBJATTR(create_hard_link_resp, "Parent Attributes");
	PULL_BASIC_OBJATTR(create_hard_link_resp, "Target Attributes");
	PULL_ASCII(create_hard_link_resp, actual_name);

	return MAX(var_offset, offset);
}


/*
  dissect Create Sym Link message
*/
static int dissect_tank_create_sym_link(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_create_sym_link pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_sym_link);

	PULL_UOID(create_sym_link, "Parent OID");
	PULL_ASCII(create_sym_link, name);
	PULL_ASCII(create_sym_link, path_string);
	PULL_TIME(create_sym_link, timestamp);
	PULL_UINT32(create_sym_link, owner_id);
	PULL_UINT32(create_sym_link, group_id);
	PULL_UINT16(create_sym_link, permissions);
	PULL_UINT32(create_sym_link, misc_attr);
	PULL_ARBSTRING(create_sym_link, stsd_val);

	return MAX(var_offset, offset);
}


/*
  dissect Create Sym Link Resp message
*/
static int dissect_tank_create_sym_link_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_create_sym_link_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_create_sym_link_resp);

	PULL_UOID(create_sym_link_resp, "Unique OID");
	PULL_UINT64(create_sym_link_resp, key);
	PULL_SRVADDR(create_sym_link_resp);
	PULL_BASIC_OBJATTR(create_sym_link_resp, "Parent Attributes");
	PULL_GSL(create_sym_link_resp);

	return offset;
}


/*
  dissect Remove Name message
*/
static int dissect_tank_remove_name(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_remove_name pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_remove_name);

	PULL_UOID(remove_name, "Parent OID");
	PULL_ASCII(remove_name, name);
	PULL_TIME(remove_name, timestamp);

	return MAX(var_offset, offset);
}


/*
  dissect Remove Name Resp message
*/
static int dissect_tank_remove_name_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_remove_name_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_remove_name_resp);

	PULL_BASIC_OBJATTR(remove_name_resp, "Parent Attributes");
	PULL_UOID(remove_name_resp, "Unique OID");
	PULL_BASIC_OBJATTR(remove_name_resp, "Child Attributes");

	return offset;
}


/*
  dissect Set Basic Obj Attr message
*/
static int dissect_tank_set_basic_obj_attr(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_set_basic_obj_attr pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_set_basic_obj_attr);

	PULL_UOID(set_basic_obj_attr, "Unique OID");
	PULL_UINT32(set_basic_obj_attr, which);
	PULL_TIME(set_basic_obj_attr, create_time);
	PULL_TIME(set_basic_obj_attr, change_time);
	PULL_TIME(set_basic_obj_attr, access_time);
	PULL_TIME(set_basic_obj_attr, modify_time);
	PULL_UINT32(set_basic_obj_attr, misc_attr);
	PULL_TIME(set_basic_obj_attr, timestamp);

	return offset;
}


/*
  dissect Set Basic Obj Attr Resp message
*/
static int dissect_tank_set_basic_obj_attr_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_set_basic_obj_attr_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_set_basic_obj_attr_resp);

	PULL_BASIC_OBJATTR(set_basic_obj_attr_resp, "Basic Attributes");

	return offset;
}


/*
  dissect Set Access Ctl Attr message
*/
static int dissect_tank_set_access_ctl_attr(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_set_access_ctl_attr pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_set_access_ctl_attr);

	PULL_UOID(set_access_ctl_attr, "Unique OID");
	PULL_UINT32(set_access_ctl_attr, which);
	PULL_UINT32(set_access_ctl_attr, owner_id);
	PULL_UINT32(set_access_ctl_attr, group_id);
	PULL_UINT16(set_access_ctl_attr, permissions);
	PULL_TIME(set_access_ctl_attr, timestamp);
	PULL_ARBSTRING(set_access_ctl_attr, stsd_val);

	return MAX(var_offset, offset);
}


/*
  dissect Set Access Ctl Attr Resp message
*/
static int dissect_tank_set_access_ctl_attr_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_set_access_ctl_attr_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_set_access_ctl_attr_resp);

	PULL_OBJATTR(set_access_ctl_attr_resp);

	return offset;
}


/*
  dissect Update Access Time message
*/
static int dissect_tank_update_access_time(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_update_access_time pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_update_access_time);

	PULL_VECTOR(update_access_time, access_time, "Access Time Updates");

	return MAX(var_offset, offset);
}


/*
  dissect Publish Access Time message
*/
static int dissect_tank_publish_access_time(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_publish_access_time pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_access_time);

	PULL_VECTOR(publish_access_time, access_time, "Access Time Updates");

	return MAX(var_offset, offset);
}


/*
  dissect Read Dir message
*/
static int dissect_tank_read_dir(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_read_dir pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_read_dir);

	PULL_UOID(read_dir, "Unique OID");
	PULL_UINT64(read_dir, restart_key);

	return offset;
}


/*
  dissect Read Dir Resp message
*/
static int dissect_tank_read_dir_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_read_dir_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_read_dir_resp);

	PULL_BASIC_OBJATTR(read_dir_resp, "Basic Attributes");
	PULL_UINT8(read_dir_resp, end_reached);
	PULL_UINT8(read_dir_resp, is_complete);
	PULL_LIST(read_dir_resp, dir_entry, "Directory Entries");

	return MAX(var_offset, offset);
}


/*
  dissect Find Object message
*/
static int dissect_tank_find_object(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_find_object pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_find_object);

	PULL_UOID(find_object, "Unique OID");
	PULL_UINT8(find_object, session_lock_mode);
	PULL_UINT8(find_object, data_lock_mode);

	return offset;
}


/*
  dissect Find Object Resp message
*/
static int dissect_tank_find_object_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_find_object_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_find_object_resp);

	PULL_UINT8(find_object_resp, obj_type);
	PULL_SRVADDR(find_object_resp);
	PULL_GSL(find_object_resp);
	PULL_UINT8(find_object_resp, not_strength_related);
	PULL_GDL(find_object_resp);

	return offset;
}


/*
  dissect Demand Session Lock message
*/
static int dissect_tank_demand_session_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_demand_session_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_demand_session_lock);

	PULL_UOID(demand_session_lock, "Unique OID");
	PULL_UINT8(demand_session_lock, lock_mode);
	PULL_UINT8(demand_session_lock, demand_flag);

	return offset;
}


/*
  dissect Deny Session Lock message
*/
static int dissect_tank_deny_session_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_deny_session_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_deny_session_lock);

	PULL_UOID(deny_session_lock, "Unique OID");

	return offset;
}


/*
  dissect Publish Lock Version message
*/
static int dissect_tank_publish_lock_version(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_publish_lock_version pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_lock_version);

	PULL_UOID(publish_lock_version, "Unique OID");
	PULL_SVERS(publish_lock_version);

	return offset;
}


/*
  dissect Downgrade Data Lock message
*/
static int dissect_tank_downgrade_data_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_downgrade_data_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_downgrade_data_lock);

	PULL_UOID(downgrade_data_lock, "Unique OID");
	PULL_UINT8(downgrade_data_lock, lock_mode);
	PULL_UINT8(downgrade_data_lock, access_time_valid);
	PULL_TIME(downgrade_data_lock, access_time);

	return offset;
}


/*
  dissect Demand Data Lock message
*/
static int dissect_tank_demand_data_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_demand_data_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_demand_data_lock);

	PULL_UOID(demand_data_lock, "Unique OID");
	PULL_UINT8(demand_data_lock, lock_mode);
	PULL_UINT8(demand_data_lock, opportunistic);

	return offset;
}


/*
  dissect Deferred Downgrade Data Lock message
*/
static int dissect_tank_deferred_downgrade_data_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_deferred_downgrade_data_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_deferred_downgrade_data_lock);

	PULL_UOID(deferred_downgrade_data_lock, "Unique OID");
	PULL_OBJATTR(deferred_downgrade_data_lock);

	return offset;
}


/*
  dissect Invalidate Directory message
*/
static int dissect_tank_invalidate_directory(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_invalidate_directory pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_invalidate_directory);

	PULL_UOID(invalidate_directory, "Unique OID");
	PULL_ASCII(invalidate_directory, removed_name);
	PULL_BASIC_OBJATTR(invalidate_directory, "Basic Attributes");
	PULL_UINT8(invalidate_directory, unlinked);
	PULL_UOID(invalidate_directory, "Unlinked OID");

	return MAX(var_offset, offset);
}


/*
  dissect Discard Directory message
*/
static int dissect_tank_discard_directory(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_discard_directory pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_discard_directory);

	PULL_UOID(discard_directory, "Unique OID");

	return offset;
}


/*
  dissect Invalidate Obj Attr message
*/
static int dissect_tank_invalidate_obj_attr(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_invalidate_obj_attr pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_invalidate_obj_attr);

	PULL_UOID(invalidate_obj_attr, "Unique OID");

	return offset;
}


/*
  dissect Discard Obj Attr message
*/
static int dissect_tank_discard_obj_attr(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_discard_obj_attr pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_discard_obj_attr);

	PULL_UOID(discard_obj_attr, "Unique OID");

	return offset;
}


/*
  dissect Publish Basic Obj Attr message
*/
static int dissect_tank_publish_basic_obj_attr(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_publish_basic_obj_attr pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_basic_obj_attr);

	PULL_UOID(publish_basic_obj_attr, "Unique OID");
	PULL_BASIC_OBJATTR(publish_basic_obj_attr, "Basic Attributes");

	return offset;
}


/*
  dissect Blk Disk Allocate message
*/
static int dissect_tank_blk_disk_allocate(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_blk_disk_allocate pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_blk_disk_allocate);

	PULL_UOID(blk_disk_allocate, "Unique OID");
	PULL_UINT64(blk_disk_allocate, block_offset);
	PULL_UINT64(blk_disk_allocate, block_count);
	PULL_UINT64(blk_disk_allocate, more_block_count);

	return offset;
}


/*
  dissect Blk Disk Allocate Resp message
*/
static int dissect_tank_blk_disk_allocate_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_blk_disk_allocate_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_blk_disk_allocate_resp);

	PULL_BASIC_OBJATTR(blk_disk_allocate_resp, "Basic Attributes");
	PULL_LIST(blk_disk_allocate_resp, seg_descr, "Segment Descriptors");

	return MAX(var_offset, offset);
}


/*
  dissect Blk Disk Update message
*/
static int dissect_tank_blk_disk_update(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_blk_disk_update pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_blk_disk_update);

	PULL_UOID(blk_disk_update, "Unique OID");
	PULL_UINT8(blk_disk_update, harden);
	PULL_VECTOR(blk_disk_update, seg_update, "Segment Updates");
	PULL_UINT8(blk_disk_update, new_lock_mode);
	PULL_UINT32(blk_disk_update, ba_which);
	PULL_UINT32(blk_disk_update, acl_which);
	PULL_TIME(blk_disk_update, create_time);
	PULL_TIME(blk_disk_update, change_time);
	PULL_TIME(blk_disk_update, access_time);
	PULL_TIME(blk_disk_update, modify_time);
	PULL_UINT32(blk_disk_update, misc_attr);
	PULL_UINT64(blk_disk_update, file_size);
	PULL_UINT32(blk_disk_update, owner_id);
	PULL_UINT32(blk_disk_update, group_id);
	PULL_UINT16(blk_disk_update, permissions);
	PULL_ARBSTRING(blk_disk_update, stsd_val);

	return MAX(var_offset, offset);
}


/*
  dissect Blk Disk Update Resp message
*/
static int dissect_tank_blk_disk_update_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_blk_disk_update_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_blk_disk_update_resp);

	PULL_BASIC_OBJATTR(blk_disk_update_resp, "Basic Attributes");

	return offset;
}


/*
  dissect Blk Disk Get Segment message
*/
static int dissect_tank_blk_disk_get_segment(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_blk_disk_get_segment pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_blk_disk_get_segment);

	PULL_UOID(blk_disk_get_segment, "Unique OID");
	PULL_UINT64(blk_disk_get_segment, seg_no);
	PULL_UINT64(blk_disk_get_segment, more_seg_count);

	return offset;
}


/*
  dissect Blk Disk Get Segment Resp message
*/
static int dissect_tank_blk_disk_get_segment_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_blk_disk_get_segment_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_blk_disk_get_segment_resp);

	PULL_LIST(blk_disk_get_segment_resp, seg_descr, "Segment Descriptors");

	return MAX(var_offset, offset);
}


/*
  dissect Set Range Lock message
*/
static int dissect_tank_set_range_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_set_range_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_set_range_lock);

	PULL_UOID(set_range_lock, "Unique OID");
	PULL_UINT64(set_range_lock, owner_id);
	PULL_UINT64(set_range_lock, start);
	PULL_UINT64(set_range_lock, length);
	PULL_UINT8(set_range_lock, mode);
	PULL_UINT64(set_range_lock, retry_id);
	PULL_UINT64(set_range_lock, alt_id);
	PULL_SVERS(set_range_lock);
	PULL_UINT32(set_range_lock, flags);

	return offset;
}


/*
  dissect Set Range Lock Resp message
*/
static int dissect_tank_set_range_lock_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_set_range_lock_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_set_range_lock_resp);

	PULL_NETADDR(set_range_lock_resp);
	PULL_UINT64(set_range_lock_resp, owner_id);
	PULL_UINT64(set_range_lock_resp, start);
	PULL_UINT64(set_range_lock_resp, length);
	PULL_UINT8(set_range_lock_resp, mode);
	PULL_SVERS(set_range_lock_resp);
	PULL_UINT32(set_range_lock_resp, flags);
	PULL_UINT32(set_range_lock_resp, rc);

	return offset;
}


/*
  dissect Demand Range Lock message
*/
static int dissect_tank_demand_range_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_demand_range_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_demand_range_lock);

	PULL_UOID(demand_range_lock, "Unique OID");
	PULL_UINT64(demand_range_lock, txn_id);
	PULL_UINT64(demand_range_lock, start);
	PULL_UINT64(demand_range_lock, length);
	PULL_UINT8(demand_range_lock, mode);

	return offset;
}


/*
  dissect Demand Range Lock Resp message
*/
static int dissect_tank_demand_range_lock_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_demand_range_lock_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_demand_range_lock_resp);

	PULL_UOID(demand_range_lock_resp, "Unique OID");
	PULL_UINT64(demand_range_lock_resp, txn_id);
	PULL_UINT64(demand_range_lock_resp, start);
	PULL_UINT64(demand_range_lock_resp, length);
	PULL_UINT8(demand_range_lock_resp, mode);

	return offset;
}


/*
  dissect Relinquish Range Lock message
*/
static int dissect_tank_relinquish_range_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_relinquish_range_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_relinquish_range_lock);

	PULL_UOID(relinquish_range_lock, "Unique OID");
	PULL_UINT64(relinquish_range_lock, owner_id);
	PULL_UINT64(relinquish_range_lock, start);
	PULL_UINT64(relinquish_range_lock, length);
	PULL_UINT8(relinquish_range_lock, mode);

	return offset;
}


/*
  dissect Retry Range Lock message
*/
static int dissect_tank_retry_range_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_retry_range_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_retry_range_lock);

	PULL_UINT64(retry_range_lock, retry_id);

	return offset;
}


/*
  dissect Compat Set Range Lock message
*/
static int dissect_tank_compat_set_range_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_compat_set_range_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_compat_set_range_lock);

	PULL_UOID(compat_set_range_lock, "Unique OID");
	PULL_UINT64(compat_set_range_lock, owner_id);
	PULL_UINT8(compat_set_range_lock, lock_mode);
	PULL_UINT64(compat_set_range_lock, range_start);
	PULL_UINT64(compat_set_range_lock, range_length);
	PULL_UINT8(compat_set_range_lock, wait_for_lock);
	PULL_UINT64(compat_set_range_lock, retry_id);

	return offset;
}


/*
  dissect Compat Check Range Lock message
*/
static int dissect_tank_compat_check_range_lock(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_compat_check_range_lock pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_compat_check_range_lock);

	PULL_UOID(compat_check_range_lock, "Unique OID");
	PULL_UINT64(compat_check_range_lock, owner_id);
	PULL_UINT8(compat_check_range_lock, lock_mode);
	PULL_UINT64(compat_check_range_lock, range_start);
	PULL_UINT64(compat_check_range_lock, range_length);

	return offset;
}


/*
  dissect Compat Check Range Lock Resp message
*/
static int dissect_tank_compat_check_range_lock_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_compat_check_range_lock_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_compat_check_range_lock_resp);

	PULL_NETADDR(compat_check_range_lock_resp);
	PULL_UINT64(compat_check_range_lock_resp, owner_id);
	PULL_UINT8(compat_check_range_lock_resp, lock_mode);
	PULL_UINT64(compat_check_range_lock_resp, range_start);
	PULL_UINT64(compat_check_range_lock_resp, range_length);

	return offset;
}


/*
  dissect Release All Range Locks message
*/
static int dissect_tank_release_all_range_locks(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_release_all_range_locks pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_release_all_range_locks);

	PULL_UOID(release_all_range_locks, "Unique OID");

	return offset;
}


/*
  dissect Publish Load Unit Info message
*/
static int dissect_tank_publish_load_unit_info(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_publish_load_unit_info pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_load_unit_info);

	PULL_UINT32(publish_load_unit_info, load_unit_id);
	PULL_SRVADDR(publish_load_unit_info);
	PULL_UINT32(publish_load_unit_info, load_unit_state);

	return offset;
}


/*
  dissect Publish Root Clt Info message
*/
static int dissect_tank_publish_root_clt_info(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_publish_root_clt_info pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_root_clt_info);

	PULL_UINT8(publish_root_clt_info, root_clt_flag);

	return offset;
}


/*
  dissect Publish Eviction Request message
*/
static int dissect_tank_publish_eviction_request(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
	struct tank_publish_eviction_request pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_publish_eviction_request);

	PULL_UINT32(publish_eviction_request, batch_size);

	return offset;
}

/*---------- Start FlexSAN message dissection functions ----------*/

/*
  dissect stpGetLunInfoMsg
*/
static int dissect_tank_get_lun_info(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_get_lun_info pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_lun_info);

	PULL_UINT8(get_lun_info, discover );
	PULL_LIST(get_lun_info, lun_id, "List of stpLunId");

        return MAX(var_offset, offset);
}

/*
  dissect stpGetLunInfoMsg
*/
static int dissect_tank_get_lun_list(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_get_lun_list pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_lun_list);

	PULL_UINT8(get_lun_list, discover );

        return MAX(var_offset, offset);
}

/*
  dissect stpGetLunInfoListRespMsg
*/
static int dissect_tank_get_lun_info_resp(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_get_lun_info_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_lun_info_resp);

        PULL_UINT8( get_lun_info_resp, more_flag );
	PULL_LIST(  get_lun_info_resp, lun_info, "List of stpLunInfo");

	return MAX(var_offset, offset);
}

/*
  dissect stpGetVolInfoMsg
*/
static int dissect_tank_get_vol_info(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_get_vol_info pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_vol_info);

	PULL_UINT8(get_vol_info, discover );
	PULL_VECTOR(get_vol_info, vol_id, "List of stpVolId");

        return MAX(var_offset, offset);
}

/*
  dissect stpGetVolListMsg
*/
static int dissect_tank_get_vol_list(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_get_vol_list pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_vol_list);

	PULL_UINT8(get_vol_list, discover );

        return MAX(var_offset, offset);
}

/*
  dissect get_vol_info_resp
*/
static int dissect_tank_get_vol_info_resp(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_get_vol_info_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_get_vol_info_resp);

        PULL_UINT8( get_vol_info_resp, more_flag );
	PULL_LIST(  get_vol_info_resp, vol_info, "List of stpVolInfo");

	return MAX(var_offset, offset);
}

/*
  dissect read_sector
*/
static int dissect_tank_read_sector(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_read_sector pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_read_sector);

        PULL_UINT32( read_sector, offset );
        PULL_LUN_OR_VOL( read_sector, rs_device, "Lun Id or Volume GID" );

	return offset;
}

/*
  dissect read_sector_resp
*/
static int dissect_tank_read_sector_resp(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_read_sector_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_read_sector_resp);

        PULL_UINT32( read_sector_resp, result_code );
        PULL_CHAR_ARRAY(write_sector, payload, "Sector Data", STP_SECTOR_SIZE );

	return MAX(var_offset, offset);
}

/*
  dissect write_sector
*/
static int dissect_tank_write_sector(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_write_sector pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;

	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_write_sector);

        PULL_UINT32( write_sector, offset );
        PULL_CHAR_ARRAY(write_sector, payload, "Sector Data", STP_SECTOR_SIZE );
        PULL_LUN_OR_VOL(write_sector, device_name, "Device Name" );

	return MAX(var_offset, offset);
}

/*
  dissect write_sector_resp
*/
static int dissect_tank_write_sector_resp(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
        struct tank_write_sector_resp pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_write_sector_resp);

        PULL_UINT32( write_sector_resp, result_code );

	return MAX(var_offset, offset);
}

/*
  dissect copy data
*/
static int dissect_tank_copy_data(tvbuff_t *tvb, packet_info *pinfo,
                                  proto_tree *parent_tree, int offset,
                                  struct tank_header *hdr)
{
//        struct tank_copy_data pkt;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_copy_data);

	PULL_VECTOR( copy_data, copy_data_request, "Copy Data Requests");

	return MAX(var_offset, offset);
}

static int dissect_tank_copy_data_resp(tvbuff_t *tvb, packet_info *pinfo,
                                       proto_tree *parent_tree, int offset,
                                       struct tank_header *hdr)
{
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	int var_offset = offset;

	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	item = proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	tree = proto_item_add_subtree(item, ett_copy_data_resp);

	PULL_VECTOR( copy_data_resp, copy_result, "Copy data responses");

	return MAX(var_offset, offset);
}


/*
  dissect Null message
*/
static int dissect_tank_null(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_null pkt;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	(void) proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	return offset;
}


/*
  dissect Null Resp message
*/
static int dissect_tank_null_resp(tvbuff_t *tvb, packet_info *pinfo,
				proto_tree *parent_tree, int offset,
				struct tank_header *hdr)
{
//	struct tank_null_resp pkt;
	struct stp_private *sinfo _U_ = (struct stp_private *)pinfo->private_data;
	(void) proto_tree_add_text(parent_tree, tvb, offset,
				   hdr->msg_length - offset, 
				   tank_cmd_type(hdr->cmd_type));
	return offset;
}

/*---------- End FlexSAN message dissect functions ----------*/

/*
  the TANK command types
 */
static struct {
	guint16 cmd;
	const char *name;
	int (*dissect)(tvbuff_t *tvb, packet_info *pinfo, 
		       proto_tree *parent_tree, int offset, 
		       struct tank_header *hdr);
} tank_cmds[] = {
	{ TANK_CMD_IDENTIFY,           "Identify" ,            dissect_tank_identify},
	{ TANK_CMD_IDENTIFY_RESPONSE,  "IdentifyResponse" ,    dissect_tank_identify_response},
	{ TANK_CMD_ACQUIRE_DATA_LOCK,  "AcquireDataLock" ,     dissect_tank_acquire_data_lock},
	{ TANK_CMD_ACQUIRE_DATA_LOCK_RESPONSE,"AcquireDataLockResponse",dissect_tank_acquire_data_lock_response},
	{ TANK_CMD_ACQUIRE_SESSION_LOCK,  "AcquireSessionLock" ,     dissect_tank_acquire_session_lock},
	{ TANK_CMD_ACQUIRE_SESSION_LOCK_RESPONSE,"AcquireSessionLockResponse",dissect_tank_acquire_session_lock_response},
	{ TANK_CMD_DOWNGRADE_SESSION_LOCK,  "DowngradeSessionLock" ,     dissect_tank_downgrade_session_lock},
	{ TANK_CMD_PUBLISH_CLUSTER_INFO,  "PublishClusterInfo" , dissect_tank_publish_cluster_info},
	{ TANK_CMD_LOOKUP_NAME,  "LookupName" , dissect_tank_lookup_name},
	{ TANK_CMD_LOOKUP_NAME_RESPONSE,  "LookupNameResponse" , dissect_tank_lookup_name_response},
	{ TANK_CMD_CHANGE_NAME,  "ChangeName" , dissect_tank_change_name},
	{ TANK_CMD_CHANGE_NAME_RESPONSE,  "ChangeNameResponse" , dissect_tank_change_name_response},
	{ TANK_CMD_REPORT_TXN_STATUS,  "ReportTxnStatus" , dissect_tank_report_txn_status},
	{ TANK_CMD_GET_STORAGE_CAPACITY,  "GetStorageCapacity" , dissect_tank_get_storage_capacity},
	{ TANK_CMD_GET_STORAGE_CAPACITY_RESPONSE,  "GetStorageCapacityResponse" , dissect_tank_get_storage_capacity_response},
	{ TANK_CMD_CREATE_FILE,  "CreateFile" , dissect_tank_create_file},
	{ TANK_CMD_CREATE_FILE_RESPONSE,  "CreateFileResponse" , dissect_tank_create_file_response},
	{ TANK_CMD_CREATE_DIR,  "CreateDir" , dissect_tank_create_dir},
	{ TANK_CMD_CREATE_DIR_RESPONSE,  "CreateDirResponse" , dissect_tank_create_dir_response},
	{ TANK_CMD_RENEW_LEASE, "RenewLease", dissect_tank_renew_lease },
	{ TANK_CMD_PING, "Ping", dissect_tank_ping },
	{ TANK_CMD_SHUTDOWN, "Shutdown", dissect_tank_shutdown },
	{ TANK_CMD_NOTIFY_FORWARD, "NotifyForward", dissect_tank_notify_forward },
	{ TANK_CMD_CREATE_HARD_LINK, "CreateHardLink", dissect_tank_create_hard_link },
	{ TANK_CMD_CREATE_HARD_LINK_RESP, "CreateHardLinkResp", dissect_tank_create_hard_link_resp },
	{ TANK_CMD_CREATE_SYM_LINK, "CreateSymLink", dissect_tank_create_sym_link },
	{ TANK_CMD_CREATE_SYM_LINK_RESP, "CreateSymLinkResp", dissect_tank_create_sym_link_resp },
	{ TANK_CMD_REMOVE_NAME, "RemoveName", dissect_tank_remove_name },
	{ TANK_CMD_REMOVE_NAME_RESP, "RemoveNameResp", dissect_tank_remove_name_resp },
	{ TANK_CMD_SET_BASIC_OBJ_ATTR, "SetBasicObjAttr", dissect_tank_set_basic_obj_attr },
	{ TANK_CMD_SET_BASIC_OBJ_ATTR_RESP, "SetBasicObjAttrResp", dissect_tank_set_basic_obj_attr_resp },
	{ TANK_CMD_SET_ACCESS_CTL_ATTR, "SetAccessCtlAttr", dissect_tank_set_access_ctl_attr },
	{ TANK_CMD_SET_ACCESS_CTL_ATTR_RESP, "SetAccessCtlAttrResp", dissect_tank_set_access_ctl_attr_resp },
	{ TANK_CMD_UPDATE_ACCESS_TIME, "UpdateAccessTime", dissect_tank_update_access_time },
	{ TANK_CMD_PUBLISH_ACCESS_TIME, "PublishAccessTime", dissect_tank_publish_access_time },
	{ TANK_CMD_READ_DIR, "ReadDir", dissect_tank_read_dir },
	{ TANK_CMD_READ_DIR_RESP, "ReadDirResp", dissect_tank_read_dir_resp },
	{ TANK_CMD_FIND_OBJECT, "FindObject", dissect_tank_find_object },
	{ TANK_CMD_FIND_OBJECT_RESP, "FindObjectResp", dissect_tank_find_object_resp },
	{ TANK_CMD_DEMAND_SESSION_LOCK, "DemandSessionLock", dissect_tank_demand_session_lock },
	{ TANK_CMD_DENY_SESSION_LOCK, "DenySessionLock", dissect_tank_deny_session_lock },
	{ TANK_CMD_PUBLISH_LOCK_VERSION, "PublishLockVersion", dissect_tank_publish_lock_version },
	{ TANK_CMD_DOWNGRADE_DATA_LOCK, "DowngradeDataLock", dissect_tank_downgrade_data_lock },
	{ TANK_CMD_DEMAND_DATA_LOCK, "DemandDataLock", dissect_tank_demand_data_lock },
	{ TANK_CMD_DEFERRED_DOWNGRADE_DATA_LOCK, "DeferredDowngradeDataLock", dissect_tank_deferred_downgrade_data_lock },
	{ TANK_CMD_INVALIDATE_DIRECTORY, "InvalidateDirectory", dissect_tank_invalidate_directory },
	{ TANK_CMD_DISCARD_DIRECTORY, "DiscardDirectory", dissect_tank_discard_directory },
	{ TANK_CMD_INVALIDATE_OBJ_ATTR, "InvalidateObjAttr", dissect_tank_invalidate_obj_attr },
	{ TANK_CMD_DISCARD_OBJ_ATTR, "DiscardObjAttr", dissect_tank_discard_obj_attr },
	{ TANK_CMD_PUBLISH_BASIC_OBJ_ATTR, "PublishBasicObjAttr", dissect_tank_publish_basic_obj_attr },
	{ TANK_CMD_BLK_DISK_ALLOCATE, "BlkDiskAllocate", dissect_tank_blk_disk_allocate },
	{ TANK_CMD_BLK_DISK_ALLOCATE_RESP, "BlkDiskAllocateResp", dissect_tank_blk_disk_allocate_resp },
	{ TANK_CMD_BLK_DISK_UPDATE, "BlkDiskUpdate", dissect_tank_blk_disk_update },
	{ TANK_CMD_BLK_DISK_UPDATE_RESP, "BlkDiskUpdateResp", dissect_tank_blk_disk_update_resp },
	{ TANK_CMD_BLK_DISK_GET_SEGMENT, "BlkDiskGetSegment", dissect_tank_blk_disk_get_segment },
	{ TANK_CMD_BLK_DISK_GET_SEGMENT_RESP, "BlkDiskGetSegmentResp", dissect_tank_blk_disk_get_segment_resp },
	{ TANK_CMD_SET_RANGE_LOCK, "SetRangeLock", dissect_tank_set_range_lock },
	{ TANK_CMD_SET_RANGE_LOCK_RESP, "SetRangeLockResp", dissect_tank_set_range_lock_resp },
	{ TANK_CMD_DEMAND_RANGE_LOCK, "DemandRangeLock", dissect_tank_demand_range_lock },
	{ TANK_CMD_DEMAND_RANGE_LOCK_RESP, "DemandRangeLockResp", dissect_tank_demand_range_lock_resp },
	{ TANK_CMD_RELINQUISH_RANGE_LOCK, "RelinquishRangeLock", dissect_tank_relinquish_range_lock },
	{ TANK_CMD_RETRY_RANGE_LOCK, "RetryRangeLock", dissect_tank_retry_range_lock },
	{ TANK_CMD_COMPAT_SET_RANGE_LOCK, "CompatSetRangeLock", dissect_tank_compat_set_range_lock },
	{ TANK_CMD_COMPAT_CHECK_RANGE_LOCK, "CompatCheckRangeLock", dissect_tank_compat_check_range_lock },
	{ TANK_CMD_COMPAT_CHECK_RANGE_LOCK_RESP, "CompatCheckRangeLockResp", dissect_tank_compat_check_range_lock_resp },
	{ TANK_CMD_RELEASE_ALL_RANGE_LOCKS, "ReleaseAllRangeLocks", dissect_tank_release_all_range_locks },
	{ TANK_CMD_PUBLISH_LOAD_UNIT_INFO, "PublishLoadUnitInfo", dissect_tank_publish_load_unit_info },
	{ TANK_CMD_PUBLISH_ROOT_CLT_INFO, "PublishRootCltInfo", dissect_tank_publish_root_clt_info },
	{ TANK_CMD_PUBLISH_EVICTION_REQUEST, "PublishEvictionRequest", dissect_tank_publish_eviction_request },

        /*---------- Start FlexSAN message mappings ----------*/

        { TANK_CMD_GET_LUN_LIST, "GetLunList", dissect_tank_get_lun_list },
        { TANK_CMD_GET_LUN_LIST_RESP, "GetLunListResp", dissect_tank_get_lun_info_resp },
        { TANK_CMD_GET_LUN_INFO, "GetLunInfo", dissect_tank_get_lun_info },
        { TANK_CMD_GET_LUN_INFO_RESP, "GetLunInfoResp", dissect_tank_get_lun_info_resp },
        { TANK_CMD_GET_VOL_LIST, "GetVolList", dissect_tank_get_vol_list },
        { TANK_CMD_GET_VOL_LIST_RESP, "GetVolListResp", dissect_tank_get_vol_info_resp },
        { TANK_CMD_GET_VOL_INFO, "GetVolInfo", dissect_tank_get_vol_info },
        { TANK_CMD_GET_VOL_INFO_RESP, "GetVolInfoResp", dissect_tank_get_vol_info_resp },
        { TANK_CMD_READ_SECTOR, "ReadSector", dissect_tank_read_sector },
        { TANK_CMD_READ_SECTOR_RESP, "ReadSectorResp", dissect_tank_read_sector_resp },
        { TANK_CMD_WRITE_SECTOR, "WriteSector", dissect_tank_write_sector },
        { TANK_CMD_WRITE_SECTOR_RESP, "WriteSectorResp", dissect_tank_write_sector_resp },
        { TANK_CMD_COPY_DATA, "CopyData", dissect_tank_copy_data },
        { TANK_CMD_COPY_DATA_RESP, "CopyDataResp", dissect_tank_copy_data_resp },
	{ TANK_CMD_CANCEL, "Cancel", dissect_tank_null },
	{ TANK_CMD_CANCEL_RESP, "CancelResp", dissect_tank_null_resp },
	{ TANK_CMD_NULL, "Null", dissect_tank_null },
	{ TANK_CMD_NULL_RESP, "NullResp", dissect_tank_null_resp },

        /*---------- End FlexSAN message mappings ----------*/
};

static const gchar *tank_cmd_type(guint16 cmd_type)
{
	guint16 i;
	for (i=0;i<array_length(tank_cmds);i++) {
		if (tank_cmds[i].cmd == cmd_type) {
			return tank_cmds[i].name;
		}
	}
	return "Unknown";
}


/*
  dissect the TANK portion of a packet
*/
static void dissect_tank(tvbuff_t *parent_tvb, packet_info *pinfo, 
			proto_tree *parent_tree, int offset, int length)
{
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	tvbuff_t *tvb;
	struct tank_header pkt;
	int remaining;
	unsigned i;
	struct stp_private *sinfo _U_ = pinfo->private_data;

	tvb = tvb_new_subset(parent_tvb, offset, length, length);

	offset = 0;

	/* check that this is a TANK packet */
	if (!tvb_bytes_exist(tvb, offset, 4) || 
	    (tvb_get_guint8(tvb, offset+0) != 'T') || 
	    (tvb_get_guint8(tvb, offset+1) != 'A') ||
	    (tvb_get_guint8(tvb, offset+2) != 'N') ||
	    (tvb_get_guint8(tvb, offset+3) != 'K')) {
		return;
	}

	item = proto_tree_add_text(parent_tree, tvb, offset, length, "TANK protocol");
	tree = proto_item_add_subtree(item, ett_tank);
	offset += 4;

	PULL_UINT16(header, version);

	pkt.msg_type = tvb_get_guint8(tvb, offset) >> 4;
	proto_tree_add_uint(tree, HF(header, msg_type), tvb, offset, 1, pkt.msg_type);

	pkt.cmd_type = tvb_get_ntohs(tvb, offset) & 0x0FFF;
	proto_tree_add_uint(tree, HF(header, cmd_type), tvb, offset, 2, pkt.cmd_type);
	offset += 2;

	PULL_UINT32(header, msg_length);
	PULL_UINT64(header, client_id);
	PULL_CHAR_ARRAY(header, padding, "padding", 12);

	if (pkt.msg_type == 0x8 ||
	    pkt.msg_type == 0x4) {
		PULL_UINT64(header, transaction_context);		
	}

	if (check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, "TANK %s : %s(0x%x)", 
			    tank_msg_type(pkt.msg_type),
			    tank_cmd_type(pkt.cmd_type),
			    pkt.cmd_type);
	}

	/* find the right scalpel for the job */
	for (i=0;i<array_length(tank_cmds); i++) {
		if (tank_cmds[i].cmd == pkt.cmd_type) {
			offset = tank_cmds[i].dissect(tvb, pinfo, tree, offset, &pkt);
			break;
		}
	}
	/* fallback to a default handler */
	if (i == array_length(tank_cmds)) {
		offset = dissect_tank_unknown(tvb, pinfo, tree, offset, &pkt);
	}

	/* any leftover data */
	remaining = tvb_reported_length(tvb) - offset;
	if (remaining != 0) {
		proto_tree_add_text(tree, tvb, offset, remaining,
				    "Unknown Data[%d]: %s", 
				    remaining,
				    tvb_bytes_to_str(tvb, offset, remaining));
	}
}
	
static void dissect_stnl(tvbuff_t *tvb, packet_info *pinfo, 
			 proto_tree *parent_tree)
{
	int offset = 0;
	proto_item *item = NULL;
	proto_tree *tree = NULL;
	struct stnl_header pkt;
	struct stp_private *sinfo;

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "STNL");
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	/* check that this is a STNL packet */
	if (!tvb_bytes_exist(tvb, 0, 4) || 
	    (tvb_get_guint8(tvb, 0) != 'S') || 
	    (tvb_get_guint8(tvb, 1) != 'T') ||
	    (tvb_get_guint8(tvb, 2) != 'N') ||
	    (tvb_get_guint8(tvb, 3) != 'L')) {
		return;
	}

	/* setup our private data */
	sinfo = &stp_private;
	pinfo->private_data = sinfo;

	if (!sinfo->initialised) {
		fprintf(stderr, "Adding private data\n");
		/* put in best guess values in case we don't see the start of the stream */
		fprintf(stderr, "Enabling unicode by default\n");
		sinfo->unicode = TRUE;
		sinfo->initialised = TRUE;
	}
	
	if (parent_tree) {
		item = proto_tree_add_item(parent_tree, proto_stnl, tvb, offset, -1, FALSE);
		tree = proto_item_add_subtree(item, ett_stnl);
	}

	proto_tree_add_text(tree, tvb, offset, 4, "STNL protocol");
	offset += 4;

	PULL_UINT16(stnl, msg_type);
	PULL_UINT16(stnl, msg_length);
	PULL_UINT64(stnl, msg_number);
	PULL_UINT64(stnl, receiver_id);
	PULL_UINT64(stnl, sender_id);

	if (pkt.msg_length == offset &&
	    check_col(pinfo->cinfo, COL_INFO)) {
		col_append_fstr(pinfo->cinfo, COL_INFO, stnl_msg_type(pkt.msg_type));
	}

	dissect_tank(tvb, pinfo, tree, offset, pkt.msg_length - offset);
}


/*
  fill and register a field array
*/
static void tank_register_field_array(int proto, hf_register_info *hf, int alen)
{
	int i;
	for (i=0;i<alen;i++) {
		hf[i].p_id = g_malloc(sizeof(int));
//		fprintf(stderr, "Added hf_id for %s\n", hf[i].hfinfo.abbrev);
	}
	proto_register_field_array(proto, hf, alen);
}

#define REGISTER_FIELD_ARRAY(sname) do { \
	tank_register_field_array(proto_stnl, hf_tank_ ## sname, array_length(hf_tank_ ## sname)); \
} while (0)

/*
  register the STNL protocol handlers and field arrays
*/
void proto_register_stnl(void)
{
	static gint *ett[] = {
		&ett_stnl,
		&ett_tank,
		&ett_identify,
		&ett_identify_response,
		&ett_acquire_data_lock,
		&ett_acquire_data_lock_response,
		&ett_acquire_session_lock,
		&ett_acquire_session_lock_response,
		&ett_downgrade_session_lock,
		&ett_uoid,
		&ett_dvers,
		&ett_svers,
		&ett_extent,
		&ett_extent_list,
		&ett_seg_update,
		&ett_seg_descr,
		&ett_net_interface,
		&ett_access_time,
		&ett_dir_entry,
		&ett_vector,
		&ett_list,
		&ett_objattr,
		&ett_basic_objattr,
		&ett_gdl,
		&ett_gsl,
		&ett_srvaddr,
		&ett_netaddr,
		&ett_publish_cluster_info,
		&ett_report_txn_status,
		&ett_lookup_name,
		&ett_lookup_name_response,
		&ett_change_name,
		&ett_change_name_response,
		&ett_get_storage_capacity,
		&ett_get_storage_capacity_response,
		&ett_create_file,
		&ett_create_file_response,
		&ett_create_dir,
		&ett_create_dir_response,
		&ett_renew_lease,
		&ett_ping,
		&ett_shutdown,
		&ett_notify_forward,
		&ett_create_hard_link,
		&ett_create_hard_link_resp,
		&ett_create_sym_link,
		&ett_create_sym_link_resp,
		&ett_remove_name,
		&ett_remove_name_resp,
		&ett_set_basic_obj_attr,
		&ett_set_basic_obj_attr_resp,
		&ett_set_access_ctl_attr,
		&ett_set_access_ctl_attr_resp,
		&ett_update_access_time,
		&ett_publish_access_time,
		&ett_read_dir,
		&ett_read_dir_resp,
		&ett_find_object,
		&ett_find_object_resp,
		&ett_demand_session_lock,
		&ett_deny_session_lock,
		&ett_publish_lock_version,
		&ett_downgrade_data_lock,
		&ett_demand_data_lock,
		&ett_deferred_downgrade_data_lock,
		&ett_invalidate_directory,
		&ett_discard_directory,
		&ett_invalidate_obj_attr,
		&ett_discard_obj_attr,
		&ett_publish_basic_obj_attr,
		&ett_blk_disk_allocate,
		&ett_blk_disk_allocate_resp,
		&ett_blk_disk_update,
		&ett_blk_disk_update_resp,
		&ett_blk_disk_get_segment,
		&ett_blk_disk_get_segment_resp,
		&ett_set_range_lock,
		&ett_set_range_lock_resp,
		&ett_demand_range_lock,
		&ett_demand_range_lock_resp,
		&ett_relinquish_range_lock,
		&ett_retry_range_lock,
		&ett_compat_set_range_lock,
		&ett_compat_check_range_lock,
		&ett_compat_check_range_lock_resp,
		&ett_release_all_range_locks,
		&ett_publish_load_unit_info,
		&ett_publish_root_clt_info,
		&ett_publish_eviction_request,

                /*---------- Start FlexSAN data mappings ----------*/

                &ett_lun_id,
                &ett_lun_or_vol,
                &ett_lun_info,
                &ett_vol_id,
                &ett_vol_info,
                &ett_copy_data_request,
                &ett_copy_result,

                /*---------- Start FlexSAN message mappings ----------*/

                &ett_get_lun_info,
                &ett_get_lun_info_resp,
                &ett_get_lun_list,
                &ett_get_lun_list_resp,
                &ett_get_vol_info,
                &ett_get_vol_info_resp,
                &ett_get_vol_list,
                &ett_get_vol_list_resp,
                &ett_read_sector,
                &ett_read_sector_resp,
                &ett_write_sector,
                &ett_write_sector_resp,
                &ett_copy_data,
                &ett_copy_data_resp,
		&ett_null,
		&ett_null_resp,

                /*---------- End FlexSAN message mappings ----------*/

		&ett_unknown
	};


	proto_stnl = proto_register_protocol("STNL Protocol", "STNL", "stnl");

	proto_register_subtree_array(ett, array_length(ett));

	REGISTER_FIELD_ARRAY(stnl);
	REGISTER_FIELD_ARRAY(header);
	REGISTER_FIELD_ARRAY(identify);
	REGISTER_FIELD_ARRAY(identify_response);
	REGISTER_FIELD_ARRAY(acquire_data_lock);
	REGISTER_FIELD_ARRAY(acquire_data_lock_response);
	REGISTER_FIELD_ARRAY(acquire_session_lock);
	REGISTER_FIELD_ARRAY(acquire_session_lock_response);
	REGISTER_FIELD_ARRAY(downgrade_session_lock);
	REGISTER_FIELD_ARRAY(uoid);
	REGISTER_FIELD_ARRAY(dvers);
	REGISTER_FIELD_ARRAY(svers);
	REGISTER_FIELD_ARRAY(extent);
	REGISTER_FIELD_ARRAY(seg_update);
	REGISTER_FIELD_ARRAY(seg_descr);
	REGISTER_FIELD_ARRAY(net_interface);
	REGISTER_FIELD_ARRAY(access_time);
	REGISTER_FIELD_ARRAY(dir_entry);
	REGISTER_FIELD_ARRAY(vector);
	REGISTER_FIELD_ARRAY(list);
	REGISTER_FIELD_ARRAY(publish_cluster_info);
	REGISTER_FIELD_ARRAY(report_txn_status);
	REGISTER_FIELD_ARRAY(gdl);
	REGISTER_FIELD_ARRAY(gsl);
	REGISTER_FIELD_ARRAY(srvaddr);
	REGISTER_FIELD_ARRAY(netaddr);
	REGISTER_FIELD_ARRAY(objattr);
	REGISTER_FIELD_ARRAY(basic_objattr);
	REGISTER_FIELD_ARRAY(lookup_name);
	REGISTER_FIELD_ARRAY(lookup_name_response);
	REGISTER_FIELD_ARRAY(change_name);
	REGISTER_FIELD_ARRAY(change_name_response);
	REGISTER_FIELD_ARRAY(get_storage_capacity);
	REGISTER_FIELD_ARRAY(get_storage_capacity_response);
	REGISTER_FIELD_ARRAY(create_file);
	REGISTER_FIELD_ARRAY(create_file_response);
	REGISTER_FIELD_ARRAY(create_dir);
	REGISTER_FIELD_ARRAY(create_dir_response);
	REGISTER_FIELD_ARRAY(renew_lease);
	REGISTER_FIELD_ARRAY(ping);
	REGISTER_FIELD_ARRAY(shutdown);
	REGISTER_FIELD_ARRAY(notify_forward);
	REGISTER_FIELD_ARRAY(create_hard_link);
	REGISTER_FIELD_ARRAY(create_hard_link_resp);
	REGISTER_FIELD_ARRAY(create_sym_link);
	REGISTER_FIELD_ARRAY(create_sym_link_resp);
	REGISTER_FIELD_ARRAY(remove_name);
	REGISTER_FIELD_ARRAY(remove_name_resp);
	REGISTER_FIELD_ARRAY(set_basic_obj_attr);
	REGISTER_FIELD_ARRAY(set_basic_obj_attr_resp);
	REGISTER_FIELD_ARRAY(set_access_ctl_attr);
	REGISTER_FIELD_ARRAY(set_access_ctl_attr_resp);
	REGISTER_FIELD_ARRAY(update_access_time);
	REGISTER_FIELD_ARRAY(publish_access_time);
	REGISTER_FIELD_ARRAY(read_dir);
	REGISTER_FIELD_ARRAY(read_dir_resp);
	REGISTER_FIELD_ARRAY(find_object);
	REGISTER_FIELD_ARRAY(find_object_resp);
	REGISTER_FIELD_ARRAY(demand_session_lock);
	REGISTER_FIELD_ARRAY(deny_session_lock);
	REGISTER_FIELD_ARRAY(publish_lock_version);
	REGISTER_FIELD_ARRAY(downgrade_data_lock);
	REGISTER_FIELD_ARRAY(demand_data_lock);
	REGISTER_FIELD_ARRAY(deferred_downgrade_data_lock);
	REGISTER_FIELD_ARRAY(invalidate_directory);
	REGISTER_FIELD_ARRAY(discard_directory);
	REGISTER_FIELD_ARRAY(invalidate_obj_attr);
	REGISTER_FIELD_ARRAY(discard_obj_attr);
	REGISTER_FIELD_ARRAY(publish_basic_obj_attr);
	REGISTER_FIELD_ARRAY(blk_disk_allocate);
	REGISTER_FIELD_ARRAY(blk_disk_allocate_resp);
	REGISTER_FIELD_ARRAY(blk_disk_update);
	REGISTER_FIELD_ARRAY(blk_disk_update_resp);
	REGISTER_FIELD_ARRAY(blk_disk_get_segment);
	REGISTER_FIELD_ARRAY(blk_disk_get_segment_resp);
	REGISTER_FIELD_ARRAY(set_range_lock);
	REGISTER_FIELD_ARRAY(set_range_lock_resp);
	REGISTER_FIELD_ARRAY(demand_range_lock);
	REGISTER_FIELD_ARRAY(demand_range_lock_resp);
	REGISTER_FIELD_ARRAY(relinquish_range_lock);
	REGISTER_FIELD_ARRAY(retry_range_lock);
	REGISTER_FIELD_ARRAY(compat_set_range_lock);
	REGISTER_FIELD_ARRAY(compat_check_range_lock);
	REGISTER_FIELD_ARRAY(compat_check_range_lock_resp);
	REGISTER_FIELD_ARRAY(release_all_range_locks);
	REGISTER_FIELD_ARRAY(publish_load_unit_info);
	REGISTER_FIELD_ARRAY(publish_root_clt_info);
	REGISTER_FIELD_ARRAY(publish_eviction_request);

        /*---------- Start FlexSAN data mappings ----------*/

        REGISTER_FIELD_ARRAY(lun_id);
        REGISTER_FIELD_ARRAY(lun_or_vol);
        REGISTER_FIELD_ARRAY(lun_info);
        REGISTER_FIELD_ARRAY(vol_id);
        REGISTER_FIELD_ARRAY(vol_info);
        REGISTER_FIELD_ARRAY(copy_data_request);
        REGISTER_FIELD_ARRAY(copy_result);

        /*---------- Start FlexSAN message mappings ----------*/

        REGISTER_FIELD_ARRAY(get_lun_list);
        REGISTER_FIELD_ARRAY(get_lun_info);
        REGISTER_FIELD_ARRAY(get_lun_info_resp);
        REGISTER_FIELD_ARRAY(get_vol_list);
        REGISTER_FIELD_ARRAY(get_vol_info);
        REGISTER_FIELD_ARRAY(get_vol_info_resp);
        REGISTER_FIELD_ARRAY(read_sector);
        REGISTER_FIELD_ARRAY(read_sector_resp);
        REGISTER_FIELD_ARRAY(write_sector);
        REGISTER_FIELD_ARRAY(write_sector_resp);
        REGISTER_FIELD_ARRAY(copy_data);
        REGISTER_FIELD_ARRAY(copy_data_resp);
	REGISTER_FIELD_ARRAY(null);
	REGISTER_FIELD_ARRAY(null_resp);

        /*---------- End FlexSAN message mappings ----------*/
}


/*
  tell ethereal which port to parse by default
*/
void proto_reg_handoff_stnl(void)
{
	dissector_handle_t stnl_handle;

	stnl_handle = create_dissector_handle(dissect_stnl, proto_stnl);
	dissector_add("tcp.port", TCP_PORT_STNL, stnl_handle);
	dissector_add("udp.port", TCP_PORT_STNL, stnl_handle);

	/* these are for debug convenience */
	dissector_add("tcp.port", 10190, stnl_handle);
	dissector_add("tcp.port", 10191, stnl_handle);
}
