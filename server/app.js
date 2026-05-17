const express = require('express');
const app = express();
const cors = require('cors');
const db = require('./config/db');
const mqttClient = require('./services/mqttClient');

app.use(cors());
app.use(express.json());

// 存储待确认的资产（临时）
let pendingAssets = [];

/**
 * 解析 MQTT 消息，提取资产数据
 */
function parseAssetData(mqttData) {
  const assets = [];
  
  try {
    let params = mqttData.params;
    if (!params) {
      params = mqttData;
    }
    
    console.log(`📊 解析资产数据...`);
    
    for (const key in params) {
      if (key === 'search') continue;
      
      const obj = params[key];
      if (!obj || !obj.value) continue;
      
      const value = obj.value;
      
      const asset = {
        id: value.serial_number || value.id || parseInt(key.replace('object', '')) || 0,
        card: (value.card || '').trim(),
        name: value.name || '',
        note: value.note || '',
        RSSI: value.RSSI || value.rssi || 0,
        time: Date.now()
      };
      
      if (asset.id && asset.id !== 0) {
        assets.push(asset);
        console.log(`📦 提取资产: id=${asset.id}, card=${asset.card}, RSSI=${asset.RSSI}`);
      }
    }
    
    console.log(`✅ 成功提取 ${assets.length} 条资产数据`);
    return assets;
    
  } catch (error) {
    console.error('❌ 解析数据失败:', error);
    return [];
  }
}

/**
 * 检查资产是否已存在
 */
function checkAssetExistsInDB(assetId) {
  return new Promise((resolve, reject) => {
    const sql = 'SELECT * FROM asset WHERE id = ? AND note != "delete"';
    db.query(sql, [assetId], (err, results) => {
      if (err) reject(err);
      else resolve(results && results.length > 0 ? results[0] : null);
    });
  });
}

/**
 * 直接更新数据库中的资产
 */
function updateAssetInDB(asset) {
  return new Promise((resolve, reject) => {
    const sql = `
      UPDATE asset 
      SET card = ?, name = ?, note = ?, RSSI = ?, time = ?
      WHERE id = ?
    `;
    const params = [asset.card, asset.name, asset.note, asset.RSSI, asset.time, asset.id];
    
    db.query(sql, params, (err, result) => {
      if (err) {
        console.error('数据库更新失败:', err);
        reject(err);
      } else {
        console.log(`✅ 资产已存在，直接更新: id=${asset.id}, RSSI=${asset.RSSI}`);
        resolve(result);
      }
    });
  });
}

/**
 * 处理接收到的资产数据
 */
async function handleReceivedAsset(asset) {
  console.log(`📥 处理资产: id=${asset.id}`);
  
  const existingAsset = await checkAssetExistsInDB(asset.id);
  
  if (existingAsset) {
    await updateAssetInDB(asset);
  } else {
    const alreadyPending = pendingAssets.some(a => a.id === asset.id);
    if (!alreadyPending) {
      pendingAssets.push({
        id: asset.id,
        card: asset.card,
        name: asset.name,
        rssi: asset.RSSI,
        time: asset.time,
        note: asset.note
      });
      console.log(`🆕 新资产加入待确认列表: id=${asset.id}, 当前待确认数: ${pendingAssets.length}`);
    }
  }
}

// ============= API 路由 =============

// 发送搜索命令
app.post('/api/mqtt/search-command', (req, res) => {
  const { search } = req.body;
  
  console.log(`📡 收到搜索请求: search=${search}`);
  
  if (search === undefined) {
    return res.status(400).json({ error: '缺少 search 参数' });
  }
  
  const result = mqttClient.sendSearchCommand(search);
  
  if (search === 1) {
    pendingAssets = [];
    console.log('清空待确认列表');
  }
  
  res.json({ success: result, message: `搜索命令已发送: search=${search}` });
});

// 获取待确认资产列表
app.get('/api/assets/pending', (req, res) => {
  res.json({ code: 0, data: pendingAssets });
});

// 搜索检测
app.get('/api/assets/search', (req, res) => {
    if(pendingAssets.length>=1){
    console.log(`🔍 搜索检测被调用，待确认资产数: ${pendingAssets.length}`);
  }
  res.json({
    code: 0,
    msg: '检测成功',
    data: { pending: pendingAssets, allAssets: [] }
  });
});

// 确认录入资产
app.post('/api/assets/approve', async (req, res) => {
  const { assets } = req.body;
  console.log(`✅ 确认录入 ${assets?.length || 0} 个资产`);
  
  if (!assets || !assets.length) {
    return res.json({ code: 1, msg: '没有需要录入的资产' });
  }
  
  try {
    for (const asset of assets) {
      const sql = `
        INSERT INTO asset (id, card, name, note, RSSI, time)
        VALUES (?, ?, ?, ?, ?, ?)
        ON DUPLICATE KEY UPDATE
          card = VALUES(card),
          name = VALUES(name),
          note = VALUES(note),
          RSSI = VALUES(RSSI),
          time = VALUES(time)
      `;
      const params = [asset.id, asset.card, asset.name, '', asset.rssi, asset.time];
      
      await new Promise((resolve, reject) => {
        db.query(sql, params, (err) => {
          if (err) reject(err);
          else resolve();
        });
      });
      
      const index = pendingAssets.findIndex(a => a.id === asset.id);
      if (index !== -1) pendingAssets.splice(index, 1);
      
      console.log(`✅ 已录入资产: id=${asset.id}`);
    }
    res.json({ code: 0, msg: `成功录入 ${assets.length} 个资产` });
  } catch (err) {
    console.error('录入失败:', err);
    res.json({ code: 1, msg: '录入失败', error: err.message });
  }
});

// 拒绝资产
app.post('/api/assets/reject', (req, res) => {
  const { assetIds } = req.body;
  console.log(`❌ 拒绝录入 ${assetIds?.length || 0} 个资产`);
  
  if (!assetIds || !assetIds.length) {
    return res.json({ code: 1, msg: '没有需要拒绝的资产' });
  }
  
  for (const id of assetIds) {
    const index = pendingAssets.findIndex(a => a.id === id);
    if (index !== -1) pendingAssets.splice(index, 1);
    console.log(`❌ 已拒绝资产: id=${id}`);
  }
  
  res.json({ code: 0, msg: `已拒绝 ${assetIds.length} 个资产` });
});

// 获取 MQTT 状态
app.get('/api/mqtt/status', (req, res) => {
  res.json(mqttClient.getMQTTStatus());
});

// 资产路由
const assetRouter = require('./routes/asset');
app.use('/api/assets', assetRouter);

app.get('/', (req, res) => {
  res.send('服务器运行成功！');
});

// 🔥 启动服务器
const PORT = 3000;

// 初始化 MQTT 并设置消息处理
mqttClient.initMQTT();

// 设置消息处理函数
mqttClient.setMessageHandler(async (topic, message) => {
  const messageStr = message.toString();
  console.log(`📨 收到设备消息: ${messageStr.substring(0, 200)}...`);
  
  try {
    const mqttData = JSON.parse(messageStr);
    const assets = parseAssetData(mqttData);
    
    for (const asset of assets) {
      await handleReceivedAsset(asset);
    }
    
    console.log(`📊 处理完成，当前待确认资产数: ${pendingAssets.length}`);
  } catch (error) {
    console.error('❌ 处理消息失败:', error);
  }
});

// 启动 HTTP 服务器
app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 HTTP 服务器启动成功: http://0.0.0.0:${PORT}`);
});

// 优雅关闭
process.on('SIGINT', () => {
  console.log('\n正在关闭服务...');
  process.exit(0);
});