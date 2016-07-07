/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "ble_data_sync.h"
#include "ble_types.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"
#include <stddef.h>
#include "sdk_common.h"
#include "app_error.h"

#define MAX_DATA_SYNC_PKT_LEN         20                                        /**< Maximum length (in bytes) of the DFU Packet characteristic. */
#define PKT_START_DATA_SYNC_PARAM_LEN 2                                         /**< Length (in bytes) of the parameters for Packet Start DFU Request. */
#define PKT_INIT_DATA_SYNC_PARAM_LEN  2                                         /**< Length (in bytes) of the parameters for Packet Init DFU Request. */
#define PKT_RCPT_NOTIF_REQ_LEN  3                                               /**< Length (in bytes) of the Packet Receipt Notification Request. */
#define MAX_PKTS_RCPT_NOTIF_LEN 6                                               /**< Maximum length (in bytes) of the Packets Receipt Notification. */
#define MAX_RESPONSE_LEN        7                                               /**< Maximum length (in bytes) of the response to a Control Point command. */
#define MAX_NOTIF_BUFFER_LEN    MAX(MAX_PKTS_RCPT_NOTIF_LEN, MAX_RESPONSE_LEN)  /**< Maximum length (in bytes) of the buffer needed by DFU Service while sending notifications to peer. */

enum
{
    OP_CODE_START_DATA_SYNC    = 1,                                             /**< Value of the Op code field for 'Start DFU' command.*/
    OP_CODE_RESPONSE           = 0X5B,                                            /**< Value of the Op code field for 'Response.*/
    
};

static bool     m_is_data_sync_service_initialized = false;                           /**< Variable to check if the DFU service was initialized by the application.*/
static uint8_t  m_notif_buffer[MAX_NOTIF_BUFFER_LEN];                           /**< Buffer used for sending notifications to peer. */

/**@brief     Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the S110 SoftDevice.
 *
 * @param[in] p_data     DFU Service Structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(ble_data_sync_t * p_data, ble_evt_t * p_ble_evt)
{
    p_data->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief     Function for checking if the CCCD of data sync Control point is configured for Notification.
 *
 * @details   This function checks if the CCCD of data sync Control Point characteristic is configured
 *            for Notification by the DFU Controller.
 *
 * @param[in] p_data data sync Service structure.
 *
 * @return    True if the CCCD of data sync Control Point characteristic is configured for Notification.
 *            False otherwise.
 */
static bool is_cccd_configured(ble_data_sync_t * p_data)
{
    // Check if the CCCDs are configured.
    uint8_t  cccd_val_buf[BLE_CCCD_VALUE_LEN];
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = BLE_CCCD_VALUE_LEN;
    gatts_value.offset  = 0;
    gatts_value.p_value = cccd_val_buf;

    // Check the CCCD Value of data sync Control Point.
    uint32_t err_code = sd_ble_gatts_value_get(p_data->conn_handle,
                                               p_data->data_sync_ctrl_pt_handles.cccd_handle,
                                               &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        if (p_data->error_handler != NULL)
        {
            p_data->error_handler(err_code);
        }
        return false;
    }

    return ble_srv_is_notification_enabled(cccd_val_buf);
}


