const db = require('../config/db')

/**
 * 查询全部资产
 */
exports.getAllAssets = callback => {
  const sql = 'SELECT id, card, name, note, RSSI, time FROM asset ORDER BY id'
  // console.log('执行SQL:', sql)
  db.query(sql, (err, results) => {
    if (err) {
      console.error('查询失败:', err)
      callback(err, null)
    } else {
      // console.log('查询成功，返回条数:', results?.length || 0)
      // if (results && results.length > 0) {
      //   console.log('第一条数据示例:', results[0])
      // }
      callback(null, results)
    }
  })
}

/**
 * 根据 id（serial_number）查询
 */
exports.getAssetById = (id, callback) => {
  const sql = 'SELECT * FROM asset WHERE id = ?'
  db.query(sql, [id], callback)
}

/**
 * 新增资产（一般不直接用，主要给测试/后台）
 */
exports.addAsset = (data, callback) => {
  const sql = `
    INSERT INTO asset (id, card, name, note, RSSI, time)
    VALUES (?, ?, ?, ?, ?, ?)
  `
  const params = [
    data.id,
    data.card,
    data.name,
    data.note,
    data.RSSI,
    data.time || Date.now()
  ]
  db.query(sql, params, callback)
}

/**
 * 修改资产
 */
exports.updateAsset = (id, data, callback) => {
  let fields = [];
  let params = [];

  if (data.card !== undefined) {
    fields.push('card = ?');
    params.push(data.card);
  }

  if (data.name !== undefined) {
    fields.push('name = ?');
    params.push(data.name);
  }

  if (data.note !== undefined) {
    fields.push('note = ?');
    params.push(data.note);
  }

  if (data.RSSI !== undefined) {
    fields.push('RSSI = ?');
    params.push(data.RSSI);
  }

  // 时间字段也要支持更新
  if (data.time !== undefined) {
    fields.push('time = ?');
    params.push(data.time);
  }

  if (fields.length === 0) {
    return callback(null, { affectedRows: 0 });
  }

  const sql = `UPDATE asset SET ${fields.join(', ')} WHERE id = ?`;
  params.push(id);

  db.query(sql, params, callback);
};

/**
 * 删除资产
 */
exports.deleteAsset = (id, callback) => {
  const sql = 'DELETE FROM asset WHERE id = ?'
  db.query(sql, [id], callback)
}

/**
 * ⭐ 设备 / 云平台同步（核心）
 * serial_number = id
 */
exports.upsertAsset = (data, callback) => {
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

  const params = [
    data.id,
    data.card,
    data.name,
    data.note,
    data.RSSI,
    data.time || Date.now()
  ];

  db.query(sql, params, callback);
};

/**
 * 根据卡号查询资产
 */
exports.getAssetByCard = (card, callback) => {
  const sql = 'SELECT * FROM asset WHERE card = ?'
  db.query(sql, [card], callback)
}