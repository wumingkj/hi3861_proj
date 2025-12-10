<?php
// 启用错误报告
error_reporting(E_ALL);
ini_set('display_errors', 1);
 
// 数据库配置
$servername = "localhost";
$username = "root";
$password = "123456"; // 替换为实际密码,修改密码命令,alter user 'root'@'localhost' identified by '123456';
$dbname = "mydb"; // 替换为实际数据库名,需要自己创建,命令,create database usr;
 
try {
    // 创建连接
    $conn = new mysqli($servername, $username, $password, $dbname);
    // 检查连接
    if ($conn->connect_error) {
        throw new Exception("连接失败: " . $conn->connect_error);
    }
    // 设置字符集
    if (!$conn->set_charset("utf8mb4")) {
        throw new Exception("字符集设置失败: " . $conn->error);
    }
    echo "MySQL连接成功！服务端版本: " . $conn->server_version;
    // 关闭连接
    $conn->close();
} catch (Exception $e) {
    die("数据库错误: " . $e->getMessage());
}
?>
