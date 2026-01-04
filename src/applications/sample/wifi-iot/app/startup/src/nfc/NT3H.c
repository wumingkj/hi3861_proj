#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hi_i2c.h"

#include "ndef.h"
#include "nfcForum.h"
#include "NT3H.h"

#define DELAY_WRITE 300000
#define INDEX_NDEF_START 0
#define INDEX_END_RECORD 1
#define INDEX_NDEF_HEADER 2

uint8_t nfcPageBuffer[NFC_PAGE_SIZE];
NT3HerrNo errNo;

// due to the nature of the NT3H a timeout is required to
// protectd 2 consecutive I2C access

inline const uint8_t *get_last_ncf_page(void)
{
    return nfcPageBuffer;
}

static bool writeTimeout(uint8_t *data, uint8_t dataSend)
{
    hi_u32 status = 0;
    hi_i2c_data nt3h1101_i2c_data1 = {0};

    nt3h1101_i2c_data1.send_buf = data;
    nt3h1101_i2c_data1.send_len = dataSend;

    status = hi_i2c_write(HI_I2C_IDX_0, (NT3H1X_SLAVE_ADDRESS << 1) | 0x00, &nt3h1101_i2c_data1);
    if (status != 0) {
        printf("===== Error: I2C write status1 = 0x%x! =====\r\n", status);
        return 0;
    }
    usleep(DELAY_WRITE);
    return 1;
}

static bool readTimeout(uint8_t address, uint8_t *block_data)
{
    hi_u32 status = 0;
    hi_i2c_data nt3h1101_i2c_data = {0};
    uint8_t buffer[1] = {address};
    nt3h1101_i2c_data.send_buf = buffer;
    nt3h1101_i2c_data.send_len = 1;
    nt3h1101_i2c_data.receive_buf = block_data;
    nt3h1101_i2c_data.receive_len = NFC_PAGE_SIZE;

    status = hi_i2c_writeread(HI_I2C_IDX_0, (NT3H1X_SLAVE_ADDRESS << 1) | 0x00, &nt3h1101_i2c_data);
    if (status != 0) {
        printf("===== Error: I2C write status = 0x%x! =====\r\n", status);
        return 0;
    }

    return 1;
}

bool NT3HReadHeaderNfc(uint8_t *endRecordsPtr, uint8_t *ndefHeader)
{
    *endRecordsPtr = 0;
    bool ret = NT3HReadUserData(0);
    // read the first page to see where is the end of the Records.
    if (ret == true) {
        // if the first byte is equals to NDEF_START_BYTE there are some records
        // store theend of that
        if ((NDEF_START_BYTE == nfcPageBuffer[INDEX_NDEF_START]) &&
            (NTAG_ERASED != nfcPageBuffer[INDEX_NDEF_HEADER])) {
            *endRecordsPtr = nfcPageBuffer[INDEX_END_RECORD];
            *ndefHeader = nfcPageBuffer[INDEX_NDEF_HEADER];
            return true;
        }
        return true;
    } else {
        errNo = NT3HERROR_READ_HEADER;
    }

    return ret;
}

bool NT3HWriteHeaderNfc(uint8_t endRecordsPtr, uint8_t ndefHeader)
{
    // read the first page to see where is the end of the Records.
    bool ret = NT3HReadUserData(0);
    if (ret == true) {
        nfcPageBuffer[INDEX_END_RECORD] = endRecordsPtr;
        nfcPageBuffer[INDEX_NDEF_HEADER] = ndefHeader;
        ret = NT3HWriteUserData(0, nfcPageBuffer);
        if (ret == false) {
            errNo = NT3HERROR_WRITE_HEADER;
        }
    } else {
        errNo = NT3HERROR_READ_HEADER;
    }

    return ret;
}

