
#ifndef BLE_DATA_SYNC_H__
#define BLE_DATA_SYNC_H__

#include <stdint.h>
#include "ble_gatts.h"
#include "ble_gap.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "vsteam_type.h"


#define BLE_DATA_SYNC_SERVICE_UUID                 0x1570                       /**< The UUID of the data sync Service. */
#define BLE_DATA_SYNC_PKT_CHAR_UUID                0x1571                       /**< The UUID of the data sync Packet Characteristic. */
#define BLE_DATA_SYNC_CTRL_PT_UUID                 0x1572                       /**< The UUID of the data sync Control Point. */
#define BLE_DATA_SYNC_STATUS_REP_UUID              0x1573                       /**< The UUID of the data sync Status Report Characteristic. */
#define BLE_DATA_SYNC_REV_CHAR_UUID                0x1574                       /**< The UUID of the data sync Revision Characteristic. */


typedef enum
{
    BLE_DATA_SYNC_START,                                                      /**< The event indicating that the peer wants the application to prepare for a new firmware update. */
    BLE_DATA_SYNC_RECEIVE_INIT_DATA,                                          /**< The event indicating that the peer wants the application to prepare to receive init parameters. */
    BLE_DATA_SYNC_RECEIVE_APP_DATA,                                           /**< The event indicating that the peer wants the application to prepare to receive the new firmware image. */
    BLE_DATA_SYNC_VALIDATE,                                                   /**< The event indicating that the peer wants the application to validate the newly received firmware image. */
    BLE_DATA_SYNC_ACTIVATE_N_RESET,                                           /**< The event indicating that the peer wants the application to undergo activate new firmware and restart with new valid application */
    BLE_DATA_SYNC_SYS_RESET,                                                  /**< The event indicating that the peer wants the application to undergo a reset and start the currently valid application image.*/
    BLE_DATA_SYNC_PKT_RCPT_NOTIF_ENABLED,                                     /**< The event indicating that the peer has enabled packet receipt notifications. It is the responsibility of the application to call @ref ble_dfu_pkts_rcpt_notify each time the number of packets indicated by num_of_pkts field in @ref ble_data_sync_evt_t is received.*/
    BLE_DATA_SYNC_PKT_RCPT_NOTIF_DISABLED,                                    /**< The event indicating that the peer has disabled the packet receipt notifications.*/
    BLE_DATA_SYNC_PACKET_WRITE,                                               /**< The event indicating that the peer has written a value to the 'DFU Packet' characteristic. The data received from the peer will be present in the @ref BLE_DATA_SYNC_PACKET_WRITE element contained within @ref ble_data_sync_evt_t.*/
    BLE_DATA_SYNC_BYTES_RECEIVED_SEND                                         /**< The event indicating that the peer is requesting for the number of bytes of firmware data last received by the application. It is the responsibility of the application to call @ref ble_dfu_pkts_rcpt_notify in response to this event. */
} ble_data_sync_evt_type_t;

typedef enum
{
    BLE_DATA_SYNC_START_PROCEDURE        = 1,                                 /**< DFU Start procedure.*/
    BLE_DATA_SYNC_INIT_PROCEDURE         = 2,                                 /**< DFU Initialization procedure.*/
    BLE_DATA_SYNC_RECEIVE_APP_PROCEDURE  = 3,                                 /**< Firmware receiving procedure.*/
    BLE_DATA_SYNC_VALIDATE_PROCEDURE     = 4,                                 /**< Firmware image validation procedure .*/
    BLE_DATA_SYNC_PKT_RCPT_REQ_PROCEDURE = 8                                  /**< Packet receipt notification request procedure. */
} ble_data_sync_procedure_t;


