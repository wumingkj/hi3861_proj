#ifndef NFC_H
#define NFC_H

#include "NT3H.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"

#define NDEF_HEADER_SIZE 0x2 // NDEF协议的头部大小

#define NDEF_PROTOCOL_HEADER_OFFSET 0           // NDEF协议头(固定)
#define NDEF_PROTOCOL_LENGTH_OFFSET 1           // NDEF协议数据的总长度位
#define NDEF_PROTOCOL_MEG_CONFIG_OFFSET 2       // 标签的控制字节位
#define NDEF_PROTOCOL_DATA_TYPE_LENGTH_OFFSET 3 // 标签数据类型的长度位
#define NDEF_PROTOCOL_DATA_LENGTH_OFFSET 4      // 标签的数据长度位
#define NDEF_PROTOCOL_DATA_TYPE_OFFSET 6        // 标签的数据类型位
#define NDEF_PROTOCOL_VALID_DATA_OFFSET 20      // 有效数据位

/*
 * The function write in the NT3H a new URI Rtd on the required position
 *
 * param:
 *      position: where add the record
 *      http:     the address to write
 *
 */
bool storeUrihttp(RecordPosEnu position, uint8_t *http);

/*
 * The function write in the NT3H a new Text Rtd on the required position
 *
 * param:
 *      position: where add the record
 *      text:     the text to write
 *
 */
bool storeText(RecordPosEnu position, uint8_t *text);

// BSP NFC功能合并
uint32_t get_NDEFDataPackage(uint8_t *dataBuff, const uint16_t dataBuff_MaxSize);
uint32_t nfc_init(void);

#endif /* NFC_H_ */