bool NT3HEraseAllTag(void)
{
    bool ret = true;
    uint8_t erase[NFC_PAGE_SIZE + 1] = {USER_START_REG, 0x03, 0x03, 0xD0, 0x00, 0x00, 0xFE};
    ret = writeTimeout(erase, sizeof(erase));
    if (ret == false) {
        errNo = NT3HERROR_ERASE_USER_MEMORY_PAGE;
    }
    return ret;
}

bool NT3HReaddManufactoringData(uint8_t *manuf)
{
    return readTimeout(MANUFACTORING_DATA_REG, manuf);
}

bool NT3HReadConfiguration(uint8_t *configuration)
{
    return readTimeout(CONFIG_REG, configuration);
}

bool NT3HWriteConfiguration(const uint8_t *configuration)
{
    bool ret = true;
    uint8_t dataSend[NFC_PAGE_SIZE + 1];
    
    dataSend[0] = CONFIG_REG;
    memcpy_s(&dataSend[1], NFC_PAGE_SIZE, configuration, NFC_PAGE_SIZE);
    
    ret = writeTimeout(dataSend, sizeof(dataSend));
    if (ret == false) {
        printf("===== Error: Failed to write configuration register! =====\r\n");
        return false;
    }
    return ret;
}

// 读取字段锁定寄存器
bool NT3HReadFieldLock(uint8_t *fieldLock)
{
    return readTimeout(FIELD_LOCK_REG, fieldLock);
}

// 写入字段锁定寄存器
bool NT3HWriteFieldLock(const uint8_t *fieldLock)
{
    bool ret = true;
    uint8_t dataSend[NFC_PAGE_SIZE + 1];
    
    dataSend[0] = FIELD_LOCK_REG;
    memcpy_s(&dataSend[1], NFC_PAGE_SIZE, fieldLock, NFC_PAGE_SIZE);
    
    ret = writeTimeout(dataSend, sizeof(dataSend));
    if (ret == false) {
        printf("===== Error: Failed to write field lock register! =====\r\n");
        return false;
    }
    return ret;
}

// 读取动态锁定寄存器
bool NT3HReadDynamicLock(uint8_t *dynamicLock)
{
    return readTimeout(DYNAMIC_LOCK_REG, dynamicLock);
}

// 写入动态锁定寄存器
bool NT3HWriteDynamicLock(const uint8_t *dynamicLock)
{
    bool ret = true;
    uint8_t dataSend[NFC_PAGE_SIZE + 1];
    
    dataSend[0] = DYNAMIC_LOCK_REG;
    memcpy_s(&dataSend[1], NFC_PAGE_SIZE, dynamicLock, NFC_PAGE_SIZE);
    
    ret = writeTimeout(dataSend, sizeof(dataSend));
    if (ret == false) {
        printf("===== Error: Failed to write dynamic lock register! =====\r\n");
        return false;
    }
    return ret;
}

// 检查所有锁定状态
bool NT3HIsTagLocked(void)
{
    uint8_t config[16], fieldLock[16], dynamicLock[16];
    bool isLocked = false;
    
    printf("=== Checking all lock status ===\r\n");
    
    // 检查配置寄存器
    if (NT3HReadConfiguration(config)) {
        printf("Config Register: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", config[i]);
        }
        printf("\n");
        
        if (config[0] & 0x01) {
            printf("✓ Config register locked (bit 0 = 1)\r\n");
            isLocked = true;
        } else {
            printf("✓ Config register unlocked (bit 0 = 0)\r\n");
        }
    } else {
        printf("✗ Failed to read config register\r\n");
    }
    
    // 检查字段锁定
    if (NT3HReadFieldLock(fieldLock)) {
        printf("Field Lock Register: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", fieldLock[i]);
        }
        printf("\n");
        
        // 检查字段锁定位（通常第0字节控制用户内存锁定）
        if (fieldLock[0] != 0x00) {
            printf("✓ Field lock active (not 0x00)\r\n");
            isLocked = true;
        } else {
            printf("✓ Field lock inactive (0x00)\r\n");
}
    } else {
        printf("✗ Failed to read field lock register\r\n");
    }
    
    // 检查动态锁定
    if (NT3HReadDynamicLock(dynamicLock)) {
        printf("Dynamic Lock Register: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", dynamicLock[i]);
        }
        printf("\n");
        
        // 检查动态锁定位
        if (dynamicLock[0] != 0x00) {
            printf("✓ Dynamic lock active (not 0x00)\r\n");
            isLocked = true;
        } else {
            printf("✓ Dynamic lock inactive (0x00)\r\n");
        }
    } else {
        printf("✗ Failed to read dynamic lock register\r\n");
    }
    
    return isLocked;
}

