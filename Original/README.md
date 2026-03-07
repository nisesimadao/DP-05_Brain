# DP-05 Dashboard

DP-05は、Next.js、Capacitor、そしてインダストリアルなデザインを融合させた、高機能なステータスダッシュボードアプリです。

## 主な機能

### 1. 統合タイムモジュール (Time Module)
- **残り時間カウントダウン**: その日の残り時間をリアルタイムで表示します。
- **1日の進捗バー**: 今日の経過時間を視覚的に把握できるプログレスバーを搭載。

### 2. ネットワーク監視 (Net Module)
- **トラフィック表示**: アップロード/ダウンロード速度のリアルタイム監視。
- **管理用URL (MGMT)**: ローカルネットワークからTodo管理画面にアクセスするためのURLを表示します。

### 3. デバイス管理 (Devices Module)
- **USB/ネットワークデバイス**: 接続されているデバイスのモニタリング。
- **焼き付き防止ガード (Burn-In Guard)**: 有機ELディスプレイ等の焼き付きを防ぐ市松模様の動的マスク機能。
- **CRTモード**: レトロでかっこいい走査線オーバーレイを適用可能。

### 4. Todo管理 (Todo Module)
- **ダッシュボード表示**: 現在のタスクリストを一覧表示。
- **Web UI管理**: ローカルIPの `/todo` にアクセスすることで、他のデバイス（PCやスマホ）からもタスクの追加・編集・削除が可能です。

## セットアップと実行

### 開発サーバーの起動
```bash
npm run dev
```

### Android/iOSでの実行 (ポート80)
Androidなどの環境で、ポート番号なしで管理画面にアクセスしたい場合は、以下のコマンドを使用してください（管理者権限が必要な場合があります）。

```bash
sudo npm run dev:android
```

### モバイルプラットフォームの同期
```bash
# Android
npx cap sync android

# iOS
npx cap sync ios
```

## 技術スタック
- **Frontend**: Next.js, React, Vanilla CSS
- **Mobile**: Capacitor (Android/iOS)
- **Backend**: Next.js API Routes (Local JSON Persistence)
- **System Information**: `systeminformation` library
