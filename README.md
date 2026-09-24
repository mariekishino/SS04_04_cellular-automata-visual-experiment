# SS04_04_cellular-automata-visual-experiment

セルオートマトンから発想した人工生命を画面に描く、C++ の視覚実験です。
無音でも自律的に動く「身体」を作り、そこに音を環境入力として与え、刺激の履歴が残るかを調べます。

## 3行で言うと

- **何を作るか:** まとまりを保ちながら流体のように動く視覚的存在。音はスイッチではなく環境入力。
- **今どこか:** **Phase 3(履歴)**。Phase 2 で音の刺激が内部更新に作用し、一様なら一過性、勾配なら向きが変わることが分かった。
- **次に何をするか:** 刺激の長期平均を遅い状態変数として持ち(順応)、同じ音でも履歴によって応答が違うか、その差がどれだけ残るかを測る。

## 現在のフェーズ

**Phase 3 — 履歴** → [docs/phases/03_history.md](docs/phases/03_history.md)

Phase 0〜2 は完了(報告: [Phase 0](docs/phases/00_report.md)、[Phase 1](docs/phases/01_report.md)、[Phase 2](docs/phases/02_report.md))。採用モデルは Lenia 型(Orbium、周期境界、64×64)、刺激の入り口は成長関数への加算。

フェーズを進めるときは、この節だけを書き換えます。コーディングエージェントはこの節を「今の作業範囲」として読みます([CLAUDE.md](CLAUDE.md) 参照)。

## ビルドと実行

動作確認した環境: Ubuntu 24.04 (exe.dev VM, x86_64, 2 コア)、g++ 13.3.0、CMake 3.28.3。依存ライブラリはありません。動画とシート画像の生成にだけ ffmpeg 6.1 と ImageMagick(`montage`)を使います。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
(cd build && ctest --output-on-failure)