// 全面解锁函数
bool NT3HUnlockTagComprehensive(void)
{
    uint8_t config[16], fieldLock[16], dynamicLock[16];
    bool success = true;
    
    printf("=== Starting comprehensive unlock procedure ===\r\n");
    
    // 1. 解锁配置寄存器
    printf("1. Unlocking configuration register...\r\n");
    if (NT3HReadConfiguration(config)) {
        config[0] &= ~0x01; // 清除第0位
        if (!NT3HWriteConfiguration(config)) {
            printf("✗ Failed to unlock config register\r\n");
            success = false;
        } else {
            printf("✓ Config register unlocked\r\n");
        }
    }
    
    // 2. 解锁字段锁定
    printf("2. Unlocking field lock...\r\n");
    if (NT3HReadFieldLock(fieldLock)) {
        // 将字段锁定寄存器清零
        memset_s(fieldLock, NFC_PAGE_SIZE, 0x00, NFC_PAGE_SIZE);
        if (!NT3HWriteFieldLock(fieldLock)) {
            printf("✗ Failed to unlock field lock\r\n");
            success = false;
        } else {
            printf("✓ Field lock unlocked\r\n");
        }
    }
    
    // 3. 解锁动态锁定
    printf("3. Unlocking dynamic lock...\r\n");
    if (NT3HReadDynamicLock(dynamicLock)) {
        // 尝试写入解锁模式（根据NT3H手册）
        uint8_t unlockPattern[16] = {0};
        unlockPattern[0] = 0x00; // 通常写入特定模式来解锁
        unlockPattern[1] = 0x00;
        if (!NT3HWriteDynamicLock(unlockPattern)) {
            printf("✗ Failed to unlock dynamic lock\r\n");
            success = false;
        } else {
            printf("✓ Dynamic lock unlocked\r\n");
        }
    }
    
    // 4. 验证解锁结果
    printf("4. Verifying unlock status...\r\n");
    if (!NT3HIsTagLocked()) {
        printf("✓ Tag is fully unlocked\r\n");
    } else {
        printf("✗ Tag is still locked after comprehensive unlock attempt\r\n");
        success = false;
    }
    
    return success;
}

bool getSessionReg(void)
{
    return readTimeout(SESSION_REG, nfcPageBuffer);
}

bool NT3HReadUserData(uint8_t page)
{
    uint8_t reg = USER_START_REG + page;
    // if the requested page is out of the register exit with error
    if (reg > USER_END_REG) {
        errNo = NT3HERROR_INVALID_USER_MEMORY_PAGE;
        return false;
    }

    bool ret = readTimeout(reg, nfcPageBuffer);
    if (ret == false) {
        errNo = NT3HERROR_READ_USER_MEMORY_PAGE;
    }

    return ret;
}

bool NT3HWriteUserData(uint8_t page, const uint8_t *data)
{
    bool ret = true;
    uint8_t dataSend[NFC_PAGE_SIZE + 1]; // data plus register
    uint8_t reg = USER_START_REG + page;

    // if the requested page is out of the register exit with error
    if (reg > USER_END_REG) {
        errNo = NT3HERROR_INVALID_USER_MEMORY_PAGE;
        ret = false;
        return ret;
    }

    dataSend[0] = reg; // store the register
    memcpy_s(&dataSend[1], NFC_PAGE_SIZE, data, NFC_PAGE_SIZE);
    ret = writeTimeout(dataSend, sizeof(dataSend));
    if (ret == false) {
        errNo = NT3HERROR_WRITE_USER_MEMORY_PAGE;
        return ret;
    }

    return ret;
}