typedef enum
{
    BLE_DATA_SYNC_RESP_VAL_SUCCESS = 1,                                       /**< Success.*/
    BLE_DATA_SYNC_RESP_VAL_INVALID_STATE,                                     /**< Invalid state.*/
    BLE_DATA_SYNC_RESP_VAL_NOT_SUPPORTED,                                     /**< Operation not supported.*/
    BLE_DATA_SYNC_RESP_VAL_DATA_SIZE,                                         /**< Data size exceeds limit.*/
    BLE_DATA_SYNC_RESP_VAL_CRC_ERROR,                                         /**< CRC Error.*/
    BLE_DATA_SYNC_RESP_VAL_OPER_FAILED                                        /**< Operation failed.*/
} ble_data_sync_resp_val_t;


typedef struct
{
    uint8_t *                    p_data;                                /**< Pointer to the received packet. This will point to a word aligned memory location.*/
    uint8_t                      len;                                   /**< Length of the packet received. */
} ble_data_sync_pkt_write_t;


typedef struct
{
    uint16_t                     num_of_pkts;                           /**< The number of packets of firmware data to be received by application before sending the next Packet Receipt Notification to the peer. */
} ble_data_sync_rcpt_notif_req_t;


typedef struct
{
    ble_data_sync_evt_type_t           ble_data_sync_evt_type;                      /**< Type of the event.*/
    union
    {
        ble_data_sync_pkt_write_t      ble_data_sync_pkt_write;                     /**< The DFU packet received. This field is when the @ref ble_data_sync_evt_type field is set to @ref BLE_DATA_SYNC_PACKET_WRITE.*/
        ble_data_sync_rcpt_notif_req_t pkt_rcpt_notif_req;                    /**< Packet receipt notification request. This field is when the @ref ble_data_sync_evt_type field is set to @ref BLE_DATA_SYNC_PKT_RCPT_NOTIF_ENABLED.*/
    } evt;
} ble_data_sync_evt_t;

// Forward declaration of the ble_data_sync_t type.
typedef struct ble_data_sync_s ble_data_sync_t;

/**@brief DFU Service event handler type. */
typedef void (*ble_data_sync_evt_handler_t) (ble_data_sync_t * p_data, ble_data_sync_evt_t * p_evt);


struct ble_data_sync_s
{
    uint16_t                     conn_handle;                           /**< Handle of the current connection (as provided by the SoftDevice). This will be BLE_CONN_HANDLE_INVALID when not in a connection. */
    uint16_t                     revision;                              /**< Handle of DFU Service (as provided by the SoftDevice). */
    uint16_t                     service_handle;                        /**< Handle of DFU Service (as provided by the SoftDevice). */
    uint8_t                      uuid_type;                             /**< UUID type assigned for DFU Service by the SoftDevice. */
    ble_gatts_char_handles_t     data_sync_pkt_handles;                 /**< Handles related to the DFU Packet characteristic. */
    ble_gatts_char_handles_t     data_sync_ctrl_pt_handles;             /**< Handles related to the DFU Control Point characteristic. */
    ble_gatts_char_handles_t     data_sync_status_rep_handles;          /**< Handles related to the DFU Status Report characteristic. */
    ble_gatts_char_handles_t     data_sync_rev_handles;                 /**< Handles related to the DFU Revision characteristic. */
    ble_data_sync_evt_handler_t  evt_handler;                           /**< The event handler to be called when an event is to be sent to the application.*/
    ble_srv_error_handler_t      error_handler;                         /**< Function to be called in case of an error. */
};


typedef struct
{
    uint16_t                     revision;                              /**< Revision number to be exposed by the DFU service. */
    ble_data_sync_evt_handler_t  evt_handler;                           /**< Event handler to be called for handling events in the Device Firmware Update Service. */
    ble_srv_error_handler_t      error_handler;                         /**< Function to be called in case of an error. */
} ble_data_sync_init_t;

 
void ble_data_sync_on_ble_evt(ble_data_sync_t * p_data, ble_evt_t * p_ble_evt);

uint32_t ble_data_sync_init(ble_data_sync_t * p_data, ble_data_sync_init_t * p_data_init);


uint32_t ble_data_sync_response_send(ble_data_sync_t *          p_data,
                               ble_data_sync_procedure_t  data_sync_proc,
                               ble_data_sync_resp_val_t   resp_val);




#endif // BLE_DATA_SYNC_H__

