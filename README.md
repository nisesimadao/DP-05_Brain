# DP-05 "Measurement Device Dashboard"

![DP-05 Icon](assets/images/icon.png)

Sharp Brain PW-SH2 (Windows Embedded CE 6.0) 向けに開発された、Teenage Engineering の精密機器デザインにインスパイアされたダッシュボード・アプリケーションです。

単なる時計アプリではなく、時間を中核とした**「計測装置」**としての緊迫感と美学を追求しています。

## ⚙️ 主な機能

- **精密なUIデザイン**: 「塗装金属板」を想定したニューモーフィズム・インダストリアルデザイン。
- **マルチモジュール表示**:
  - **Clock**: 巨大なデジタル時計。
  - **Calendar**: 精密なグリッド形式のカレンダー。
  - **Time Remaining**: 1日の残り時間をパーセンテージとバーで視覚化。
  - **Dictionary**: 中学レベルの英単語を「Word of the Day」として表示。
  - **Todo List**: 現在のタスクステータスを確認。
  - **System Status**: デバイスのアップタイム等を監視。
- **高度なカスタマイズ**:
  - **Night Mode**: 暗所での運用を想定した発光モード（ディープレッド、アンバー、フォスフォグリーン等のアクセント）。
  - **多彩なフォント**: `teno` シリーズや `CP period` など、計測器らしいビットマップライクなフォントをサポート。
  - **Accent Colors**: グリーン、アンバー、ブルー、レッドなどのアクセント選択。
- **保護機能**: 液晶焼き付き防止のための **Sub-Pixel Drift Matrix** 機構を搭載。
- **スムーズな遷移**: Dashboard と Settings 間の物理的な構造分解アニメーション。

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
