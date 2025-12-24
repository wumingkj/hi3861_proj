// lib/presentation/screens/nfc/nfc_wifi_screen.dart
import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_nfc_kit/flutter_nfc_kit.dart';
import 'package:ndef/ndef.dart' as ndef;
import 'package:ndef/utilities.dart';
// import 'package:device_monitor/presentation/widgets/loading_widget.dart';

/// NFC WIFI配置屏幕
class NfcWifiScreen extends StatefulWidget {
  const NfcWifiScreen({super.key});

  @override
  State<NfcWifiScreen> createState() => _NfcWifiScreenState();
}

class _NfcWifiScreenState extends State<NfcWifiScreen> {
  final GlobalKey<FormState> _formKey = GlobalKey<FormState>();
  final TextEditingController _ssidController = TextEditingController();
  final TextEditingController _passwordController = TextEditingController();
  final TextEditingController _authTypeController = TextEditingController(text: 'WPA2');
  
  String _nfcStatus = '准备就绪';
  bool _isPolling = false;
  bool _isWriting = false;
  String? _currentWiFiConfig;
  
  // WIFI认证类型列表
  final List<String> _authTypes = [
    'NONE',        // 开放网络
    'WEP',         // WEP加密
    'WPA',         // WPA加密
    'WPA2',        // WPA2加密 (最常见)
    'WPA2_EAP',    // WPA2企业级
    'WPA3',        // WPA3加密
  ];

  @override
  void dispose() {
    _ssidController.dispose();
    _passwordController.dispose();
    _authTypeController.dispose();
    super.dispose();
  }

