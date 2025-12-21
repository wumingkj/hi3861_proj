<?php
/**
 * Hi3861 WiFi-IoT设备控制API
 * 实际连接版本 - 只做设备通信，不模拟数据
 */

header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

// ============ Hi3861设备客户端类 ============
class Hi3861Client {
    private $base_url;
    private $timeout;
    private $charset;
    
    public function __construct($ip_address = '192.168.0.1', $port = 80, $timeout = 3, $charset = 'UTF-8') {
        $this->base_url = "http://{$ip_address}:{$port}";
        $this->timeout = $timeout;
        $this->charset = $charset;
    }
    
    /**
     * 发送HTTP请求（快速版）
     */
    private function sendRequest($url, $method = 'GET', $data = []) {
        $full_url = $this->base_url . $url;
        $ch = curl_init();
        
        curl_setopt($ch, CURLOPT_URL, $full_url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, $this->timeout);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 1); // 连接超时1秒
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
        curl_setopt($ch, CURLOPT_USERAGENT, 'Hi3861-Controller/1.0');
        
        $headers = [
            'Content-Type: application/json; charset=' . $this->charset,
            'Accept: application/json; charset=' . $this->charset
        ];
        
        if ($method === 'POST') {
            curl_setopt($ch, CURLOPT_POST, true);
            if (!empty($data)) {
                $json_data = json_encode($data, JSON_UNESCAPED_UNICODE);
                curl_setopt($ch, CURLOPT_POSTFIELDS, $json_data);
                $headers = [
                    'Content-Type: application/json; charset=' . $this->charset,
                    'Content-Length: ' . strlen($json_data),
                    'Accept: application/json; charset=' . $this->charset
                ];
            }
        }
        
        curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
        
        $response = curl_exec($ch);
        $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $error = curl_error($ch);
        curl_close($ch);
        
        if ($response === false) {
            return [
                'success' => false,
                'error' => '连接超时或设备无响应',
                'http_code' => 0
            ];
        }
        
        // 处理字符编码
        if ($this->charset != 'UTF-8') {
            $response = mb_convert_encoding($response, 'UTF-8', $this->charset);
        }
        
        $json_response = json_decode($response, true);
        if ($json_response === null) {
            // 尝试解析设备可能返回的其他格式
            $data = $this->parseRawResponse($response, $http_code);
            return $data;
        }
        
        $json_response['http_code'] = $http_code;
        $json_response['success'] = $json_response['success'] ?? ($http_code >= 200 && $http_code < 300);
        