./build/cave --list                     # プリセットと初期条件の一覧
./build/cave --model lenia --preset orbium --init orbium --width 64 --height 64 --steps 1000 --every 10 --out experiments/out/L1
./build/cave --model grayscott --preset coral --init square --width 128 --height 128 --steps 10000 --every 100 --out experiments/out/G1
scripts/make_media.sh experiments/out/L1    # video.mp4 と sheet.png を作る
```

出力先には `config.json`(再現に必要な設定)、`metrics.csv`、`frames/*.png`、`state.bin`(再開用)、`summary.txt`(健全性フラグと計測時間)が書かれます。Phase 0 では窓を開かず画像に書き出します([decisions/0002](docs/decisions/0002_headless_rendering_phase0.md))。

### 窓アプリ(Phase 1、SDL3)

```bash
cmake -S . -B build-sdl -DCMAKE_BUILD_TYPE=Release -DCAVE_WITH_SDL3=ON   # SDL3 がなければ自動取得(初回は数分)
cmake --build build-sdl -j --target cave_window
./build-sdl/cave_window --width 64 --height 64 --scale 8 --sps 60
```

キー: Space 停止/再開、N 1 ステップ(停止中)、R 同じ seed でリセット、S スクリーンショット、+/- 速度を 2 倍/半分、Q 終了。タイトルバーにステップ数・速度・総量・連結成分数が出ます。

### 音の刺激(Phase 2)

音源は `experiments/audio/` に置いて名前で指定します(git 管理外)。MP3 / m4a / ogg などは初回に ffmpeg で WAV に変換し、隣にキャッシュします(macOS は `brew install ffmpeg`)。

```bash
python3 scripts/gen_test_wav.py experiments/audio        # kick120.wav / tone.wav / silence.wav を生成
cp ~/Downloads/song.mp3 experiments/audio/               # Suno などの MP3 をそのまま置く
./build/cave --list-audio                                # 置いてある音源の一覧
./build-sdl/cave_window --wav song --stim-mode growth --stim-gain 0.2 --stim-shape gradient_x   # 名前だけで指定

# CLI: 合成パルス(t=2 s から 0.5 秒)を成長関数に加算、無音の基準と比較
./build/cave --out experiments/out/base  --steps 1200 --every 20
./build/cave --out experiments/out/pulse --steps 1200 --every 20 --stim pulse --stim-mode growth --stim-gain 0.2 --pulse-start 2 --pulse-dur 0.5
scripts/compare_runs.py experiments/out/base experiments/out/pulse

# CLI: WAV の RMS 包絡で mu を変調
./build/cave --out experiments/out/wav --steps 720 --every 20 --stim wav --wav experiments/audio/kick120.wav --stim-mode mu --stim-gain 0.01 --stim-shape gradient_x

# 窓アプリ: WAV を再生しながら、再生クロックに合わせて刺激を与える(パスでも名前でも可)
./build-sdl/cave_window --wav kick120 --stim-mode growth --stim-gain 0.2
```

各 run の `stimulus.csv` に毎ステップの振幅、`metrics.csv` に進行方向(`heading_deg`)と区間平均の刺激(`stim_mean`)が加わります。設計の理由は [decisions/0005](docs/decisions/0005_stimulus_path_and_audio.md)。

### 履歴(Phase 3、順応)

```bash
# 密なパルス列(0〜20 s)の後、2 秒の無音を置いて 22 s に探針。順応 k=3、時定数 10 s
./build/cave --out experiments/out/h2 --steps 1620 --every 30 --stim pulse --pulse-start 1 --pulse-dur 0.5 --pulse-period 1 --pulse-count 19 \
  --probe-at 22 --probe-dur 0.5 --stim-mode growth --stim-gain 0.3 --adapt-k 3 --adapt-tau 10
# 同じ探針を、無音の履歴の後に
./build/cave --out experiments/out/h0 --steps 1620 --every 30 --stim none --probe-at 22 --probe-dur 0.5 --stim-mode growth --stim-gain 0.3 --adapt-k 3 --adapt-tau 10
scripts/phase3_summary.py experiments/out          # 探針応答(総量増)と探針時の m を一覧

# 窓アプリで曲を聞きながら順応を見る(タイトルの m が遅い状態変数)
./build-sdl/cave_window --wav 曲名 --stim-mode growth --stim-gain 0.3 --stim-shape cos_x --adapt-k 3 --adapt-tau 10
```

`metrics.csv` に `adapt_m` 列が加わります。`--wav-until T` で曲を T 秒以降無音にでき、`--stim-shape cos_x` は周期境界で連続な形です。設計の理由は [decisions/0006](docs/decisions/0006_adaptation_as_history.md)。

Linux でソースからビルドする場合は X11 の開発パッケージが要ります(一覧は [decisions/0003](docs/decisions/0003_sdl3_window_fetchcontent.md))。VM では仮想ディスプレイで、Mac では実機で動作を確認しています。Windows は未確認です。

## ドキュメントの地図

各ファイルの冒頭に「読者」と「役割」を書いてあります。迷ったらこの表から入ってください。

| 読者 | ファイル | 内容 |
| --- | --- | --- |
| 全員 | [docs/00_vision.md](docs/00_vision.md) | 何を作りたいか、何を問いたいか |
| 全員 | [docs/03_roadmap.md](docs/03_roadmap.md) | Phase 0〜7 の全体像 |
| 本人 | [docs/04_development_workflow.md](docs/04_development_workflow.md) | 1 フェーズをどう回すか(人間側の手順) |
| 実装者 | [docs/01_architecture.md](docs/01_architecture.md) | 技術方針、責務の境界、性能の考え方 |
| 実装者 | [docs/02_experiment_protocol.md](docs/02_experiment_protocol.md) | 何を記録し、何を測るか |
| 実装者 | [docs/phases/](docs/phases/) | 各フェーズの具体的な実装範囲と完了条件。報告: [Phase 0](docs/phases/00_report.md)、[Phase 1](docs/phases/01_report.md)、[Phase 2](docs/phases/02_report.md)、[Phase 3](docs/phases/03_report.md) |
| 実装者 | [docs/decisions/](docs/decisions/) | 後から変えるときに理由が要る決定 |
| 本人 | [docs/learning/](docs/learning/) | C++ とモデルの学習メモ |
| 本人 | [docs/future/](docs/future/) | 今は範囲外の将来構想 |
| エージェント | [CLAUDE.md](CLAUDE.md) | 常時守るルール(自動で読み込まれる) |
| エージェント | [.claude/skills/phase-report/SKILL.md](.claude/skills/phase-report/SKILL.md) | フェーズ完了時の報告様式(`/phase-report`) |

「実装者」は本人とコーディングエージェントの両方を指します。

## 参考にしたもの

コードはどのリポジトリからもコピーしていません。式・定数・データの出典は以下のとおりで、各ソースファイルの冒頭コメントにも同じ内容を書いてあります。

| 対象 | 出典 | 使った範囲 |
| --- | --- | --- |
| Lenia 型の更新則 | B. W.-C. Chan, "Lenia: Biology of Artificial Life", Complex Systems 28(3), 2019 ([arXiv:1812.05433](https://arxiv.org/abs/1812.05433)) | カーネル・成長関数・更新式の定義。単一チャンネル・単一リング・直接畳み込みに単純化 |
| Orbium の初期配置とパラメータ | [Chakazul/Lenia](https://github.com/Chakazul/Lenia)(MIT License、(c) Bert Wang-Chak Chan)の `Python/animals.json` | 20×20 のセル値と R, T, m, s をデコードして [src/core/orbium_cells.hpp](src/core/orbium_cells.hpp) に同梱 |
| Gray–Scott の式 | J. E. Pearson, "Complex Patterns in a Simple System", Science 261 (1993) 189–192 | 反応拡散方程式 |
| Gray–Scott の離散化と定数 | [Karl Sims, Reaction-Diffusion Tutorial](https://www.karlsims.com/rd.html) | 9 点ラプラシアンの重み、Du, Dv, dt、coral / mitosis の F, k |

Grid、指標、PNG 出力、CLI、テスト、実験の設計はこのリポジトリで書いたものです。

## ライセンス

[MIT License](LICENSE)。同梱している Orbium のデータは Chakazul/Lenia 由来で、同じく MIT です。
