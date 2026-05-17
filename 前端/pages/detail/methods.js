// 环境配置
const ENV_CONFIG = {
  // #ifdef H5
  API_BASE_URL: 'http://localhost:3000',
  // #endif
  // #ifdef APP-PLUS
  API_BASE_URL: 'http://localhost:3000',
//  下面是内网穿透的地址
//  API_BASE_URL: 'https://unhailable-unmanoeuvred-eddy.ngrok-free.dev',
  // #endif
  // #ifdef MP-WEIXIN
  API_BASE_URL: 'http://localhost:3000',
  // API_BASE_URL: 'https://unhailable-unmanoeuvred-eddy.ngrok-free.dev',
  // #endif
}

const PRODUCT_ID = '9H1EHSRbeo'
const DEVICE_NAME = 't1'
const ASSET_COUNT = 9

export default {
  data() {
    return {
      cardList: [],
      isModalVisible: false,
      detailModalVisible: false,
      selectedCard: null,
      isEditing: false,
      isSyncing: false,
      isSearching: false,
      editForm: { card: '', name: '', note: '' },
      timer: null,
      token: '',
      tableHeight: '450px',
      pendingAssetsModalVisible: false,
      pendingAssetsList: [],
      lastDataSnapshot: null,
      isBatchProcessing: false,
      pendingListHeight: '400px',
      showPendingTip: false,
      pendingTipDismissed: false,
      searchTimer: null,
      searchInterval: 500,
      // 新增：MQTT 连接状态
      mqttStatus: { connected: false, clientId: '' }
    }
  },
  
  computed: {
    inStockAssets() {
      return this.cardList.filter(card => card.exist).length
    },
    inStockCardList() {
      return this.cardList.filter(card => card.exist)
    },
    recentActivities() {
      const activities = []
      const now = new Date()
      const timeStr = `${now.getMonth()+1}/${now.getDate()} ${now.getHours().toString().padStart(2,'0')}:${now.getMinutes().toString().padStart(2,'0')}`
      const inStock = this.cardList.filter(c => c.exist)
      if (inStock.length > 0) {
        activities.push({ icon: '✓', title: `${inStock.length} 项资产在库`, time: timeStr, status: '正常', statusClass: 'status-normal', type: 'success' })
      }
      const withRssi = this.cardList.filter(c => c.rssi !== null && c.rssi !== undefined && c.exist)
      if (withRssi.length > 0) {
        const avgRssi = withRssi.reduce((sum, c) => sum + c.rssi, 0) / withRssi.length
        activities.push({ icon: '📶', title: `平均信号强度 ${avgRssi.toFixed(1)} dBm`, time: timeStr, status: this.getRssiLevel(avgRssi), statusClass: this.getRssiStatusClass(avgRssi), type: 'info' })
      }
      return activities
    }
  },
  
  onLoad() {
    this.initCardList()
    this.calculateTableHeight()
    this.calculatePendingListHeight()
    this.fetchAllAssets()
    this.checkMQTTStatus()  // 新增：检查 MQTT 连接状态
  },

  onShow() {
    this.startPolling()
    if (this.isSearching) {
      this.startSearchPolling()
    }
    this.checkMQTTStatus()  // 新增：每次显示时检查状态
  },
  
  onHide() {
    this.stopPolling()
    this.stopSearchPolling()
  },
  
  beforeDestroy() {
    this.stopPolling()
    this.stopSearchPolling()
  },
  
  methods: {   
    initCardList() {
      this.cardList = Array.from({ length: ASSET_COUNT }, (_, i) => ({
        id: i + 1, serialNumber: i + 1, card: '', name: '', note: '', rssi: null, time: null, exist: false
      }))
    },
    
    // 新增：检查 MQTT 连接状态
    checkMQTTStatus() {
      uni.request({
        url: `${ENV_CONFIG.API_BASE_URL}/api/mqtt/status`,
        method: 'GET',
        // header: { 'ngrok-skip-browser-warning': 'true' },
        success: (res) => {
          if (res.data.connected) {
            this.mqttStatus = res.data
            console.log('✅ MQTT 已连接:', res.data.clientId)
          } else {
            console.log('⚠️ MQTT 未连接')
          }
        },
        fail: (err) => console.error('获取 MQTT 状态失败:', err)
      })
    },
    
    fetchAllAssets() {
      console.log('获取所有资产')
      uni.request({
        url: `${ENV_CONFIG.API_BASE_URL}/api/assets`,
        method: 'GET',
        header: { 'ngrok-skip-browser-warning': 'true' },
        success: (res) => {
          if (res.data.code === 0) {
            this.updateCardListFromData(res.data.data || [])
          }
        },
        fail: (err) => console.error('获取资产失败:', err)
      })
    },
    
	// 搜索检测（前端轮询调用）
	searchDetect() {
	console.log('🔍 搜索模式：调用后端检测接口');
	uni.request({
		url: `${ENV_CONFIG.API_BASE_URL}/api/assets/search`,
		method: 'GET',
		timeout: 10000,
		header: { 'Content-Type': 'application/json', 'ngrok-skip-browser-warning': 'true' },
		data: { productId: PRODUCT_ID, deviceName: DEVICE_NAME },
		success: (res) => {
		if (res.data.code === 0) {
			const { pending, allAssets } = res.data.data;
			
			// 关键修改：同时刷新资产列表数据
			if (allAssets && allAssets.length > 0) {
			this.updateCardListFromData(allAssets);
			} else {
			// 如果后端没有返回 allAssets，单独获取资产列表
			this.fetchAllAssets();
			}
			
			// 处理待确认资产
			if (pending && pending.length > 0) {
			this.handlePendingAssets(pending);
			}
		}
		},
		fail: (err) => console.error('❌ 搜索检测失败:', err)
	});
	},
    
    // 修改：处理待确认资产（适配新格式）
    handlePendingAssets(pending) {
      console.log('处理待处理资产:', pending)
      // 去重
      const existingIds = new Set(this.pendingAssetsList.map(a => a.dbData.id))
      const newPending = pending.filter(asset => !existingIds.has(asset.id))
      if (newPending.length === 0) return
      
      const pendingList = newPending.map((asset, index) => ({
        tempId: `pending_${asset.id}_${Date.now()}_${index}`,
        tempNumber: asset.isUpdate ? `更新${index + 1}` : `新增${index + 1}`,
        id: asset.id,
        card: asset.card,
        name: asset.name || '',
        rssi: asset.rssi,
        time: asset.time,
        isUpdate: asset.isUpdate || false,
        processing: false,
        dbData: { id: asset.id, card: asset.card, name: asset.name || '', note: asset.note || '', RSSI: asset.rssi, time: asset.time },
        oldName: asset.oldData?.name,
        oldTime: asset.oldData?.time
      }))
      
      this.pendingAssetsList = [...this.pendingAssetsList, ...pendingList]
      this.pendingAssetsModalVisible = true
      this.showPendingTip = false
    },
    
    updateCardListFromData(dbAssets) {
      const newCardList = [...this.cardList]
      dbAssets.forEach(item => {
        const index = item.id - 1
        if (index >= 0 && index < ASSET_COUNT) {
          newCardList[index] = {
            ...newCardList[index],
            card: item.card || '',
            name: item.name || '',
            note: item.note || '',
            rssi: (item.RSSI && item.RSSI !== 0) ? item.RSSI : null,
            time: item.time || null,
            exist: (item.note !== 'delete')
          }
        }
      })
      this.cardList = newCardList
      this.saveDataSnapshot()
    },
    
    saveDataSnapshot() {
      this.lastDataSnapshot = JSON.parse(JSON.stringify(this.cardList))
    },
    
    // 修改：录入单个资产 - 调用后端 API
    async approveAsset(asset) {
      console.log('🔥 录入资产:', asset.card)
      if (asset.processing) return
      asset.processing = true
      
      try {
        const result = await this.approveAssetsToBackend([asset.dbData])
        if (result.success) {
          uni.showToast({ title: '已录入资产', icon: 'success' })
          const index = this.pendingAssetsList.findIndex(a => a.tempId === asset.tempId)
          if (index !== -1) this.pendingAssetsList.splice(index, 1)
          if (this.pendingAssetsList.length === 0) {
            this.pendingAssetsModalVisible = false
            this.showPendingTip = false
          }
          await this.fetchAllAssets()
        }
      } catch (error) {
        console.error('录入失败:', error)
        uni.showToast({ title: '录入失败', icon: 'none' })
      } finally {
        asset.processing = false
      }
    },
    
    // 修改：拒绝单个资产 - 调用后端 API
    async rejectAsset(asset) {
      if (asset.processing) return
      asset.processing = true
      
      try {
        // 调用后端拒绝接口
        const result = await this.rejectAssetsToBackend([asset.dbData.id])
        if (result.success) {
          uni.showToast({ title: asset.isUpdate ? '已取消更新' : '已拒绝录入', icon: 'success' })
          const index = this.pendingAssetsList.findIndex(a => a.tempId === asset.tempId)
          if (index !== -1) this.pendingAssetsList.splice(index, 1)
          if (this.pendingAssetsList.length === 0) this.pendingAssetsModalVisible = false
          await this.fetchAllAssets()
        }
      } catch (error) {
        console.error('操作失败:', error)
        uni.showToast({ title: '操作失败', icon: 'none' })
      } finally {
        asset.processing = false
      }
    },
    
    // 修改：批量录入
    async approveAllAssets() {
      if (this.isBatchProcessing) return
      this.isBatchProcessing = true
      uni.showLoading({ title: '批量录入中...', mask: true })
      
      const assetsToApprove = [...this.pendingAssetsList]
      
      try {
        const result = await this.approveAssetsToBackend(assetsToApprove.map(a => a.dbData))
        uni.hideLoading()
        if (result.success) {
          uni.showToast({ title: `成功录入 ${assetsToApprove.length} 项资产`, icon: 'success' })
          this.pendingAssetsList = []
          this.pendingAssetsModalVisible = false
          this.showPendingTip = false
          await this.fetchAllAssets()
        }
      } catch (error) {
        uni.hideLoading()
        console.error('批量录入失败:', error)
        uni.showToast({ title: '批量录入失败', icon: 'none' })
      } finally {
        this.isBatchProcessing = false
      }
    },
    
    // 修改：批量拒绝
    async rejectAllAssets() {
      if (this.isBatchProcessing) return
      const newCount = this.pendingAssetsList.filter(a => !a.isUpdate).length
      if (newCount === 0) {
        uni.showToast({ title: '没有可取消的新增资产', icon: 'none' })
        return
      }
      
      uni.showModal({
        title: '确认操作',
        content: `确定删除 ${newCount} 项新增资产吗？`,
        success: async (res) => {
          if (res.confirm) {
            this.isBatchProcessing = true
            uni.showLoading({ title: '处理中...', mask: true })
            
            const newAssets = this.pendingAssetsList.filter(a => !a.isUpdate)
            const newAssetIds = newAssets.map(a => a.dbData.id)
            
            try {
              const result = await this.rejectAssetsToBackend(newAssetIds)
              uni.hideLoading()
              if (result.success) {
                uni.showToast({ title: `已取消 ${newAssetIds.length} 项资产`, icon: 'success' })
                this.pendingAssetsList = this.pendingAssetsList.filter(a => a.isUpdate)
                if (this.pendingAssetsList.length === 0) this.pendingAssetsModalVisible = false
                await this.fetchAllAssets()
              }
            } catch (error) {
              uni.hideLoading()
              console.error('批量取消失败:', error)
              uni.showToast({ title: '取消失败', icon: 'none' })
            } finally {
              this.isBatchProcessing = false
            }
          }
        }
      })
    },
    
    approveAssetsToBackend(assets) {
      console.log('📤 调用录入接口，参数:', assets);
      return new Promise((resolve, reject) => {
        uni.request({
          url: `${ENV_CONFIG.API_BASE_URL}/api/assets/approve`,
          method: 'POST',
          header: { 'Content-Type': 'application/json', 'ngrok-skip-browser-warning': 'true' },
          data: { assets },
          success: (res) => {
            console.log('📥 录入接口返回:', res.data);
            if (res.data.code === 0) {
              resolve({ success: true })
            } else {
              resolve({ success: false, message: res.data.msg })
            }
          },
          fail: reject
        })
      })
    },
    
    rejectAssetsToBackend(assetIds) {
      console.log('📤 调用拒绝接口，参数:', assetIds);
      return new Promise((resolve, reject) => {
        uni.request({
          url: `${ENV_CONFIG.API_BASE_URL}/api/assets/reject`,
          method: 'POST',
          header: { 'Content-Type': 'application/json', 'ngrok-skip-browser-warning': 'true' },
          data: { assetIds },
          success: (res) => {
            if (res.data.code === 0) resolve({ success: true })
            else reject(new Error(res.data.msg))
          },
          fail: reject
        })
      })
    },
    
    closePendingModal() {
      this.pendingAssetsModalVisible = false
      if (this.pendingAssetsList.length > 0) this.showPendingTip = true
    },
    
    openPendingModalFromTip() {
      this.showPendingTip = false
      this.pendingAssetsModalVisible = true
    },
    
    closePendingTip() {
      this.showPendingTip = false
      this.pendingTipDismissed = true
    },
    
    startPolling() {
      if (!this.timer && !this.isSearching) {
        this.timer = setInterval(() => {
          if (!this.isSearching) this.fetchAllAssets()
        }, 10000)
      }
    },
    
    stopPolling() {
      if (this.timer) { clearInterval(this.timer); this.timer = null }
    },
    
    // 修改：开启/关闭搜索模式 - 调用后端 API
	toggleSearchMode() {
	console.log('toggleSearchMode 被调用, 当前 isSearching:', this.isSearching)
	const action = this.isSearching ? 0 : 1
	uni.showModal({
		title: '确认操作',
		content: this.isSearching ? '是否关闭搜索？' : '是否开启搜索？',
		success: (res) => {
		if (res.confirm) {
			console.log('用户确认, action:', action)
			this.sendSearchCommand(action)
		}
		}
	})
	},
		
	// 发送搜索命令（通过后端 MQTT）
	sendSearchCommand(searchValue) {
	console.log('前端调用 sendSearchCommand, searchValue:', searchValue);
	
	if (searchValue === 1) {
		this.isSearching = true
		this.stopPolling()
		this.startSearchPolling()
	} else {
		this.stopSearchPolling()
		this.isSearching = false
		this.startPolling()
	}
	
	uni.request({
		url: `${ENV_CONFIG.API_BASE_URL}/api/mqtt/search-command`,  // 检查这个 URL
		method: 'POST',
		header: { 
		'Content-Type': 'application/json',
		'ngrok-skip-browser-warning': 'true' 
		},
		data: { search: searchValue },
		success: (res) => {
		console.log('搜索命令响应:', res.data)
		if (res.data && res.data.success) {
			console.log('搜索命令发送成功')
		} else {
			console.error('搜索命令发送失败:', res.data)
		}
		},
		fail: (err) => {
		console.error('发送搜索命令请求失败:', err)
		}
	})
	},
    startSearchPolling() {
      if (this.searchTimer) clearInterval(this.searchTimer);
      this.searchTimer = setInterval(() => {
        if (this.isSearching) {
          this.searchDetect();      // 获取待确认资产
          this.fetchAllAssets();    // 🔥 关键：同时刷新资产列表
        }
      }, this.searchInterval);  // searchInterval 建议改为 1000 或更小
      this.searchDetect();
      this.fetchAllAssets();        // 🔥 立即执行一次
    },
    
    stopSearchPolling() {
      if (this.searchTimer) { clearInterval(this.searchTimer); this.searchTimer = null }
    },
    
    openAssetListModal() { this.isModalVisible = true },
    closeModal() { this.isModalVisible = false },
    
    openEditModal(card) {
      if (this.isSearching) {
        console.log('编辑资产，暂停搜索')
        this.stopSearchPolling()
      }
      
      this.selectedCard = JSON.parse(JSON.stringify(card))
      this.detailModalVisible = true
      this.isEditing = true
      this.editForm = { ...this.selectedCard }
    },
    
    closeDetailModal() {
      this.detailModalVisible = false
      this.isEditing = false
      this.selectedCard = null
      this.editForm = { card: '', name: '', note: '' }
    },
    
    enterEditMode() { this.isEditing = true },
    cancelEdit() { this.isEditing = false },
    
    // 修改：删除资产 - 调用后端 API
    confirmDelete(card) {
      if (this.isSearching) {
        console.log('删除资产，暂停搜索')
        this.stopSearchPolling()
      }
      this.selectedCard = card
      uni.showModal({
        title: '确认删除',
        content: `确认删除资产「${card.name || card.card || '未命名'}」？`,
        confirmText: '取消',
        cancelText: '确认删除',
        cancelColor: '#f56c6c',
        success: (res) => { if (res.cancel) this.doDeleteAsset() }
      })
    },
    
    doDeleteAsset() {
      const serialNumber = this.selectedCard.serialNumber
      console.log('========== 开始删除资产 ==========')
      console.log('serialNumber:', serialNumber)
      
      uni.showLoading({ title: '删除中...', mask: true })
      
      // 调用后端删除接口
      uni.request({
        url: `${ENV_CONFIG.API_BASE_URL}/api/assets/${serialNumber}`,
        method: 'DELETE',
        header: { 'ngrok-skip-browser-warning': 'true' },
        success: (res) => {
          uni.hideLoading()
          if (res.data.code === 0) {
            const index = this.cardList.findIndex(card => card.serialNumber === serialNumber)
            if (index !== -1) {
              this.cardList[index].exist = false
              this.cardList[index].card = ''
              this.cardList[index].name = ''
              this.cardList[index].note = 'delete'
            }
            uni.showToast({ title: '删除成功', icon: 'success' })
            this.closeDetailModal()
          } else {
            uni.showToast({ title: res.data.message || '删除失败', icon: 'none' })
          }
        },
        fail: (err) => {
          uni.hideLoading()
          console.error('删除失败:', err)
          uni.showToast({ title: '删除失败', icon: 'none' })
        }
      })
    },
    
    saveEdit() {
      const sn = this.editForm.serialNumber
      uni.showLoading({ title: '保存中...', mask: true })
      uni.request({
        url: `${ENV_CONFIG.API_BASE_URL}/api/assets/${sn}`,
        method: 'PUT',
        header: { 'Content-Type': 'application/json', 'ngrok-skip-browser-warning': 'true' },
        data: { name: this.editForm.name || '', note: this.editForm.note || '' },
        success: (res) => {
          uni.hideLoading()
          if (res.data.code === 0) {
            uni.showToast({ title: '修改成功', icon: 'success' })
            this.closeDetailModal()
            this.fetchAllAssets()
          }
        },
        fail: (err) => {
          uni.hideLoading()
          console.error('保存失败:', err)
        }
      })
    },
    
    fetchDevData() { this.fetchAllAssets() },
    
    calculateTableHeight() {
      const systemInfo = uni.getSystemInfoSync()
      let height = systemInfo.windowHeight * 0.55
      height = Math.max(350, Math.min(550, height))
      this.tableHeight = height + 'px'
    },
    
    calculatePendingListHeight() {
      const systemInfo = uni.getSystemInfoSync()
      let height = systemInfo.windowHeight * 0.5
      height = Math.max(300, Math.min(500, height))
      this.pendingListHeight = height + 'px'
    },
    
    formatTimestamp(timestamp) {
      if (!timestamp) return '暂无'
      const date = new Date(timestamp)
      return `${date.getFullYear()}-${String(date.getMonth()+1).padStart(2,'0')}-${String(date.getDate()).padStart(2,'0')} ${String(date.getHours()).padStart(2,'0')}:${String(date.getMinutes()).padStart(2,'0')}:${String(date.getSeconds()).padStart(2,'0')}`
    },
    
    getSignalWidth(rssi) {
      if (rssi === null || rssi === undefined) return 0
      let width = ((Number(rssi) + 100) / 70) * 100
      return Math.max(0, Math.min(100, width))
    },
    
    getRssiLevel(rssi) {
      if (rssi === null || rssi === undefined) return '无信号'
      const val = Number(rssi)
      if (val >= -50) return '极强'
      if (val >= -60) return '强'
      if (val >= -70) return '中等'
      if (val >= -80) return '弱'
      return '极弱'
    },
    
    getRssiStatusClass(rssi) {
      if (rssi === null || rssi === undefined) return 'status-info'
      const val = Number(rssi)
      if (val >= -60) return 'status-normal'
      if (val >= -70) return 'status-warning'
      return 'status-info'
    }
  }
}