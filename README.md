# DP-05

![DP-05 Icon](assets/images/icon.png)

Sharp Brain PW-SH2 (Windows Embedded CE 6.0) 向けに開発された、Teenage Engineering の精密機器デザインにインスパイアされたダッシュボード・アプリケーションです。

単なる時計アプリではなく、時間を中核とした **「計測装置」** としての緊迫感と美学を追求しています。

## ⚙️ 主な機能

- **精密なUIデザイン**: Nothing OSやTeenage Engineeringのデザインフィッシュにインスパイアされた、インダストリアル・ミニマリズム。
- **マルチページ・ナビゲーション**: 左右キーによるスムーズな横スライド遷移（Cubic Out easing）を搭載。
- **4つのビューモード**:
  - **Dashboard**: 時計、カレンダー、残り時間、Todo、辞書、システム状況を一望。
  - **Huge Clock & Calendar**: 上半分に巨大な時計、下半分に巨大なカレンダーを配置した詳細表示。
  - **Todo List (TofuMental Sync)**: Todoアプリ「TofuMental」の `tasks.txt` と完全同期（UTF-16LE）。
  - **Dictionary Detail**: 「Word of the Day」の詳細情報を表示。
- **高度なカスタマイズ**:
  - **Settings Menu**: Enterキーで「構造分解アニメーション」と共に開く多機能設定。
  - **Night Mode**: 明暗に合わせた配色切り替え。アクセントカラー（Green, Amber, Blue, Red, White）を選択可能。
  - **Font Matrix**: `TenoText` シリーズや `CP period` など、計測器らしいビットマップフォントを自在に選択。
- **永続性 & 保護**:
  - **Auto Saving**: 設定変更は即座に `DP05_Settings.cfg` へ保存。
  - **Burn-in Guard**: 液晶焼き付き防止のための **Sub-Pixel Drift Matrix** 機構。

## ⌨️ 操作方法

| キー | アクション |
| :--- | :--- |
| **左右キー** | ページ（Dashboard / Clock / Todo / Dictionary）の切り替え |
| **Enter (Dashboard)** | 設定画面（Settings）を開く |
| **Enter (Settings)** | 設定の確定・設定画面を閉じる |
| **上下キー (Settings)** | 項目選択 / 値の変更 |
| **上下キー (Todo/Dict)** | 項目選択 / スクロール |
| **Space (Todo)** | タスクの完了状態を切り替え（`tasks.txt` へ即座に保存） |
| **Escape** | アプリの終了（および設定画面を閉じる） |

## 📁 プロジェクト構成

```text
.
├── src/                # C++ ソースコード (main.cpp, Dictionary.h)
├── assets/             # アセット類
│   ├── fonts/          # TTF フォント
│   ├── sounds/         # 効果音 (WAV)
│   └── images/         # アイコン、スプラッシュ画像
├── build_scripts/      # ビルド用スクリプト (WinCE / Windows 10)
├── scripts/            # Python ユーティリティ (BMP/Sound 生成)
├── Example/            # ビルド済みバイナリと実行用アセットの同期用フォルダ
├── dist_win10/         # Windows 10 用ビルド出力先
└── Original/           # オリジナルの Next.js プロジェクト (リファレンス用)
```

## 🛠 ビルド方法

### Windows Embedded CE 6.0 (Sharp Brain)

CeGCC 環境が必要です。

```bash
cd build_scripts
./build.sh
```

ビルド成功後、アセットと共に `Example` フォルダに同期されます。SDカードへの自動デプロイ機能も含まれています。

### Windows 10 (Desktop Preview)

MinGW-w64 (`x86_64-w64-mingw32-g++`) 環境が必要です。

```bash
cd build_scripts
./build_win10.sh
```

`dist_win10` フォルダに実行ファイル (`AppMain_win10.exe`) が生成されます。

## 🎨 デザイン原理

本プロジェクトは **Teenage Engineering** のデザインフィズムを強く継承しています。
「柔らかい影」を排除し、エッジの効いた物理的実在感を重視しています。

- **光源**: 左上 35° 固定。
- **レイアウト**: 上下 50:50 分割。上部は「静寂」、下部は「作業机（情報密度高）」。
- **物理モデル**: ボタンの沈み込みは 0.5px、アニメーションは 0.1〜0.2s 以内に収め、装置としての緊張感を維持します。

---

*Developed for the legacy, repurposed with aesthetics.*