  /// 显示提示消息
  void _showToast(String msg) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(msg),
        duration: const Duration(seconds: 2),
        backgroundColor: Colors.blue,
      ),
    );
  }

  /// 将WIFI配置转换为NDEF记录
  ndef.NDEFRecord _createWiFiRecord() {
    // 创建WIFI配置的JSON字符串
    final wifiConfig = {
      's': _ssidController.text,       // SSID
      'p': _passwordController.text,   // 密码
      't': _authTypeController.text,   // 认证类型
      'n': 1,                          // 网络优先级
    };
    
    final wifiConfigJson = jsonEncode(wifiConfig);
    
    // 创建NDEF记录
    // 对于WIFI配置，我们使用MIME类型 "application/vnd.wfa.wsc"
    return ndef.NDEFRecord(
      tnf: ndef.TypeNameFormat.media,
      type: 'application/vnd.wfa.wsc'.toBytes(),
      payload: wifiConfigJson.toBytes(),
    );
  }

  /// 从NDEF记录中解析WIFI配置
  Map<String, dynamic>? _parseWiFiRecord(ndef.NDEFRecord record) {
    try {
      if (record.tnf == ndef.TypeNameFormat.media &&
          utf8.decode(record.type!) == 'application/vnd.wfa.wsc') {
        final payloadBytes = record.payload;
        if (payloadBytes != null && payloadBytes.isNotEmpty) {
          final payloadStr = utf8.decode(payloadBytes);
          return jsonDecode(payloadStr);
        }
      }
      return null;
    } catch (e) {
      print('解析WIFI配置失败: $e');
      return null;
    }
  }

  /// 获取记录的类型字符串
  String? _getTypeString(ndef.NDEFRecord record) {
    if (record.type == null) return null;
    return utf8.decode(record.type!);
  }

  /// 读取NFC标签中的WIFI配置
  Future<void> _readWiFiConfig() async {
    if (_isPolling) return;
    
    setState(() {
      _isPolling = true;
      _nfcStatus = '请将NFC标签靠近设备...';
    });

    try {
      final tag = await FlutterNfcKit.poll(
        timeout: const Duration(seconds: 30),
      );

      setState(() {
        _nfcStatus = '标签检测到: ${tag.type}\n正在读取配置...';
      });

      if (tag.ndefAvailable ?? false) {
        final records = await FlutterNfcKit.readNDEFRecords();
        
        bool foundConfig = false;
        for (final record in records) {
          final wifiConfig = _parseWiFiRecord(record);
          if (wifiConfig != null) {
            setState(() {
              _ssidController.text = wifiConfig['s'] ?? '';
              _passwordController.text = wifiConfig['p'] ?? '';
              _authTypeController.text = wifiConfig['t'] ?? 'WPA2';
              _currentWiFiConfig = '已从NFC标签读取WIFI配置';
              _nfcStatus = 'WIFI配置读取成功！';
            });
            _showToast('WIFI配置读取成功');
            foundConfig = true;
            break;
          }
        }
        
        if (!foundConfig) {
          setState(() {
            _nfcStatus = '未找到WIFI配置';
          });
          _showToast('未找到WIFI配置');
          
          // 显示读取到的记录信息用于调试
          if (records.isNotEmpty) {
            print('读取到的记录：');
            for (int i = 0; i < records.length; i++) {
              final record = records[i];
              print('记录 $i: TNF=${record.tnf}, Type=${_getTypeString(record)}');
              if (record.payload != null) {
                print('Payload: ${record.payload!.toHexString()}');
              }
            }
          }
        }
      } else {
        setState(() {
          _nfcStatus = '标签不支持NDEF';
        });
        _showToast('标签不支持NDEF');
      }
    } catch (e) {
      setState(() {
        _nfcStatus = '读取失败: $e';
      });
      _showToast('读取失败: ${e.toString()}');
    } finally {
      try {
        await FlutterNfcKit.finish();
      } catch (e) {
        print('结束NFC会话失败: $e');
      }
      setState(() {
        _isPolling = false;
      });
    }
  }

  /// 将WIFI配置写入NFC标签
  Future<void> _writeWiFiConfig() async {
    if (!_formKey.currentState!.validate()) {
      _showToast('请填写完整的WIFI配置信息');
      return;
    }

    if (_isWriting) return;
    
    setState(() {
      _isWriting = true;
      _nfcStatus = '请将NFC标签靠近设备...';
    });

    try {
      final tag = await FlutterNfcKit.poll(
        timeout: const Duration(seconds: 30),
      );

      setState(() {
        _nfcStatus = '标签检测到: ${tag.type}\n正在写入配置...';
      });

      if (tag.ndefWritable ?? false) {
        final wifiRecord = _createWiFiRecord();
        await FlutterNfcKit.writeNDEFRecords([wifiRecord]);
        
        setState(() {
          _nfcStatus = 'WIFI配置写入成功！';
          _currentWiFiConfig = 'SSID: ${_ssidController.text}\n'
                              '认证类型: ${_authTypeController.text}\n'
                              '写入时间: ${DateTime.now().toString()}';
        });
        _showToast('WIFI配置写入成功');
      } else {
        setState(() {
          _nfcStatus = '标签不可写';
        });
        _showToast('标签不可写');
      }
    } catch (e) {
      setState(() {
        _nfcStatus = '写入失败: $e';
      });
      _showToast('写入失败: ${e.toString()}');
    } finally {
      try {
        await FlutterNfcKit.finish();
      } catch (e) {
        print('结束NFC会话失败: $e');
      }
      setState(() {
        _isWriting = false;
      });
    }
  }

  /// 清除表单
  void _clearForm() {
    _formKey.currentState?.reset();
    _ssidController.clear();
    _passwordController.clear();
    _authTypeController.text = 'WPA2';
    setState(() {
      _currentWiFiConfig = null;
      _nfcStatus = '准备就绪';
    });
    _showToast('表单已清除');
  }

  /// 验证WIFI密码（针对不同类型的网络）
  String? _validatePassword(String? value) {
    if (_authTypeController.text != 'NONE' && 
        (value == null || value.isEmpty)) {
      return '请输入WIFI密码';
    }
    
    // 针对WPA2/WPA3，建议密码长度至少8位
    if ((_authTypeController.text == 'WPA2' || 
         _authTypeController.text == 'WPA3') && 
        value != null && 
        value.length < 8) {
      return 'WPA2/WPA3密码建议至少8位';
    }
    
    return null;
  }

  /// 模拟写入功能（用于测试）
  Future<void> _simulateWrite() async {
    if (!_formKey.currentState!.validate()) {
      _showToast('请填写完整的WIFI配置信息');
      return;
    }

    setState(() {
      _isWriting = true;
      _nfcStatus = '模拟写入中...';
    });

    // 模拟延迟
    await Future.delayed(const Duration(seconds: 2));

    setState(() {
      _nfcStatus = '模拟写入完成！';
      _currentWiFiConfig = 'SSID: ${_ssidController.text}\n'
                          '认证类型: ${_authTypeController.text}\n'
                          '状态: 模拟写入成功';
      _isWriting = false;
    });
    
    _showToast('模拟写入完成（仅用于测试）');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // 背景图片（与首页保持一致）
          Image.asset(
            "assets/images/bg.jpg",
            fit: BoxFit.cover,
            width: double.infinity,
            height: double.infinity,
          ),
          
          // 内容层
          Column(
            children: [
              // 顶部标题栏
              Container(
                width: double.infinity,
                padding: const EdgeInsets.only(top: 40, bottom: 16),
                color: Colors.black.withOpacity(0.7),
                child: Row(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.arrow_back, color: Colors.white),
                      onPressed: () => Navigator.pop(context),
                    ),
                    const SizedBox(width: 16),
                    const Text(
                      'NFC WIFI配置',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ],
                ),
              ),

              Expanded(
                child: SingleChildScrollView(
                  padding: const EdgeInsets.all(20.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      // NFC状态显示卡片
                      Card(
                        elevation: 4,
                        color: Colors.white.withOpacity(0.9),
                        child: Padding(
                          padding: const EdgeInsets.all(16.0),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Row(
                                children: [
                                  const Icon(Icons.nfc, color: Colors.blue),
                                  const SizedBox(width: 8),
                                  const Text(
                                    'NFC状态',
                                    style: TextStyle(
                                      fontSize: 18,
                                      fontWeight: FontWeight.bold,
                                      color: Colors.blue,
                                    ),
                                  ),
                                  const Spacer(),
                                  // 模拟模式开关（用于测试）
                                  Switch(
                                    value: false,
                                    onChanged: (value) {
                                      // 可以在这里添加模拟模式切换逻辑
                                      _showToast('真实NFC模式');
                                    },
                                    activeColor: Colors.blue,
                                  ),
                                  const Text(
                                    '真实模式',
                                    style: TextStyle(fontSize: 12),
                                  ),
                                ],
                              ),
                              const SizedBox(height: 10),
                              Text(
                                _nfcStatus,
                                style: TextStyle(
                                  fontSize: 16,
                                  color: _isPolling || _isWriting 
                                      ? Colors.orange 
                                      : Colors.green,
                                ),
                              ),
                              if (_isPolling || _isWriting) ...[
                                const SizedBox(height: 10),
                                const LinearProgressIndicator(),
                              ],
                            ],
                          ),
                        ),
                      ),

                      const SizedBox(height: 20),

                      // WIFI配置表单卡片
                      Card(
                        elevation: 4,
                        color: Colors.white.withOpacity(0.9),
                        child: Padding(
                          padding: const EdgeInsets.all(16.0),
                          child: Form(
                            key: _formKey,
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                const Text(
                                  'WIFI配置',
                                  style: TextStyle(
                                    fontSize: 18,
                                    fontWeight: FontWeight.bold,
                                    color: Colors.blue,
                                  ),
                                ),
                                const SizedBox(height: 20),
                                
                                // SSID输入
                                TextFormField(
                                  controller: _ssidController,
                                  decoration: InputDecoration(
                                    labelText: 'WIFI名称 (SSID)',
                                    prefixIcon: const Icon(Icons.wifi),
                                    border: const OutlineInputBorder(),
                                    filled: true,
                                    fillColor: Colors.grey[50],
                                  ),
                                  validator: (value) {
                                    if (value == null || value.isEmpty) {
                                      return '请输入WIFI名称';
                                    }
                                    return null;
                                  },
                                ),
                                
                                const SizedBox(height: 16),
                                
                                // 认证类型选择
                                DropdownButtonFormField<String>(
                                  value: _authTypeController.text,
                                  decoration: InputDecoration(
                                    labelText: '认证类型',
                                    prefixIcon: const Icon(Icons.security),
                                    border: const OutlineInputBorder(),
                                    filled: true,
                                    fillColor: Colors.grey[50],
                                  ),
                                  items: _authTypes.map((type) {
                                    return DropdownMenuItem<String>(
                                      value: type,
                                      child: Text(type),
                                    );
                                  }).toList(),
                                  onChanged: (value) {
                                    setState(() {
                                      _authTypeController.text = value!;
                                    });
                                  },
                                ),
                                
                                const SizedBox(height: 16),
                                
                                // 密码输入
                                TextFormField(
                                  controller: _passwordController,
                                  decoration: InputDecoration(
                                    labelText: 'WIFI密码',
                                    prefixIcon: const Icon(Icons.lock),
                                    border: const OutlineInputBorder(),
                                    filled: true,
                                    fillColor: Colors.grey[50],
                                  ),
                                  obscureText: true,
                                  validator: _validatePassword,
                                ),
                              ],
                            ),
                          ),
                        ),
                      ),

                      const SizedBox(height: 20),

                      // 操作按钮行
                      Row(
                        children: [
                          Expanded(
                            child: ElevatedButton.icon(
                              onPressed: _isPolling ? null : _readWiFiConfig,
                              icon: const Icon(Icons.nfc_rounded),
                              label: const Text('读取NFC配置'),
                              style: ElevatedButton.styleFrom(
                                padding: const EdgeInsets.symmetric(vertical: 15),
                                backgroundColor: Colors.green,
                                foregroundColor: Colors.white,
                              ),
                            ),
                          ),
                          const SizedBox(width: 10),
                          Expanded(
                            child: ElevatedButton.icon(
                              onPressed: _isWriting ? null : _writeWiFiConfig,
                              icon: const Icon(Icons.edit),
                              label: const Text('写入NFC标签'),
                              style: ElevatedButton.styleFrom(
                                padding: const EdgeInsets.symmetric(vertical: 15),
                                backgroundColor: Colors.blue,
                                foregroundColor: Colors.white,
                              ),
                            ),
                          ),
                        ],
                      ),

                      const SizedBox(height: 10),

                      // 辅助按钮行
                      Row(
                        children: [
                          Expanded(
                            child: OutlinedButton.icon(
                              onPressed: _clearForm,
                              icon: const Icon(Icons.clear),
                              label: const Text('清除配置'),
                              style: OutlinedButton.styleFrom(
                                padding: const EdgeInsets.symmetric(vertical: 15),
                              ),
                            ),
                          ),
                          const SizedBox(width: 10),
                          Expanded(
                            child: OutlinedButton.icon(
                              onPressed: _simulateWrite,
                              icon: const Icon(Icons.sim_card),
                              label: const Text('模拟写入'),
                              style: OutlinedButton.styleFrom(
                                padding: const EdgeInsets.symmetric(vertical: 15),
                                side: const BorderSide(color: Colors.orange),
                              ),
                            ),
                          ),
                        ],
                      ),

                      const SizedBox(height: 20),

                      // 当前配置显示
                      if (_currentWiFiConfig != null)
                        Card(
                          elevation: 4,
                          color: Colors.green[50]?.withOpacity(0.9),
                          child: Padding(
                            padding: const EdgeInsets.all(16.0),
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Row(
                                  children: [
                                    const Icon(Icons.check_circle, color: Colors.green),
                                    const SizedBox(width: 8),
                                    const Text(
                                      '当前配置',
                                      style: TextStyle(
                                        fontSize: 18,
                                        fontWeight: FontWeight.bold,
                                        color: Colors.green,
                                      ),
                                    ),
                                  ],
                                ),
                                const SizedBox(height: 10),
                                Text(
                                  _currentWiFiConfig!,
                                  style: const TextStyle(fontSize: 16),
                                ),
                              ],
                            ),
                          ),
                        ),

                      const SizedBox(height: 20),

                      // 使用说明卡片
                      Card(
                        elevation: 2,
                        color: Colors.white.withOpacity(0.9),
                        child: Padding(
                          padding: const EdgeInsets.all(16.0),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              const Text(
                                '使用说明',
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                  color: Colors.blue,
                                ),
                              ),
                              const SizedBox(height: 10),
                              const Text(
                                '1. 输入WIFI配置信息（SSID、认证类型、密码）\n'
                                '2. 点击"写入NFC标签"将配置保存到NFC标签\n'
                                '3. 开发板扫描NFC标签即可自动连接WIFI\n'
                                '4. 点击"读取NFC配置"可读取标签中的WIFI信息\n'
                                '5. 支持的认证类型：开放网络、WEP、WPA、WPA2、WPA3\n'
                                '6. "模拟写入"仅用于测试，无需真实NFC设备',
                                style: TextStyle(fontSize: 14),
                              ),
                              const SizedBox(height: 10),
                              const Text(
                                '注意：确保设备支持NFC功能，并已开启NFC',
                                style: TextStyle(
                                  fontSize: 12,
                                  color: Colors.red,
                                  fontStyle: FontStyle.italic,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}