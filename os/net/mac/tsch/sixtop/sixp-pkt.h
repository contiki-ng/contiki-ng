/*
 * Copyright (c) 2016, Yasuyuki Tanaka
 * Copyright (c) 2016, Centre for Development of Advanced Computing (C-DAC).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup sixtop
 * @{
 */
/**
 * \file
 *         6top Protocol (6P) Packet Manipulation APIs
 * \author
 *         Shalu R <shalur@cdac.in>
 *         Lijo Thomas <lijo@cdac.in>
 *         Yasuyuki Tanaka <yasuyuki.tanaka@inf.ethz.ch>
 */
#ifndef _SIXTOP_6P_PACKET_H_
#define _SIXTOP_6P_PACKET_H_

#define SIXP_PKT_VERSION  0x00

/* typedefs for code readability */
typedef uint8_t sixp_pkt_version_t;
typedef uint8_t sixp_pkt_cell_options_t;
typedef uint8_t sixp_pkt_num_cells_t;
typedef uint8_t sixp_pkt_reserved_t;
typedef uint16_t sixp_pkt_metadata_t;
typedef uint16_t sixp_pkt_max_num_cells_t;
typedef uint16_t sixp_pkt_offset_t;
typedef uint32_t sixp_pkt_cell_t;
typedef uint16_t sixp_pkt_total_num_cells_t;

/**
 * \brief 6P Message Types
 */
typedef enum  {
  SIXP_PKT_TYPE_REQUEST      = 0x00, /**< 6P Request */
  SIXP_PKT_TYPE_RESPONSE     = 0x01, /**< 6P Response */
  SIXP_PKT_TYPE_CONFIRMATION = 0x02, /**< 6P Confirmation */
  SIXP_PKT_TYPE_RESERVED     = 0x03, /**< Reserved */
} sixp_pkt_type_t;

/**
 * \brief 6P Command Identifiers
 */
typedef enum {
  SIXP_PKT_CMD_ADD          = 0x01, /**< CMD_ADD */
  SIXP_PKT_CMD_DELETE       = 0x02, /**< CMD_DELETE */
  SIXP_PKT_CMD_RELOCATE     = 0x03, /**< CMD_STATUS */
  SIXP_PKT_CMD_COUNT        = 0x04, /**< CMD_STATUS */
  SIXP_PKT_CMD_LIST         = 0x05, /**< CMD_LIST */
  SIXP_PKT_CMD_SIGNAL       = 0x06, /**< CMD_SIGNAL */
  SIXP_PKT_CMD_CLEAR        = 0x07, /**< CMD_CLEAR */
  SIXP_PKT_CMD_UNAVAILABLE  = 0xff, /**< for internal use */
} sixp_pkt_cmd_t;

/**
 * \brief 6P Return Codes
 */
typedef enum {
  SIXP_PKT_RC_SUCCESS      = 0x00, /**< RC_SUCCESS */
  SIXP_PKT_RC_EOL          = 0x01, /**< RC_EOL */
  SIXP_PKT_RC_ERR          = 0x02, /**< RC_ERR */
  SIXP_PKT_RC_RESET        = 0x03, /**< RC_RESET */
  SIXP_PKT_RC_ERR_VERSION  = 0x04, /**< RC_ERR_VERSION */
  SIXP_PKT_RC_ERR_SFID     = 0x05, /**< RC_ERR_SFID */
  SIXP_PKT_RC_ERR_SEQNUM   = 0x06, /**< RC_ERR_SEQNUM */
  SIXP_PKT_RC_ERR_CELLLIST = 0x07, /**< RC_ERR_CELLLIST */
  SIXP_PKT_RC_ERR_BUSY     = 0x08, /**< RC_ERR_BUSY */
  SIXP_PKT_RC_ERR_LOCKED   = 0x09, /**< RC_ERR_LOCKED */

} sixp_pkt_rc_t;

/**
 * \brief 6P Codes integrating Command IDs and Return Codes
 */
typedef union {
  sixp_pkt_cmd_t cmd; /**< 6P Command Identifier */
  sixp_pkt_rc_t rc;   /**< 6P Return Code */
  uint8_t value;      /**< 8-bit unsigned integer value */
} sixp_pkt_code_t;

/**
 * \brief 6P Cell Options
 */
typedef enum {
  SIXP_PKT_CELL_OPTION_TX     = 0x01, /**< TX Cell */
  SIXP_PKT_CELL_OPTION_RX     = 0x02, /**< RX Cell */
  SIXP_PKT_CELL_OPTION_SHARED = 0x04  /**< SHARED Cell */
} sixp_pkt_cell_option_t;

/**
 * \brief 6top IE Structure
 */