        return $json_response;
    }
    
    /**
     * 解析原始响应
     */
    private function parseRawResponse($response, $http_code) {
        $data = [
            'success' => $http_code >= 200 && $http_code < 300,
            'raw_response' => $response,
            'http_code' => $http_code,
            'data' => []
        ];
        
        // 如果是HTML页面，设备可能在线但API路径错误
        if (strpos($response, '<html') !== false || strpos($raw, '<!DOCTYPE') !== false) {
            $data['data'] = [
                'device_online' => true,
                'message' => '设备已连接，但API路径可能需要调整'
            ];
        }
        // 如果是纯文本，尝试解析温湿度
        elseif (preg_match('/(温度|temp)[\s:：]*([\d.]+)/i', $response, $tempMatches)) {
            $data['data']['temperature'] = floatval($tempMatches[2]);
        }
        elseif (preg_match('/(湿度|humi)[\s:：]*([\d.]+)/i', $response, $humiMatches)) {
            $data['data']['humidity'] = floatval($humiMatches[2]);
        }
        
        return $data;
    }
    
    /**
     * 快速连接测试
     */
    public function quickConnectionTest() {
        try {
            $socket = @fsockopen('192.168.0.1', 80, $errno, $errstr, 1);
            if ($socket) {
                fclose($socket);
                return [
                    'success' => true,
                    'data' => [
                        'connected' => true,
                        'message' => '设备连接正常',
                        'latency' => '低延迟'
                    ]
                ];
            }
            
            return [
                'success' => false,
                'data' => [
                    'connected' => false,
                    'message' => '设备连接失败',
                    'error' => $errstr
                ]
            ];
            
        } catch (Exception $e) {
            return [
                'success' => false,
                'error' => '连接测试异常'
            ];
        }
    }
    
    /**
     * 获取设备状态（快速）
     */
    public function getStatus() {
        return $this->sendRequest('/api/status');
    }
    
    /**
     * 获取传感器数据（高精度）
     */
    public function getSensorData() {
        $result = $this->sendRequest('/api/sensor/data');
        
        // 如果没有数据但连接成功，返回空数据
        if ($result['success'] && empty($result['data'])) {
            $result['data'] = [
                'temperature' => null,
                'humidity' => null,
                'timestamp' => time()
            ];
        }
        
        return $result;
    }
    
    /**
     * 仅获取温度数据
     */
    public function getTemperature() {
        $result = $this->sendRequest('/api/sensor/temperature');
        if (!$result['success']) {
            // 如果专用API失败，尝试完整数据
            $fullResult = $this->getSensorData();
            if ($fullResult['success'] && isset($fullResult['data']['temperature'])) {
                return [
                    'success' => true,
                    'data' => ['temperature' => $fullResult['data']['temperature']]
                ];
            }
        }
        return $result;
    }
    
    /**
     * 仅获取湿度数据
     */
    public function getHumidity() {
        $result = $this->sendRequest('/api/sensor/humidity');
        if (!$result['success']) {
            // 如果专用API失败，尝试完整数据
            $fullResult = $this->getSensorData();
            if ($fullResult['success'] && isset($fullResult['data']['humidity'])) {
                return [
                    'success' => true,
                    'data' => ['humidity' => $fullResult['data']['humidity']]
                ];
            }
        }
        return $result;
    }
    
    /**
     * 控制红色指示灯
     */
    public function controlIndicator($state) {
        $data = ['state' => $state];
        $result = $this->sendRequest('/api/led/control', 'POST', $data);
        
        // 快速响应，不等待完整结果
        if ($result['http_code'] >= 200 && $result['http_code'] < 300) {
            return [
                'success' => true,
                'data' => [
                    'state' => $state,
                    'message' => '指令已发送'
                ]
            ];
        }
        
        return $result;
    }
    
    /**
     * 控制蜂鸣器
     */
    public function controlBuzzer($action, $params = []) {
        $data = array_merge(['action' => $action], $params);
        $result = $this->sendRequest('/api/buzzer/control', 'POST', $data);
        
        // 快速响应
        if ($result['http_code'] >= 200 && $result['http_code'] < 300) {
            return [
                'success' => true,
                'data' => [
                    'action' => $action,
                    'message' => '蜂鸣器指令已发送'
                ]
            ];
        }
        
        return $result;
    }
    
    /**
     * 配置WiFi网络
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
     * 扫描WiFi网络
     */
    public function scanWiFi() {
        return $this->sendRequest('/api/wifi/scan');
    }
}

// ============ 主处理逻辑 ============
$action = $_REQUEST['action'] ?? '';
$response = [
    'success' => false,
    'timestamp' => time(),
    'action' => $action,
    'device_type' => 'Hi3861 WiFi-IoT'
];

