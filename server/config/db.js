const mysql = require('mysql')

const db = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: '123456',
  database: 'asset', // 先用已有的库
  charset: 'utf8mb4'
})

db.connect(err => {
  if (err) {
    console.error('❌ MySQL 连接失败:', err)
    return
  }
  console.log('✅ MySQL 连接成功')
})

module.exports = db