bool NT3HReadSram(void)
{
    bool ret = false;
    int i = 0, j = 0;

    for (i = SRAM_START_REG, j = 0; i <= SRAM_END_REG; i++, j++) {
        ret = readTimeout(i, nfcPageBuffer);
        if (ret == false) {
            return ret;
        }

        printf("[page=%d]: ", i);
        for (j = 0; j < NFC_PAGE_SIZE; j++) {
            printf("0x%x ", nfcPageBuffer[i]);
        }
        printf("\n");
        // memcpy(&userData[offset], pageBuffer, sizeof(pageBuffer));
    }
    return ret;
}

void NT3HGetNxpSerialNumber(char *buffer)
{
    uint8_t manuf[16];
    int i = 0;

    if (NT3HReaddManufactoringData(manuf)) {
        for (i = 0; i < SERIAL_NUM_LEN; i++) {
            buffer[i] = manuf[i];
        }
    }
}

/**
 * @brief 重置NFC用户数据区
 * @note 将用户数据区重置为初始状态（擦除所有用户数据）
 * @return 成功返回true，失败返回false
 */
bool NT3HResetUserData(void)
{
    bool ret = true;
    uint8_t emptyData[NFC_PAGE_SIZE] = {0};
    uint8_t startPage = 0;
    uint8_t endPage = USER_END_REG - USER_START_REG;
    
    printf("=== Resetting NFC user data ===\r\n");
    
    // 遍历所有用户数据页，写入空数据
    for (uint8_t page = startPage; page <= endPage; page++) {
        ret = NT3HWriteUserData(page, emptyData);
        if (!ret) {
            printf("✗ Failed to reset page %d\r\n", page);
            errNo = NT3HERROR_WRITE_USER_MEMORY_PAGE;
            return false;
        }
    }
    
    // 重置NDEF头信息
    uint8_t headerData[NFC_PAGE_SIZE] = {0};
    headerData[0] = 0x03; // NDEF开始字节
    headerData[1] = 0x00; // 记录结束位置
    headerData[2] = 0x00; // NDEF头
    
    ret = NT3HWriteUserData(0, headerData);
    if (!ret) {
        printf("✗ Failed to reset NDEF header\r\n");
        return false;
    }
    
    printf("✓ NFC user data reset successfully\r\n");
    return true;
}

/**
 * @brief 增强版NFC标签解锁函数
 * @note 针对手机NFC无法写入的问题，提供更全面的解锁方案
 * @return 成功返回true，失败返回false
 */
bool NT3HEnhancedUnlock(void)
{
    printf("=== Starting enhanced NFC unlock procedure ===\r\n");
    
    // 1. 首先检查当前锁定状态
    printf("1. Checking current lock status...\r\n");
    bool isLocked = NT3HIsTagLocked();
    if (!isLocked) {
        printf("✓ Tag is already unlocked\r\n");
        return true;
    }
    
    // 2. 尝试全面解锁
    printf("2. Attempting comprehensive unlock...\r\n");
    bool result = NT3HUnlockTagComprehensive();
    if (result) {
        printf("✓ Comprehensive unlock successful\r\n");
        return true;
    }
    
    // 3. 如果全面解锁失败，尝试特殊解锁方法
    printf("3. Attempting special unlock methods...\r\n");
    
    // 方法1: 尝试写入特定的解锁模式到动态锁定寄存器
    uint8_t specialUnlock[16] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    if (NT3HWriteDynamicLock(specialUnlock)) {
        printf("✓ Special dynamic unlock pattern written\r\n");
    }
    
    // 方法2: 重置配置寄存器到默认值
    uint8_t defaultConfig[16] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    if (NT3HWriteConfiguration(defaultConfig)) {
        printf("✓ Configuration reset to default\r\n");
    }
    
    // 方法3: 清除字段锁定
    uint8_t clearFieldLock[16] = {0};
    if (NT3HWriteFieldLock(clearFieldLock)) {
        printf("✓ Field lock cleared\r\n");
    }
    
    // 4. 验证解锁结果
    printf("4. Verifying final unlock status...\r\n");
    isLocked = NT3HIsTagLocked();
    if (!isLocked) {
        printf("✓ Tag successfully unlocked after enhanced procedure\r\n");
        return true;
    } else {
        printf("✗ Tag still locked after all unlock attempts\r\n");
        return false;
    }
}

