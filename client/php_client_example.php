<?php
/**
 * Hi3861 WiFi-IoT设备 PHP客户端示例
 * 
 * 这个示例展示了如何使用PHP与Hi3861设备进行通信
 * 设备提供HTTP API接口，支持设备状态查询和WiFi配置
 * 
 * 字符编码修复版本 - 支持UTF-8编码，解决中文乱码问题
 */

class Hi3861Client {
    private $base_url;
    private $timeout;
    private $charset;
    
    /**
     * 构造函数
     * @param string $ip_address 设备IP地址
     * @param int $port 设备端口，默认80
     * @param int $timeout 请求超时时间，默认10秒
     * @param string $charset 字符编码，默认UTF-8
     */
    public function __construct($ip_address = '192.168.0.1', $port = 80, $timeout = 10, $charset = 'UTF-8') {
        $this->base_url = "http://{$ip_address}:{$port}";
        $this->timeout = $timeout;
        $this->charset = $charset;
    }
    
    /**
     * 发送HTTP请求
     * @param string $url 请求URL
     * @param string $method 请求方法，GET或POST
     * @param array $data POST数据
     * @return array 响应数据
     */
    private function sendRequest($url, $method = 'GET', $data = []) {
        $full_url = $this->base_url . $url;
        $ch = curl_init();
        
        curl_setopt($ch, CURLOPT_URL, $full_url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, $this->timeout);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
        
        // 设置字符编码支持
        curl_setopt($ch, CURLOPT_HTTPHEADER, [
            'Content-Type: application/json; charset=' . $this->charset,
            'Accept: application/json; charset=' . $this->charset
        ]);
        
        if ($method === 'POST') {
            curl_setopt($ch, CURLOPT_POST, true);
            if (!empty($data)) {
                $json_data = json_encode($data, JSON_UNESCAPED_UNICODE);
                curl_setopt($ch, CURLOPT_POSTFIELDS, $json_data);
                curl_setopt($ch, CURLOPT_HTTPHEADER, [
                    'Content-Type: application/json; charset=' . $this->charset,
                    'Content-Length: ' . strlen($json_data),
                    'Accept: application/json; charset=' . $this->charset
                ]);
            }
        }
        
        $response = curl_exec($ch);
        $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $error = curl_error($ch);
        curl_close($ch);
        
        if ($response === false) {
            return [
                'success' => false,
                'error' => '请求失败：网络连接错误 - ' . $error
            ];
        }
        
        // 处理字符编码
        if ($this->charset != 'UTF-8') {
            $response = mb_convert_encoding($response, 'UTF-8', $this->charset);
        }
        
        $json_response = json_decode($response, true);
        if ($json_response === null) {
            return [
                'success' => false,
                'error' => '响应解析失败',
                'raw_response' => $response,
                'http_code' => $http_code
            ];
        }
        
        $json_response['http_code'] = $http_code;
        return $json_response;
    }
    
    /**
     * 获取设备状态
     * @return array 设备状态信息
     */
    public function getStatus() {
        return $this->sendRequest('/api/status');
    }
    
    /**
     * 获取Web界面HTML内容
     * @return string Web界面HTML内容
     */
    public function getWebInterface() {
        $full_url = $this->base_url . '/';
        $ch = curl_init();
        
        curl_setopt($ch, CURLOPT_URL, $full_url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, $this->timeout);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
        
        // 设置HTML内容请求头
        curl_setopt($ch, CURLOPT_HTTPHEADER, [
            'Accept: text/html; charset=' . $this->charset
        ]);
        
        $response = curl_exec($ch);
        $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        
        if ($response !== false && $http_code == 200) {
            // 处理字符编码
            if ($this->charset != 'UTF-8') {
                $response = mb_convert_encoding($response, 'UTF-8', $this->charset);
            }
            return $response;
        }
        
        return false;
    }
    
    /**
     * 扫描可用的WiFi网络
     * @return array WiFi扫描结果
     */
    public function scanWiFi() {
        return $this->sendRequest('/api/wifi/scan');
    }
    
    /**
     * 配置WiFi网络
     * @param string $ssid WiFi网络SSID
     * @param string $password WiFi密码
     * @param int $security_type 安全类型，0-开放，1-WEP，2-WPA
     * @return array 配置结果
     */
    public function configureWiFi($ssid, $password, $security_type = 2) {
        $data = [
            'ssid' => $ssid,
            'password' => $password,
            'security_type' => $security_type
        ];
        return $this->sendRequest('/api/wifi/config', 'POST', $data);
    }
    
    /**
     * 获取传感器数据
     * @return array 传感器数据
     */
    public function getSensorData() {
        return $this->sendRequest('/api/sensor/data');
    }
    
    /**
     * 获取系统信息
     * @return array 系统信息
     */
    public function getSystemInfo() {
        return $this->sendRequest('/api/system/info');
    }
    
    /**
     * 设置WiFi连接超时时间
     * @param int $scan_timeout 扫描超时时间（毫秒）
     * @param int $connect_timeout 连接超时时间（毫秒）
     * @return array 设置结果
     */
    public function setWiFiTimeout($scan_timeout, $connect_timeout) {
        $data = [
            'scan_timeout' => $scan_timeout,
            'connect_timeout' => $connect_timeout
        ];
        return $this->sendRequest('/api/wifi/timeout', 'POST', $data);
    }
    
    /**
     * 测试设备连接
     * @return bool 连接是否成功
     */
    public function testConnection() {
        $status = $this->getStatus();
        return $status['success'] && isset($status['http_code']) && $status['http_code'] == 200;
    }
}

