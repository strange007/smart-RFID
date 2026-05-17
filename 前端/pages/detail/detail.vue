<template>
	<!-- 添加一个根元素包裹所有内容 -->
	<view class="page-container">
		<view class="wrap">
			<!-- 精美的头部区域 -->
			<view class="header-section">
				<view class="header-bg"></view>
				<view class="header-content">
					<view class="greeting">
						<text class="greeting-text">资产管理</text>
						<text class="greeting-sub">智能仓储管理系统</text>
					</view>
					<view class="header-stats">
						<view class="stat-item">
							<text class="stat-number">{{ inStockAssets }}</text>
							<text class="stat-label">在库资产</text>
						</view>
					</view>
				</view>
			</view>

			<!-- 功能入口区域 -->
			<view class="entry-wrapper">
				<view class="entry-card" @click="openAssetListModal" hover-class="card-hover">
					<view class="card-icon">
						<text class="iconfont icon-asset">📦</text>
					</view>
					<view class="card-info">
						<view class="entry-title">资产管理</view>
						<view class="entry-subtitle">查看 / 删除 / 管理资产</view>
					</view>
					<view class="card-arrow">
						<text class="arrow-icon">→</text>
					</view>
				</view>
			</view>

			<!-- 快捷操作区域 -->
			<view class="quick-actions">
				<view class="section-title">
					<text class="title-text">快捷操作</text>
					<text class="title-line"></text>
				</view>
				<view class="actions-grid">
					<view class="action-item" @click="toggleSearchMode" :class="{ 'action-disabled': isSyncing, 'search-active': isSearching }">
						<view class="action-icon" :class="{ 'search-rotate': isSearching }">
							{{ isSearching ? '⏳' : '🔍' }}
						</view>
						<text class="action-name">{{ isSearching ? '搜索中...' : '开始搜索' }}</text>
						<text v-if="isSearching" class="search-tip">自动获取中</text>
					</view>
					<view class="action-item" @click="openAssetListModal">
						<view class="action-icon">📋</view>
						<text class="action-name">资产列表</text>
					</view>
					<view class="action-item" @click="fetchDevData">
						<view class="action-icon">📊</view>
						<text class="action-name">刷新数据</text>
					</view>
				</view>
			</view>
			
			<!-- 待处理资产提示条（新增） -->
			<view v-if="showPendingTip && pendingAssetsList.length > 0" class="pending-tip-bar" @click="openPendingModalFromTip">
				<view class="tip-bar-icon">⚠️</view>
				<view class="tip-bar-content">
					<text class="tip-bar-title">有 {{ pendingAssetsList.length }} 项资产待处理</text>
					<text class="tip-bar-desc">点击查看并处理待录入资产</text>
				</view>
				<view class="tip-bar-close" @click.stop="closePendingTip">
					<text>✕</text>
				</view>
			</view>

			<!-- 最近动态区域 -->
			<view class="recent-section">
				<view class="section-title">
					<text class="title-text">最近动态</text>
					<text class="title-line"></text>
					<text class="title-refresh" @click="fetchDevData">🔄 刷新</text>
				</view>
				<view class="recent-list">
					<view v-for="(activity, index) in recentActivities" :key="index" class="recent-item">
						<view class="recent-icon" :class="activity.type">
							<text>{{ activity.icon }}</text>
						</view>
						<view class="recent-content">
							<view class="recent-title">{{ activity.title }}</view>
							<view class="recent-time">{{ activity.time }}</view>
						</view>
						<view class="recent-status" :class="activity.statusClass">
							{{ activity.status }}
						</view>
					</view>
					<view v-if="recentActivities.length === 0" class="empty-recent">
						<text>暂无动态，快去管理资产吧~</text>
					</view>
				</view>
			</view>
		</view> <!-- 闭合第一个 wrap -->

		<!-- 资产管理列表弹窗 -->
		<view v-if="isModalVisible" class="modal-container" @touchmove.prevent @click.self="closeModal">
			<view class="modal-content asset-table-modal">
				<view class="modal-header">
					<text class="modal-title">资产管理列表</text>
					<view class="modal-close" @click="closeModal">✕</view>
				</view>
				
				<view class="table-wrapper">
					<scroll-view 
						scroll-x="true" 
						scroll-y="true" 
						class="table-scroll-container"
						:style="{ height: tableHeight }"
						show-scrollbar="true"
					>
						<view class="asset-table">
							<!-- 表头 -->
							<view class="table-header">
								<view class="table-cell col-sn">序号</view>
								<view class="table-cell col-card">卡号</view>
								<view class="table-cell col-name">名称</view>
								<view class="table-cell col-rssi">信号强度</view>
								<view class="table-cell col-time">更新时间</view>
								<view class="table-cell col-note">备注</view>
								<view class="table-cell col-action">操作</view>
							</view>
							
							<!-- 表格内容 -->
							<view class="table-body">
								<view
									v-for="(card, index) in inStockCardList"
									:key="card.id || index"
									class="table-row"
								>
									<view class="table-cell col-sn">{{ card.serialNumber }}</view>
									<view class="table-cell col-card">{{ card.card || '--' }}</view>
									<view class="table-cell col-name">{{ card.name || '未命名' }}</view>
									<view class="table-cell col-rssi">
										<view v-if="card.rssi !== null && card.rssi !== undefined" class="rssi-cell">
											<view class="rssi-bar-mini">
												<view class="rssi-bar-fill" :style="{ width: getSignalWidth(card.rssi) + '%' }"></view>
											</view>
											<text class="rssi-value">{{ card.rssi }} dBm</text>
										</view>
										<text v-else class="no-signal">无信号</text>
									</view>
									<view class="table-cell col-time">{{ formatTimestamp(card.time) }}</view>
									<view class="table-cell col-note">{{ card.note || '--' }}</view>
									<view class="table-cell col-action">
										<view class="action-buttons">
											<button
												class="action-btn-table edit-btn-table"
												size="mini"
												@click.stop="openEditModal(card)"
											>
												修改
											</button>
											<button
												class="action-btn-table delete-btn-table"
												size="mini"
												@click.stop="confirmDelete(card)"
											>
												删除
											</button>
										</view>
									</view>
								</view>
								<view v-if="inStockCardList.length === 0" class="empty-table">
									<text>暂无在库资产</text>
								</view>
							</view>
						</view>
					</scroll-view>
				</view>

				<view class="modal-footer">
					<button class="action-btn close-modal-btn" @click="closeModal">关闭</button>
				</view>
			</view>
		</view>
		
		<!-- 资产详情/编辑弹窗 -->
		<view v-if="detailModalVisible" class="modal-container" @click.self="closeDetailModal" @touchmove.prevent>
			<view class="modal-content detail-modal-content">
				<view class="modal-header">
					<text class="modal-title">{{ isEditing ? '编辑资产' : '资产详情' }}</text>
					<view class="modal-close" @click="closeDetailModal">✕</view>
				</view>
				
				<view class="modal-body">
				  <view class="detail-table">
				    <view class="table-row">
				      <view class="table-label">序号</view>
				      <view class="table-value">
				        <text class="detail-text">{{ selectedCard ? selectedCard.serialNumber : '' }}</text>
				      </view>
				    </view>
				
				    <view class="table-row">
				      <view class="table-label">卡号</view>
				      <view class="table-value">
				        <view class="card-edit-wrapper">
				          <input
				            v-model="editForm.card"
				            class="detail-input readonly-input"
				            disabled
				          />
				          <text class="hint">卡号为资产唯一标识，不可修改</text>
				        </view>
				      </view>
				    </view>
				
				    <view class="table-row">
				      <view class="table-label">名称</view>
				      <view class="table-value">
				        <input
				          v-if="isEditing"
				          v-model="editForm.name"
				          class="detail-input"
				          placeholder="请输入名称"
				        />
				        <text v-else class="detail-text">
				          {{ selectedCard ? (selectedCard.name || '暂无') : '暂无' }}
				        </text>
				      </view>
				    </view>
					
				    <view class="table-row">
				      <view class="table-label">信号强度</view>
				      <view class="table-value">
				        <view class="signal-detail">
				          <view class="signal-bar-container detail-bar">
				            <view class="signal-bar" :style="{ width: getSignalWidth(selectedCard ? selectedCard.rssi : null) + '%' }"></view>
				          </view>
				          <view class="signal-info">
				            <text class="signal-value">{{ selectedCard && selectedCard.rssi !== undefined && selectedCard.rssi !== null ? selectedCard.rssi + ' dBm' : '暂无' }}</text>
				            <text class="signal-level">{{ getRssiLevel(selectedCard ? selectedCard.rssi : null) }}</text>
				          </view>
				        </view>
				      </view>
				    </view>
				
				    <view class="table-row">
				      <view class="table-label">备注</view>
				      <view class="table-value">
				        <input
				          v-if="isEditing"
				          v-model="editForm.note"
				          class="detail-input"
				          placeholder="请输入备注"
				        />
				        <text v-else class="detail-text">
				          {{ selectedCard ? (selectedCard.note || '暂无') : '暂无' }}
				        </text>
				      </view>
				    </view>
				  </view>
				</view>

				<view class="modal-footer detail-actions">
				  <button
				    class="action-btn edit-btn"
				    hover-class="btn-pressed"
				    v-if="!isEditing"
				    @click="enterEditMode"
				  >
				    修改
				  </button>
				  
				  <button
				    class="action-btn edit-btn"
				    hover-class="btn-pressed"
				    v-if="isEditing"
				    @click="saveEdit"
				  >
				    保存
				  </button>
				  
				  <button
				    class="action-btn close-btn"
				    hover-class="btn-pressed"
				    @click="isEditing ? cancelEdit() : closeDetailModal()"
				  >
				    {{ isEditing ? '取消' : '关闭' }}
				  </button>
				  
				  <button
				    v-if="!isEditing && selectedCard"
				    class="action-btn delete-btn"
				    hover-class="btn-pressed"
				    @click="confirmDelete(selectedCard)"
				  >
				    删除
				  </button>
				</view>
			</view>
		</view>
		
		<!-- 待处理资产弹窗 -->
		<view v-if="pendingAssetsModalVisible" class="modal-container" @touchmove.prevent @click.self="closePendingModal">
			<view class="modal-content pending-assets-modal">
				<view class="modal-header">
					<text class="modal-title">待处理资产 ({{ pendingAssetsList.length }})</text>
					<view class="modal-close" @click="closePendingModal">✕</view>
				</view>
				
				<view class="pending-assets-body">
					<view class="pending-tip">
						<text class="tip-icon">🔔</text>
						<text class="tip-text">检测到新的或更新的资产，请确认是否录入</text>
					</view>
					
					<scroll-view scroll-y="true" class="pending-list-scroll" :style="{ maxHeight: pendingListHeight }">
						<view class="pending-list">
							<view 
								v-for="(asset, index) in pendingAssetsList" 
								:key="asset.tempId || index"
								class="pending-item"
								:class="{ 'pending-item-update': asset.isUpdate }"
							>
								<view class="pending-item-header">
									<!-- 临时编号 -->
									<view class="pending-number">
										<text class="number-text">{{ asset.tempNumber }}</text>
									</view>
									<view class="pending-badge" :class="{ 'badge-new': !asset.isUpdate, 'badge-update': asset.isUpdate }">
										<text>{{ asset.isUpdate ? '更新' : '新增' }}</text>
									</view>
									<view class="pending-card-number">卡号: {{ asset.card }}</view>
								</view>
								
								<view class="pending-item-info">
									<view class="info-row">
										<text class="info-label">名称：</text>
										<text class="info-value">{{ asset.name || '未命名' }}</text>
									</view>
									<view class="info-row">
										<text class="info-label">信号强度：</text>
										<view class="rssi-info">
											<view class="rssi-bar-mini pending-rssi-bar">
												<view class="rssi-bar-fill" :style="{ width: getSignalWidth(asset.rssi) + '%' }"></view>
											</view>
											<text class="rssi-value">{{ asset.rssi }} dBm</text>
										</view>
									</view>
									<view class="info-row">
										<text class="info-label">检测时间：</text>
										<text class="info-value">{{ formatTimestamp(asset.time) }}</text>
									</view>
									<view v-if="asset.isUpdate" class="info-row old-value-row">
										<text class="info-label">原数据：</text>
										<text class="info-value old-value">{{ asset.oldName || '未命名' }} | {{ formatTimestamp(asset.oldTime) }}</text>
									</view>
								</view>
								
								<view class="pending-item-actions">
									<button 
										class="pending-btn pending-approve-btn" 
										@click="approveAsset(asset)"
										:disabled="asset.processing"
									>
										{{ asset.processing ? '处理中...' : '✓ 录入' }}
									</button>
									<button 
										class="pending-btn pending-reject-btn" 
										@click="rejectAsset(asset)"
										:disabled="asset.processing"
									>
										{{ asset.processing ? '处理中...' : '✗ 取消' }}
									</button>
								</view>
							</view>
						</view>
					</scroll-view>
				</view>
				
				<view class="modal-footer pending-footer">
					<button class="action-btn pending-all-approve" @click="approveAllAssets" :disabled="isBatchProcessing">
						{{ isBatchProcessing ? '处理中...' : '全部录入' }}
					</button>
					<button class="action-btn pending-all-reject" @click="rejectAllAssets" :disabled="isBatchProcessing">
						全部取消
					</button>
					<button class="action-btn close-modal-btn" @click="closePendingModal">暂不处理</button>
				</view>
			</view>
		</view>
	</view> <!-- 闭合最外层的 page-container -->
</template>

<script>
import methods from './methods.js'
export default methods
</script>

<style lang="scss" scoped>
@import './styles.css';
</style>