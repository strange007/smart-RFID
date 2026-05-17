// mqttClient.js
const mqtt = require('mqtt');

let client = null;
let isMQTTConnected = false;
let messageHandler = null;  // 添加消息处理函数

// 初始化 MQTT 连接
function initMQTT() {
  console.log('正在连接 MQTT Broker...');
  
  client = mqtt.connect({
    host: '127.0.0.1',
    port: 1883,
    protocol: 'mqtt',
    clientId: 'local_client',
    clean: true,
    keepalive: 60,
    connectTimeout: 5000,
    reconnectPeriod: 3000
  });
  
  client.on('connect', () => {
    console.log('✅ MQTT 连接成功');
    isMQTTConnected = true;
    
    client.subscribe('asset/post', { qos: 1 }, (err) => {
      if (err) {
        console.error('订阅 asset/post 失败:', err);
      } else {
        console.log('✅ 订阅成功: asset/post');
      }
    });
  });
  
  // 🔥 添加消息接收处理
  client.on('message', (topic, message) => {
    console.log(`📨 收到 MQTT 消息 - topic: ${topic}`);
    
    if (topic === 'asset/post') {
      if (messageHandler) {
        messageHandler(topic, message);
      } else {
        console.log('⚠️ 没有设置消息处理函数，消息被忽略');
      }
    }
  });
  
  client.on('error', (err) => {
    console.error('❌ MQTT 错误:', err.message);
    isMQTTConnected = false;
  });
  
  client.on('close', () => {
    console.log('MQTT 连接关闭');
    isMQTTConnected = false;
  });
  
  return client;
}

// 🔥 添加设置消息处理函数的接口
function setMessageHandler(handler) {
  messageHandler = handler;
  console.log('✅ 消息处理函数已设置');
}

// 发送搜索命令
function sendSearchCommand(searchValue) {
  if (!client || !isMQTTConnected) {
    console.error('❌ MQTT 未连接，无法发送命令');
    return false;
  }
  
  const commandMessage = {
    params: { search: searchValue }
  };
  
  const payload = JSON.stringify(commandMessage);
  console.log(`📤 发送搜索命令: topic=asset/command, payload=${payload}`);
  
  client.publish('asset/command', payload, { qos: 1 }, (err) => {
    if (err) {
      console.error(`❌ 发送搜索命令失败: search=${searchValue}`, err);
    } else {
      console.log(`✅ 搜索命令已发送: search=${searchValue}`);
    }
  });
  
  return true;
}

// 发送删除命令
function sendDeleteCommand(assetId) {
  if (!client || !isMQTTConnected) {
    console.error('❌ MQTT 未连接，无法发送删除命令');
    return false;
  }
  
  const commandMessage = {
    params: {
      delete: { id: assetId }
    }
  };
  
  const payload = JSON.stringify(commandMessage);
  console.log(`📤 发送删除命令: topic=asset/command, payload=${payload}`);
  
  client.publish('asset/command', payload, { qos: 1 }, (err) => {
    if (err) {
      console.error(`❌ 发送删除命令失败: id=${assetId}`, err);
    } else {
      console.log(`✅ 删除命令已发送: id=${assetId}`);
    }
  });
  
  return true;
}

// 发送资产数据到设备
function sendAssetToDevice(assetId, assetData) {
  if (!client || !isMQTTConnected) {
    console.error('❌ MQTT 未连接，无法发送资产数据');
    return false;
  }
  
  const objectKey = `object${assetId}`;
  const message = {
    params: {
      [objectKey]: {
        value: {
          serial_number: assetId,
          card: assetData.card || '',
          name: assetData.name || '',
          note: assetData.note || '',
          RSSI: assetData.RSSI || 0
        }
      }
    }
  };
  
  client.publish('asset/post', JSON.stringify(message), { qos: 1 }, (err) => {
    if (err) {
      console.error(`❌ 发送资产数据失败: id=${assetId}`, err);
    } else {
      console.log(`✅ 资产数据已发送: id=${assetId}`);
    }
  });
  
  return true;
}

// 获取连接状态
function getMQTTStatus() {
  return {
    connected: isMQTTConnected,
    clientId: client?.options?.clientId
  };
}

module.exports = {
  initMQTT,
  sendSearchCommand,
  sendDeleteCommand,
  sendAssetToDevice,
  getMQTTStatus,
  setMessageHandler  // 导出设置消息处理函数的接口
};