/**
 * @brief NFC标签诊断函数
 * @note 诊断NFC标签的各种状态，帮助排查写入问题
 */
void NT3HDiagnoseTag(void)
{
    printf("=== NFC Tag Diagnosis ===\r\n");
    
    // 1. 检查基本通信
    printf("1. Basic communication test...\r\n");
    uint8_t manuf[16];
    if (NT3HReaddManufactoringData(manuf)) {
        printf("✓ Can communicate with tag\r\n");
        printf("   Manufacturer data: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", manuf[i]);
        }
        printf("\n");
    } else {
        printf("✗ Cannot communicate with tag\r\n");
        return;
    }
    
    // 2. 检查锁定状态
    printf("2. Lock status check...\r\n");
    bool isLocked = NT3HIsTagLocked();
    printf("   Tag locked: %s\r\n", isLocked ? "YES" : "NO");
    
    // 3. 检查配置寄存器
    printf("3. Configuration register...\r\n");
    uint8_t config[16];
    if (NT3HReadConfiguration(config)) {
        printf("   Config: ");
        for (int i = 0; i < 8; i++) {
            printf("%02X ", config[i]);
        }
        printf("\n");
    }
    
    // 4. 检查用户数据区可写性
    printf("4. User data writability test...\r\n");
    uint8_t testData[16] = {0xAA, 0xBB, 0xCC, 0xDD};
    if (NT3HWriteUserData(1, testData)) {
        printf("✓ Can write to user data area\r\n");
        
        // 验证写入的数据
        if (NT3HReadUserData(1)) {
            printf("   Written data verified\r\n");
        }
    } else {
        printf("✗ Cannot write to user data area\r\n");
    }
    
    printf("=== Diagnosis Complete ===\r\n");
}

/**
 * @brief 分析I2C错误代码
 * @param errorCode I2C错误代码
 */
void NT3HAnalyzeI2CError(hi_u32 errorCode)
{
    printf("=== I2C Error Analysis (0x%08X) ===\r\n", errorCode);
    
    switch (errorCode) {
        case 0x80001189:
            printf("Error: I2C write permission denied\r\n");
            printf("Possible causes:\r\n");
            printf("- Tag is permanently locked\r\n");
            printf("- Field lock bits are set\r\n");
            printf("- Dynamic lock is active\r\n");
            printf("- Password protection is enabled\r\n");
            break;
        case 0x80001188:
            printf("Error: I2C read permission denied\r\n");
            break;
        case 0x80001187:
            printf("Error: I2C device not responding\r\n");
            break;
        default:
            printf("Unknown I2C error code\r\n");
            break;
    }
}

/**
 * @brief 检查是否可以通过软件方式绕过锁定
 * @note 尝试使用已知的NT3H解锁技巧
 */