try {
    // 创建Hi3861客户端实例（超时时间缩短）
    $client = new Hi3861Client('192.168.0.1', 80, 2, 'UTF-8');
    
    switch ($action) {
        // ============ 快速连接测试 ============
        case 'quick_connection_test':
            $result = $client->quickConnectionTest();
            $response = array_merge($response, $result);
            break;
            
        // ============ 连接测试 ============
        case 'test_connection':
            $result = $client->getStatus();
            $response = array_merge($response, $result);
            
            // 同时测试快速连接
            if (!$result['success']) {
                $quickResult = $client->quickConnectionTest();
                if ($quickResult['success']) {
                    $response['success'] = true;
                    $response['data'] = array_merge(
                        $response['data'] ?? [],
                        $quickResult['data'],
                        ['quick_test' => true]
                    );
                }
            }
            break;
            
        // ============ 设备状态查询 ============
        case 'get_status':
            $result = $client->getStatus();
            $response = array_merge($response, $result);
            break;
            
        case 'get_sensor_data':
            $result = $client->getSensorData();
            $response = array_merge($response, $result);
            break;
            
        // ============ 高精度温湿度查询 ============
        case 'get_temperature':
            $result = $client->getTemperature();
            $response = array_merge($response, $result);
            break;
            
        case 'get_humidity':
            $result = $client->getHumidity();
            $response = array_merge($response, $result);
            break;
            
        // ============ 红色指示灯控制 ============
        case 'control_indicator':
            $state = $_REQUEST['state'] ?? '';
            if (!in_array($state, ['0', '1'])) {
                $response['error'] = '无效的指示灯状态 (必须是0或1)';
                break;
            }
            
            $result = $client->controlIndicator($state);
            $response = array_merge($response, $result);
            break;
            
        // ============ 蜂鸣器控制 ============
        case 'buzzer_beep':
            $result = $client->controlBuzzer('beep');
            $response = array_merge($response, $result);
            break;
            
        case 'buzzer_alert':
            $result = $client->controlBuzzer('alert');
            $response = array_merge($response, $result);
            break;
            
        case 'buzzer_custom':
            $frequency = intval($_REQUEST['frequency'] ?? 1000);
            $duration = intval($_REQUEST['duration'] ?? 100);
            
            $result = $client->controlBuzzer('custom', [
                'frequency' => $frequency,
                'duration' => $duration
            ]);
            $response = array_merge($response, $result);
            break;
            
        case 'buzzer_test':
            $result = $client->controlBuzzer('test');
            $response = array_merge($response, $result);
            break;
            
        // ============ WiFi控制 ============
        case 'scan_wifi':
            $result = $client->scanWiFi();
            $response = array_merge($response, $result);
            break;
            
        case 'configure_wifi':
            $ssid = $_REQUEST['ssid'] ?? '';
            $password = $_REQUEST['password'] ?? '';
            $security = intval($_REQUEST['security'] ?? 2);
            
            if (empty($ssid)) {
                $response['error'] = 'SSID不能为空';
                break;
            }
            
            $result = $client->configureWiFi($ssid, $password, $security);
            $response = array_merge($response, $result);
            break;
            
        // ============ 默认操作 ============
        default:
            // 快速测试连接
            $quickResult = $client->quickConnectionTest();
            
            $response['success'] = $quickResult['success'];
            $response['connection_test'] = $quickResult['data'] ?? $quickResult;
            
            $response['available_actions'] = [
                'quick_connection_test' => '快速连接测试',
                'test_connection' => '完整连接测试',
                'get_status' => '获取设备状态',
                'get_sensor_data' => '获取传感器数据',
                'get_temperature' => '仅获取温度（高精度）',
                'get_humidity' => '仅获取湿度（高精度）',
                'control_indicator' => '控制红色指示灯 (需state参数: 0或1)',
                'buzzer_beep' => '蜂鸣器短鸣',
                'buzzer_alert' => '蜂鸣器警报',
                'buzzer_custom' => '自定义蜂鸣器 (需frequency,duration参数)',
                'buzzer_test' => '蜂鸣器测试序列',
                'scan_wifi' => '扫描WiFi网络',
                'configure_wifi' => '配置WiFi网络 (需ssid,password参数)'
            ];
            $response['device_ip'] = '192.168.0.1:80';
            break;
    }
    
} catch (Exception $e) {
    $response['error'] = '服务器异常: ' . $e->getMessage();
}

echo json_encode($response, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
?>