/**@brief     Function for handling the @ref BLE_GATTS_EVT_WRITE event from the S110 SoftDevice.
 *
 * @param[in] p_data     DFU Service Structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_data_sync_t * p_data, ble_evt_t * p_ble_evt)
{
		/*
    if (p_ble_evt->evt.gatts_evt.params.write.handle == p_data->dfu_pkt_handles.value_handle)
    {
        // DFU Packet written
        ble_dfu_evt_t ble_dfu_evt;

        ble_dfu_evt.ble_dfu_evt_type             = BLE_DFU_PACKET_WRITE;
        ble_dfu_evt.evt.ble_dfu_pkt_write.len    = p_ble_evt->evt.gatts_evt.params.write.len;
        ble_dfu_evt.evt.ble_dfu_pkt_write.p_data = p_ble_evt->evt.gatts_evt.params.write.data;

        p_data->evt_handler(p_data, &ble_dfu_evt);
    }
	*/
		if (p_ble_evt->evt.gatts_evt.params.write.handle == p_data->data_sync_ctrl_pt_handles.value_handle)
		{
			  if (!is_cccd_configured(p_data))
				{
						// Send an error response to the peer indicating that the CCCD is improperly configured.
						return ;
				}
				
				ble_gatts_evt_write_t p_ble_write_evt = p_ble_evt->evt.gatts_evt.params.write;
				uint8_t header = p_ble_evt->evt.gatts_evt.params.write.data[0];

				if (header == 0x5A)
				{
						ble_data_sync_response_send(p_data, BLE_DATA_SYNC_INIT_PROCEDURE, BLE_DATA_SYNC_RESP_VAL_SUCCESS);
						return;
				}
				
		}
}


/**@brief     Function for handling the BLE_GAP_EVT_DISCONNECTED event from the S110 SoftDevice.
 *
 * @param[in] p_data     DFU Service Structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_disconnect(ble_data_sync_t * p_data, ble_evt_t * p_ble_evt)
{
    p_data->conn_handle = BLE_CONN_HANDLE_INVALID;
}


void ble_data_sync_on_ble_evt(ble_data_sync_t * p_data, ble_evt_t * p_ble_evt)
{
	
    if ((p_data == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    //if (p_data->evt_handler != NULL)
    {
        switch (p_ble_evt->header.evt_id)
        {
            case BLE_GAP_EVT_CONNECTED:
                on_connect(p_data, p_ble_evt);
                break;

            case BLE_GATTS_EVT_WRITE:
                on_write(p_data, p_ble_evt);
                break;

            case BLE_GAP_EVT_DISCONNECTED:
                on_disconnect(p_data, p_ble_evt);
                break;

            default:
                // No implementation needed.
                break;
        }
    }
}


/**@brief       Function for adding data layout Revision characteristic to the BLE Stack.
 *
 * @param[in]   p_data data sync Service structure.
 *
 * @return      NRF_SUCCESS on success. Otherwise an error code.
 */
static uint32_t data_sync_rev_char_add(ble_data_sync_t * const p_data, ble_data_sync_init_t const * const p_data_sync_init)
{
    // OUR_JOB: Step 2.A, Add a custom characteristic UUID
		uint32_t            err_code;
		ble_uuid_t          char_uuid;
	
	  char_uuid.type = p_data->uuid_type;
		char_uuid.uuid = BLE_DATA_SYNC_REV_CHAR_UUID;
		
    
    // OUR_JOB: Step 2.F Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));		
		char_md.char_props.read = 1;
		char_md.char_props.write = 0;
    
    // OUR_JOB: Step 3.A, Configuring Client Characteristic Configuration Descriptor metadata and add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));
    
    // OUR_JOB: Step 2.B, Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));  
		attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    
    // OUR_JOB: Step 2.G, Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    
    // OUR_JOB: Step 2.C, Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));
		
		attr_char_value.p_uuid      = &char_uuid;
		attr_char_value.p_attr_md   = &attr_md;
    
    // OUR_JOB: Step 2.H, Set characteristic length in number of bytes
		attr_char_value.max_len     = sizeof(uint16_t);
		attr_char_value.init_len    = sizeof(uint16_t);
		
		attr_char_value.p_value     = (uint8_t *) &p_data_sync_init->revision;

    // OUR_JOB: Step 2.E, Add our new characteristic to the service
	  err_code = sd_ble_gatts_characteristic_add(p_data->service_handle,\
                                   &char_md,\
                                   &attr_char_value,\
                                   &p_data->data_sync_rev_handles);
		APP_ERROR_CHECK(err_code);
		
    return NRF_SUCCESS;
}