/**
 * 使用示例 - 字符编码修复版本
 */

echo "=== Hi3861 WiFi-IoT设备 PHP客户端示例（字符编码修复版） ===\n\n";

// 创建客户端实例，支持字符编码设置
// 注意：当设备处于AP模式时，IP地址通常是192.168.0.1
$client = new Hi3861Client('192.168.0.1', 80, 15, 'UTF-8');

try {
    // 0. 测试连接
    echo "0. 测试设备连接...\n";
    if ($client->testConnection()) {
        echo "   设备连接成功！\n";
    } else {
        echo "   设备连接失败，请检查设备IP地址和网络连接\n";
        exit(1);
    }
    
    // 1. 获取设备状态
    echo "\n1. 获取设备状态...\n";
    $status = $client->getStatus();
    if ($status['success']) {
        echo "   设备名称: {$status['data']['device_name']}\n";
        echo "   固件版本: {$status['data']['firmware_version']}\n";
        echo "   运行时间: {$status['data']['uptime_ms']} 毫秒\n";
        echo "   WiFi状态: {$status['data']['wifi_status']}\n";
        echo "   IP地址: {$status['data']['ip_address']}\n";
        if (isset($status['data']['temperature'])) {
            echo "   温度: {$status['data']['temperature']}°C\n";
        }
        if (isset($status['data']['humidity'])) {
            echo "   湿度: {$status['data']['humidity']}%\n";
        }
        echo "   HTTP状态码: {$status['http_code']}\n";
    } else {
        echo "   获取状态失败: {$status['error']}\n";
        if (isset($status['http_code'])) {
            echo "   HTTP状态码: {$status['http_code']}\n";
        }
    }
    
    // 2. 获取Web界面（测试字符编码）
    echo "\n2. 测试Web界面字符编码...\n";
    $web_content = $client->getWebInterface();
    if ($web_content !== false) {
        if (strpos($web_content, 'Hi3861配置界面') !== false) {
            echo "   Web界面字符编码测试通过 - 中文正常显示\n";
        } else {
            echo "   Web界面字符编码可能存在异常\n";
        }
        echo "   Web页面大小: " . strlen($web_content) . " 字节\n";
    } else {
        echo "   获取Web界面失败\n";
    }
    
    echo "\n3. 获取传感器数据...\n";
    $sensor_data = $client->getSensorData();
    if ($sensor_data['success']) {
        echo "   传感器数据获取成功\n";
        // 显示传感器数据
        echo "   数据: " . json_encode($sensor_data['data'], JSON_UNESCAPED_UNICODE) . "\n";
        echo "   HTTP状态码: {$sensor_data['http_code']}\n";
    } else {
        echo "   获取传感器数据失败: {$sensor_data['error']}\n";
    }
    
    echo "\n4. WiFi配置示例（取消注释以启用）\n";
    /*
    // 配置WiFi网络
    $wifi_config = $client->configureWiFi('Your_WiFi_SSID', 'Your_WiFi_Password');
    if ($wifi_config['success']) {
        echo "   WiFi配置成功: {$wifi_config['message']}\n";
        
        // 等待设备重新连接
        sleep(10);
        
        // 使用新的IP地址重新连接
        $client = new Hi3861Client('新的设备IP', 80, 15);
        
        // 检查连接状态
        $new_status = $client->getStatus();
        if ($new_status['success']) {
            echo "   重新连接成功，新IP: {$new_status['data']['ip_address']}\n";
        }
    } else {
        echo "   WiFi配置失败: {$wifi_config['error']}\n";
    }
    */
    
    echo "\n5. 设置WiFi超时时间...\n";
    $timeout_config = $client->setWiFiTimeout(30000, 60000); // 30秒扫描，60秒连接
    if ($timeout_config['success']) {
        echo "   超时时间设置成功\n";
        echo "   HTTP状态码: {$timeout_config['http_code']}\n";
    } else {
        echo "   超时时间设置失败: {$timeout_config['error']}\n";
    }
    
} catch (Exception $e) {
    echo "错误: " . $e->getMessage() . "\n";
}

echo "\n=== 示例执行完成 ===\n";
echo "字符编码: UTF-8\n";
echo "设备支持: 中文Web界面，无乱码问题\n";

/**
 * 高级使用示例 - Web界面集成（字符编码修复版）
 */

/*
// 在Web应用中使用
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (isset($_POST['action'])) {
        // 设置字符编码
        header('Content-Type: application/json; charset=UTF-8');
        
        $client = new Hi3861Client('192.168.1.1', 80, 10, 'UTF-8');
        
        switch ($_POST['action']) {
            case 'get_status':
                $status = $client->getStatus();
                echo json_encode($status, JSON_UNESCAPED_UNICODE);
                break;
                
            case 'configure_wifi':
                $ssid = $_POST['ssid'] ?? '';
                $password = $_POST['password'] ?? '';
                if (!empty($ssid)) {
                    $result = $client->configureWiFi($ssid, $password);
                    echo json_encode($result, JSON_UNESCAPED_UNICODE);
                }
                break;
                
            case 'get_web_interface':
                $content = $client->getWebInterface();
                if ($content !== false) {
                    header('Content-Type: text/html; charset=UTF-8');
                    echo $content;
                } else {
                    echo json_encode(['success' => false, 'error' => '获取Web界面失败']);
                }
                break;
                
            default:
                echo json_encode(['success' => false, 'error' => '未知操作'], JSON_UNESCAPED_UNICODE);
        }
        exit;
    }
}
*/

?>