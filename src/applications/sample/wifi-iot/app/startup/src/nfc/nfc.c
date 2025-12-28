#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "rtdText.h"
#include "rtdUri.h"
#include "ndef.h"
#include "nfc.h"
#include "hi_errno.h"
#include "hi_i2c.h"

/**
 * @brief  写网址链接
 * @note
 * @param  position: 存储的位置
 * @param  *http: 存储的网址链接
 * @retval
 */
bool storeUrihttp(RecordPosEnu position, uint8_t *http)
{
    NDEFDataStr data;

    prepareUrihttp(&data, position, http);
    return NT3HwriteRecord(&data);
}

/**
 * @brief  写文本信息
 * @note
 * @param  position: 存储的位置
 * @param  *text: 存储的内容
 * @retval
 */
bool storeText(RecordPosEnu position, uint8_t *text)
{
    NDEFDataStr data;

    prepareText(&data, position, text);
    return NT3HwriteRecord(&data);
}

/**
 * @brief  从Page页中组成NDEF协议的包裹
 * @note
 * @param  *dataBuff: 最终的内容
 * @param  dataBuff_MaxSize: 存储缓冲区的长度
 * @retval
 */
uint32_t get_NDEFDataPackage(uint8_t *dataBuff, const uint16_t dataBuff_MaxSize)
{
    if (dataBuff == NULL || dataBuff_MaxSize <= 0) 
    {
        printf("dataBuff==NULL or dataBuff_MaxSize<=0\r\n");
        return HI_ERR_FAILURE;
    }

    uint8_t userMemoryPageNum = 0; // 用户的数据操作页数

    // 算出要取多少页
    if (dataBuff_MaxSize <= NFC_PAGE_SIZE) 
    {
        userMemoryPageNum = 1; // 1页
    } 
    else 
    {
        // 需要访问多少页
        userMemoryPageNum = (dataBuff_MaxSize / NFC_PAGE_SIZE) + \
                            ((dataBuff_MaxSize % NFC_PAGE_SIZE) >= 0 ? 1 : 0);
    }

    // 内存拷贝
    uint8_t *p_buff = (uint8_t *)malloc(userMemoryPageNum * NFC_PAGE_SIZE);
    if (p_buff == NULL) 
    {
        printf("p_buff == NULL.\r\n");
        return HI_ERR_FAILURE;
    }

    // 读取数据
    for (int i = 0; i < userMemoryPageNum; i++) 
    {
        if (NT3HReadUserData(i) == true) 
        {
            memcpy(p_buff + i * NFC_PAGE_SIZE, nfcPageBuffer, NFC_PAGE_SIZE);
        }
    }

    memcpy(dataBuff, p_buff, dataBuff_MaxSize);

    free(p_buff);
    p_buff = NULL;

    return HI_ERR_SUCCESS;
}

// NFC初始化
uint32_t nfc_init(void)
{
    uint32_t result;

    // gpio_9 复用为 I2C_SCL
    hi_io_set_pull(HI_IO_NAME_GPIO_9, HI_IO_PULL_UP);
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_I2C0_SCL);
    // gpio_10 复用为 I2C_SDA
    hi_io_set_pull(HI_IO_NAME_GPIO_10, HI_IO_PULL_UP);
    hi_io_set_func(HI_IO_NAME_GPIO_10, HI_IO_FUNC_GPIO_10_I2C0_SDA);

    result = hi_i2c_init(HI_I2C_IDX_1, I2C_RATE_DEFAULT);
    if (result != HI_ERR_SUCCESS) 
    {
        printf("I2C nfc Init status is 0x%x!!!\r\n", result);
        return result;
    }
    printf("I2C nfc Init is succeeded!!!\r\n");

    return HI_ERR_SUCCESS;
}

/**
 * @brief  清除NFC标签中的所有NDEF数据
 * @note   使用NT3H芯片的擦除功能清除所有用户数据
 * @retval 成功返回true，失败返回false
 */
bool nfc_clear_ndef_data(void)
{
    bool result = NT3HEraseAllTag();
    if (result) {
        printf("NFC NDEF data cleared successfully.\r\n");
    } else {
        printf("Failed to clear NFC NDEF data.\r\n");
    }
    return result;
}

/**
 * @brief  重置NFC用户数据区
 * @note   重置用户数据区到初始状态
 * @retval 成功返回true，失败返回false
 */
bool nfc_reset_user_data(void)
{
    bool result = NT3HResetUserData();
    if (result) {
        printf("NFC user data reset successfully.\r\n");
    } else {
        printf("Failed to reset NFC user data.\r\n");
    }
    return result;
}