/**@brief       Function for adding data layout Revision characteristic to the BLE Stack.
 *
 * @param[in]   p_data data sync Service structure.
 *
 * @return      NRF_SUCCESS on success. Otherwise an error code.
 */
static uint32_t data_sync_ctrl_pt_add(ble_data_sync_t * const p_data)
{
    // OUR_JOB: Step 2.A, Add a custom characteristic UUID
		uint32_t            err_code;
		ble_uuid_t          char_uuid;
	
		char_uuid.type = p_data->uuid_type;
		char_uuid.uuid = BLE_DATA_SYNC_CTRL_PT_UUID;

    // OUR_JOB: Step 2.F Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));			;
    
	  char_md.char_props.write  = 1;
    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
	
    // OUR_JOB: Step 3.A, Configuring Client Characteristic Configuration Descriptor metadata and add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));
		
    
    // OUR_JOB: Step 2.B, Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));  
		attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    
    // OUR_JOB: Step 2.G, Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.read_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    // OUR_JOB: Step 2.C, Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));
		
		attr_char_value.p_uuid      = &char_uuid;
		attr_char_value.p_attr_md   = &attr_md;
    
    // OUR_JOB: Step 2.H, Set characteristic length in number of bytes
    attr_char_value.init_len  = 0;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = BLE_L2CAP_MTU_DEF;
    attr_char_value.p_value   = NULL;
		
		
    // OUR_JOB: Step 2.E, Add our new characteristic to the service
	  err_code = sd_ble_gatts_characteristic_add(p_data->service_handle,\
                                   &char_md,\
                                   &attr_char_value,\
                                   &p_data->data_sync_ctrl_pt_handles);
		APP_ERROR_CHECK(err_code);
		
    return NRF_SUCCESS;
}

uint32_t ble_data_sync_init(ble_data_sync_t * p_data, ble_data_sync_init_t * p_data_init)
{
    uint32_t   err_code; // Variable to hold return codes from library and softdevice functions

    // FROM_SERVICE_TUTORIAL: Declare 16-bit service and 128-bit base UUIDs and add them to the BLE stack

    ble_uuid_t        service_uuid;
    ble_uuid128_t     base_uuid = VSTEAM_BLE_BASE_UUID;
    service_uuid.uuid = BLE_DATA_SYNC_SERVICE_UUID;
    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    
    
    // OUR_JOB: Step 3.B, Set our service connection handle to default value. I.e. an invalid handle since we are not yet in a connection.
		p_data->conn_handle = BLE_CONN_HANDLE_INVALID;
	
    // FROM_SERVICE_TUTORIAL: Add our service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &service_uuid,
                                        &p_data->service_handle);
    
    p_data->uuid_type = service_uuid.type;

    // OUR_JOB: Call the function our_char_add() to add our new characteristic to the service. 
		//revsion characteristic
    err_code = data_sync_rev_char_add(p_data, p_data_init); 
		
		
		//data sync control point
		err_code = data_sync_ctrl_pt_add(p_data);
    
		
		m_is_data_sync_service_initialized = true;
		
		return NRF_SUCCESS; 
}

uint32_t ble_data_sync_response_send(ble_data_sync_t         * p_data,
                               ble_data_sync_procedure_t data_sync_proc,
                               ble_data_sync_resp_val_t  resp_val)
{
    if (p_data == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if ((p_data->conn_handle == BLE_CONN_HANDLE_INVALID) || !m_is_data_sync_service_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ble_gatts_hvx_params_t hvx_params;
    uint16_t               index = 0;

    m_notif_buffer[index++] = OP_CODE_RESPONSE;

    // Encode the Request Op code
    m_notif_buffer[index++] = (uint8_t)data_sync_proc;

    // Encode the Response Value.
    m_notif_buffer[index++] = (uint8_t)resp_val;

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_data->data_sync_ctrl_pt_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len  = &index;
    hvx_params.p_data = m_notif_buffer;

    return sd_ble_gatts_hvx(p_data->conn_handle, &hvx_params);
}