typedef struct {
  sixp_pkt_version_t version; /**< Version */
  sixp_pkt_type_t type;       /**< Type */
  sixp_pkt_code_t code;       /**< Code */
  uint8_t sfid;               /**< SFID */
  uint8_t seqno;              /**< SeqNum */
  const uint8_t *body;        /**< Other Fields... */
  uint16_t body_len;          /**< The length of Other Fields */
} sixp_pkt_t;

/**
 * \brief Write Metadata into "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param metadata Metadata to write
 * \param body The pointer to "Other Fields" in a buffer
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_metadata(sixp_pkt_type_t type, sixp_pkt_code_t code,
                          sixp_pkt_metadata_t metadata,
                          uint8_t *body, uint16_t body_len);

/**
 * \brief Read Metadata stored in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param metadata The pointer to a buffer to store Metadata in
 * \param body The pointer to the buffer having "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_metadata(sixp_pkt_type_t type, sixp_pkt_code_t code,
                          sixp_pkt_metadata_t *metadata,
                          const uint8_t *body, uint16_t body_len);

/**
 * \brief Write CellOptions in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param cell_options "CellOptions" to write
 * \param body The pointer to buffer having "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_cell_options(sixp_pkt_type_t type, sixp_pkt_code_t code,
                              sixp_pkt_cell_options_t cell_options,
                              uint8_t *body, uint16_t body_len);

/**
 * \brief Read CellOptions in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param cell_options The pointer to buffer to store CellOptions in
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_cell_options(sixp_pkt_type_t type, sixp_pkt_code_t code,
                              sixp_pkt_cell_options_t *cell_options,
                              const uint8_t *body, uint16_t body_len);

/**
 * \brief Write NumCells in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param num_cells "NumCells" to write
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_num_cells(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           sixp_pkt_num_cells_t num_cells,
                           uint8_t *body, uint16_t body_len);

/**
 * \brief Read NumCells in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param num_cells The pointer to buffer to store NumCells in
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_num_cells(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           sixp_pkt_num_cells_t *num_cells,
                           const uint8_t *body, uint16_t body_len);
/**
 * \brief Write Reserved in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param reserved "Reserved" to write
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_reserved(sixp_pkt_type_t type, sixp_pkt_code_t code,
                          sixp_pkt_reserved_t reserved,
                          uint8_t *body, uint16_t body_len);

/**
 * \brief Read Reserved in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param reserved The pointer to buffer to store Reserved in
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_reserved(sixp_pkt_type_t type, sixp_pkt_code_t code,
                          sixp_pkt_reserved_t *reserved,
                          const uint8_t *body, uint16_t body_len);

/**
 * \brief Write Offset in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param offset "Offset" to write
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_offset(sixp_pkt_type_t type, sixp_pkt_code_t code,
                        sixp_pkt_offset_t offset,
                        uint8_t *body, uint16_t body_len);
/**
 * \brief Read Offset in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param offset The pointer to buffer to store Offset in
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_offset(sixp_pkt_type_t type, sixp_pkt_code_t code,
                        sixp_pkt_offset_t *offset,
                        const uint8_t *body, uint16_t body_len);

/**
 * \brief Write MaxNumCells in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param max_num_cells "MaxNumCells" to write
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_max_num_cells(sixp_pkt_type_t type,
                               sixp_pkt_code_t code,
                               sixp_pkt_max_num_cells_t max_num_cells,
                               uint8_t *body, uint16_t body_len);

/**
 * \brief Read MaxNumCells in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param max_num_cells The pointer to buffer to store MaxNumCells in
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_max_num_cells(sixp_pkt_type_t type,
                               sixp_pkt_code_t code,
                               sixp_pkt_max_num_cells_t *max_num_cells,
                               const uint8_t *body, uint16_t body_len);

/**
 * \brief Write CellList in "Other Fields" of 6P packet
 * \note "offset" is specified by index in CellList
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param cell_list The pointer to "CellList" to write
 * \param cell_list_len Length to write
 * \param offset Offset in the "CellList" field to start writing
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_cell_list(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           const uint8_t *cell_list,
                           uint16_t cell_list_len,
                           uint16_t offset,
                           uint8_t *body, uint16_t body_len);
/**
 * \brief Read CellList in "Other Fields" of 6P packet
 * \note If you want only the length of CellList, you can set null to cell_list.
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param cell_list The double pointer to store the starting address of CellList
 * \param cell_list_len Pointer to store the length of CellList
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_cell_list(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           const uint8_t **cell_list,
                           sixp_pkt_offset_t *cell_list_len,
                           const uint8_t *body, uint16_t body_len);

/**
 * \brief Write RelCellList in "Other Fields" of 6P packet
 * \note "offset" is specified by index in RelCellList
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param rel_cell_list The pointer to "RelCellList" to write
 * \param rel_cell_list_len Length to write
 * \param offset Offset in the "RelCellList" field to start writing
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_rel_cell_list(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           const uint8_t *rel_cell_list,
                           uint16_t rel_cell_list_len,
                           uint16_t offset,
                           uint8_t *body, uint16_t body_len);

/**
 * \brief Read RelCellList in "Other Fields" of 6P packet
 * \note If you want only the length of RelCellList, you can set null to
 * rel_cell_list.
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param rel_cell_list The double pointer to store the starting address of
 * RelCellList
 * \param rel_cell_list_len Pointer to store the length of a returned
 * RelCellList
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_rel_cell_list(sixp_pkt_type_t type, sixp_pkt_code_t code,
                               const uint8_t **rel_cell_list,
                               sixp_pkt_offset_t *rel_cell_list_len,
                               const uint8_t *body, uint16_t body_len);

/**
 * \brief Write CandCellList in "Other Fields" of 6P packet
 * \note "offset" is specified by index in CandCellList
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param cand_cell_list The pointer to "CandCellList" to write
 * \param cand_cell_list_len Length to write
 * \param offset Offset in the "CandCellList" field to start writing
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_cand_cell_list(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           const uint8_t *cand_cell_list,
                           uint16_t cand_cell_list_len,
                           uint16_t offset,
                           uint8_t *body, uint16_t body_len);

/**
 * \brief Read CandCellList in "Other Fields" of 6P packet
 * \note If you want only the length of CandCellList, you can set null to
 * cell_list.
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param cand_cell_list The double pointer to store the starting address of
 * CandCellList
 * \param cand_cell_list_len Pointer to store the length of CandCellList
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_cand_cell_list(sixp_pkt_type_t type, sixp_pkt_code_t code,
                           const uint8_t **cand_cell_list,
                           sixp_pkt_offset_t *cand_cell_list_len,
                           const uint8_t *body, uint16_t body_len);

/**
 * \brief Write TotalNumCells in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param total_num_cells "TotalNumCells" to write
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_total_num_cells(sixp_pkt_type_t type, sixp_pkt_code_t code,
                                 sixp_pkt_total_num_cells_t total_num_cells,
                                 uint8_t *body, uint16_t body_len);

/**
 * \brief Read TotalNumCells in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param total_num_cells The pointer to buffer to store TotalNumCells in
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_total_num_cells(sixp_pkt_type_t type, sixp_pkt_code_t code,
                                 sixp_pkt_total_num_cells_t *total_num_cells,
                                 const uint8_t *body, uint16_t body_len);

/**
 * \brief Write Payload in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param payload "Payload" to write
 * \param payload_len The length of "Payload" to write
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_set_payload(sixp_pkt_type_t type, sixp_pkt_code_t code,
                         const uint8_t *payload, uint16_t payload_len,
                         uint8_t *body, uint16_t body_len);

/**
 * \brief Read Payload in "Other Fields" of 6P packet
 * \param type 6P Message Type
 * \param code 6P Command Identifier or Return Code
 * \param buf The pointer to buffer to store "Payload" in
 * \param buf_len The length of buf
 * \param body The pointer to buffer pointing to "Other Fields"
 * \param body_len The length of body, typically "Other Fields" length
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_get_payload(sixp_pkt_type_t type, sixp_pkt_code_t code,
                         uint8_t *buf, uint16_t buf_len,
                         const uint8_t *body, uint16_t body_len);

/**
 * \brief Parse a 6P packet
 * \param buf The pointer to a buffer pointing 6top IE Content
 * \param len The length of the buffer
 * \param pkt The pointer to a sixp_pkt_t structure to store packet info
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_parse(const uint8_t *buf, uint16_t len,
                   sixp_pkt_t *pkt);

/**
 * \brief Create a 6P packet
 * \param type 6P Message Type
 * \param code 6P Message Code, Command Identifier or Return Code
 * \param sfid Scheduling Function Identifier
 * \param seqno Sequence Number
 * \param body The pointer to "Other Fields" in a buffer
 * \param body_len The length of body, typically "Other Fields" length
 * \param pkt The pointer to a sixp_pkt_t structure to store packet info
 * (option)
 * \return 0 on success, -1 on failure
 */
int sixp_pkt_create(sixp_pkt_type_t type, sixp_pkt_code_t code,
                    uint8_t sfid, uint8_t seqno,
                    const uint8_t *body, uint16_t body_len,
                    sixp_pkt_t *pkt);

#endif /* !_SIXP_PKT_H_ */
/** @} */
