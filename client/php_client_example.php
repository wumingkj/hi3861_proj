<?php
/**
 * Hi3861 WiFi-IoT设备 PHP客户端示例
 * 
 * 这个示例展示了如何使用PHP与Hi3861设备进行通信
 * 设备提供HTTP API接口，支持设备状态查询和WiFi配置
 */

class Hi3861Client {
    private $base_url;
    private $timeout;
    
    /**
     * 构造函数
     * @param string $ip_address 设备IP地址
     * @param int $port 设备端口，默认80
     * @param int $timeout 请求超时时间，默认10秒
     */
    public function __construct($ip_address = '192.168.1.1', $port = 80, $timeout = 10) {
        $this->base_url = "http://{$ip_address}:{$port}";
        $this->timeout = $timeout;
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
        
        if ($method === 'POST') {
            curl_setopt($ch, CURLOPT_POST, true);
            if (!empty($data)) {
                $json_data = json_encode($data);
                curl_setopt($ch, CURLOPT_POSTFIELDS, $json_data);
                curl_setopt($ch, CURLOPT_HTTPHEADER, [
                    'Content-Type: application/json',
                    'Content-Length: ' . strlen($json_data)
                ]);
            }
        }
        
        $response = curl_exec($ch);
        $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        
        if ($response === false) {
            return [
                'success' => false,
                'error' => '请求失败：网络连接错误'
            ];
        }
        
        $json_response = json_decode($response, true);
        if ($json_response === null) {
            return [
                'success' => false,
                'error' => '响应解析失败',
                'raw_response' => $response
            ];
        }
        
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
}

/**
 * 使用示例
 */

echo "=== Hi3861 WiFi-IoT设备 PHP客户端示例 ===\n\n";

// 创建客户端实例
// 注意：当设备处于AP模式时，IP地址通常是192.168.1.1
$client = new Hi3861Client('192.168.1.1', 80, 15);

try {
    // 1. 获取设备状态
    echo "1. 获取设备状态...\n";
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
    } else {
        echo "   获取状态失败: {$status['error']}\n";
    }
    
    echo "\n2. 获取传感器数据...\n";
    $sensor_data = $client->getSensorData();
    if ($sensor_data['success']) {
        echo "   传感器数据获取成功\n";
        // 显示传感器数据
        print_r($sensor_data['data']);
    } else {
        echo "   获取传感器数据失败: {$sensor_data['error']}\n";
    }
    
    echo "\n3. WiFi配置示例（取消注释以启用）\n";
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
    
    echo "\n4. 设置WiFi超时时间...\n";
    $timeout_config = $client->setWiFiTimeout(30000, 60000); // 30秒扫描，60秒连接
    if ($timeout_config['success']) {
        echo "   超时时间设置成功\n";
    } else {
        echo "   超时时间设置失败: {$timeout_config['error']}\n";
    }
    
} catch (Exception $e) {
    echo "错误: " . $e->getMessage() . "\n";
}

echo "\n=== 示例执行完成 ===\n";

/**
 * 高级使用示例 - Web界面集成
 */

/*
// 在Web应用中使用
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (isset($_POST['action'])) {
        $client = new Hi3861Client('192.168.1.1');
        
        switch ($_POST['action']) {
            case 'get_status':
                $status = $client->getStatus();
                header('Content-Type: application/json');
                echo json_encode($status);
                break;
                
            case 'configure_wifi':
                $ssid = $_POST['ssid'] ?? '';
                $password = $_POST['password'] ?? '';
                if (!empty($ssid)) {
                    $result = $client->configureWiFi($ssid, $password);
                    header('Content-Type: application/json');
                    echo json_encode($result);
                }
                break;
                
            default:
                echo json_encode(['success' => false, 'error' => '未知操作']);
        }
        exit;
    }
}
*/

?>