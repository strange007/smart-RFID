const express = require('express')
const router = express.Router()
const assetController = require('../controllers/assetController')

// 搜索检测 - 返回待确认资产
router.get('/search', assetController.searchDetect)

// 获取待确认资产列表
router.get('/pending', assetController.getPendingAssets)

// 确认录入资产
router.post('/approve', assetController.approveAssets)

// 拒绝资产
router.post('/reject', assetController.rejectAssets)

// 基础 CRUD
router.get('/', assetController.getAssets)
router.get('/:id', assetController.getAssetById)
router.post('/', assetController.addAsset)
router.put('/:id', assetController.updateAsset)
router.delete('/:id', assetController.deleteAsset)
router.post('/sync/cloud', assetController.syncFromCloud)

module.exports = router