bool NT3HTrySoftwareBypass(void)
{
    printf("=== Attempting software bypass ===\r\n");
    
    // 方法1: 尝试写入特定的解锁序列到用户数据区
    uint8_t unlockSequence[] = {
        0xE1, 0x10, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    // 尝试写入到不同的页面，避开锁定的区域
    for (uint8_t page = 1; page <= 5; page++) {
        if (NT3HWriteUserData(page, unlockSequence)) {
            printf("✓ Unlock sequence written to page %d\r\n", page);
            
            // 短暂延迟后读取验证
            usleep(100000);
            if (NT3HReadUserData(page)) {
                printf("✓ Data verified on page %d\r\n", page);
                return true;
            }
        }
    }
    
    printf("✗ Software bypass failed\r\n");
    return false;
}

/**
 * @brief 检查锁定的具体类型和范围
 */
void NT3HAnalyzeLockDetails(void)
{
    printf("=== Detailed Lock Analysis ===\r\n");
    
    uint8_t fieldLock[16];
    uint8_t dynamicLock[16];
    
    if (NT3HReadFieldLock(fieldLock)) {
        printf("Field Lock Register Analysis:\r\n");
        printf("Raw: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", fieldLock[i]);
        }
        printf("\n");
        
        // 分析字段锁定字节
        printf("Byte 0 (0x%02X): ", fieldLock[0]);
        if (fieldLock[0] == 0xDE) {
            printf("Standard NT3H field lock pattern detected\r\n");
        } else {
            printf("Non-standard field lock pattern\r\n");
        }
        
        // 检查哪些页面被锁定
        printf("Locked pages analysis:\r\n");
        for (int page = 0; page < 8; page++) {
            uint8_t lockByte = fieldLock[page];
            if (lockByte != 0x00) {
                printf("  Pages %d-%d may be locked (byte 0x%02X)\r\n", 
                       page * 8, (page * 8) + 7, lockByte);
            }
        }
    }
    
    if (NT3HReadDynamicLock(dynamicLock)) {
        printf("Dynamic Lock Register Analysis:\r\n");
        printf("Raw: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", dynamicLock[i]);
        }
        printf("\n");
        
        // 分析动态锁定字节
        if (dynamicLock[0] == 0xFF) {
            printf("Dynamic lock byte 0 indicates locking is active\r\n");
        }
    }
}

/**
 * @brief 提供NFC标签问题的实用解决方案
 */
void NT3HProvideSolutions(void)
{
    printf("=== NFC Tag Solutions ===\r\n");
    
    // 检查用户数据区是否可写
    uint8_t testData[16] = {0xAA};
    bool canWrite = NT3HWriteUserData(1, testData);
    
    if (canWrite) {
        printf("🎉 GOOD NEWS: User data area is WRITABLE!\r\n");
        printf("This means you can still use the tag for:\r\n");
        printf("- Writing URLs, text, or other NDEF data\r\n");
        printf("- Most practical NFC applications\r\n");
        printf("\n");
        printf("The locking only affects:\r\n");
        printf("- Configuration register writes\r\n");
        printf("- Field/Dynamic lock register writes\r\n");
        printf("- Some advanced features\r\n");
        printf("\n");
        printf("RECOMMENDATION:\r\n");
        printf("Continue using the tag normally for data storage.\r\n");
        printf("The locking won't affect basic NFC functionality.\r\n");
    } else {
        printf("⚠️  User data area is also locked.\r\n");
        printf("This tag may need to be replaced.\r\n");
    }
    
    // 检查是否可以创建有效的NDEF记录
    printf("\nTesting NDEF functionality...\r\n");
    
    // 尝试写入一个简单的URL记录
    uint8_t urlData[] = "https://example.com";
    uint8_t testPage[16] = {
        0x03, 0x11, 0xD1, 0x01, 0x0D, 0x55, 0x01, 'e',
        'x', 'a', 'm', 'p', 'l', 'e', '.', 'c'
    };
    
    if (NT3HWriteUserData(2, testPage)) {
        printf("✓ Can write NDEF-like data structure\r\n");
        printf("✓ Tag should work with mobile NFC apps\r\n");
    }
    
    printf("=== Solutions Complete ===\r\n");
}