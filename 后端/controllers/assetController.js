const assetService = require('../services/assetService');
const mqttClient = require('../services/mqttClient'); // 导入 MQTT 模块

// 存储待确认的资产（临时）
let pendingAssets = [];

// 查询全部
const getAssets = (req, res) => {
  assetService.getAllAssets((err, results) => {
    if (err) return res.status(500).json({ code: 1, msg: '查询失败', error: err });
    res.json({ code: 0, msg: '查询成功', data: results });
  });
};

// 根据 id 查询
const getAssetById = (req, res) => {
  const { id } = req.params;
  assetService.getAssetById(id, (err, results) => {
    if (err) return res.status(500).json({ code: 1, msg: '查询失败', error: err });
    if (!results.length) return res.json({ code: 1, msg: '资产不存在' });
    res.json({ code: 0, msg: '查询成功', data: results[0] });
  });
};

// 新增
const addAsset = (req, res) => {
  const data = req.body;
  assetService.addAsset(data, (err, result) => {
    if (err) return res.status(500).json({ code: 1, msg: '添加失败', error: err });
    res.json({ code: 0, msg: '添加成功', id: result.insertId });
  });
};

// 修改
const updateAsset = (req, res) => {
  const { id } = req.params;
  const data = req.body;
  assetService.updateAsset(id, data, (err, result) => {
    if (err) return res.status(500).json({ code: 1, msg: '修改失败', error: err });
    if (result.affectedRows === 0) return res.json({ code: 1, msg: '资产不存在' });
    res.json({ code: 0, msg: '修改成功' });
  });
};

// 删除
const deleteAsset = (req, res) => {
  const { id } = req.params;
  console.log(`🗑️ 删除资产: id=${id}`);
  
  assetService.deleteAsset(id, (err, result) => {
    if (err) {
      console.error('数据库删除失败:', err);
      return res.status(500).json({ code: 1, msg: '删除失败', error: err });
    }
    
    if (result.affectedRows === 0) {
      return res.json({ code: 1, msg: '资产不存在' });
    }
    
    // 发送 MQTT 删除命令到设备
    const deleteId = parseInt(id);
    mqttClient.sendDeleteCommand(deleteId);
    
    res.json({ code: 0, msg: '删除成功' });
  });
};

// 搜索检测 - 前端轮询调用，返回待确认资产
const searchDetect = (req, res) => {
  console.log(`🔍 搜索检测被调用，待确认资产数: ${pendingAssets.length}`);
  res.json({
    code: 0,
    msg: '检测成功',
    data: {
      pending: pendingAssets,
      allAssets: []
    }
  });
};

// 获取待确认资产列表
const getPendingAssets = (req, res) => {
  res.json({
    code: 0,
    data: pendingAssets
  });
};

// 确认录入资产（批量）
const approveAssets = async (req, res) => {
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
        const db = require('../config/db');
        db.query(sql, params, (err) => {
          if (err) reject(err);
          else resolve();
        });
      });
      
      // 从待确认列表中移除
      const index = pendingAssets.findIndex(a => a.id === asset.id);
      if (index !== -1) pendingAssets.splice(index, 1);
      
      console.log(`✅ 已录入资产: id=${asset.id}`);
    }
    res.json({ code: 0, msg: `成功录入 ${assets.length} 个资产` });
  } catch (err) {
    console.error('录入失败:', err);
    res.json({ code: 1, msg: '录入失败', error: err.message });
  }
};

// 拒绝资产（不录入）- 批量
const rejectAssets = (req, res) => {
  const { assetIds } = req.body;
  console.log(`❌ 拒绝录入 ${assetIds?.length || 0} 个资产`);
  
  if (!assetIds || !assetIds.length) {
    return res.json({ code: 1, msg: '没有需要拒绝的资产' });
  }
  
  // 从待确认列表中移除
  for (const id of assetIds) {
    const index = pendingAssets.findIndex(a => a.id === id);
    if (index !== -1) {
      pendingAssets.splice(index, 1);
      console.log(`❌ 已拒绝资产: id=${id}`);
    }
  }
  
  res.json({ code: 0, msg: `已拒绝 ${assetIds.length} 个资产` });
};

// 添加待确认资产（供 MQTT 模块调用）
function addPendingAsset(asset) {
  const alreadyPending = pendingAssets.some(a => a.id === asset.id);
  if (!alreadyPending) {
    pendingAssets.push(asset);
    console.log(`🆕 新资产加入待确认列表: id=${asset.id}, 当前待确认数: ${pendingAssets.length}`);
  }
}

// 清空待确认列表
function clearPendingAssets() {
  pendingAssets = [];
  console.log('清空待确认列表');
}

module.exports = {
  getAssets,
  getAssetById,
  addAsset,
  updateAsset,
  deleteAsset,
  syncFromCloud,
  searchDetect,
  getPendingAssets,
  approveAssets,
  rejectAssets,
  addPendingAsset,
  clearPendingAssets
};