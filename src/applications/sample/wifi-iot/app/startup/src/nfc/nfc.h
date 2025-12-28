#ifndef NFC_H
#define NFC_H

#include "NT3H.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"

// NDEF协议相关常量定义
#define NDEF_HEADER_SIZE 0x2 // NDEF协议的头部大小
#define NDEF_START_BYTE 0x03  // NDEF记录开始字节
#define NTAG_ERASED 0x00      // NTAG擦除状态

// NDEF协议偏移量定义
#define NDEF_PROTOCOL_HEADER_OFFSET 0           // NDEF协议头(固定)
#define NDEF_PROTOCOL_LENGTH_OFFSET 1           // NDEF协议数据的总长度位
#define NDEF_PROTOCOL_MEG_CONFIG_OFFSET 2       // 标签的控制字节位
#define NDEF_PROTOCOL_DATA_TYPE_LENGTH_OFFSET 3 // 标签数据类型的长度位
#define NDEF_PROTOCOL_DATA_LENGTH_OFFSET 4      // 标签的数据长度位
#define NDEF_PROTOCOL_DATA_TYPE_OFFSET 6        // 标签的数据类型位
#define NDEF_PROTOCOL_VALID_DATA_OFFSET 20      // 有效数据位

/*
 * 写入URI记录到NT3H指定位置
 * 
 * @param position: 存储的位置
 * @param http: 存储的网址链接
 * @return 成功返回true，失败返回false
 */
bool storeUrihttp(RecordPosEnu position, uint8_t *http);

/*
 * 写入文本记录到NT3H指定位置
 * 
 * @param position: 存储的位置
 * @param text: 存储的文本内容
 * @return 成功返回true，失败返回false
 */
bool storeText(RecordPosEnu position, uint8_t *text);

/*
 * 从Page页中组成NDEF协议的包裹
 * 
 * @param dataBuff: 最终的内容缓冲区
 * @param dataBuff_MaxSize: 存储缓冲区的最大长度
 * @return 成功返回HI_ERR_SUCCESS，失败返回错误码
 */
uint32_t get_NDEFDataPackage(uint8_t *dataBuff, const uint16_t dataBuff_MaxSize);

/*
 * NFC模块初始化
 * 
 * @return 成功返回HI_ERR_SUCCESS，失败返回错误码
 */
uint32_t nfc_init(void);

#endif /* NFC_H_ */